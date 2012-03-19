
// node
#include <node_buffer.h>
#include <node_version.h>

// mapnik
#include <mapnik/image_util.hpp>
#include <mapnik/graphics.hpp>
#include <mapnik/image_reader.hpp>
#include <mapnik/image_compositing.hpp>

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
    NODE_SET_PROTOTYPE_METHOD(constructor, "width", width);
    NODE_SET_PROTOTYPE_METHOD(constructor, "height", height);
    NODE_SET_PROTOTYPE_METHOD(constructor, "painted", painted);
    NODE_SET_PROTOTYPE_METHOD(constructor, "composite", composite);

    ATTR(constructor, "background", get_prop, set_prop);

    // This *must* go after the ATTR setting
    NODE_SET_METHOD(constructor->GetFunction(),
                  "open",
                  Image::open);

    target->Set(String::NewSymbol("Image"),constructor->GetFunction());
}

Image::Image(unsigned int width, unsigned int height) :
  ObjectWrap(),
  this_(boost::make_shared<mapnik::image_32>(width,height)) {
      V8::AdjustAmountOfExternalAllocatedMemory(4 * width * height);
  }

Image::Image(image_ptr this_) :
  ObjectWrap(),
  this_(this_) {
      V8::AdjustAmountOfExternalAllocatedMemory(4 * this_->width() * this_->height());
  }

Image::~Image()
{
    V8::AdjustAmountOfExternalAllocatedMemory(-4 * this_->width() * this_->height());
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

Handle<Value> Image::open(const Arguments& args)
{
    HandleScope scope;
    
    if (!args[0]->IsString()) {
      return ThrowException(Exception::TypeError(String::New(
              "Argument must be a string")));
    }
  
    try {
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
    catch (std::exception & ex)
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
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

        #if NODE_VERSION_AT_LEAST(0,3,0)
        node::Buffer *retbuf = Buffer::New((char*)s.data(),s.size());
        #else
        node::Buffer *retbuf = Buffer::New(s.size());
        memcpy(retbuf->data(), s.data(), s.size());
        #endif
        return scope.Close(retbuf->handle_);
    }
    catch (std::exception & ex)
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::Error(
          String::New("unknown exception happened when encoding image: please file bug report")));
    }
}

typedef struct {
    uv_work_t request;
    Image* im;
    boost::shared_ptr<mapnik::image_32> image;
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
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, EIO_AfterEncode);
    uv_ref(uv_default_loop());
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
    catch (std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happened when encoding image: please file bug report";
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
    } else {
        #if NODE_VERSION_AT_LEAST(0,3,0)
        node::Buffer *retbuf = Buffer::New((char*)closure->result.data(),closure->result.size());
        #else
        node::Buffer *retbuf = Buffer::New(closure->result.size());
        memcpy(retbuf->data(), closure->result.data(), closure->result.size());
        #endif
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(retbuf->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    uv_unref(uv_default_loop());
    closure->im->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> Image::view(const Arguments& args)
{
    HandleScope scope;

    if ( (!args.Length() == 4) || (!args[0]->IsNumber() && !args[1]->IsNumber() && !args[2]->IsNumber() && !args[3]->IsNumber() ))
        return ThrowException(Exception::TypeError(
          String::New("requires 4 integer arguments: x, y, width, height")));
    
    // TODO parse args
    unsigned x = args[0]->IntegerValue();
    unsigned y = args[1]->IntegerValue();
    unsigned w = args[2]->IntegerValue();
    unsigned h = args[3]->IntegerValue();

    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    return scope.Close(ImageView::New(im->get(),x,y,w,h));
}

Handle<Value> Image::save(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() >= 1 || !args[0]->IsString()){
        return ThrowException(Exception::TypeError(
          String::New("filename required")));
    }
    
    std::string filename = TOSTR(args[0]);
    
    std::string format("");
    
    if (args.Length() >= 2) {
        if (!args[1]->IsString())
          return ThrowException(Exception::TypeError(
            String::New("both 'filename' and 'format' arguments must be strings")));

        format = mapnik::guess_type(TOSTR(args[1]));
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
        mapnik::save_to_file<mapnik::image_data_32>(im->get()->data(),filename);
    }
    catch (const std::exception & ex)
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::TypeError(
          String::New("unknown exception happened while saving an image, please submit a bug report")));
    }
    
    return Undefined();
}

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

    // accept custom format
    if (!args.Length() >= 2){
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
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Composite, EIO_AfterComposite);
    uv_ref(uv_default_loop());
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
    catch (std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happened when compositing image: please file bug report";
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
      FatalException(try_catch);
    }

    uv_unref(uv_default_loop());
    closure->im1->Unref();
    closure->im2->Unref();
    closure->cb.Dispose();
    delete closure;
}
