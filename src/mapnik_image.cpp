
#include "mapnik3x_compatibility.hpp"

// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/graphics.hpp>          // for image_32
#include <mapnik/image_data.hpp>        // for image_data_32
#include <mapnik/image_reader.hpp>      // for get_image_reader, etc
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include <mapnik/image_compositing.hpp>
#include <mapnik/image_filter_types.hpp>
#include <mapnik/image_filter.hpp> // filter_visitor

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_palette.hpp"
#include "mapnik_color.hpp"

#include "utils.hpp"

// boost
#include MAPNIK_MAKE_SHARED_INCLUDE
#include <boost/optional/optional.hpp>

// std
#include <exception>
#include <ostream>                      // for operator<<, basic_ostream
#include <sstream>                      // for basic_ostringstream, etc
#include <cstdlib>

Persistent<FunctionTemplate> Image::constructor;

void Image::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Image::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Image"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "getPixel", getPixel);
    NODE_SET_PROTOTYPE_METHOD(lcons, "setPixel", setPixel);
    NODE_SET_PROTOTYPE_METHOD(lcons, "encodeSync", encodeSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "encode", encode);
    NODE_SET_PROTOTYPE_METHOD(lcons, "view", view);
    NODE_SET_PROTOTYPE_METHOD(lcons, "save", save);
    NODE_SET_PROTOTYPE_METHOD(lcons, "setGrayScaleToAlpha", setGrayScaleToAlpha);
    NODE_SET_PROTOTYPE_METHOD(lcons, "width", width);
    NODE_SET_PROTOTYPE_METHOD(lcons, "height", height);
    NODE_SET_PROTOTYPE_METHOD(lcons, "painted", painted);
    NODE_SET_PROTOTYPE_METHOD(lcons, "composite", composite);
    NODE_SET_PROTOTYPE_METHOD(lcons, "premultiplySync", premultiplySync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "premultiply", premultiply);
    NODE_SET_PROTOTYPE_METHOD(lcons, "demultiplySync", demultiplySync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "demultiply", demultiply);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clearSync", clear);
    NODE_SET_PROTOTYPE_METHOD(lcons, "compare", compare);

    ATTR(lcons, "background", get_prop, set_prop);

    // This *must* go after the ATTR setting
    NODE_SET_METHOD(lcons->GetFunction(),
                    "open",
                    Image::open);
    NODE_SET_METHOD(lcons->GetFunction(),
                    "fromBytes",
                    Image::fromBytes);
    NODE_SET_METHOD(lcons->GetFunction(),
                    "openSync",
                    Image::openSync);
    NODE_SET_METHOD(lcons->GetFunction(),
                    "fromBytesSync",
                    Image::fromBytesSync);
    target->Set(NanNew("Image"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Image::Image(unsigned int width, unsigned int height) :
    ObjectWrap(),
    this_(MAPNIK_MAKE_SHARED<mapnik::image_32>(width,height)),
    estimated_size_(width * height * 4)
{
    NanAdjustExternalMemory(estimated_size_);
}

Image::Image(image_ptr _this) :
    ObjectWrap(),
    this_(_this),
    estimated_size_(this_->width() * this_->height() * 4)
{
    NanAdjustExternalMemory(estimated_size_);
}

Image::~Image()
{
    NanAdjustExternalMemory(-estimated_size_);
}

NAN_METHOD(Image::New)
{
    NanScope();
    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args[0]->IsExternal())
    {
        Local<External> ext = args[0].As<External>();
        void* ptr = ext->Value();
        Image* im =  static_cast<Image*>(ptr);
        im->Wrap(args.This());
        NanReturnValue(args.This());
    }

    try
    {
        if (args.Length() == 2)
        {
            if (!args[0]->IsNumber() || !args[1]->IsNumber())
            {
                NanThrowTypeError("Image 'width' and 'height' must be a integers");
                NanReturnUndefined();
            }
            Image* im = new Image(args[0]->IntegerValue(),args[1]->IntegerValue());
            im->Wrap(args.This());
            NanReturnValue(args.This());
        }
        else
        {
            NanThrowError("please provide Image width and height");
            NanReturnUndefined();
        }
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

NAN_GETTER(Image::get_prop)
{
    NanScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    std::string a = TOSTR(property);
    if (a == "background") {
        boost::optional<mapnik::color> c = im->get()->get_background();
        if (c) {
            NanReturnValue(Color::New(*c));
        } else {
            NanReturnUndefined();
        }
    }
    NanReturnUndefined();
}

NAN_SETTER(Image::set_prop)
{
    NanScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    std::string a = TOSTR(property);
    if (a == "background") {
        if (!value->IsObject())
        {
            NanThrowTypeError("mapnik.Color expected");
            return;
        }

        Local<Object> obj = value->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !NanNew(Color::constructor)->HasInstance(obj)) {
            NanThrowTypeError("mapnik.Color expected");
            return;
        }
        Color *c = node::ObjectWrap::Unwrap<Color>(obj);
        im->get()->set_background(*c->get());
    }
}

NAN_METHOD(Image::getPixel)
{
    NanScope();
    int x = 0;
    int y = 0;
    if (args.Length() >= 2) {
        if (!args[0]->IsNumber()) {
            NanThrowTypeError("first arg, 'x' must be an integer");
            NanReturnUndefined();
        }
        if (!args[1]->IsNumber()) {
            NanThrowTypeError("second arg, 'y' must be an integer");
            NanReturnUndefined();
        }
        x = args[0]->IntegerValue();
        y = args[1]->IntegerValue();
    } else {
        NanThrowError("must supply x,y to query pixel color");
        NanReturnUndefined();
    }
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    mapnik::image_data_32 const& data = im->this_->data();
    if (x >= 0 && x < static_cast<int>(data.width())
        && y >= 0 && y < static_cast<int>(data.height()))
    {
        unsigned pixel = data(x,y);
        unsigned r = pixel & 0xff;
        unsigned g = (pixel >> 8) & 0xff;
        unsigned b = (pixel >> 16) & 0xff;
        unsigned a = (pixel >> 24) & 0xff;
        NanReturnValue(Color::New(mapnik::color(r,g,b,a)));
    }
    NanReturnUndefined();
}

NAN_METHOD(Image::setPixel)
{
    NanScope();
    if (args.Length() < 3 || (!args[0]->IsNumber() && !args[1]->IsNumber())) {
        NanThrowTypeError("expects three arguments: x, y, and pixel value");
        NanReturnUndefined();
    }
    Local<Object> obj = args[2]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !NanNew(Color::constructor)->HasInstance(obj)) {
        NanThrowTypeError("mapnik.Color expected as third arg");
        NanReturnUndefined();
    }
    Color * color = node::ObjectWrap::Unwrap<Color>(obj);
    int x = args[0]->IntegerValue();
    int y = args[1]->IntegerValue();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    mapnik::image_data_32 & data = im->this_->data();
    if (x < static_cast<int>(data.width()) && y < static_cast<int>(data.height()))
    {
        data(x,y) = color->get()->rgba();
        NanReturnUndefined();
    }
    NanThrowTypeError("invalid pixel requested");
    NanReturnUndefined();
}

NAN_METHOD(Image::compare)
{
    NanScope();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        NanThrowTypeError("first argument should be a mapnik.Image");
        NanReturnUndefined();
    }
    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !NanNew(Image::constructor)->HasInstance(obj)) {
        NanThrowTypeError("mapnik.Image expected as first arg");
        NanReturnUndefined();
    }

    Local<Object> options = NanNew<Object>();
    int threshold = 16;
    unsigned alpha = true;
    unsigned difference = 0;

    if (args.Length() > 1) {

        if (!args[1]->IsObject()) {
            NanThrowTypeError("optional second argument must be an options object");
            NanReturnUndefined();
        }

        options = args[1]->ToObject();

        if (options->Has(NanNew("threshold"))) {
            Local<Value> bind_opt = options->Get(NanNew("threshold"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'threshold' must be a number");
                NanReturnUndefined();
            }
            threshold = bind_opt->IntegerValue();
        }

        if (options->Has(NanNew("alpha"))) {
            Local<Value> bind_opt = options->Get(NanNew("alpha"));
            if (!bind_opt->IsBoolean()) {
                NanThrowTypeError("optional arg 'alpha' must be a boolean");
                NanReturnUndefined();
            }
            alpha = bind_opt->BooleanValue();
        }

    }
    Image* im = node::ObjectWrap::Unwrap<Image>(args.This());
    Image* im2 = node::ObjectWrap::Unwrap<Image>(obj);
    if (im->this_->width() != im2->this_->width() ||
        im->this_->height() != im2->this_->height()) {
            NanThrowTypeError("image dimensions do not match");
            NanReturnUndefined();
    }
    mapnik::image_data_32 const& data = im->this_->data();
    mapnik::image_data_32 const& data2 = im2->this_->data();
    for (unsigned int y = 0; y < data.height(); ++y)
    {
        const unsigned int* row_from = data.getRow(y);
        const unsigned int* row_from2 = data2.getRow(y);
        for (unsigned int x = 0; x < data.width(); ++x)
        {
            unsigned rgba = row_from[x];
            unsigned rgba2 = row_from2[x];
            unsigned r = rgba & 0xff;
            unsigned g = (rgba >> 8 ) & 0xff;
            unsigned b = (rgba >> 16) & 0xff;
            unsigned r2 = rgba2 & 0xff;
            unsigned g2 = (rgba2 >> 8 ) & 0xff;
            unsigned b2 = (rgba2 >> 16) & 0xff;
            if (std::abs(static_cast<int>(r - r2)) > threshold ||
                std::abs(static_cast<int>(g - g2)) > threshold ||
                std::abs(static_cast<int>(b - b2)) > threshold) {
                ++difference;
                continue;
            }
            if (alpha) {
                unsigned a = (rgba >> 24) & 0xff;
                unsigned a2 = (rgba2 >> 24) & 0xff;
                if (std::abs(static_cast<int>(a - a2)) > threshold) {
                    ++difference;
                    continue;
                }
            }
        }
    }
    NanReturnValue(NanNew<Integer>(difference));
}

