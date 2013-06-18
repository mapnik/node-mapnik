// node
#include <node.h>
#include <node_buffer.h>

// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/graphics.hpp>          // for image_32
#include <mapnik/image_data.hpp>        // for image_data_32
#include <mapnik/image_reader.hpp>      // for get_image_reader, etc
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc
#include <mapnik/version.hpp>           // for MAPNIK_VERSION

#if MAPNIK_VERSION >= 200100
#include <mapnik/image_compositing.hpp>
#endif

// boost
#include <boost/make_shared.hpp>
#include <boost/optional/optional.hpp>

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_palette.hpp"
#include "mapnik_color.hpp"

#include "utils.hpp"

// std
#include <exception>
#include <memory>                       // for auto_ptr, etc
#include <ostream>                      // for operator<<, basic_ostream
#include <sstream>                      // for basic_ostringstream, etc

Persistent<FunctionTemplate> Image::constructor;

void Image::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Image::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Image"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "encodeSync", encodeSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "encode", encode);
    NODE_SET_PROTOTYPE_METHOD(constructor, "view", view);
    NODE_SET_PROTOTYPE_METHOD(constructor, "save", save);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setGrayScaleToAlpha", setGrayScaleToAlpha);
    NODE_SET_PROTOTYPE_METHOD(constructor, "width", width);
    NODE_SET_PROTOTYPE_METHOD(constructor, "height", height);
    NODE_SET_PROTOTYPE_METHOD(constructor, "painted", painted);
    NODE_SET_PROTOTYPE_METHOD(constructor, "composite", composite);
    NODE_SET_PROTOTYPE_METHOD(constructor, "premultiplySync", premultiplySync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "premultiply", premultiply);
    NODE_SET_PROTOTYPE_METHOD(constructor, "demultiplySync", demultiplySync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "demultiply", demultiply);
    NODE_SET_PROTOTYPE_METHOD(constructor, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(constructor, "clearSync", clear);

    ATTR(constructor, "background", get_prop, set_prop);

    // This *must* go after the ATTR setting
    NODE_SET_METHOD(constructor->GetFunction(),
                    "open",
                    Image::open);
    NODE_SET_METHOD(constructor->GetFunction(),
                    "fromBytes",
                    Image::fromBytes);
    NODE_SET_METHOD(constructor->GetFunction(),
                    "openSync",
                    Image::openSync);
    NODE_SET_METHOD(constructor->GetFunction(),
                    "fromBytesSync",
                    Image::fromBytesSync);
    target->Set(String::NewSymbol("Image"),constructor->GetFunction());
}

Image::Image(unsigned int width, unsigned int height) :
    ObjectWrap(),
    this_(boost::make_shared<mapnik::image_32>(width,height)),
    estimated_size_(width * height * 4)
{
    V8::AdjustAmountOfExternalAllocatedMemory(estimated_size_);
}

Image::Image(image_ptr _this) :
    ObjectWrap(),
    this_(_this),
    estimated_size_(this_->width() * this_->height() * 4)
{
    V8::AdjustAmountOfExternalAllocatedMemory(estimated_size_);
}

Image::~Image()
{
    V8::AdjustAmountOfExternalAllocatedMemory(-estimated_size_);
}

Handle<Value> Image::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Image* im =  static_cast<Image*>(ptr);
        im->Wrap(args.This());
        return args.This();
    }

    if (args.Length() == 2)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
            return ThrowException(Exception::Error(
                                      String::New("Image 'width' and 'height' must be a integers")));
        Image* im = new Image(args[0]->IntegerValue(),args[1]->IntegerValue());
        im->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
                                  String::New("please provide Image width and height")));
    }
    return Undefined();
}

Handle<Value> Image::get_prop(Local<String> property,
                              const AccessorInfo& info)
{
    HandleScope scope;
    Image* im = ObjectWrap::Unwrap<Image>(info.Holder());
    std::string a = TOSTR(property);
    if (a == "background") {
        boost::optional<mapnik::color> c = im->get()->get_background();
        if (c)
            return scope.Close(Color::New(*c));
        else
            return Undefined();
    }
    return Undefined();
}

