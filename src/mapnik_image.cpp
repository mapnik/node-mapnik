
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
#include <mapnik/image_scaling.hpp>

#include <mapnik/marker.hpp>
#include <mapnik/marker_cache.hpp>
#include <mapnik/svg/svg_parser.hpp>
#include <mapnik/svg/svg_storage.hpp>
#include <mapnik/svg/svg_converter.hpp>
#include <mapnik/svg/svg_path_adapter.hpp>
#include <mapnik/svg/svg_path_attributes.hpp>
#include <mapnik/svg/svg_path_adapter.hpp>
#include <mapnik/svg/svg_renderer_agg.hpp>
#include <mapnik/svg/svg_path_attributes.hpp>

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_palette.hpp"
#include "mapnik_color.hpp"

#include "utils.hpp"

#include "agg_rasterizer_scanline_aa.h"
#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_renderer_base.h"
#include "agg_pixfmt_rgba.h"
#include "agg_scanline_u.h"

// boost
#include <boost/optional/optional.hpp>

// std
#include <exception>
#include <ostream>                      // for operator<<, basic_ostream
#include <sstream>                      // for basic_ostringstream, etc
#include <cstdlib>

Nan::Persistent<v8::FunctionTemplate> Image::constructor;

/**
 * @name mapnik.Image
 * @class
 * @param {number} width
 * @param {number} height
 * @param {Object} options valid options are `premultiplied`, `painted`,
 * `type` and `initialize`.
 * @throws {TypeError} if any argument is the wrong type, like if width
 * or height is not numeric.
 * @example
 * var im = new mapnik.Image(256, 256, {
 *   premultiplied: true,
 *   type: mapnik.imageType.gray8
 * });
 */
void Image::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Image::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Image").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "getType", getType);
    Nan::SetPrototypeMethod(lcons, "getPixel", getPixel);
    Nan::SetPrototypeMethod(lcons, "setPixel", setPixel);
    Nan::SetPrototypeMethod(lcons, "encodeSync", encodeSync);
    Nan::SetPrototypeMethod(lcons, "encode", encode);
    Nan::SetPrototypeMethod(lcons, "view", view);
    Nan::SetPrototypeMethod(lcons, "saveSync", saveSync);
    Nan::SetPrototypeMethod(lcons, "save", save);
    Nan::SetPrototypeMethod(lcons, "setGrayScaleToAlpha", setGrayScaleToAlpha);
    Nan::SetPrototypeMethod(lcons, "width", width);
    Nan::SetPrototypeMethod(lcons, "height", height);
    Nan::SetPrototypeMethod(lcons, "painted", painted);
    Nan::SetPrototypeMethod(lcons, "composite", composite);
    Nan::SetPrototypeMethod(lcons, "filter", filter);
    Nan::SetPrototypeMethod(lcons, "filterSync", filterSync);
    Nan::SetPrototypeMethod(lcons, "fillSync", fillSync);
    Nan::SetPrototypeMethod(lcons, "fill", fill);
    Nan::SetPrototypeMethod(lcons, "premultiplySync", premultiplySync);
    Nan::SetPrototypeMethod(lcons, "premultiply", premultiply);
    Nan::SetPrototypeMethod(lcons, "premultiplied", premultiplied);
    Nan::SetPrototypeMethod(lcons, "demultiplySync", demultiplySync);
    Nan::SetPrototypeMethod(lcons, "demultiply", demultiply);
    Nan::SetPrototypeMethod(lcons, "clear", clear);
    Nan::SetPrototypeMethod(lcons, "clearSync", clearSync);
    Nan::SetPrototypeMethod(lcons, "compare", compare);
    Nan::SetPrototypeMethod(lcons, "isSolid", isSolid);
    Nan::SetPrototypeMethod(lcons, "isSolidSync", isSolidSync);
    Nan::SetPrototypeMethod(lcons, "copy", copy);
    Nan::SetPrototypeMethod(lcons, "copySync", copySync);
    Nan::SetPrototypeMethod(lcons, "resize", resize);
    Nan::SetPrototypeMethod(lcons, "resizeSync", resizeSync);
    Nan::SetPrototypeMethod(lcons, "data", data);
    
    // properties
    ATTR(lcons, "scaling", get_scaling, set_scaling);
    ATTR(lcons, "offset", get_offset, set_offset);

    // This *must* go after the ATTR setting
    Nan::SetMethod(lcons->GetFunction(),
                    "open",
                    Image::open);
    Nan::SetMethod(lcons->GetFunction(),
                    "fromBytes",
                    Image::fromBytes);
    Nan::SetMethod(lcons->GetFunction(),
                    "openSync",
                    Image::openSync);
    Nan::SetMethod(lcons->GetFunction(),
                    "fromBytesSync",
                    Image::fromBytesSync);
    Nan::SetMethod(lcons->GetFunction(),
                    "fromBufferSync",
                    Image::fromBufferSync);
    Nan::SetMethod(lcons->GetFunction(),
                    "fromSVG",
                    Image::fromSVG);
    Nan::SetMethod(lcons->GetFunction(),
                    "fromSVGSync",
                    Image::fromSVGSync);
    Nan::SetMethod(lcons->GetFunction(),
                    "fromSVGBytes",
                    Image::fromSVGBytes);
    Nan::SetMethod(lcons->GetFunction(),
                    "fromSVGBytesSync",
                    Image::fromSVGBytesSync);
    target->Set(Nan::New("Image").ToLocalChecked(),lcons->GetFunction());
    constructor.Reset(lcons);
}

Image::Image(unsigned int width, unsigned int height, mapnik::image_dtype type, bool initialized, bool premultiplied, bool painted) :
    Nan::ObjectWrap(),
    this_(std::make_shared<mapnik::image_any>(width,height,type,initialized,premultiplied,painted))
{
}

Image::Image(image_ptr _this) :
    Nan::ObjectWrap(),
    this_(_this)
{
}

Image::~Image()
{
}

NAN_METHOD(Image::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = info[0].As<v8::External>();
        void* ptr = ext->Value();
        Image* im =  static_cast<Image*>(ptr);
        im->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }

    if (info.Length() >= 2)
    {
        mapnik::image_dtype type = mapnik::image_dtype_rgba8;
        bool initialize = true;
        bool premultiplied = false;
        bool painted = false;
        if (!info[0]->IsNumber() || !info[1]->IsNumber())
        {
            Nan::ThrowTypeError("Image 'width' and 'height' must be a integers");
            return;
        }
        if (info.Length() >= 3)
        {
            if (info[2]->IsObject())
            {
                v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[2]);
                if (options->Has(Nan::New("type").ToLocalChecked()))
                {
                    v8::Local<v8::Value> init_val = options->Get(Nan::New("type").ToLocalChecked());

                    if (!init_val.IsEmpty() && init_val->IsNumber())
                    {
                        type = static_cast<mapnik::image_dtype>(init_val->IntegerValue());
                        if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
                        {
                            Nan::ThrowTypeError("Image 'type' must be a valid image type");
                            return;
                        }
                    }
                    else
                    {
                        Nan::ThrowTypeError("'type' option must be a valid 'mapnik.imageType'");
                        return;
                    }
                }

                if (options->Has(Nan::New("initialize").ToLocalChecked()))
                {
                    v8::Local<v8::Value> init_val = options->Get(Nan::New("initialize").ToLocalChecked());
                    if (!init_val.IsEmpty() && init_val->IsBoolean())
                    {
                        initialize = init_val->BooleanValue();
                    }
                    else
                    {
                        Nan::ThrowTypeError("initialize option must be a boolean");
                        return;
                    }
                }

                if (options->Has(Nan::New("premultiplied").ToLocalChecked()))
                {
                    v8::Local<v8::Value> pre_val = options->Get(Nan::New("premultiplied").ToLocalChecked());
                    if (!pre_val.IsEmpty() && pre_val->IsBoolean())
                    {
                        premultiplied = pre_val->BooleanValue();
                    }
                    else
                    {
                        Nan::ThrowTypeError("premultiplied option must be a boolean");
                        return;
                    }
                }

                if (options->Has(Nan::New("painted").ToLocalChecked()))
                {
                    v8::Local<v8::Value> painted_val = options->Get(Nan::New("painted").ToLocalChecked());
                    if (!painted_val.IsEmpty() && painted_val->IsBoolean())
                    {
                        painted = painted_val->BooleanValue();
                    }
                    else
                    {
                        Nan::ThrowTypeError("painted option must be a boolean");
                        return;
                    }
                }
            }
            else
            {
                Nan::ThrowTypeError("Options parameter must be an object");
                return;
            }
        }
        
        Image* im = new Image(info[0]->IntegerValue(),
                              info[1]->IntegerValue(),
                              type,
                              initialize,
                              premultiplied,
                              painted);
        im->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    else
    {
        Nan::ThrowError("please provide at least Image width and height");
        return;
    }
    return;
}

/**
 * Determine the image type
 *
 * @name getType
 * @instance
 * @returns {number} Number of the image type
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::getType)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    unsigned type = im->this_->get_dtype();
    info.GetReturnValue().Set(Nan::New<v8::Number>(type));
}

struct visitor_get_pixel
{
    visitor_get_pixel(int x, int y)
        : x_(x), y_(y) {}

    v8::Local<v8::Value> operator() (mapnik::image_null const&)
    {
        // This should never be reached because the width and height of 0 for a null
        // image will prevent the visitor from being called.
        /* LCOV_EXCL_START */
        Nan::EscapableHandleScope scope;
        return scope.Escape(Nan::Undefined());
        /* LCOV_EXCL_END */

    }

    v8::Local<v8::Value> operator() (mapnik::image_gray8 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Uint32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_gray8s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Int32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_gray16 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Uint32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_gray16s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Int32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_gray32 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Uint32>(val));
    }
    
    v8::Local<v8::Value> operator() (mapnik::image_gray32s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Int32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_gray32f const& data)
    {
        Nan::EscapableHandleScope scope;
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_gray64 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint64_t val = mapnik::get_pixel<std::uint64_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_gray64s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int64_t val = mapnik::get_pixel<std::int64_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_gray64f const& data)
    {
        Nan::EscapableHandleScope scope;
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_rgba8 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

  private:
    int x_;
    int y_;
        
};