NAN_METHOD(Image::clearSync)
{
    NanScope();
    NanReturnValue(_clearSync(args));
}

Local<Value> Image::_clearSync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    im->get()->clear();
    return NanEscapeScope(NanUndefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string format;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} clear_image_baton_t;

NAN_METHOD(Image::clear)
{
    NanScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());

    if (args.Length() == 0) {
        NanReturnValue(_clearSync(args));
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }
    clear_image_baton_t *closure = new clear_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Clear, (uv_after_work_cb)EIO_AfterClear);
    im->Ref();
    NanReturnUndefined();
}

void Image::EIO_Clear(uv_work_t* req)
{
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
}

void Image::EIO_AfterClear(uv_work_t* req)
{
    NanScope();
    clear_image_baton_t *closure = static_cast<clear_image_baton_t *>(req->data);
    TryCatch try_catch;
    if (closure->error)
    {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        Local<Value> argv[1] = { NanNull() };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    closure->im->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}

NAN_METHOD(Image::setGrayScaleToAlpha)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    if (args.Length() == 0) {
        im->this_->set_grayscale_to_alpha();
    } else {
        if (!args[0]->IsObject()) {
            NanThrowTypeError("optional first arg must be a mapnik.Color");
            NanReturnUndefined();
        }

        Local<Object> obj = args[0]->ToObject();

        if (obj->IsNull() || obj->IsUndefined() || !NanNew(Color::constructor)->HasInstance(obj)) {
            NanThrowTypeError("mapnik.Color expected as first arg");
            NanReturnUndefined();
        }

        Color * color = node::ObjectWrap::Unwrap<Color>(obj);

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

    NanReturnUndefined();
}

typedef struct {
    uv_work_t request;
    Image* im;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} image_op_baton_t;


NAN_METHOD(Image::premultiplySync)
{
    NanScope();
    NanReturnValue(_premultiplySync(args));
}

Local<Value> Image::_premultiplySync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    im->get()->premultiply();
    return NanEscapeScope(NanUndefined());
}

