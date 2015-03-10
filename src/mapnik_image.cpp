
// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>             // for image_any
#include <mapnik/image_reader.hpp>      // for get_image_reader, etc
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc
#include <mapnik/image_copy.hpp>

#include <mapnik/image_compositing.hpp>
#include <mapnik/image_filter_types.hpp>
#include <mapnik/image_filter.hpp> // filter_visitor

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_palette.hpp"
#include "mapnik_color.hpp"

#include "utils.hpp"

// boost
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
    NODE_SET_PROTOTYPE_METHOD(lcons, "fillSync", fillSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "fill", fill);
    NODE_SET_PROTOTYPE_METHOD(lcons, "premultiplySync", premultiplySync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "premultiply", premultiply);
    NODE_SET_PROTOTYPE_METHOD(lcons, "premultiplied", premultiplied);
    NODE_SET_PROTOTYPE_METHOD(lcons, "demultiplySync", demultiplySync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "demultiply", demultiply);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clearSync", clearSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "compare", compare);
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolid", isSolid);
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolidSync", isSolidSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "copy", copy);
    NODE_SET_PROTOTYPE_METHOD(lcons, "copySync", copySync);
    
    // properties
    ATTR(lcons, "scaling", get_scaling, set_scaling);
    ATTR(lcons, "offset", get_offset, set_offset);

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

Image::Image(unsigned int width, unsigned int height, mapnik::image_dtype type, bool initialized, bool premultiplied, bool painted) :
    node::ObjectWrap(),
    this_(std::make_shared<mapnik::image_any>(width,height,type,initialized,premultiplied,painted))
{
    NanAdjustExternalMemory(this_->getSize());
}

Image::Image(image_ptr _this) :
    node::ObjectWrap(),
    this_(_this)
{
    NanAdjustExternalMemory(this_->getSize());
}