/**
 * @name getPixel
 * @instance
 * @memberof mapnik.Image
 * @param {number} x position within image from top left
 * @param {number} y position within image from top left
 * @param {Object} options the only valid option is `get_color`, which
 * should be a `boolean`.
 * @returns {number} color
 * @example
 * var im = new mapnik.Image(256, 256);
 * var view = im.view(0, 0, 256, 256);
 * var pixel = view.getPixel(0, 0, {get_color:true});
 * assert.equal(pixel.r, 0);
 * assert.equal(pixel.g, 0);
 * assert.equal(pixel.b, 0);
 * assert.equal(pixel.a, 0);
 */
NAN_METHOD(Image::getPixel)
{
    int x = 0;
    int y = 0;
    bool get_color = false;
    if (info.Length() >= 3) {

        if (!info[2]->IsObject()) {
            Nan::ThrowTypeError("optional third argument must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[2]->ToObject();

        if (options->Has(Nan::New("get_color").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("get_color").ToLocalChecked());
            if (!bind_opt->IsBoolean()) {
                Nan::ThrowTypeError("optional arg 'color' must be a boolean");
                return;
            }
            get_color = bind_opt->BooleanValue();
        }

    }

    if (info.Length() >= 2) {
        if (!info[0]->IsNumber()) {
            Nan::ThrowTypeError("first arg, 'x' must be an integer");
            return;
        }
        if (!info[1]->IsNumber()) {
            Nan::ThrowTypeError("second arg, 'y' must be an integer");
            return;
        }
        x = info[0]->IntegerValue();
        y = info[1]->IntegerValue();
    } else {
        Nan::ThrowError("must supply x,y to query pixel color");
        return;
    }
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (x >= 0 && x < static_cast<int>(im->this_->width())
        && y >= 0 && y < static_cast<int>(im->this_->height()))
    {
        if (get_color)
        {
            mapnik::color val = mapnik::get_pixel<mapnik::color>(*im->this_, x, y);
            info.GetReturnValue().Set(Color::NewInstance(val));
        } else {
            visitor_get_pixel visitor(x, y);
            info.GetReturnValue().Set(mapnik::util::apply_visitor(visitor, *im->this_));
        }
    }
    return;
}

/**
 * @name setPixel
 * @instance
 * @memberof mapnik.Image
 * @param {number} x position within image from top left
 * @param {number} y position within image from top left
 * @param {Object|number} numeric or object representation of a color
 */
NAN_METHOD(Image::setPixel)
{
    if (info.Length() < 3 || (!info[0]->IsNumber() && !info[1]->IsNumber())) {
        Nan::ThrowTypeError("expects three arguments: x, y, and pixel value");
        return;
    }
    int x = info[0]->IntegerValue();
    int y = info[1]->IntegerValue();
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (x < 0 || x >= static_cast<int>(im->this_->width()) || y < 0 || y >= static_cast<int>(im->this_->height()))
    {
        Nan::ThrowTypeError("invalid pixel requested");
        return;
    }
    if (info[2]->IsUint32())
    {
        std::uint32_t val = info[2]->Uint32Value();
        mapnik::set_pixel<std::uint32_t>(*im->this_,x,y,val);
    }
    else if (info[2]->IsInt32())
    {
        std::int32_t val = info[2]->Int32Value();
        mapnik::set_pixel<std::int32_t>(*im->this_,x,y,val);
    }
    else if (info[2]->IsNumber())
    {
        double val = info[2]->NumberValue();
        mapnik::set_pixel<double>(*im->this_,x,y,val);
    }
    else if (info[2]->IsObject())
    {
        v8::Local<v8::Object> obj = info[2]->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Color::constructor)->HasInstance(obj)) 
        {
            Nan::ThrowTypeError("A numeric or color value is expected as third arg");
        }
        else 
        {
            Color * color = Nan::ObjectWrap::Unwrap<Color>(obj);
            mapnik::set_pixel(*im->this_,x,y,*(color->get()));
        }
    }
    else
    {
        Nan::ThrowTypeError("A numeric or color value is expected as third arg");
    }
    return;
}

/**
 * Compare two images visually. This is useful for algorithms and tests that
 * confirm whether a certain image has changed significantly.
 *
 * @name compare
 * @instance
 * @memberof mapnik.Image
 * @param {mapnik.Image} other another image instance
 * @param {Object} [options={threshold:16,alpha:true}]
 * @returns {number} quantified visual difference between these two images
 */
NAN_METHOD(Image::compare)
{
    if (info.Length() < 1 || !info[0]->IsObject()) {
        Nan::ThrowTypeError("first argument should be a mapnik.Image");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Image::constructor)->HasInstance(obj)) {
        Nan::ThrowTypeError("mapnik.Image expected as first arg");
        return;
    }

    int threshold = 16;
    unsigned alpha = true;

    if (info.Length() > 1) {

        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("optional second argument must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[1]->ToObject();

        if (options->Has(Nan::New("threshold").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("threshold").ToLocalChecked());
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'threshold' must be a number");
                return;
            }
            threshold = bind_opt->IntegerValue();
        }

        if (options->Has(Nan::New("alpha").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("alpha").ToLocalChecked());
            if (!bind_opt->IsBoolean()) {
                Nan::ThrowTypeError("optional arg 'alpha' must be a boolean");
                return;
            }
            alpha = bind_opt->BooleanValue();
        }

    }
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.This());
    Image* im2 = Nan::ObjectWrap::Unwrap<Image>(obj);
    if (im->this_->width() != im2->this_->width() ||
        im->this_->height() != im2->this_->height()) {
            Nan::ThrowTypeError("image dimensions do not match");
            return;
    }
    unsigned difference = mapnik::compare(*im->this_, *im2->this_, threshold, alpha);
    info.GetReturnValue().Set(Nan::New<v8::Integer>(difference));
}

NAN_METHOD(Image::filterSync)
{
    info.GetReturnValue().Set(_filterSync(info));
}

/**
 * Filter this image
 *
 * @name filterSync
 * @instance
 * @memberof mapnik.Image
 * @param {string} filter
 */
v8::Local<v8::Value> Image::_filterSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    if (info.Length() < 1) {
        Nan::ThrowTypeError("expects one argument: string filter argument");
        return scope.Escape(Nan::Undefined());
    }
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (!info[0]->IsString())
    {
        Nan::ThrowTypeError("A string is expected for filter argument");
        return scope.Escape(Nan::Undefined());
    }
    std::string filter = TOSTR(info[0]);
    try
    {
        mapnik::filter::filter_image(*im->this_,filter);
    }
    catch(std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string filter;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} filter_image_baton_t;

/**
 * Asynchronously filter this image.
 *
 * @name filter
 * @instance
 * @memberof mapnik.Image
 * @param {string} filter
 * @param {Function} callback
 * @example
 * var im = new mapnik.Image(5, 5);
 * im.filter("blur", function(err, im_res) {
 *   if (err) throw err;
 *   assert.equal(im_res.getPixel(0, 0), 1);
 * });
 */
NAN_METHOD(Image::filter)
{
    if (info.Length() <= 1) {
        info.GetReturnValue().Set(_filterSync(info));
        return;
    }

    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }
    
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (!info[0]->IsString())
    {
        Nan::ThrowTypeError("A string is expected for filter argument");
        return;
    }
    filter_image_baton_t *closure = new filter_image_baton_t();
    closure->filter = TOSTR(info[0]);
    
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Filter, (uv_after_work_cb)EIO_AfterFilter);
    im->Ref();
    return;
}

void Image::EIO_Filter(uv_work_t* req)
{
    filter_image_baton_t *closure = static_cast<filter_image_baton_t *>(req->data);
    try
    {
        mapnik::filter::filter_image(*closure->im->this_,closure->filter);
    }
    catch(std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterFilter(uv_work_t* req)
{
    Nan::HandleScope scope;
    filter_image_baton_t *closure = static_cast<filter_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

NAN_METHOD(Image::fillSync)
{
    info.GetReturnValue().Set(_fillSync(info));
}

/**
 * Fill this image with a given color
 *
 * @name fillSync
 * @instance
 * @memberof mapnik.Image
 * @param {mapnik.Color|number} color
 */
v8::Local<v8::Value> Image::_fillSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    if (info.Length() < 1 ) {
        Nan::ThrowTypeError("expects one argument: Color object or a number");
        return scope.Escape(Nan::Undefined());
    }
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    try
    {
        if (info[0]->IsUint32())
        {
            std::uint32_t val = info[0]->Uint32Value();
            mapnik::fill<std::uint32_t>(*im->this_,val);
        }
        else if (info[0]->IsInt32())
        {
            std::int32_t val = info[0]->Int32Value();
            mapnik::fill<std::int32_t>(*im->this_,val);
        }
        else if (info[0]->IsNumber())
        {
            double val = info[0]->NumberValue();
            mapnik::fill<double>(*im->this_,val);
        }
        else if (info[0]->IsObject())
        {
            v8::Local<v8::Object> obj = info[0]->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Color::constructor)->HasInstance(obj)) 
            {
                Nan::ThrowTypeError("A numeric or color value is expected");
            }
            else 
            {
                Color * color = Nan::ObjectWrap::Unwrap<Color>(obj);
                mapnik::fill(*im->this_,*(color->get()));
            }
        }
        else
        {
            Nan::ThrowTypeError("A numeric or color value is expected");
        }
    }
    catch(std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
    }
    return scope.Escape(Nan::Undefined());
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
    Nan::Persistent<v8::Function> cb;
} fill_image_baton_t;