NAN_METHOD(Image::premultiply)
{
    NanScope();
    if (args.Length() == 0) {
        NanReturnValue(_premultiplySync(args));
    }
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Premultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    NanReturnUndefined();
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
    NanScope();
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    if (closure->error)
    {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->im) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }
    closure->im->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}

NAN_METHOD(Image::demultiplySync)
{
    NanScope();
    NanReturnValue(_demultiplySync(args));
}

Local<Value> Image::_demultiplySync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    im->get()->demultiply();
    return NanEscapeScope(NanUndefined());
}

NAN_METHOD(Image::demultiply)
{
    NanScope();
    if (args.Length() == 0) {
        NanReturnValue(_demultiplySync(args));
    }
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Demultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    NanReturnUndefined();
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

NAN_METHOD(Image::painted)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    NanReturnValue(NanNew<Boolean>(im->get()->painted()));
}

NAN_METHOD(Image::width)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    NanReturnValue(NanNew<Integer>(im->get()->width()));
}

NAN_METHOD(Image::height)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    NanReturnValue(NanNew<Integer>(im->get()->height()));
}

NAN_METHOD(Image::openSync)
{
    NanScope();
    NanReturnValue(_openSync(args));
}

Local<Value> Image::_openSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();

    if (args.Length() < 1) {
        NanThrowError("must provide a string argument");
        return NanEscapeScope(NanUndefined());
    }

    if (!args[0]->IsString()) {
        NanThrowTypeError("Argument must be a string");
        return NanEscapeScope(NanUndefined());
    }

    try
    {
        std::string filename = TOSTR(args[0]);
        boost::optional<std::string> type = mapnik::type_from_filename(filename);
        if (type)
        {
            MAPNIK_UNIQUE_PTR<mapnik::image_reader> reader(mapnik::get_image_reader(filename,*type));
            if (reader.get())
            {
                MAPNIK_SHARED_PTR<mapnik::image_32> image_ptr(new mapnik::image_32(reader->width(),reader->height()));
                reader->read(0,0,image_ptr->data());
                Image* im = new Image(image_ptr);
                Handle<Value> ext = NanNew<External>(im);
                Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
                return NanEscapeScope(obj);
            }
            NanThrowTypeError(("Failed to load: " + filename).c_str());
            return NanEscapeScope(NanUndefined());
        }
        NanThrowTypeError(("Unsupported image format:" + filename).c_str());
        return NanEscapeScope(NanUndefined());
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        return NanEscapeScope(NanUndefined());
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
} image_mem_ptr_baton_t;

typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string filename;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} image_file_ptr_baton_t;

NAN_METHOD(Image::open)
{
    NanScope();

    if (args.Length() == 1) {
        NanReturnValue(_openSync(args));
    }

    if (args.Length() < 2) {
        NanThrowError("must provide a string argument");
        NanReturnUndefined();
    }

    if (!args[0]->IsString()) {
        NanThrowTypeError("Argument must be a string");
        NanReturnUndefined();
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    image_file_ptr_baton_t *closure = new image_file_ptr_baton_t();
    closure->request.data = closure;
    closure->filename = TOSTR(args[0]);
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Open, (uv_after_work_cb)EIO_AfterOpen);
    NanReturnUndefined();
}

void Image::EIO_Open(uv_work_t* req)
{
    image_file_ptr_baton_t *closure = static_cast<image_file_ptr_baton_t *>(req->data);

    try
    {
        boost::optional<std::string> type = mapnik::type_from_filename(closure->filename);
        if (!type)
        {
            closure->error = true;
            closure->error_name = "Unsupported image format: " + closure->filename;
        }
        else
        {
            MAPNIK_UNIQUE_PTR<mapnik::image_reader> reader(mapnik::get_image_reader(closure->filename,*type));
            if (reader.get())
            {
                closure->im = MAPNIK_MAKE_SHARED<mapnik::image_32>(reader->width(),reader->height());
                reader->read(0,0,closure->im->data());
            }
            else
            {
                closure->error = true;
                closure->error_name = "Failed to load: " + closure->filename;
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
    NanScope();
    image_file_ptr_baton_t *closure = static_cast<image_file_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        Handle<Value> ext = NanNew<External>(im);
        Local<Object> image_obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(ObjectWrap::Unwrap<Image>(image_obj)) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }
    NanDisposePersistent(closure->cb);
    delete closure;
}

NAN_METHOD(Image::fromBytesSync)
{
    NanScope();
    NanReturnValue(_fromBytesSync(args));
}

Local<Value> Image::_fromBytesSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        NanThrowTypeError("must provide a buffer argument");
        return NanEscapeScope(NanUndefined());
    }

    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined()) {
        NanThrowTypeError("first argument is invalid, must be a Buffer");
        return NanEscapeScope(NanUndefined());
    }
    if (!node::Buffer::HasInstance(obj)) {
        NanThrowTypeError("first argument must be a buffer");
        return NanEscapeScope(NanUndefined());
    }

    try
    {
        MAPNIK_UNIQUE_PTR<mapnik::image_reader> reader(mapnik::get_image_reader(node::Buffer::Data(obj),node::Buffer::Length(obj)));
        if (reader.get())
        {
            MAPNIK_SHARED_PTR<mapnik::image_32> image_ptr(new mapnik::image_32(reader->width(),reader->height()));
            reader->read(0,0,image_ptr->data());
            Image* im = new Image(image_ptr);
            Handle<Value> ext = NanNew<External>(im);
            return NanEscapeScope(NanNew(constructor)->GetFunction()->NewInstance(1, &ext));
        }
        NanThrowTypeError("Failed to load from buffer");
        return NanEscapeScope(NanUndefined());
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        return NanEscapeScope(NanUndefined());
    }
}