Image::~Image()
{
    NanAdjustExternalMemory(-this_->getSize());
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

    if (args.Length() >= 2)
    {
        mapnik::image_dtype type = mapnik::image_dtype_rgba8;
        bool initialize = true;
        bool premultiplied = false;
        bool painted = false;
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
        {
            NanThrowTypeError("Image 'width' and 'height' must be a integers");
            NanReturnUndefined();
        }
        if (args.Length() >= 3)
        {
            if (args[2]->IsNumber())
            {
                type = static_cast<mapnik::image_dtype>(args[2]->IntegerValue());
                if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
                {
                    NanThrowTypeError("Image 'type' must be a valid image type");
                    NanReturnUndefined();
                }
            }
            else
            {
                NanThrowTypeError("Image 'type' must be a valid image type");
                NanReturnUndefined();
            }
        }
        if (args.Length() >= 4)
        {
            if (args[3]->IsObject())
            {
                Local<Object> options = Local<Object>::Cast(args[3]);
                if (options->Has(NanNew("intialize")))
                {
                    Local<Value> init_val = options->Get(NanNew("intialize"));
                    if (!init_val.IsEmpty() && init_val->IsBoolean())
                    {
                        initialize = init_val->BooleanValue();
                    }
                    else
                    {
                        NanThrowTypeError("intialize option must be a boolean");
                        NanReturnUndefined();
                    }
                }

                if (options->Has(NanNew("premultiplied")))
                {
                    Local<Value> pre_val = options->Get(NanNew("premultiplied"));
                    if (!pre_val.IsEmpty() && pre_val->IsBoolean())
                    {
                        premultiplied = pre_val->BooleanValue();
                    }
                    else
                    {
                        NanThrowTypeError("premulitplied option must be a boolean");
                        NanReturnUndefined();
                    }
                }

                if (options->Has(NanNew("painted")))
                {
                    Local<Value> painted_val = options->Get(NanNew("painted"));
                    if (!painted_val.IsEmpty() && painted_val->IsBoolean())
                    {
                        painted = painted_val->BooleanValue();
                    }
                    else
                    {
                        NanThrowTypeError("painted option must be a boolean");
                        NanReturnUndefined();
                    }
                }
            }
            else
            {
                NanThrowTypeError("Options parameter must be an object");
                NanReturnUndefined();
            }
        }
        
        Image* im = new Image(args[0]->IntegerValue(),
                              args[1]->IntegerValue(),
                              type,
                              initialize,
                              premultiplied,
                              painted);
        im->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else
    {
        NanThrowError("please provide at least Image width and height");
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

struct visitor_get_pixel
{
    visitor_get_pixel(int x, int y)
        : x_(x), y_(y) {}

    Local<Value> operator() (mapnik::image_null const&)
    {
        // This should never be reached because the width and height of 0 for a null
        // image will prevent the visitor from being called.
        /* LCOV_EXCL_START */
        NanEscapableScope();
        return NanEscapeScope(NanUndefined());
        /* LCOV_EXCL_END */

    }

    Local<Value> operator() (mapnik::image_gray8 const& data)
    {
        NanEscapableScope();
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Uint32>(val));
    }

    Local<Value> operator() (mapnik::image_gray8s const& data)
    {
        NanEscapableScope();
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Int32>(val));
    }

    Local<Value> operator() (mapnik::image_gray16 const& data)
    {
        NanEscapableScope();
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Uint32>(val));
    }

    Local<Value> operator() (mapnik::image_gray16s const& data)
    {
        NanEscapableScope();
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Int32>(val));
    }

    Local<Value> operator() (mapnik::image_gray32 const& data)
    {
        NanEscapableScope();
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Uint32>(val));
    }
    
    Local<Value> operator() (mapnik::image_gray32s const& data)
    {
        NanEscapableScope();
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Int32>(val));
    }

    Local<Value> operator() (mapnik::image_gray32f const& data)
    {
        NanEscapableScope();
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

    Local<Value> operator() (mapnik::image_gray64 const& data)
    {
        NanEscapableScope();
        std::uint64_t val = mapnik::get_pixel<std::uint64_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

    Local<Value> operator() (mapnik::image_gray64s const& data)
    {
        NanEscapableScope();
        std::int64_t val = mapnik::get_pixel<std::int64_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

    Local<Value> operator() (mapnik::image_gray64f const& data)
    {
        NanEscapableScope();
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

    Local<Value> operator() (mapnik::image_rgba8 const& data)
    {
        NanEscapableScope();
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

  private:
    int x_;
    int y_;
        
};

NAN_METHOD(Image::getPixel)
{
    NanScope();
    int x = 0;
    int y = 0;
    bool get_color = false;
    if (args.Length() >= 3) {

        if (!args[2]->IsObject()) {
            NanThrowTypeError("optional third argument must be an options object");
            NanReturnUndefined();
        }

        Local<Object> options = args[2]->ToObject();

        if (options->Has(NanNew("get_color"))) {
            Local<Value> bind_opt = options->Get(NanNew("get_color"));
            if (!bind_opt->IsBoolean()) {
                NanThrowTypeError("optional arg 'color' must be a boolean");
                NanReturnUndefined();
            }
            get_color = bind_opt->BooleanValue();
        }

    }

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
    if (x >= 0 && x < static_cast<int>(im->this_->width())
        && y >= 0 && y < static_cast<int>(im->this_->height()))
    {
        if (get_color)
        {
            mapnik::color val = mapnik::get_pixel<mapnik::color>(*im->this_, x, y);
            NanReturnValue(Color::NewInstance(val));
        } else {
            visitor_get_pixel visitor(x, y);
            NanReturnValue(mapnik::util::apply_visitor(visitor, *im->this_));
        }
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
    int x = args[0]->IntegerValue();
    int y = args[1]->IntegerValue();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    if (x < 0 || x >= static_cast<int>(im->this_->width()) || y < 0 || y >= static_cast<int>(im->this_->height()))
    {
        NanThrowTypeError("invalid pixel requested");
        NanReturnUndefined();
    }
    if (args[2]->IsUint32())
    {
        std::uint32_t val = args[2]->Uint32Value();
        mapnik::set_pixel<std::uint32_t>(*im->this_,x,y,val);
    }
    else if (args[2]->IsInt32())
    {
        std::int32_t val = args[2]->Int32Value();
        mapnik::set_pixel<std::int32_t>(*im->this_,x,y,val);
    }
    else if (args[2]->IsNumber())
    {
        double val = args[2]->NumberValue();
        mapnik::set_pixel<double>(*im->this_,x,y,val);
    }
    else if (args[2]->IsObject())
    {
        Local<Object> obj = args[2]->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !NanNew(Color::constructor)->HasInstance(obj)) 
        {
            NanThrowTypeError("A numeric or color value is expected as third arg");
        }
        else 
        {
            Color * color = node::ObjectWrap::Unwrap<Color>(obj);
            mapnik::set_pixel(*im->this_,x,y,*(color->get()));
        }
    }
    else
    {
        NanThrowTypeError("A numeric or color value is expected as third arg");
    }
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

    int threshold = 16;
    unsigned alpha = true;

    if (args.Length() > 1) {

        if (!args[1]->IsObject()) {
            NanThrowTypeError("optional second argument must be an options object");
            NanReturnUndefined();
        }

        Local<Object> options = args[1]->ToObject();

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
    unsigned difference = mapnik::compare(*im->this_, *im2->this_, threshold, alpha);
    NanReturnValue(NanNew<Integer>(difference));
}

NAN_METHOD(Image::fillSync)
{
    NanScope();
    NanReturnValue(_fillSync(args));
}

Local<Value> Image::_fillSync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    if (args.Length() < 1 ) {
        NanThrowTypeError("expects one argument: Color object or a number");
        return NanEscapeScope(NanUndefined());
    }
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    try
    {
        if (args[0]->IsUint32())
        {
            std::uint32_t val = args[0]->Uint32Value();
            mapnik::fill<std::uint32_t>(*im->this_,val);
        }
        else if (args[0]->IsInt32())
        {
            std::int32_t val = args[0]->Int32Value();
            mapnik::fill<std::int32_t>(*im->this_,val);
        }
        else if (args[0]->IsNumber())
        {
            double val = args[0]->NumberValue();
            mapnik::fill<double>(*im->this_,val);
        }
        else if (args[0]->IsObject())
        {
            Local<Object> obj = args[0]->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(Color::constructor)->HasInstance(obj)) 
            {
                NanThrowTypeError("A numeric or color value is expected");
            }
            else 
            {
                Color * color = node::ObjectWrap::Unwrap<Color>(obj);
                mapnik::fill(*im->this_,*(color->get()));
            }
        }
        else
        {
            NanThrowTypeError("A numeric or color value is expected");
        }
    }
    catch(std::exception const& ex)
    {
        NanThrowError(ex.what());
    }
    return NanEscapeScope(NanUndefined());
}

enum fill_type : std::uint8_t
{
    FILL_COLOR = 0,
    FILL_UINT32,
    FILL_INT32,
    FILL_DOUBLE
};

typedef struct {
    uv_work_t request;
    Image* im;
    fill_type type;
    mapnik::color c;
    std::uint32_t val_u32;
    std::int32_t val_32;
    double val_double;
    //std::string format;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} fill_image_baton_t;

NAN_METHOD(Image::fill)
{
    NanScope();
    if (args.Length() <= 1) {
        NanReturnValue(_fillSync(args));
    }
    
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    fill_image_baton_t *closure = new fill_image_baton_t();
    if (args[0]->IsUint32())
    {
        closure->val_u32 = args[0]->Uint32Value();
        closure->type = FILL_UINT32;
    }
    else if (args[0]->IsInt32())
    {
        closure->val_32 = args[0]->Int32Value();
        closure->type = FILL_INT32;
    }
    else if (args[0]->IsNumber())
    {
        closure->val_double = args[0]->NumberValue();
        closure->type = FILL_DOUBLE;
    }
    else if (args[0]->IsObject())
    {
        Local<Object> obj = args[0]->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !NanNew(Color::constructor)->HasInstance(obj)) 
        {
            NanThrowTypeError("A numeric or color value is expected");
            NanReturnUndefined();
        }
        else 
        {
            Color * color = node::ObjectWrap::Unwrap<Color>(obj);
            closure->c = *(color->get());
        }
    }
    else
    {
        NanThrowTypeError("A numeric or color value is expected");
        NanReturnUndefined();
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Fill, (uv_after_work_cb)EIO_AfterFill);
    im->Ref();
    NanReturnUndefined();
}

void Image::EIO_Fill(uv_work_t* req)
{
    fill_image_baton_t *closure = static_cast<fill_image_baton_t *>(req->data);
    try
    {
        switch (closure->type)
        {
            case FILL_UINT32:
                mapnik::fill(*closure->im->this_, closure->val_u32);
                break;
            case FILL_INT32:
                mapnik::fill(*closure->im->this_, closure->val_32);
                break;
            default:
            case FILL_DOUBLE:
                mapnik::fill(*closure->im->this_, closure->val_double);
                break;
            case FILL_COLOR:
                mapnik::fill(*closure->im->this_,closure->c);
                break;
        }
    }
    catch(std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterFill(uv_work_t* req)
{
    NanScope();
    fill_image_baton_t *closure = static_cast<fill_image_baton_t *>(req->data);
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


NAN_METHOD(Image::clearSync)
{
    NanScope();
    NanReturnValue(_clearSync(args));
}

Local<Value> Image::_clearSync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    try
    {
        mapnik::fill(*im->this_, 0);
    }
    catch(std::exception const& ex)
    {
        NanThrowError(ex.what());
    }
    return NanEscapeScope(NanUndefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    //std::string format;
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
        mapnik::fill(*closure->im->this_, 0);
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

NAN_METHOD(Image::setGrayScaleToAlpha)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    if (args.Length() == 0) {
        mapnik::set_grayscale_to_alpha(*im->this_);
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
        mapnik::set_grayscale_to_alpha(*im->this_, *color->get());
    }

    NanReturnUndefined();
}

typedef struct {
    uv_work_t request;
    Image* im;
    Persistent<Function> cb;
} image_op_baton_t;

NAN_METHOD(Image::premultiplied)
{
    NanScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    bool premultiplied = im->this_->get_premultiplied();
    NanReturnValue(NanNew<Boolean>(premultiplied));
}

NAN_METHOD(Image::premultiplySync)
{
    NanScope();
    NanReturnValue(_premultiplySync(args));
}

Local<Value> Image::_premultiplySync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    mapnik::premultiply_alpha(*im->this_);
    return NanEscapeScope(NanUndefined());
}

NAN_METHOD(Image::premultiply)
{
    NanScope();
    if (args.Length() == 0) {
        NanReturnValue(_premultiplySync(args));
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Premultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    NanReturnUndefined();
}

void Image::EIO_Premultiply(uv_work_t* req)
{
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    mapnik::premultiply_alpha(*closure->im->this_);
}

void Image::EIO_AfterMultiply(uv_work_t* req)
{
    NanScope();
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->im) };
    NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
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
    mapnik::demultiply_alpha(*im->this_);
    return NanEscapeScope(NanUndefined());
}

NAN_METHOD(Image::demultiply)
{
    NanScope();
    if (args.Length() == 0) {
        NanReturnValue(_demultiplySync(args));
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Demultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    NanReturnUndefined();
}

void Image::EIO_Demultiply(uv_work_t* req)
{
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    mapnik::demultiply_alpha(*closure->im->this_);
}

typedef struct {
    uv_work_t request;
    Image* im;
    Persistent<Function> cb;
    bool error;
    std::string error_name;
    bool result;
} is_solid_image_baton_t;

NAN_METHOD(Image::isSolid)
{
    NanScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());

    if (args.Length() == 0) {
        NanReturnValue(_isSolidSync(args));
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    is_solid_image_baton_t *closure = new is_solid_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->result = true;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    im->Ref();
    NanReturnUndefined();
}

void Image::EIO_IsSolid(uv_work_t* req)
{
    is_solid_image_baton_t *closure = static_cast<is_solid_image_baton_t *>(req->data);
    if (closure->im->this_->width() > 0 && closure->im->this_->height() > 0)
    {
        closure->result = mapnik::is_solid(*(closure->im->this_));
    }
    else
    {
        closure->error = true;
        closure->error_name = "image does not have valid dimensions";
    }
}

void Image::EIO_AfterIsSolid(uv_work_t* req)
{
    NanScope();
    is_solid_image_baton_t *closure = static_cast<is_solid_image_baton_t *>(req->data);
    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            Local<Value> argv[3] = { NanNull(),
                                     NanNew(closure->result),
                                     mapnik::util::apply_visitor(visitor_get_pixel(0,0),*(closure->im->this_)),
            };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 3, argv);
        }
        else
        {
            Local<Value> argv[2] = { NanNull(), NanNew(closure->result) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
    }
    closure->im->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}


NAN_METHOD(Image::isSolidSync)
{
    NanScope();
    NanReturnValue(_isSolidSync(args));
}

Local<Value> Image::_isSolidSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    if (im->this_->width() > 0 && im->this_->height() > 0)
    {
        return NanEscapeScope(NanNew<Boolean>(mapnik::is_solid(*(im->this_))));
    }
    NanThrowError("image does not have valid dimensions");
    return NanEscapeScope(NanUndefined());
}

typedef struct {
    uv_work_t request;
    Image* im1;
    std::shared_ptr<mapnik::image_any> im2;
    mapnik::image_dtype type;  
    double offset;
    double scaling;
    Persistent<Function> cb;
    bool error;
    std::string error_name;
} copy_image_baton_t;

NAN_METHOD(Image::copy)
{
    NanScope();

    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanReturnValue(_copySync(args));
    }
    
    Image* im1 = node::ObjectWrap::Unwrap<Image>(args.Holder());
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = im1->this_->get_dtype();
    Local<Object> options = NanNew<Object>();
    
    if (args.Length() >= 2)
    {
        if (args[0]->IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(args[0]->IntegerValue());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                NanThrowTypeError("Image 'type' must be a valid image type");
                NanReturnUndefined();
            }
        }
        else if (args[0]->IsObject())
        {
            options = args[0]->ToObject();
        }
        else
        {
            NanThrowTypeError("Unknown parameters passed");
            NanReturnUndefined();
        }
    }
    if (args.Length() >= 3)
    {
        if (args[1]->IsObject())
        {
            options = args[1]->ToObject();
        }
        else
        {
            NanThrowTypeError("Expected options object as second argument");
            NanReturnUndefined();
        }
    }
    
    if (options->Has(NanNew("scaling")))
    {
        Local<Value> scaling_val = options->Get(NanNew("scaling"));
        if (scaling_val->IsNumber())
        {
            scaling = scaling_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            NanThrowTypeError("scaling argument must be a number");
            NanReturnUndefined();
        }
    }
    
    if (options->Has(NanNew("offset")))
    {
        Local<Value> offset_val = options->Get(NanNew("offset"));
        if (offset_val->IsNumber())
        {
            offset = offset_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            NanThrowTypeError("offset argument must be a number");
            NanReturnUndefined();
        }
    }

    if (!scaling_or_offset_set && type == im1->this_->get_dtype())
    {
        scaling = im1->this_->get_scaling();
        offset = im1->this_->get_offset();
    }
    
    copy_image_baton_t *closure = new copy_image_baton_t();
    closure->request.data = closure;
    closure->im1 = im1;
    closure->offset = offset;
    closure->scaling = scaling;
    closure->type = type;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Copy, (uv_after_work_cb)EIO_AfterCopy);
    im1->Ref();
    NanReturnUndefined();
}