/**
 * Asynchronously fill this image with a given color.
 *
 * @name fill
 * @instance
 * @memberof mapnik.Image
 * @param {mapnik.Color|number} color
 * @param {Function} callback
 * @example
 * var im = new mapnik.Image(5, 5);
 * im.fill(1, function(err, im_res) {
 *   if (err) throw err;
 *   assert.equal(im_res.getPixel(0, 0), 1);
 * });
 */
NAN_METHOD(Image::fill)
{
    if (info.Length() <= 1) {
        info.GetReturnValue().Set(_fillSync(info));
        return;
    }
    
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    fill_image_baton_t *closure = new fill_image_baton_t();
    if (info[0]->IsUint32())
    {
        closure->val_u32 = info[0]->Uint32Value();
        closure->type = FILL_UINT32;
    }
    else if (info[0]->IsInt32())
    {
        closure->val_32 = info[0]->Int32Value();
        closure->type = FILL_INT32;
    }
    else if (info[0]->IsNumber())
    {
        closure->val_double = info[0]->NumberValue();
        closure->type = FILL_DOUBLE;
    }
    else if (info[0]->IsObject())
    {
        v8::Local<v8::Object> obj = info[0]->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Color::constructor)->HasInstance(obj)) 
        {
            delete closure;
            Nan::ThrowTypeError("A numeric or color value is expected");
            return;
        }
        else 
        {
            Color * color = Nan::ObjectWrap::Unwrap<Color>(obj);
            closure->c = *(color->get());
        }
    }
    else
    {
        delete closure;
        Nan::ThrowTypeError("A numeric or color value is expected");
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        delete closure;
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }
    else
    {
        closure->request.data = closure;
        closure->im = im;
        closure->error = false;
        closure->cb.Reset(callback.As<v8::Function>());
        uv_queue_work(uv_default_loop(), &closure->request, EIO_Fill, (uv_after_work_cb)EIO_AfterFill);
        im->Ref();
    }
    return;
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
    Nan::HandleScope scope;
    fill_image_baton_t *closure = static_cast<fill_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Make this image transparent.
 *
 * @name clearSync
 * @instance
 * @memberof mapnik.Image
 * @example
 * var im = new mapnik.Image(5,5);
 * im.fillSync(1);
 * assert.equal(im.getPixel(0, 0), 1);
 * im.clearSync();
 * assert.equal(im.getPixel(0, 0), 0);
 */
NAN_METHOD(Image::clearSync)
{
    info.GetReturnValue().Set(_clearSync(info));
}

v8::Local<v8::Value> Image::_clearSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    try
    {
        mapnik::fill(*im->this_, 0);
    }
    catch(std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    //std::string format;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} clear_image_baton_t;

/**
 * Make this image transparent, removing all image data from it.
 *
 * @name clear
 * @instance
 * @param {Function} callback
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::clear)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    if (info.Length() == 0) {
        info.GetReturnValue().Set(_clearSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }
    clear_image_baton_t *closure = new clear_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Clear, (uv_after_work_cb)EIO_AfterClear);
    im->Ref();
    return;
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
    Nan::HandleScope scope;
    clear_image_baton_t *closure = static_cast<clear_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

NAN_METHOD(Image::setGrayScaleToAlpha)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info.Length() == 0) {
        mapnik::set_grayscale_to_alpha(*im->this_);
    } else {
        if (!info[0]->IsObject()) {
            Nan::ThrowTypeError("optional first arg must be a mapnik.Color");
            return;
        }

        v8::Local<v8::Object> obj = info[0]->ToObject();

        if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Color::constructor)->HasInstance(obj)) {
            Nan::ThrowTypeError("mapnik.Color expected as first arg");
            return;
        }

        Color * color = Nan::ObjectWrap::Unwrap<Color>(obj);
        mapnik::set_grayscale_to_alpha(*im->this_, *color->get());
    }

    return;
}

typedef struct {
    uv_work_t request;
    Image* im;
    Nan::Persistent<v8::Function> cb;
} image_op_baton_t;

/**
 * Determine whether the given image is premultiplied.
 *
 * @name premultiplied
 * @instance
 * @returns {boolean} premultiplied true if the image is premultiplied
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::premultiplied)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    bool premultiplied = im->this_->get_premultiplied();
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(premultiplied));
}

/**
 * Premultiply the pixels in this image
 *
 * @name premultiplySync
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::premultiplySync)
{
    info.GetReturnValue().Set(_premultiplySync(info));
}

v8::Local<v8::Value> Image::_premultiplySync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    mapnik::premultiply_alpha(*im->this_);
    return scope.Escape(Nan::Undefined());
}

/**
 * Premultiply the pixels in this image, asynchronously
 *
 * @name premultiply
 * @param {Function} callback
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::premultiply)
{
    if (info.Length() == 0) {
        info.GetReturnValue().Set(_premultiplySync(info));
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Premultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    return;
}

void Image::EIO_Premultiply(uv_work_t* req)
{
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    mapnik::premultiply_alpha(*closure->im->this_);
}

void Image::EIO_AfterMultiply(uv_work_t* req)
{
    Nan::HandleScope scope;
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
    Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Demultiply the pixels in this image. The opposite of
 * premultiplying
 *
 * @name demultiplySync
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::demultiplySync)
{
    Nan::HandleScope scope;
    info.GetReturnValue().Set(_demultiplySync(info));
}

v8::Local<v8::Value> Image::_demultiplySync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    mapnik::demultiply_alpha(*im->this_);
    return scope.Escape(Nan::Undefined());
}

/**
 * Demultiply the pixels in this image, asynchronously. The opposite of
 * premultiplying
 *
 * @name demultiply
 * @param {Function} callback
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::demultiply)
{
    if (info.Length() == 0) {
        info.GetReturnValue().Set(_demultiplySync(info));
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Demultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    return;
}

void Image::EIO_Demultiply(uv_work_t* req)
{
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    mapnik::demultiply_alpha(*closure->im->this_);
}

typedef struct {
    uv_work_t request;
    Image* im;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
    bool result;
} is_solid_image_baton_t;

NAN_METHOD(Image::isSolid)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    if (info.Length() == 0) {
        info.GetReturnValue().Set(_isSolidSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    is_solid_image_baton_t *closure = new is_solid_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->result = true;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    im->Ref();
    return;
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
    Nan::HandleScope scope;
    is_solid_image_baton_t *closure = static_cast<is_solid_image_baton_t *>(req->data);
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            v8::Local<v8::Value> argv[3] = { Nan::Null(),
                                     Nan::New(closure->result),
                                     mapnik::util::apply_visitor(visitor_get_pixel(0,0),*(closure->im->this_)),
            };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 3, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::New(closure->result) };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Determine whether the image is solid - whether it has alpha values of greater
 * than one.
 *
 * @name isSolidSync
 * @returns {boolean} whether the image is solid
 * @instance
 * @memberof mapnik.Image
 * @example
 * var im = new mapnik.Image(256, 256);
 * var view = im.view(0, 0, 256, 256);
 * assert.equal(view.isSolidSync(), true);
 */
NAN_METHOD(Image::isSolidSync)
{
    info.GetReturnValue().Set(_isSolidSync(info));
}

v8::Local<v8::Value> Image::_isSolidSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (im->this_->width() > 0 && im->this_->height() > 0)
    {
        return scope.Escape(Nan::New<v8::Boolean>(mapnik::is_solid(*(im->this_))));
    }
    Nan::ThrowError("image does not have valid dimensions");
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im1;
    std::shared_ptr<mapnik::image_any> im2;
    mapnik::image_dtype type;  
    double offset;
    double scaling;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
} copy_image_baton_t;