NAN_METHOD(Image::fromBytes)
{
    NanScope();

    if (args.Length() == 1) {
        NanReturnValue(_fromBytesSync(args));
    }

    if (args.Length() < 2) {
        NanThrowError("must provide a buffer argument");
        NanReturnUndefined();
    }

    if (!args[0]->IsObject()) {
        NanThrowTypeError("must provide a buffer argument");
        NanReturnUndefined();
    }

    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined()) {
        NanThrowTypeError("first argument is invalid, must be a Buffer");
        NanReturnUndefined();
    }
    if (!node::Buffer::HasInstance(obj)) {
        NanThrowTypeError("first argument must be a buffer");
        NanReturnUndefined();
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    image_mem_ptr_baton_t *closure = new image_mem_ptr_baton_t();
    closure->request.data = closure;
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromBytes, (uv_after_work_cb)EIO_AfterFromBytes);
    NanReturnUndefined();
}

void Image::EIO_FromBytes(uv_work_t* req)
{
    image_mem_ptr_baton_t *closure = static_cast<image_mem_ptr_baton_t *>(req->data);

    try
    {
        MAPNIK_UNIQUE_PTR<mapnik::image_reader> reader(mapnik::get_image_reader(closure->data,closure->dataLength));
        if (reader.get())
        {
            closure->im = MAPNIK_MAKE_SHARED<mapnik::image_32>(reader->width(),reader->height());
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

void Image::EIO_AfterFromBytes(uv_work_t* req)
{
    NanScope();
    image_mem_ptr_baton_t *closure = static_cast<image_mem_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        Handle<Value> ext = NanNew<External>(im);
        Local<Object> image_obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(ObjectWrap::Unwrap<Image>(image_obj)) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }
    NanDisposePersistent(closure->cb);
    delete closure;
}

NAN_METHOD(Image::encodeSync)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (args.Length() >= 1){
        if (!args[0]->IsString()) {
            NanThrowTypeError("first arg, 'format' must be a string");
            NanReturnUndefined();
        }
        format = TOSTR(args[0]);
    }


    // options hash
    if (args.Length() >= 2) {
        if (!args[1]->IsObject()) {
            NanThrowTypeError("optional second arg must be an options object");
            NanReturnUndefined();
        }
        Local<Object> options = args[1]->ToObject();
        if (options->Has(NanNew("palette")))
        {
            Local<Value> format_opt = options->Get(NanNew("palette"));
            if (!format_opt->IsObject()) {
                NanThrowTypeError("'palette' must be an object");
                NanReturnUndefined();
            }

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(Palette::constructor)->HasInstance(obj)) {
                NanThrowTypeError("mapnik.Palette expected as second arg");
                NanReturnUndefined();
            }
            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
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

        NanReturnValue(NanNewBufferHandle((char*)s.data(), s.size()));
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
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

NAN_METHOD(Image::encode)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (args.Length() >= 1){
        if (!args[0]->IsString()) {
            NanThrowTypeError("first arg, 'format' must be a string");
            NanReturnUndefined();
        }
        format = TOSTR(args[0]);
    }

    // options hash
    if (args.Length() >= 2) {
        if (!args[1]->IsObject()) {
            NanThrowTypeError("optional second arg must be an options object");
            NanReturnUndefined();
        }

        Local<Object> options = args[1].As<Object>();

        if (options->Has(NanNew("palette")))
        {
            Local<Value> format_opt = options->Get(NanNew("palette"));
            if (!format_opt->IsObject()) {
                NanThrowTypeError("'palette' must be an object");
                NanReturnUndefined();
            }

            Local<Object> obj = format_opt.As<Object>();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(Palette::constructor)->HasInstance(obj)) {
                NanThrowTypeError("mapnik.Palette expected as second arg");
                NanReturnUndefined();
            }

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    encode_image_baton_t *closure = new encode_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->format = format;
    closure->palette = palette;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    im->Ref();

    NanReturnUndefined();
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
    NanScope();

    encode_image_baton_t *closure = static_cast<encode_image_baton_t *>(req->data);

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        Local<Value> argv[2] = { NanNull(), NanNewBufferHandle((char*)closure->result.data(), closure->result.size()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->im->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}

NAN_METHOD(Image::view)
{
    NanScope();

    if ( (args.Length() != 4) || (!args[0]->IsNumber() && !args[1]->IsNumber() && !args[2]->IsNumber() && !args[3]->IsNumber() )) {
        NanThrowTypeError("requires 4 integer arguments: x, y, width, height");
        NanReturnUndefined();
    }

    // TODO parse args
    unsigned x = args[0]->IntegerValue();
    unsigned y = args[1]->IntegerValue();
    unsigned w = args[2]->IntegerValue();
    unsigned h = args[3]->IntegerValue();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    NanReturnValue(ImageView::New(im,x,y,w,h));
}

NAN_METHOD(Image::save)
{
    NanScope();

    if (args.Length() == 0 || !args[0]->IsString()){
        NanThrowTypeError("filename required");
        NanReturnUndefined();
    }

    std::string filename = TOSTR(args[0]);

    std::string format("");

    if (args.Length() >= 2) {
        if (!args[1]->IsString()) {
            NanThrowTypeError("both 'filename' and 'format' arguments must be strings");
            NanReturnUndefined();
        }
        format = TOSTR(args[1]);
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            NanThrowError(s.str().c_str());
            NanReturnUndefined();
        }
    }

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    try
    {
        mapnik::save_to_file<mapnik::image_data_32>(im->get()->data(),filename, format);
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

typedef struct {
    uv_work_t request;
    Image* im1;
    Image* im2;
    mapnik::composite_mode_e mode;
    int dx;
    int dy;
    float opacity;
    std::vector<mapnik::filter::filter_type> filters;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} composite_image_baton_t;

NAN_METHOD(Image::composite)
{
    NanScope();

    if (args.Length() < 1){
        NanThrowTypeError("requires at least one argument: an image mask");
        NanReturnUndefined();
    }

    if (!args[0]->IsObject()) {
        NanThrowTypeError("first argument must be an image mask");
        NanReturnUndefined();
    }

    Local<Object> im2 = args[0].As<Object>();
    if (im2->IsNull() || im2->IsUndefined() || !NanNew(Image::constructor)->HasInstance(im2))
    {
        NanThrowTypeError("mapnik.Image expected as first arg");
        NanReturnUndefined();
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!args[args.Length()-1]->IsFunction())
    {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    try
    {
        mapnik::composite_mode_e mode = mapnik::src_over;
        float opacity = 1.0;
        std::vector<mapnik::filter::filter_type> filters;
        int dx = 0;
        int dy = 0;
        if (args.Length() >= 2) {
            if (!args[1]->IsObject())
            {
                NanThrowTypeError("optional second arg must be an options object");
                NanReturnUndefined();
            }

            Local<Object> options = args[1].As<Object>();

            if (options->Has(NanNew("comp_op")))
            {
                Local<Value> opt = options->Get(NanNew("comp_op"));
                if (!opt->IsNumber()) {
                    NanThrowTypeError("comp_op must be a mapnik.compositeOp value");
                    NanReturnUndefined();
                }
                mode = static_cast<mapnik::composite_mode_e>(opt->IntegerValue());
            }

            if (options->Has(NanNew("opacity")))
            {
                Local<Value> opt = options->Get(NanNew("opacity"));
                if (!opt->IsNumber()) {
                    NanThrowTypeError("opacity must be a floating point number");
                    NanReturnUndefined();
                }
                opacity = opt->NumberValue();
            }

            if (options->Has(NanNew("dx")))
            {
                Local<Value> opt = options->Get(NanNew("dx"));
                if (!opt->IsNumber()) {
                    NanThrowTypeError("dx must be an integer");
                    NanReturnUndefined();
                }
                dx = opt->IntegerValue();
            }

            if (options->Has(NanNew("dy")))
            {
                Local<Value> opt = options->Get(NanNew("dy"));
                if (!opt->IsNumber()) {
                    NanThrowTypeError("dy must be an integer");
                    NanReturnUndefined();
                }
                dy = opt->IntegerValue();
            }

            if (options->Has(NanNew("image_filters")))
            {
                Local<Value> opt = options->Get(NanNew("image_filters"));
                if (!opt->IsString()) {
                    NanThrowTypeError("image_filters argument must string of filter names");
                    NanReturnUndefined();
                }
                std::string filter_str = TOSTR(opt);
                bool result = mapnik::filter::parse_image_filters(filter_str, filters);
                if (!result)
                {
                    NanThrowTypeError("could not parse image_filters");
                    NanReturnUndefined();
                }
            }
        }

        composite_image_baton_t *closure = new composite_image_baton_t();
        closure->request.data = closure;
        closure->im1 = node::ObjectWrap::Unwrap<Image>(args.Holder());
        closure->im2 = node::ObjectWrap::Unwrap<Image>(im2);
        closure->mode = mode;
        closure->opacity = opacity;
        closure->filters = filters;
        closure->dx = dx;
        closure->dy = dy;
        closure->error = false;
        NanAssignPersistent(closure->cb, callback.As<Function>());
        uv_queue_work(uv_default_loop(), &closure->request, EIO_Composite, (uv_after_work_cb)EIO_AfterComposite);
        closure->im1->Ref();
        closure->im2->Ref();
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

void Image::EIO_Composite(uv_work_t* req)
{
    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    try
    {
        if (closure->filters.size() > 0)
        {
            mapnik::filter::filter_visitor<mapnik::image_32> visitor(*closure->im2->this_);
            for (mapnik::filter::filter_type const& filter_tag : closure->filters)
            {
                MAPNIK_APPLY_VISITOR(visitor, filter_tag);
            }
        }
        mapnik::composite(closure->im1->this_->data(),closure->im2->this_->data(), closure->mode, closure->opacity, closure->dx, closure->dy);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterComposite(uv_work_t* req)
{
    NanScope();

    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    } else {
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->im1) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->im1->Unref();
    closure->im2->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}