void Image::EIO_Copy(uv_work_t* req)
{
    copy_image_baton_t *closure = static_cast<copy_image_baton_t *>(req->data);
    try
    {
        closure->im2 = std::make_shared<mapnik::image_any>(
                               std::move(mapnik::image_copy(*(closure->im1->this_), 
                                                            closure->type, 
                                                            closure->offset, 
                                                            closure->scaling)
                               ));
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterCopy(uv_work_t* req)
{
    NanScope();
    copy_image_baton_t *closure = static_cast<copy_image_baton_t *>(req->data);
    if (closure->error || !closure->im1)
    {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im2);
        Handle<Value> ext = NanNew<External>(im);
        Local<Object> image_obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(ObjectWrap::Unwrap<Image>(image_obj)) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }
    closure->im1->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}


NAN_METHOD(Image::copySync)
{
    NanScope();
    NanReturnValue(_copySync(args));
}

Local<Value> Image::_copySync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = im->this_->get_dtype();
    Local<Object> options = NanNew<Object>();
    if (args.Length() >= 1)
    {
        if (args[0]->IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(args[0]->IntegerValue());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                NanThrowTypeError("Image 'type' must be a valid image type");
                return NanEscapeScope(NanUndefined());
            }
        }
        else if (args[0]->IsObject())
        {
            options = args[0]->ToObject();
        }
        else
        {
            NanThrowTypeError("Unknown parameters passed");
            return NanEscapeScope(NanUndefined());
        }
    }
    if (args.Length() >= 2)
    {
        if (args[1]->IsObject())
        {
            options = args[1]->ToObject();
        }
        else
        {
            NanThrowTypeError("Expected options object as second argument");
            return NanEscapeScope(NanUndefined());
        }
    }
    
    if (options->Has(NanNew("scaling")))
    {
        Local<Value> scaling_val = options->Get(NanNew("scaling"));
        if (scaling_val->IsNumber())
        {
            scaling = scaling_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            NanThrowTypeError("scaling argument must be a number");
            return NanEscapeScope(NanUndefined());
        }
    }
    
    if (options->Has(NanNew("offset")))
    {
        Local<Value> offset_val = options->Get(NanNew("offset"));
        if (offset_val->IsNumber())
        {
            offset = offset_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            NanThrowTypeError("offset argument must be a number");
            return NanEscapeScope(NanUndefined());
        }
    }

    if (!scaling_or_offset_set && type == im->this_->get_dtype())
    {
        scaling = im->this_->get_scaling();
        offset = im->this_->get_offset();
    }

    try
    {
        std::shared_ptr<mapnik::image_any> image_ptr = std::make_shared<mapnik::image_any>(
                                               std::move(mapnik::image_copy(*(im->this_), 
                                                                            type, 
                                                                            offset, 
                                                                            scaling)
                                               ));
        Image* im = new Image(image_ptr);
        Handle<Value> ext = NanNew<External>(im);
        Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
        return NanEscapeScope(obj);
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        return NanEscapeScope(NanUndefined());
    }
}