/**
 * Copy this image data so that changes can be made to a clone of it.
 *
 * @name copy
 * @param {number} type
 * @param {Object} [options={}]
 * @param {Function} callback
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::copy)
{
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_copySync(info));
        return;
    }
    
    Image* im1 = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = im1->this_->get_dtype();
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    
    if (info.Length() >= 2)
    {
        if (info[0]->IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(info[0]->IntegerValue());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Nan::ThrowTypeError("Image 'type' must be a valid image type");
                return;
            }
        }
        else if (info[0]->IsObject())
        {
            options = info[0]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Unknown parameters passed");
            return;
        }
    }
    if (info.Length() >= 3)
    {
        if (info[1]->IsObject())
        {
            options = info[1]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as second argument");
            return;
        }
    }
    
    if (options->Has(Nan::New("scaling").ToLocalChecked()))
    {
        v8::Local<v8::Value> scaling_val = options->Get(Nan::New("scaling").ToLocalChecked());
        if (scaling_val->IsNumber())
        {
            scaling = scaling_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("scaling argument must be a number");
            return;
        }
    }
    
    if (options->Has(Nan::New("offset").ToLocalChecked()))
    {
        v8::Local<v8::Value> offset_val = options->Get(Nan::New("offset").ToLocalChecked());
        if (offset_val->IsNumber())
        {
            offset = offset_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("offset argument must be a number");
            return;
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
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Copy, (uv_after_work_cb)EIO_AfterCopy);
    closure->im1->Ref();
    return;
}

void Image::EIO_Copy(uv_work_t* req)
{
    copy_image_baton_t *closure = static_cast<copy_image_baton_t *>(req->data);
    try
    {
        closure->im2 = std::make_shared<mapnik::image_any>(
                               mapnik::image_copy(*(closure->im1->this_),
                                                            closure->type,
                                                            closure->offset,
                                                            closure->scaling)
                               );
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterCopy(uv_work_t* req)
{
    Nan::HandleScope scope;
    copy_image_baton_t *closure = static_cast<copy_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else if (!closure->im2)
    {
        // Not quite sure if this is even required or ever can be reached, but leaving it
        // and simply removing it from coverage tests.
        /* LCOV_EXCL_START */
        v8::Local<v8::Value> argv[1] = { Nan::Error("could not render to image") };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        /* LCOV_EXCL_END */
    }
    else
    {
        Image* im = new Image(closure->im2);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        v8::Local<v8::Object> image_obj = Nan::New(constructor)->GetFunction()->NewInstance(1, &ext);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), image_obj };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im1->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Copy this image data so that changes can be made to a clone of it.
 *
 * @name copySync
 * @param {number} type
 * @param {Object} [options={}]
 * @returns {mapnik.Image} copy
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::copySync)
{
    info.GetReturnValue().Set(_copySync(info));
}

v8::Local<v8::Value> Image::_copySync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = im->this_->get_dtype();
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() >= 1)
    {
        if (info[0]->IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(info[0]->IntegerValue());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Nan::ThrowTypeError("Image 'type' must be a valid image type");
                return scope.Escape(Nan::Undefined());
            }
        }
        else if (info[0]->IsObject())
        {
            options = info[0]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Unknown parameters passed");
            return scope.Escape(Nan::Undefined());
        }
    }
    if (info.Length() >= 2)
    {
        if (info[1]->IsObject())
        {
            options = info[1]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as second argument");
            return scope.Escape(Nan::Undefined());
        }
    }
    
    if (options->Has(Nan::New("scaling").ToLocalChecked()))
    {
        v8::Local<v8::Value> scaling_val = options->Get(Nan::New("scaling").ToLocalChecked());
        if (scaling_val->IsNumber())
        {
            scaling = scaling_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("scaling argument must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }
    
    if (options->Has(Nan::New("offset").ToLocalChecked()))
    {
        v8::Local<v8::Value> offset_val = options->Get(Nan::New("offset").ToLocalChecked());
        if (offset_val->IsNumber())
        {
            offset = offset_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("offset argument must be a number");
            return scope.Escape(Nan::Undefined());
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
                                               mapnik::image_copy(*(im->this_),
                                                                            type,
                                                                            offset,
                                                                            scaling)
                                               );
        Image* new_im = new Image(image_ptr);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(new_im);
        return scope.Escape(Nan::New(constructor)->GetFunction()->NewInstance(1, &ext));
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}

typedef struct {
    uv_work_t request;
    Image* im1;
    std::shared_ptr<mapnik::image_any> im2;
    mapnik::scaling_method_e scaling_method;
    std::size_t size_x;
    std::size_t size_y;
    double filter_factor;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
} resize_image_baton_t;

/**
 * Create a copy this image that is resized
 *
 * @name resize
 * @param {number} width
 * @param {number} height
 * @param {Object} [options={}]
 * @param {Function} callback
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::resize)
{    
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_resizeSync(info));
        return;
    }
    Image* im1 = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    std::size_t width = 0;
    std::size_t height = 0;
    double filter_factor = 1.0;
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    
    if (info.Length() >= 3)
    {
        if (info[0]->IsNumber())
        {
            auto width_tmp = info[0]->IntegerValue();
            if (width_tmp <= 0)
            {
                Nan::ThrowTypeError("Width must be a integer greater then zero");
                return;
            }
            width = static_cast<std::size_t>(width_tmp);
        }
        else
        {
            Nan::ThrowTypeError("Width must be a number");
            return;
        }
        if (info[1]->IsNumber())
        {
            auto height_tmp = info[1]->IntegerValue();
            if (height_tmp <= 0)
            {
                Nan::ThrowTypeError("Height must be a integer greater then zero");
                return;
            }
            height = static_cast<std::size_t>(height_tmp);
        }
        else
        {
            Nan::ThrowTypeError("Height must be a number");
            return;
        }
    }
    else
    {
        Nan::ThrowTypeError("resize requires a width and height paramter.");
        return;
    }
    if (info.Length() >= 4)
    {
        if (info[2]->IsObject())
        {
            options = info[2]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as third argument");
            return;
        }
    }
    
    if (options->Has(Nan::New("scaling_method").ToLocalChecked()))
    {
        v8::Local<v8::Value> scaling_val = options->Get(Nan::New("scaling_method").ToLocalChecked());
        if (scaling_val->IsNumber())
        {
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_val->IntegerValue());
            if (scaling_method > mapnik::SCALING_BLACKMAN)
            {
                Nan::ThrowTypeError("Invalid scaling_method");
                return;
            }
        }
        else
        {
            Nan::ThrowTypeError("scaling_method argument must be an integer");
            return;
        }
    }
    
    if (options->Has(Nan::New("filter_factor").ToLocalChecked()))
    {
        v8::Local<v8::Value> ff_val = options->Get(Nan::New("filter_factor").ToLocalChecked());
        if (ff_val->IsNumber())
        {
            filter_factor = ff_val->NumberValue();
        }
        else
        {
            Nan::ThrowTypeError("filter_factor argument must be a number");
            return;
        }
    }
    resize_image_baton_t *closure = new resize_image_baton_t();
    closure->request.data = closure;
    closure->im1 = im1;
    closure->scaling_method = scaling_method;
    closure->size_x = width;
    closure->size_y = height;
    closure->filter_factor = filter_factor;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Resize, (uv_after_work_cb)EIO_AfterResize);
    closure->im1->Ref();
    return;
}

struct resize_visitor
{

    resize_visitor(mapnik::image_any const& im1, 
                   mapnik::scaling_method_e scaling_method,
                   double image_ratio_x,
                   double image_ratio_y,
                   double filter_factor) :
        im1_(im1),
        scaling_method_(scaling_method),
        image_ratio_x_(image_ratio_x),
        image_ratio_y_(image_ratio_y),
        filter_factor_(filter_factor) {}

    void operator()(mapnik::image_rgba8 & im2) const
    {
        if (!im1_.get_premultiplied())
        {
            throw std::runtime_error("RGBA8 images must be premultiplied prior to using resize");
        }
        mapnik::scale_image_agg(im2, 
                                mapnik::util::get<mapnik::image_rgba8>(im1_),
                                scaling_method_,
                                image_ratio_x_,
                                image_ratio_y_,
                                0,
                                0,
                                filter_factor_);
    }

    template <typename T>
    void operator()(T & im2) const
    {
        mapnik::scale_image_agg(im2, 
                                mapnik::util::get<T>(im1_),
                                scaling_method_,
                                image_ratio_x_,
                                image_ratio_y_,
                                0,
                                0,
                                filter_factor_);
    }
    
    void operator()(mapnik::image_null &) const
    {
        // Should be caught earlier so no test coverage should reach here.
        /* LCOV_EXCL_START */
        throw std::runtime_error("Can not resize null images");
        /* LCOV_EXCL_END */
    }
    
    void operator()(mapnik::image_gray8s &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 8 bit integer rasters");
    }

    void operator()(mapnik::image_gray16s &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 16 bit integer rasters");
    }
    
    void operator()(mapnik::image_gray32 &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing unsigned 32 bit integer rasters");
    }
    
    void operator()(mapnik::image_gray32s &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 32 bit integer rasters");
    }
    
    void operator()(mapnik::image_gray64 &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing unsigned 64 bit integer rasters");
    }
    
    void operator()(mapnik::image_gray64s &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 64 bit integer rasters");
    }
    
    void operator()(mapnik::image_gray64f &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing 64 bit floating point rasters");
    }


  private:
    mapnik::image_any const & im1_;
    mapnik::scaling_method_e scaling_method_;
    double image_ratio_x_;
    double image_ratio_y_;
    double filter_factor_;

};

void Image::EIO_Resize(uv_work_t* req)
{
    resize_image_baton_t *closure = static_cast<resize_image_baton_t *>(req->data);
    if (closure->im1->this_->is<mapnik::image_null>())
    {
        closure->error = true;
        closure->error_name = "Can not resize a null image.";
        return;
    }
    try
    {
        double offset = closure->im1->this_->get_offset();
        double scaling = closure->im1->this_->get_scaling();

        closure->im2 = std::make_shared<mapnik::image_any>(closure->size_x, 
                                                           closure->size_y, 
                                                           closure->im1->this_->get_dtype(),
                                                           false,
                                                           true,
                                                           false);
        closure->im2->set_offset(offset);
        closure->im2->set_scaling(scaling);
        int im_width = closure->im1->this_->width();
        int im_height = closure->im1->this_->height();
        if (im_width <= 0 || im_height <= 0)
        {
            closure->error = true;
            closure->error_name = "Image width or height is zero or less then zero.";
            return;
        }
        double image_ratio_x = static_cast<double>(closure->size_x) / im_width; 
        double image_ratio_y = static_cast<double>(closure->size_y) / im_height;
        resize_visitor visit(*(closure->im1->this_),
                             closure->scaling_method,
                             image_ratio_x,
                             image_ratio_y,
                             closure->filter_factor);
        mapnik::util::apply_visitor(visit, *(closure->im2));
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterResize(uv_work_t* req)
{
    Nan::HandleScope scope;
    resize_image_baton_t *closure = static_cast<resize_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im2);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        v8::Local<v8::Object> image_obj = Nan::New(constructor)->GetFunction()->NewInstance(1, &ext);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), image_obj };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im1->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Make a resized copy of an image
 *
 * @name resizeSync
 * @param {number} width
 * @param {number} height
 * @param {Object} [options={}]
 * @returns {mapnik.Image} copy
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::resizeSync)
{
    info.GetReturnValue().Set(_resizeSync(info));
}