void Image::set_prop(Local<String> property,
                     Local<Value> value,
                     const AccessorInfo& info)
{
    HandleScope scope;
    Image* im = ObjectWrap::Unwrap<Image>(info.Holder());
    std::string a = TOSTR(property);
    if (a == "background") {
        if (!value->IsObject())
            ThrowException(Exception::TypeError(
                               String::New("mapnik.Color expected")));

        Local<Object> obj = value->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !Color::constructor->HasInstance(obj))
            ThrowException(Exception::TypeError(String::New("mapnik.Color expected")));
        Color *c = ObjectWrap::Unwrap<Color>(obj);
        im->get()->set_background(*c->get());
    }
}

Handle<Value> Image::clearSync(const Arguments& args)
{
    HandleScope scope;
#if MAPNIK_VERSION >= 200200
    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    im->get()->clear();
#endif
    return Undefined();
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string format;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} clear_image_baton_t;

Handle<Value> Image::clear(const Arguments& args)
{
    HandleScope scope;
    Image* im = ObjectWrap::Unwrap<Image>(args.This());

    if (args.Length() == 0) {
        return clearSync(args);
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));
    clear_image_baton_t *closure = new clear_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Clear, (uv_after_work_cb)EIO_AfterClear);
    im->Ref();
    return Undefined();
}

void Image::EIO_Clear(uv_work_t* req)
{
#if MAPNIK_VERSION >= 200200
    clear_image_baton_t *closure = static_cast<clear_image_baton_t *>(req->data);
    try
    {
        closure->im->get()->clear();
    }
    catch(std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
#endif
}

void Image::EIO_AfterClear(uv_work_t* req)
{
    HandleScope scope;
    clear_image_baton_t *closure = static_cast<clear_image_baton_t *>(req->data);
    TryCatch try_catch;
    if (closure->error)
    {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        Local<Value> argv[2] = { Local<Value>::New(Null()) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    if (try_catch.HasCaught())
    {
        node::FatalException(try_catch);
    }
    closure->im->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> Image::setGrayScaleToAlpha(const Arguments& args)
{
    HandleScope scope;

    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    if (args.Length() == 0) {
        im->this_->set_grayscale_to_alpha();
    } else {
        if (!args[0]->IsObject())
            return ThrowException(Exception::TypeError(
                                      String::New("optional second arg must be a mapnik.Color")));

        Local<Object> obj = args[0]->ToObject();

        if (obj->IsNull() || obj->IsUndefined() || !Color::constructor->HasInstance(obj))
            return ThrowException(Exception::TypeError(String::New("mapnik.Color expected as second arg")));

        Color * color = ObjectWrap::Unwrap<Color>(obj);

        mapnik::image_data_32 & data = im->this_->data();
        for (unsigned int y = 0; y < data.height(); ++y)
        {
            unsigned int* row_from = data.getRow(y);
            for (unsigned int x = 0; x < data.width(); ++x)
            {
                unsigned rgba = row_from[x];
                // TODO - big endian support
                unsigned r = rgba & 0xff;
                unsigned g = (rgba >> 8 ) & 0xff;
                unsigned b = (rgba >> 16) & 0xff;

                // magic numbers for grayscale
                unsigned a = (int)((r * .3) + (g * .59) + (b * .11));

                row_from[x] = (a << 24) |
                    (color->get()->blue() << 16) |
                    (color->get()->green() << 8) |
                    (color->get()->red()) ;
            }
        }
    }

    return Undefined();
}

typedef struct {
    uv_work_t request;
    Image* im;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} image_op_baton_t;


Handle<Value> Image::premultiplySync(const Arguments& args)
{
    HandleScope scope;
#if MAPNIK_VERSION >= 200100
    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    im->get()->premultiply();
#endif
    return Undefined();
}

Handle<Value> Image::premultiply(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() == 0) {
        return premultiplySync(args);
    }
    Image* im = ObjectWrap::Unwrap<Image>(args.This());

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Premultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    return Undefined();
}

void Image::EIO_Premultiply(uv_work_t* req)
{
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);

    try
    {
        closure->im->get()->premultiply();
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterMultiply(uv_work_t* req)
{
    HandleScope scope;
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    TryCatch try_catch;
    if (closure->error)
    {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->im->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }
    closure->im->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> Image::demultiplySync(const Arguments& args)
{
    HandleScope scope;
#if MAPNIK_VERSION >= 200100
    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    im->get()->demultiply();
#endif
    return Undefined();
}

Handle<Value> Image::demultiply(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() == 0) {
        return demultiplySync(args);
    }
    Image* im = ObjectWrap::Unwrap<Image>(args.This());

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Demultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    return Undefined();
}

void Image::EIO_Demultiply(uv_work_t* req)
{
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);

    try
    {
        closure->im->get()->demultiply();
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

Handle<Value> Image::painted(const Arguments& args)
{
    HandleScope scope;

    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    return scope.Close(Boolean::New(im->get()->painted()));
}

Handle<Value> Image::width(const Arguments& args)
{
    HandleScope scope;

    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    return scope.Close(Integer::New(im->get()->width()));
}

Handle<Value> Image::height(const Arguments& args)
{
    HandleScope scope;

    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    return scope.Close(Integer::New(im->get()->height()));
}

Handle<Value> Image::openSync(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 1) {
        return ThrowException(Exception::TypeError(
                                  String::New("must provide a string argument")));
    }

    if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(String::New(
                                                       "Argument must be a string")));
    }

    try
    {
        std::string filename = TOSTR(args[0]);
        boost::optional<std::string> type = mapnik::type_from_filename(filename);
        if (type)
        {
            std::auto_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(filename,*type));
            if (reader.get())
            {
                boost::shared_ptr<mapnik::image_32> image_ptr(new mapnik::image_32(reader->width(),reader->height()));
                reader->read(0,0,image_ptr->data());
                Image* im = new Image(image_ptr);
                Handle<Value> ext = External::New(im);
                Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
                return scope.Close(obj);
            }
            return ThrowException(Exception::TypeError(String::New(
                                                           ("Failed to load: " + filename).c_str())));
        }
        return ThrowException(Exception::TypeError(String::New(
                                                       ("Unsupported image format:" + filename).c_str())));
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

typedef struct {
    uv_work_t request;
    image_ptr im;
    const char *data;
    size_t dataLength;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} image_ptr_baton_t;

Handle<Value> Image::open(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() == 1) {
        return openSync(args);
    }

    if (args.Length() < 2) {
        return ThrowException(Exception::TypeError(
                                  String::New("must provide a string argument")));
    }

    if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(String::New(
                                                       "Argument must be a string")));
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    image_ptr_baton_t *closure = new image_ptr_baton_t();
    closure->request.data = closure;
    std::string filename = TOSTR(args[0]);
    closure->data = filename.data();
    closure->dataLength = filename.size();
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Open, (uv_after_work_cb)EIO_AfterOpen);
    return Undefined();
}