NAN_METHOD(Image::painted)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    NanReturnValue(NanNew<Boolean>(im->this_->painted()));
}

NAN_METHOD(Image::width)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    NanReturnValue(NanNew<Int32>(static_cast<std::int32_t>(im->this_->width())));
}

NAN_METHOD(Image::height)
{
    NanScope();

    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    NanReturnValue(NanNew<Int32>(static_cast<std::int32_t>(im->this_->height())));
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
            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(filename,*type));
            if (reader.get())
            {
                std::shared_ptr<mapnik::image_any> image_ptr = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(), reader->height()));
                Image* im = new Image(image_ptr);
                Handle<Value> ext = NanNew<External>(im);
                Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
                return NanEscapeScope(obj);
            }
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
    Persistent<Object> buffer;
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
            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(closure->filename,*type));
            if (reader.get())
            {
                closure->im = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
            }
            else
            {
                // The only way this is ever reached is if the reader factory in 
                // mapnik was not providing an image type it should. This should never
                // be occuring so marking this out from coverage
                /* LCOV_EXCL_START */
                closure->error = true;
                closure->error_name = "Failed to load: " + closure->filename;
                /* LCOV_EXCL_END */
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
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        NanThrowTypeError("first argument is invalid, must be a Buffer");
        return NanEscapeScope(NanUndefined());
    }

    try
    {
        std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(node::Buffer::Data(obj),node::Buffer::Length(obj)));
        if (reader.get())
        {
            std::shared_ptr<mapnik::image_any> image_ptr = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
            Image* im = new Image(image_ptr);
            Handle<Value> ext = NanNew<External>(im);
            return NanEscapeScope(NanNew(constructor)->GetFunction()->NewInstance(1, &ext));
        }
        // The only way this is ever reached is if the reader factory in 
        // mapnik was not providing an image type it should. This should never
        // be occuring so marking this out from coverage
        /* LCOV_EXCL_START */
        NanThrowTypeError("Failed to load from buffer");
        return NanEscapeScope(NanUndefined());
        /* LCOV_EXCL_END */
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
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        NanThrowTypeError("first argument is invalid, must be a Buffer");
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
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    NanAssignPersistent(closure->buffer, obj.As<Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromBytes, (uv_after_work_cb)EIO_AfterFromBytes);
    NanReturnUndefined();
}