v8::Local<v8::Value> Image::_resizeSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    std::size_t width = 0;
    std::size_t height = 0;
    double filter_factor = 1.0;
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() >= 2)
    {
        if (info[0]->IsNumber())
        {
            int width_tmp = info[0]->IntegerValue();
            if (width_tmp <= 0)
            {
                Nan::ThrowTypeError("Width parameter must be an integer greater then zero");
                return scope.Escape(Nan::Undefined());
            }
            width = static_cast<std::size_t>(width_tmp);
        }
        else
        {
            Nan::ThrowTypeError("Width must be a number");
            return scope.Escape(Nan::Undefined());
        }
        if (info[1]->IsNumber())
        {
            int height_tmp = info[1]->IntegerValue();
            if (height_tmp <= 0)
            {
                Nan::ThrowTypeError("Height parameter must be an integer greater then zero");
                return scope.Escape(Nan::Undefined());
            }
            height = static_cast<std::size_t>(height_tmp);
        }
        else
        {
            Nan::ThrowTypeError("Height must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }
    else
    {
        Nan::ThrowTypeError("Resize requires at least a width and height parameter");
        return scope.Escape(Nan::Undefined());
    }    
    if (info.Length() >= 3)
    {
        if (info[2]->IsObject())
        {
            options = info[2]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as third argument");
            return scope.Escape(Nan::Undefined());
        }
    }
    
    if (options->Has(Nan::New("scaling_method").ToLocalChecked()))
    {
        v8::Local<v8::Value> scaling_val = options->Get(Nan::New("scaling_method").ToLocalChecked());
        if (scaling_val->IsNumber())
        {
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_val->IntegerValue());
            if (scaling_method > mapnik::SCALING_BLACKMAN)
            {
                Nan::ThrowTypeError("Invalid scaling_method");
                return scope.Escape(Nan::Undefined());
            }
        }
        else
        {
            Nan::ThrowTypeError("scaling_method argument must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }
    
    if (options->Has(Nan::New("filter_factor").ToLocalChecked()))
    {
        v8::Local<v8::Value> ff_val = options->Get(Nan::New("filter_factor").ToLocalChecked());
        if (ff_val->IsNumber())
        {
            filter_factor = ff_val->NumberValue();
        }
        else
        {
            Nan::ThrowTypeError("filter_factor argument must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (im->this_->is<mapnik::image_null>())
    {
        Nan::ThrowTypeError("Can not resize a null image");
        return scope.Escape(Nan::Undefined());
    }
    int im_width = im->this_->width();
    int im_height = im->this_->height();
    if (im_width <= 0 || im_height <= 0)
    {
        Nan::ThrowTypeError("Image width or height is zero or less then zero.");
        return scope.Escape(Nan::Undefined());
    }
    try
    {
        double offset = im->this_->get_offset();
        double scaling = im->this_->get_scaling();

        std::shared_ptr<mapnik::image_any> image_ptr = std::make_shared<mapnik::image_any>(width, 
                                                           height, 
                                                           im->this_->get_dtype(),
                                                           false,
                                                           true,
                                                           false);
        image_ptr->set_offset(offset);
        image_ptr->set_scaling(scaling);
        double image_ratio_x = static_cast<double>(width) / im_width; 
        double image_ratio_y = static_cast<double>(height) / im_height; 
        resize_visitor visit(*(im->this_),
                             scaling_method,
                             image_ratio_x,
                             image_ratio_y,
                             filter_factor);
        mapnik::util::apply_visitor(visit, *image_ptr);
        Image* new_im = new Image(image_ptr);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(new_im);
        return scope.Escape(Nan::New(constructor)->GetFunction()->NewInstance(1, &ext));
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}


NAN_METHOD(Image::painted)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(im->this_->painted()));
}

/**
 * Get this image's width
 *
 * @name width
 * @returns {number} width
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::width)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<std::int32_t>(im->this_->width())));
}

/**
 * Get this image's height
 *
 * @name height
 * @returns {number} height
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::height)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<std::int32_t>(im->this_->height())));
}

NAN_METHOD(Image::openSync)
{
    info.GetReturnValue().Set(_openSync(info));
}

v8::Local<v8::Value> Image::_openSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;

    if (info.Length() < 1) {
        Nan::ThrowError("must provide a string argument");
        return scope.Escape(Nan::Undefined());
    }

    if (!info[0]->IsString()) {
        Nan::ThrowTypeError("Argument must be a string");
        return scope.Escape(Nan::Undefined());
    }

    try
    {
        std::string filename = TOSTR(info[0]);
        boost::optional<std::string> type = mapnik::type_from_filename(filename);
        if (type)
        {
            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(filename,*type));
            if (reader.get())
            {
                std::shared_ptr<mapnik::image_any> image_ptr = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(), reader->height()));
                if (!reader->has_alpha())
                {
                    mapnik::set_premultiplied_alpha(*image_ptr, true);
                }
                Image* im = new Image(image_ptr);
                v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
                return scope.Escape(Nan::New(constructor)->GetFunction()->NewInstance(1, &ext));
            }
        }
        Nan::ThrowTypeError(("Unsupported image format:" + filename).c_str());
        return scope.Escape(Nan::Undefined());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}

typedef struct {
    uv_work_t request;
    image_ptr im;
    const char *data;
    size_t dataLength;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Object> buffer;
    Nan::Persistent<v8::Function> cb;
} image_mem_ptr_baton_t;

typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string filename;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} image_file_ptr_baton_t;

NAN_METHOD(Image::open)
{
    if (info.Length() == 1) {
        info.GetReturnValue().Set(_openSync(info));
        return;
    }

    if (info.Length() < 2) {
        Nan::ThrowError("must provide a string argument");
        return;
    }

    if (!info[0]->IsString()) {
        Nan::ThrowTypeError("Argument must be a string");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    image_file_ptr_baton_t *closure = new image_file_ptr_baton_t();
    closure->request.data = closure;
    closure->filename = TOSTR(info[0]);
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Open, (uv_after_work_cb)EIO_AfterOpen);
    return;
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
                if (!reader->has_alpha())
                {
                    mapnik::set_premultiplied_alpha(*(closure->im), true);
                }
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
    Nan::HandleScope scope;
    image_file_ptr_baton_t *closure = static_cast<image_file_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        v8::Local<v8::Object> image_obj = Nan::New(constructor)->GetFunction()->NewInstance(1, &ext);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), image_obj };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->cb.Reset();
    delete closure;
}

// Read from a Buffer
NAN_METHOD(Image::fromSVGBytesSync)
{
    info.GetReturnValue().Set(_fromSVGSync(false, info));
}

// Read from a file
NAN_METHOD(Image::fromSVGSync)
{
    info.GetReturnValue().Set(_fromSVGSync(true, info));
}

v8::Local<v8::Value> Image::_fromSVGSync(bool fromFile, Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;

    if (!fromFile && (info.Length() < 1 || !info[0]->IsObject())) 
    {
        Nan::ThrowTypeError("must provide a buffer argument");
        return scope.Escape(Nan::Undefined());
    }

    if (fromFile && (info.Length() < 1 || !info[0]->IsString())) 
    {
        Nan::ThrowTypeError("must provide a filename argument");
        return scope.Escape(Nan::Undefined());
    }


    double scale = 1.0;
    if (info.Length() >= 2) 
    {
        if (!info[1]->IsObject()) 
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("scale").ToLocalChecked()))
        {
            v8::Local<v8::Value> scale_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!scale_opt->IsNumber()) 
            {
                Nan::ThrowTypeError("'scale' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            scale = scale_opt->NumberValue();
            if (scale <= 0)
            {
                Nan::ThrowTypeError("'scale' must be a positive non zero number");
                return scope.Escape(Nan::Undefined());
            }
        }
    }

    try
    {
        using namespace mapnik::svg;
        mapnik::svg_path_ptr marker_path(std::make_shared<mapnik::svg_storage_type>());
        vertex_stl_adapter<svg_path_storage> stl_storage(marker_path->source());
        svg_path_adapter svg_path(stl_storage);
        svg_converter_type svg(svg_path, marker_path->attributes());
        svg_parser p(svg);
        if (fromFile)
        {
            if (!p.parse(TOSTR(info[0])))
            {
                std::ostringstream errorMessage("");
                errorMessage << "SVG parse error:" << std::endl;
                auto const& errors = p.error_messages();
                for (auto error : errors) {
                    errorMessage <<  error << std::endl;
                }
                Nan::ThrowTypeError(errorMessage.str().c_str());
                return scope.Escape(Nan::Undefined());
            }
        }
        else
        {
            v8::Local<v8::Object> obj = info[0]->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) 
            {
                Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
                return scope.Escape(Nan::Undefined());
            }
            std::string svg_buffer(node::Buffer::Data(obj),node::Buffer::Length(obj));
            if (!p.parse_from_string(svg_buffer))
            {
                std::ostringstream errorMessage("");
                errorMessage << "SVG parse error:" << std::endl;
                auto const& errors = p.error_messages();
                for (auto error : errors) {
                    errorMessage <<  error << std::endl;
                }
                Nan::ThrowTypeError(errorMessage.str().c_str());
                return scope.Escape(Nan::Undefined());
            }
        }

        double lox,loy,hix,hiy;
        svg.bounding_rect(&lox, &loy, &hix, &hiy);
        marker_path->set_bounding_box(lox,loy,hix,hiy);
        marker_path->set_dimensions(svg.width(),svg.height());

        using pixfmt = agg::pixfmt_rgba32_pre;
        using renderer_base = agg::renderer_base<pixfmt>;
        using renderer_solid = agg::renderer_scanline_aa_solid<renderer_base>;
        agg::rasterizer_scanline_aa<> ras_ptr;
        agg::scanline_u8 sl;

        double opacity = 1;
        int svg_width = svg.width() * scale;
        int svg_height = svg.height() * scale;
        
        if (svg_width <= 0 || svg_height <= 0)
        {
            Nan::ThrowTypeError("image created from svg must have a width and height greater then zero");
            return scope.Escape(Nan::Undefined());
        }

        mapnik::image_rgba8 im(svg_width, svg_height, true, true);
        agg::rendering_buffer buf(im.bytes(), im.width(), im.height(), im.row_size());
        pixfmt pixf(buf);
        renderer_base renb(pixf);

        mapnik::box2d<double> const& bbox = marker_path->bounding_box();
        mapnik::coord<double,2> c = bbox.center();
        // center the svg marker on '0,0'
        agg::trans_affine mtx = agg::trans_affine_translation(-c.x,-c.y);
        // Scale the image
        mtx.scale(scale);
        // render the marker at the center of the marker box
        mtx.translate(0.5 * im.width(), 0.5 * im.height());

        mapnik::svg::svg_renderer_agg<mapnik::svg::svg_path_adapter,
            agg::pod_bvector<mapnik::svg::path_attributes>,
            renderer_solid,
            agg::pixfmt_rgba32_pre > svg_renderer_this(svg_path,
                                                       marker_path->attributes());

        svg_renderer_this.render(ras_ptr, sl, renb, mtx, opacity, bbox);
        mapnik::demultiply_alpha(im);

        std::shared_ptr<mapnik::image_any> image_ptr = std::make_shared<mapnik::image_any>(im);
        Image *im2 = new Image(image_ptr);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im2);
        return scope.Escape(Nan::New(constructor)->GetFunction()->NewInstance(1, &ext));
    }
    catch (std::exception const& ex)
    {
        // There is currently no known way to make these operations throw an exception, however,
        // since the underlying agg library does possibly have some operation that might throw
        // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_END
    }
}

typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string filename;
    bool error;
    double scale;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} svg_file_ptr_baton_t;

typedef struct {
    uv_work_t request;
    image_ptr im;
    const char *data;
    size_t dataLength;
    bool error;
    double scale;
    std::string error_name;
    Nan::Persistent<v8::Object> buffer;
    Nan::Persistent<v8::Function> cb;
} svg_mem_ptr_baton_t;

/**
 * Create a new image from an SVG file
 *
 * @name fromSVG
 * @param {String} filename
 * @param {Function} callback
 * @static
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::fromSVG)
{
    if (info.Length() == 1) {
        info.GetReturnValue().Set(_fromSVGSync(true, info));
        return;
    }

    if (info.Length() < 2 || !info[0]->IsString()) 
    {
        Nan::ThrowTypeError("must provide a filename argument");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    double scale = 1.0;
    if (info.Length() >= 3) 
    {
        if (!info[1]->IsObject()) 
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("scale").ToLocalChecked()))
        {
            v8::Local<v8::Value> scale_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!scale_opt->IsNumber()) 
            {
                Nan::ThrowTypeError("'scale' must be a number");
                return;
            }
            scale = scale_opt->NumberValue();
            if (scale <= 0)
            {
                Nan::ThrowTypeError("'scale' must be a positive non zero number");
                return;
            }
        }
    }

    svg_file_ptr_baton_t *closure = new svg_file_ptr_baton_t();
    closure->request.data = closure;
    closure->filename = TOSTR(info[0]);
    closure->error = false;
    closure->scale = scale;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromSVG, (uv_after_work_cb)EIO_AfterFromSVG);
    return;
}

void Image::EIO_FromSVG(uv_work_t* req)
{
    svg_file_ptr_baton_t *closure = static_cast<svg_file_ptr_baton_t *>(req->data);

    try
    {
        using namespace mapnik::svg;
        mapnik::svg_path_ptr marker_path(std::make_shared<mapnik::svg_storage_type>());
        vertex_stl_adapter<svg_path_storage> stl_storage(marker_path->source());
        svg_path_adapter svg_path(stl_storage);
        svg_converter_type svg(svg_path, marker_path->attributes());
        svg_parser p(svg);
        if (!p.parse(closure->filename))
        {
            std::ostringstream errorMessage("");
            errorMessage << "SVG parse error:" << std::endl;
            auto const& errors = p.error_messages();
            for (auto error : errors) {
                errorMessage <<  error << std::endl;
            }
            closure->error = true;
            closure->error_name = errorMessage.str();
            return;
        }

        double lox,loy,hix,hiy;
        svg.bounding_rect(&lox, &loy, &hix, &hiy);
        marker_path->set_bounding_box(lox,loy,hix,hiy);
        marker_path->set_dimensions(svg.width(),svg.height());

        using pixfmt = agg::pixfmt_rgba32_pre;
        using renderer_base = agg::renderer_base<pixfmt>;
        using renderer_solid = agg::renderer_scanline_aa_solid<renderer_base>;
        agg::rasterizer_scanline_aa<> ras_ptr;
        agg::scanline_u8 sl;

        double opacity = 1;
        int svg_width = svg.width() * closure->scale;
        int svg_height = svg.height() * closure->scale;

        if (svg_width <= 0 || svg_height <= 0)
        {
            closure->error = true;
            closure->error_name = "image created from svg must have a width and height greater then zero";
            return;
        }

        mapnik::image_rgba8 im(svg_width, svg_height, true, true);
        agg::rendering_buffer buf(im.bytes(), im.width(), im.height(), im.row_size());
        pixfmt pixf(buf);
        renderer_base renb(pixf);

        mapnik::box2d<double> const& bbox = marker_path->bounding_box();
        mapnik::coord<double,2> c = bbox.center();
        // center the svg marker on '0,0'
        agg::trans_affine mtx = agg::trans_affine_translation(-c.x,-c.y);
        // Scale the image
        mtx.scale(closure->scale);
        // render the marker at the center of the marker box
        mtx.translate(0.5 * im.width(), 0.5 * im.height());

        mapnik::svg::svg_renderer_agg<mapnik::svg::svg_path_adapter,
            agg::pod_bvector<mapnik::svg::path_attributes>,
            renderer_solid,
            agg::pixfmt_rgba32_pre > svg_renderer_this(svg_path,
                                                       marker_path->attributes());

        svg_renderer_this.render(ras_ptr, sl, renb, mtx, opacity, bbox);
        mapnik::demultiply_alpha(im);
        closure->im = std::make_shared<mapnik::image_any>(im);
    }
    catch (std::exception const& ex)
    {
        // There is currently no known way to make these operations throw an exception, however,
        // since the underlying agg library does possibly have some operation that might throw
        // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = "Failed to load: " + closure->filename;
        // LCOV_EXCL_END
    }
}

void Image::EIO_AfterFromSVG(uv_work_t* req)
{
    Nan::HandleScope scope;
    svg_file_ptr_baton_t *closure = static_cast<svg_file_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        v8::Local<v8::Object> image_obj = Nan::New(constructor)->GetFunction()->NewInstance(1, &ext);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), image_obj };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->cb.Reset();
    delete closure;
}

/**
 * Create a new image from an SVG file
 *
 * @name fromSVGBytes
 * @param {String} filename
 * @param {Function} callback
 * @static
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::fromSVGBytes)
{
    if (info.Length() == 1) {
        info.GetReturnValue().Set(_fromSVGSync(false, info));
        return;
    }

    if (info.Length() < 2 || !info[0]->IsObject()) {
        Nan::ThrowError("must provide a buffer argument");
        return;
    }

    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    double scale = 1.0;
    if (info.Length() >= 3) 
    {
        if (!info[1]->IsObject()) 
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("scale").ToLocalChecked()))
        {
            v8::Local<v8::Value> scale_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!scale_opt->IsNumber()) 
            {
                Nan::ThrowTypeError("'scale' must be a number");
                return;
            }
            scale = scale_opt->NumberValue();
            if (scale <= 0)
            {
                Nan::ThrowTypeError("'scale' must be a positive non zero number");
                return;
            }
        }
    }

    svg_mem_ptr_baton_t *closure = new svg_mem_ptr_baton_t();
    closure->request.data = closure;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->scale = scale;
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromSVGBytes, (uv_after_work_cb)EIO_AfterFromSVGBytes);
    return;
}

void Image::EIO_FromSVGBytes(uv_work_t* req)
{
    svg_mem_ptr_baton_t *closure = static_cast<svg_mem_ptr_baton_t *>(req->data);

    try
    {
        using namespace mapnik::svg;
        mapnik::svg_path_ptr marker_path(std::make_shared<mapnik::svg_storage_type>());
        vertex_stl_adapter<svg_path_storage> stl_storage(marker_path->source());
        svg_path_adapter svg_path(stl_storage);
        svg_converter_type svg(svg_path, marker_path->attributes());
        svg_parser p(svg);

        std::string svg_buffer(closure->data,closure->dataLength);
        if (!p.parse_from_string(svg_buffer))
        {
            std::ostringstream errorMessage("");
            errorMessage << "SVG parse error:" << std::endl;
            auto const& errors = p.error_messages();
            for (auto error : errors) {
                errorMessage <<  error << std::endl;
            }
            closure->error = true;
            closure->error_name = errorMessage.str();
            return;
        }

        double lox,loy,hix,hiy;
        svg.bounding_rect(&lox, &loy, &hix, &hiy);
        marker_path->set_bounding_box(lox,loy,hix,hiy);
        marker_path->set_dimensions(svg.width(),svg.height());

        using pixfmt = agg::pixfmt_rgba32_pre;
        using renderer_base = agg::renderer_base<pixfmt>;
        using renderer_solid = agg::renderer_scanline_aa_solid<renderer_base>;
        agg::rasterizer_scanline_aa<> ras_ptr;
        agg::scanline_u8 sl;

        double opacity = 1;
        int svg_width = svg.width() * closure->scale;
        int svg_height = svg.height() * closure->scale;
        
        if (svg_width <= 0 || svg_height <= 0)
        {
            closure->error = true;
            closure->error_name = "image created from svg must have a width and height greater then zero";
            return;
        }

        mapnik::image_rgba8 im(svg_width, svg_height, true, true);
        agg::rendering_buffer buf(im.bytes(), im.width(), im.height(), im.row_size());
        pixfmt pixf(buf);
        renderer_base renb(pixf);

        mapnik::box2d<double> const& bbox = marker_path->bounding_box();
        mapnik::coord<double,2> c = bbox.center();
        // center the svg marker on '0,0'
        agg::trans_affine mtx = agg::trans_affine_translation(-c.x,-c.y);
        // Scale the image
        mtx.scale(closure->scale);
        // render the marker at the center of the marker box
        mtx.translate(0.5 * im.width(), 0.5 * im.height());

        mapnik::svg::svg_renderer_agg<mapnik::svg::svg_path_adapter,
            agg::pod_bvector<mapnik::svg::path_attributes>,
            renderer_solid,
            agg::pixfmt_rgba32_pre > svg_renderer_this(svg_path,
                                                       marker_path->attributes());

        svg_renderer_this.render(ras_ptr, sl, renb, mtx, opacity, bbox);
        mapnik::demultiply_alpha(im);
        closure->im = std::make_shared<mapnik::image_any>(im);
    }
    catch (std::exception const& ex)
    {
        // There is currently no known way to make these operations throw an exception, however,
        // since the underlying agg library does possibly have some operation that might throw
        // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = ex.what();
        // LCOV_EXCL_END
    }
}

void Image::EIO_AfterFromSVGBytes(uv_work_t* req)
{
    Nan::HandleScope scope;
    svg_mem_ptr_baton_t *closure = static_cast<svg_mem_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        v8::Local<v8::Object> image_obj = Nan::New(constructor)->GetFunction()->NewInstance(1, &ext);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), image_obj };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Create an image of the existing buffer. BUFFER MUST LIVE AS LONG AS THE IMAGE. 
 * It is recommended that you do not use this method! Be warned!
 *
 * @name fromBufferSync
 * @param {number} width
 * @param {number} height
 * @param {Buffer} buffer
 * @returns {mapnik.Image} image object
 * @static
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::fromBufferSync)
{
    info.GetReturnValue().Set(_fromBufferSync(info));
}

v8::Local<v8::Value> Image::_fromBufferSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;

    if (info.Length() < 3 || !info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsObject()) {
        Nan::ThrowTypeError("must provide a width, height, and buffer argument");
        return scope.Escape(Nan::Undefined());
    }

    unsigned width = info[0]->IntegerValue();
    unsigned height = info[1]->IntegerValue();

    if (width <= 0 || height <= 0) 
    {
        Nan::ThrowTypeError("width and height must be greater then zero");
        return scope.Escape(Nan::Undefined());
    }

    v8::Local<v8::Object> obj = info[2]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("third argument is invalid, must be a Buffer");
        return scope.Escape(Nan::Undefined());
    }

    // TODO - support other image types?
    auto im_size = mapnik::image_rgba8::pixel_size * width * height;
    if (im_size != node::Buffer::Length(obj)) {
        Nan::ThrowTypeError("invalid image size");
        return scope.Escape(Nan::Undefined());
    }

    bool premultiplied = false;
    bool painted = false;

    if (info.Length() >= 4)
    {
        if (info[3]->IsObject())
        {
            v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[3]);
            if (options->Has(Nan::New("type").ToLocalChecked()))
            {
                Nan::ThrowTypeError("'type' option not supported (only rgba images currently viable)");
                return scope.Escape(Nan::Undefined());
            }

            if (options->Has(Nan::New("premultiplied").ToLocalChecked()))
            {
                v8::Local<v8::Value> pre_val = options->Get(Nan::New("premultiplied").ToLocalChecked());
                if (!pre_val.IsEmpty() && pre_val->IsBoolean())
                {
                    premultiplied = pre_val->BooleanValue();
                }
                else
                {
                    Nan::ThrowTypeError("premultiplied option must be a boolean");
                    return scope.Escape(Nan::Undefined());
                }
            }

            if (options->Has(Nan::New("painted").ToLocalChecked()))
            {
                v8::Local<v8::Value> painted_val = options->Get(Nan::New("painted").ToLocalChecked());
                if (!painted_val.IsEmpty() && painted_val->IsBoolean())
                {
                    painted = painted_val->BooleanValue();
                }
                else
                {
                    Nan::ThrowTypeError("painted option must be a boolean");
                    return scope.Escape(Nan::Undefined());
                }
            }
        }
        else
        {
            Nan::ThrowTypeError("Options parameter must be an object");
            return scope.Escape(Nan::Undefined());
        }
    }

    try
    {
        mapnik::image_rgba8 im_wrapper(width, height, reinterpret_cast<unsigned char*>(node::Buffer::Data(obj)), premultiplied, painted);
        std::shared_ptr<mapnik::image_any> image_ptr = std::make_shared<mapnik::image_any>(im_wrapper);
        Image* im = new Image(image_ptr);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        return scope.Escape(Nan::New(constructor)->GetFunction()->NewInstance(1, &ext));
    }
    catch (std::exception const& ex)
    {
        // There is no known way for this exception to be reached currently.
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_END
    }
}

NAN_METHOD(Image::fromBytesSync)
{
    info.GetReturnValue().Set(_fromBytesSync(info));
}

v8::Local<v8::Value> Image::_fromBytesSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;

    if (info.Length() < 1 || !info[0]->IsObject()) {
        Nan::ThrowTypeError("must provide a buffer argument");
        return scope.Escape(Nan::Undefined());
    }

    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
        return scope.Escape(Nan::Undefined());
    }

    try
    {
        std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(node::Buffer::Data(obj),node::Buffer::Length(obj)));
        if (reader.get())
        {
            std::shared_ptr<mapnik::image_any> image_ptr = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
            Image* im = new Image(image_ptr);
            v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
            return scope.Escape(Nan::New(constructor)->GetFunction()->NewInstance(1, &ext));
        }
        // The only way this is ever reached is if the reader factory in 
        // mapnik was not providing an image type it should. This should never
        // be occuring so marking this out from coverage
        /* LCOV_EXCL_START */
        Nan::ThrowTypeError("Failed to load from buffer");
        return scope.Escape(Nan::Undefined());
        /* LCOV_EXCL_END */
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}

/**
 * Create a new image from a buffer
 *
 * @name fromBytes
 * @param {Buffer} buffer
 * @param {Function} callback
 * @static
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::fromBytes)
{
    if (info.Length() == 1) {
        info.GetReturnValue().Set(_fromBytesSync(info));
        return;
    }

    if (info.Length() < 2) {
        Nan::ThrowError("must provide a buffer argument");
        return;
    }

    if (!info[0]->IsObject()) {
        Nan::ThrowTypeError("must provide a buffer argument");
        return;
    }

    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    image_mem_ptr_baton_t *closure = new image_mem_ptr_baton_t();
    closure->request.data = closure;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromBytes, (uv_after_work_cb)EIO_AfterFromBytes);
    return;
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
    Nan::HandleScope scope;
    image_mem_ptr_baton_t *closure = static_cast<image_mem_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        v8::Local<v8::Object> image_obj = Nan::New(constructor)->GetFunction()->NewInstance(1, &ext);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), image_obj };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Encode this image into a buffer of encoded data
 *
 * @name encodeSync
 * @param {string} [format=png] image format
 * @returns {Buffer} encoded image data
 * @instance
 * @memberof mapnik.Image
 * @example
 * var fs = require('fs');
 * fs.writeFileSync('myimage.png', myImage.encodeSync('png'));
 */
NAN_METHOD(Image::encodeSync)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() >= 1){
        if (!info[0]->IsString()) {
            Nan::ThrowTypeError("first arg, 'format' must be a string");
            return;
        }
        format = TOSTR(info[0]);
    }


    // options hash
    if (info.Length() >= 2) {
        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("palette").ToLocalChecked()))
        {
            v8::Local<v8::Value> format_opt = options->Get(Nan::New("palette").ToLocalChecked());
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }
            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
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

        info.GetReturnValue().Set(Nan::CopyBuffer((char*)s.data(), s.size()).ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string format;
    palette_ptr palette;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    std::string result;
} encode_image_baton_t;

/**
 * Encode this image into a buffer of encoded data
 *
 * @name encode
 * @param {string} [format=png] image format
 * @param {Function} callback
 * @returns {Buffer} encoded image data
 * @instance
 * @memberof mapnik.Image
 * @example
 * var fs = require('fs');
 * myImage.encode('png', function(err, encoded) {
 *   fs.writeFileSync('myimage.png', encoded);
 * });
 */
NAN_METHOD(Image::encode)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() >= 1){
        if (!info[0]->IsString()) {
            Nan::ThrowTypeError("first arg, 'format' must be a string");
            return;
        }
        format = TOSTR(info[0]);
    }

    // options hash
    if (info.Length() >= 2) {
        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[1].As<v8::Object>();

        if (options->Has(Nan::New("palette").ToLocalChecked()))
        {
            v8::Local<v8::Value> format_opt = options->Get(Nan::New("palette").ToLocalChecked());
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt.As<v8::Object>();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }

            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    encode_image_baton_t *closure = new encode_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->format = format;
    closure->palette = palette;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    im->Ref();

    return;
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
    Nan::HandleScope scope;

    encode_image_baton_t *closure = static_cast<encode_image_baton_t *>(req->data);

    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::CopyBuffer((char*)closure->result.data(), closure->result.size()).ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Get a constrained view of this image given x, y, width, height parameters.
 * @memberof mapnik.Image
 * @instance
 * @name view
 * @param {number} x
 * @param {number} y
 * @param {number} width
 * @param {number} height
 * @returns {mapnik.Image} an image constrained to this new view
 */
NAN_METHOD(Image::view)
{
    if ( (info.Length() != 4) || (!info[0]->IsNumber() && !info[1]->IsNumber() && !info[2]->IsNumber() && !info[3]->IsNumber() )) {
        Nan::ThrowTypeError("requires 4 integer arguments: x, y, width, height");
        return;
    }

    // TODO parse args
    unsigned x = info[0]->IntegerValue();
    unsigned y = info[1]->IntegerValue();
    unsigned w = info[2]->IntegerValue();
    unsigned h = info[3]->IntegerValue();

    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(ImageView::NewInstance(im,x,y,w,h));
}

/**
 * Encode this image and save it to disk as a file.
 *
 * @name saveSync
 * @param {string} filename
 * @param {string} [format=png]
 * @instance
 * @memberof mapnik.Image
 * @example
 * myImage.saveSync('foo.png');
 */
NAN_METHOD(Image::saveSync)
{
    info.GetReturnValue().Set(_saveSync(info));
}

v8::Local<v8::Value> Image::_saveSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    
    if (info.Length() == 0 || !info[0]->IsString()){
        Nan::ThrowTypeError("filename required to save file");
        return scope.Escape(Nan::Undefined());
    }
    
    std::string filename = TOSTR(info[0]);
    std::string format("");

    if (info.Length() >= 2) {
        if (!info[1]->IsString()) {
            Nan::ThrowTypeError("both 'filename' and 'format' arguments must be strings");
            return scope.Escape(Nan::Undefined());
        }
        format = TOSTR(info[1]);
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Nan::ThrowError(s.str().c_str());
            return scope.Escape(Nan::Undefined());
        }
    }

    try
    {
        mapnik::save_to_file(*(im->this_),filename, format);
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string format;
    std::string filename;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} save_image_baton_t;

/**
 * Encode this image and save it to disk as a file.
 *
 * @name save
 * @param {string} filename
 * @param {string} [format=png]
 * @param {Function} callback
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::save)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    
    if (info.Length() == 0 || !info[0]->IsString()){
        Nan::ThrowTypeError("filename required to save file");
        return;
    }

    if (!info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_saveSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    
    std::string filename = TOSTR(info[0]);
    std::string format("");

    if (info.Length() >= 3) {
        if (!info[1]->IsString()) {
            Nan::ThrowTypeError("both 'filename' and 'format' arguments must be strings");
            return;
        }
        format = TOSTR(info[1]);
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Nan::ThrowError(s.str().c_str());
            return;
        }
    }

    save_image_baton_t *closure = new save_image_baton_t();
    closure->request.data = closure;
    closure->format = format;
    closure->filename = filename;
    closure->im = im;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Save, (uv_after_work_cb)EIO_AfterSave);
    im->Ref();
    return;
}

void Image::EIO_Save(uv_work_t* req)
{
    save_image_baton_t *closure = static_cast<save_image_baton_t *>(req->data);
    try
    {
        mapnik::save_to_file(*(closure->im->this_),
                             closure->filename, 
                             closure->format);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterSave(uv_work_t* req)
{
    Nan::HandleScope scope;
    save_image_baton_t *closure = static_cast<save_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
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
    Nan::Persistent<v8::Function> cb;
} composite_image_baton_t;

/**
 * Overlay this image with another image, creating a layered composite as
 * a new image
 *
 * @name composite
 * @param {mapnik.Image} other
 * @param {Function} callback
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::composite)
{
    if (info.Length() < 1){
        Nan::ThrowTypeError("requires at least one argument: an image mask");
        return;
    }

    if (!info[0]->IsObject()) {
        Nan::ThrowTypeError("first argument must be an image mask");
        return;
    }

    v8::Local<v8::Object> im2 = info[0].As<v8::Object>();
    if (im2->IsNull() || im2->IsUndefined() || !Nan::New(Image::constructor)->HasInstance(im2))
    {
        Nan::ThrowTypeError("mapnik.Image expected as first arg");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    Image * dest_image = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    Image * source_image = Nan::ObjectWrap::Unwrap<Image>(im2);

    if (!dest_image->this_->get_premultiplied())
    {
        Nan::ThrowTypeError("destination image must be premultiplied");
        return;
    }

    if (!source_image->this_->get_premultiplied())
    {
        Nan::ThrowTypeError("source image must be premultiplied");
        return;
    }

    mapnik::composite_mode_e mode = mapnik::src_over;
    float opacity = 1.0;
    std::vector<mapnik::filter::filter_type> filters;
    int dx = 0;
    int dy = 0;
    if (info.Length() >= 2) {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[1].As<v8::Object>();

        if (options->Has(Nan::New("comp_op").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("comp_op").ToLocalChecked());
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("comp_op must be a mapnik.compositeOp value");
                return;
            }
            mode = static_cast<mapnik::composite_mode_e>(opt->IntegerValue());
            if (mode > mapnik::composite_mode_e::divide || mode < 0)
            {
                Nan::ThrowTypeError("Invalid comp_op value");
                return;
            }
        }

        if (options->Has(Nan::New("opacity").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("opacity").ToLocalChecked());
            if (!opt->IsNumber()) {
                Nan::ThrowTypeError("opacity must be a floating point number");
                return;
            }
            opacity = opt->NumberValue();
            if (opacity < 0 || opacity > 1) {
                Nan::ThrowTypeError("opacity must be a floating point number between 0-1");
                return;
            }
        }

        if (options->Has(Nan::New("dx").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("dx").ToLocalChecked());
            if (!opt->IsNumber()) {
                Nan::ThrowTypeError("dx must be an integer");
                return;
            }
            dx = opt->IntegerValue();
        }

        if (options->Has(Nan::New("dy").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("dy").ToLocalChecked());
            if (!opt->IsNumber()) {
                Nan::ThrowTypeError("dy must be an integer");
                return;
            }
            dy = opt->IntegerValue();
        }

        if (options->Has(Nan::New("image_filters").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("image_filters").ToLocalChecked());
            if (!opt->IsString()) {
                Nan::ThrowTypeError("image_filters argument must string of filter names");
                return;
            }
            std::string filter_str = TOSTR(opt);
            bool result = mapnik::filter::parse_image_filters(filter_str, filters);
            if (!result)
            {
                Nan::ThrowTypeError("could not parse image_filters");
                return;
            }
        }
    }

    composite_image_baton_t *closure = new composite_image_baton_t();
    closure->request.data = closure;
    closure->im1 = dest_image;
    closure->im2 = source_image;
    closure->mode = mode;
    closure->opacity = opacity;
    closure->filters = std::move(filters);
    closure->dx = dx;
    closure->dy = dy;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Composite, (uv_after_work_cb)EIO_AfterComposite);
    closure->im1->Ref();
    closure->im2->Ref();
    return;
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
            mapnik::premultiply_alpha(*closure->im2->this_);
        }
        mapnik::composite(*closure->im1->this_,*closure->im2->this_, closure->mode, closure->opacity, closure->dx, closure->dy);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterComposite(uv_work_t* req)
{
    Nan::HandleScope scope;

    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    if (closure->error) 
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    } else {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im1->handle() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->im1->Unref();
    closure->im2->Unref();
    closure->cb.Reset();
    delete closure;
}

NAN_GETTER(Image::get_scaling)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(im->this_->get_scaling()));
}

NAN_GETTER(Image::get_offset)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(im->this_->get_offset()));
}

NAN_SETTER(Image::set_scaling)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    } 
    else 
    {
        double val = value->NumberValue();
        if (val == 0.0)
        {
            Nan::ThrowError("Scaling value can not be zero");
            return;
        }
        im->this_->set_scaling(val);
    }
}

NAN_SETTER(Image::set_offset)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    } 
    else 
    {
        double val = value->NumberValue();
        im->this_->set_offset(val);
    }
}

/**
 * Return a copy of the pixel data in this image as a buffer
 *
 * @name data
 * @instance
 * @memberof mapnik.Image
 */
NAN_METHOD(Image::data)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    // TODO - make this zero copy
    info.GetReturnValue().Set(Nan::CopyBuffer(reinterpret_cast<const char *>(im->this_->bytes()), im->this_->size()).ToLocalChecked());
}