void Image::EIO_Open(uv_work_t* req)
{
    image_ptr_baton_t *closure = static_cast<image_ptr_baton_t *>(req->data);

    try
    {
        std::string filename(closure->data,closure->dataLength);
        boost::optional<std::string> type = mapnik::type_from_filename(filename);
        if (!type)
        {
            closure->error = true;
            closure->error_name = "Unsupported image format: " + filename;
        }
        else
        {
            std::auto_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(filename,*type));
            if (reader.get())
            {
                closure->im = boost::make_shared<mapnik::image_32>(reader->width(),reader->height());
                reader->read(0,0,closure->im->data());
            }
            else
            {
                closure->error = true;
                closure->error_name = "Failed to load: " + filename;
            }
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterOpen(uv_work_t* req)
{
    HandleScope scope;
    image_ptr_baton_t *closure = static_cast<image_ptr_baton_t *>(req->data);
    TryCatch try_catch;
    if (closure->error || !closure->im)
    {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        Handle<Value> ext = External::New(im);
        Local<Object> image_obj = constructor->GetFunction()->NewInstance(1, &ext);
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(ObjectWrap::Unwrap<Image>(image_obj)->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> Image::fromBytesSync(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 1) {
        return ThrowException(Exception::TypeError(
                                  String::New("must provide a buffer argument")));
    }

    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined())
        return ThrowException(Exception::TypeError(String::New("first argument is invalid, must be a Buffer")));
    if (!node::Buffer::HasInstance(obj)) {
        return ThrowException(Exception::TypeError(String::New(
                                                       "first argument must be a buffer")));
    }

    try
    {
        std::auto_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(node::Buffer::Data(obj),node::Buffer::Length(obj)));
        if (reader.get())
        {
            boost::shared_ptr<mapnik::image_32> image_ptr(new mapnik::image_32(reader->width(),reader->height()));
            reader->read(0,0,image_ptr->data());
            Image* im = new Image(image_ptr);
            Handle<Value> ext = External::New(im);
            Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
            return scope.Close(obj);
        }
        return ThrowException(Exception::TypeError(String::New(
                                                       "Failed to load from buffer")));
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

Handle<Value> Image::fromBytes(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() == 1) {
        return fromBytesSync(args);
    }

    if (args.Length() < 2) {
        return ThrowException(Exception::TypeError(
                                  String::New("must provide a buffer argument")));
    }

    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined())
        return ThrowException(Exception::TypeError(String::New("first argument is invalid, must be a Buffer")));
    if (!node::Buffer::HasInstance(obj)) {
        return ThrowException(Exception::TypeError(String::New(
                                                       "first argument must be a buffer")));
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    image_ptr_baton_t *closure = new image_ptr_baton_t();
    closure->request.data = closure;
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromBytes, (uv_after_work_cb)EIO_AfterOpen);
    return Undefined();
}

void Image::EIO_FromBytes(uv_work_t* req)
{
    image_ptr_baton_t *closure = static_cast<image_ptr_baton_t *>(req->data);

    try
    {
        std::auto_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(closure->data,closure->dataLength));
        if (reader.get())
        {
            closure->im = boost::make_shared<mapnik::image_32>(reader->width(),reader->height());
            reader->read(0,0,closure->im->data());
        }
        else
        {
            closure->error = true;
            closure->error_name = "Failed to load from buffer";
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}


Handle<Value> Image::encodeSync(const Arguments& args)
{
    HandleScope scope;

    Image* im = ObjectWrap::Unwrap<Image>(args.This());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (args.Length() >= 1){
        if (!args[0]->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("first arg, 'format' must be a string")));
        format = TOSTR(args[0]);
    }


    // options hash
    if (args.Length() >= 2) {
        if (!args[1]->IsObject())
            return ThrowException(Exception::TypeError(
                                      String::New("optional second arg must be an options object")));

        Local<Object> options = args[1]->ToObject();

        if (options->Has(String::New("palette")))
        {
            Local<Value> format_opt = options->Get(String::New("palette"));
            if (!format_opt->IsObject())
                return ThrowException(Exception::TypeError(
                                          String::New("'palette' must be an object")));

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Palette::constructor->HasInstance(obj))
                return ThrowException(Exception::TypeError(String::New("mapnik.Palette expected as second arg")));

            palette = ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    try {
        std::string s;
        if (palette.get())
        {
            s = save_to_string(*(im->this_), format, *palette);
        }
        else {
            s = save_to_string(*(im->this_), format);
        }

        node::Buffer *retbuf = node::Buffer::New((char*)s.data(),s.size());
        return scope.Close(retbuf->handle_);
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string format;
    palette_ptr palette;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    std::string result;
} encode_image_baton_t;

Handle<Value> Image::encode(const Arguments& args)
{
    HandleScope scope;

    Image* im = ObjectWrap::Unwrap<Image>(args.This());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (args.Length() >= 1){
        if (!args[0]->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("first arg, 'format' must be a string")));
        format = TOSTR(args[0]);
    }

    // options hash
    if (args.Length() >= 2) {
        if (!args[1]->IsObject())
            return ThrowException(Exception::TypeError(
                                      String::New("optional second arg must be an options object")));

        Local<Object> options = args[1]->ToObject();

        if (options->Has(String::New("palette")))
        {
            Local<Value> format_opt = options->Get(String::New("palette"));
            if (!format_opt->IsObject())
                return ThrowException(Exception::TypeError(
                                          String::New("'palette' must be an object")));

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Palette::constructor->HasInstance(obj))
                return ThrowException(Exception::TypeError(String::New("mapnik.Palette expected as second arg")));

            palette = ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    encode_image_baton_t *closure = new encode_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->format = format;
    closure->palette = palette;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    im->Ref();

    return Undefined();
}

void Image::EIO_Encode(uv_work_t* req)
{
    encode_image_baton_t *closure = static_cast<encode_image_baton_t *>(req->data);

    try {
        if (closure->palette.get())
        {
            closure->result = save_to_string(*(closure->im->this_), closure->format, *closure->palette);
        }
        else
        {
            closure->result = save_to_string(*(closure->im->this_), closure->format);
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterEncode(uv_work_t* req)
{
    HandleScope scope;

    encode_image_baton_t *closure = static_cast<encode_image_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        node::Buffer *retbuf = node::Buffer::New((char*)closure->result.data(),closure->result.size());
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(retbuf->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->im->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> Image::view(const Arguments& args)
{
    HandleScope scope;

    if ( (args.Length() != 4) || (!args[0]->IsNumber() && !args[1]->IsNumber() && !args[2]->IsNumber() && !args[3]->IsNumber() ))
        return ThrowException(Exception::TypeError(
                                  String::New("requires 4 integer arguments: x, y, width, height")));

    // TODO parse args
    unsigned x = args[0]->IntegerValue();
    unsigned y = args[1]->IntegerValue();
    unsigned w = args[2]->IntegerValue();
    unsigned h = args[3]->IntegerValue();

    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    return scope.Close(ImageView::New(im,x,y,w,h));
}

Handle<Value> Image::save(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() == 0 || !args[0]->IsString()){
        return ThrowException(Exception::TypeError(
                                  String::New("filename required")));
    }

    std::string filename = TOSTR(args[0]);

    std::string format("");

    if (args.Length() >= 2) {
        if (!args[1]->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("both 'filename' and 'format' arguments must be strings")));
        format = TOSTR(args[1]);
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            return ThrowException(Exception::Error(
                                      String::New(s.str().c_str())));
        }
    }

    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    try
    {
        mapnik::save_to_file<mapnik::image_data_32>(im->get()->data(),filename, format);
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    return Undefined();
}

#if MAPNIK_VERSION >= 200100

typedef struct {
    uv_work_t request;
    Image* im1;
    Image* im2;
    mapnik::composite_mode_e mode;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} composite_image_baton_t;

Handle<Value> Image::composite(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 2){
        return ThrowException(Exception::TypeError(
                                  String::New("requires two arguments: an image mask and a compositeOp")));
    }

    if (!args[0]->IsObject()) {
        return ThrowException(Exception::TypeError(
                                  String::New("first argument must be an image mask")));
    }

    if (!args[1]->IsNumber()) {
        return ThrowException(Exception::TypeError(
                                  String::New("second argument must be an compositeOp value")));
    }

    Local<Object> im2 = args[0]->ToObject();
    if (im2->IsNull() || im2->IsUndefined() || !Image::constructor->HasInstance(im2))
        return ThrowException(Exception::TypeError(String::New("mapnik.Image expected as first arg")));

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    composite_image_baton_t *closure = new composite_image_baton_t();
    closure->request.data = closure;
    closure->im1 = ObjectWrap::Unwrap<Image>(args.This());
    closure->im2 = ObjectWrap::Unwrap<Image>(im2);
    closure->mode = static_cast<mapnik::composite_mode_e>(args[1]->IntegerValue());
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Composite, (uv_after_work_cb)EIO_AfterComposite);
    closure->im1->Ref();
    closure->im2->Ref();
    return Undefined();
}

void Image::EIO_Composite(uv_work_t* req)
{
    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    try
    {
        mapnik::composite(closure->im1->this_->data(),closure->im2->this_->data(), closure->mode);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterComposite(uv_work_t* req)
{
    HandleScope scope;

    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->im1->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->im1->Unref();
    closure->im2->Unref();
    closure->cb.Dispose();
    delete closure;
}

#else

Handle<Value> Image::composite(const Arguments& args)
{
    HandleScope scope;

    return ThrowException(Exception::TypeError(
                              String::New("compositing is only supported if node-mapnik is built against >= Mapnik 2.1.x")));

}

#endif