void Image::EIO_FromBytes(uv_work_t* req)
{
    image_mem_ptr_baton_t *closure = static_cast<image_mem_ptr_baton_t *>(req->data);

    try
    {
        std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(closure->data,closure->dataLength));
        if (reader.get())
        {
            closure->im = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
        }
        else
        {
            // The only way this is ever reached is if the reader factory in 
            // mapnik was not providing an image type it should. This should never
            // be occuring so marking this out from coverage
            /* LCOV_EXCL_START */
            closure->error = true;
            closure->error_name = "Failed to load from buffer";
            /* LCOV_EXCL_END */
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
    NanDisposePersistent(closure->buffer);
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
    NanReturnValue(ImageView::NewInstance(im,x,y,w,h));
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
        mapnik::save_to_file(*(im->this_),filename, format);
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
    NanReturnUndefined();
}

void Image::EIO_Composite(uv_work_t* req)
{
    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    try
    {
        if (closure->filters.size() > 0)
        {
            mapnik::filter::filter_visitor<mapnik::image_any> visitor(*closure->im2->this_);
            for (mapnik::filter::filter_type const& filter_tag : closure->filters)
            {
                mapnik::util::apply_visitor(visitor, filter_tag);
            }
        }
        bool demultiply_im1 = mapnik::premultiply_alpha(*closure->im1->this_);
        bool demultiply_im2 = mapnik::premultiply_alpha(*closure->im2->this_);
        mapnik::composite(*closure->im1->this_,*closure->im2->this_, closure->mode, closure->opacity, closure->dx, closure->dy);
        if (demultiply_im1) 
        {
            mapnik::demultiply_alpha(*closure->im1->this_);
        }
        if (demultiply_im2)
        {
            mapnik::demultiply_alpha(*closure->im2->this_);
        }
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

NAN_GETTER(Image::get_scaling)
{
    NanScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    NanReturnValue(NanNew<Number>(im->this_->get_scaling()));
}

NAN_GETTER(Image::get_offset)
{
    NanScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    NanReturnValue(NanNew<Number>(im->this_->get_offset()));
}

NAN_SETTER(Image::set_scaling)
{
    NanScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    if (!value->IsNumber())
    {
        NanThrowError("Must provide a number");
    } 
    else 
    {
        double val = value->NumberValue();
        if (val == 0.0)
        {
            NanThrowError("Scaling value can not be zero");
            return;
        }
        im->this_->set_scaling(val);
    }
}

NAN_SETTER(Image::set_offset)
{
    NanScope();
    Image* im = node::ObjectWrap::Unwrap<Image>(args.Holder());
    if (!value->IsNumber())
    {
        NanThrowError("Must provide a number");
    } 
    else 
    {
        double val = value->NumberValue();
        im->this_->set_offset(val);
    }
}
