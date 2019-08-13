// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_reader.hpp>      // for get_image_reader, etc
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc
#include <mapnik/image_copy.hpp>

#include <mapnik/image_compositing.hpp>
#include <mapnik/image_filter_types.hpp>
#include <mapnik/image_filter.hpp>      // filter_visitor
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
 * **`mapnik.Image`**
 *
 * Create a new image object (surface) that can be used for rendering data to.
 * @class Image
 * @param {number} width - width in pixels
 * @param {number} height - height in pixels
 * @param {Object} [options]
 * @param {Object<mapnik.imageType>} [options.type=mapnik.imageType.rgb8] - a {@link mapnik.imageType} object
 * @param {boolean} [options.initialize=true]
 * @param {boolean} [options.premultiplied=false]
 * @param {boolean} [options.painted=false]
 * @property {number} offset - offset number
 * @property {number} scaling - scaling number
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
    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "open",
                    Image::open);
    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "fromBytes",
                    Image::fromBytes);
    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "openSync",
                    Image::openSync);
    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "fromBytesSync",
                    Image::fromBytesSync);
    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "fromBufferSync",
                    Image::fromBufferSync);
    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "fromSVG",
                    Image::fromSVG);
    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "fromSVGSync",
                    Image::fromSVGSync);
    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "fromSVGBytes",
                    Image::fromSVGBytes);
    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "fromSVGBytesSync",
                    Image::fromSVGBytesSync);
    Nan::Set(target, Nan::New("Image").ToLocalChecked(),Nan::GetFunction(lcons).ToLocalChecked());
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
                if (Nan::Has(options, Nan::New("type").ToLocalChecked()).FromMaybe(false))
                {
                    v8::Local<v8::Value> init_val = Nan::Get(options, Nan::New("type").ToLocalChecked()).ToLocalChecked();

                    if (!init_val.IsEmpty() && init_val->IsNumber())
                    {
                        int int_val = Nan::To<int>(init_val).FromJust();
                        if (int_val >= mapnik::image_dtype::IMAGE_DTYPE_MAX || int_val < 0)
                        {
                            Nan::ThrowTypeError("Image 'type' must be a valid image type");
                            return;
                        }
                        type = static_cast<mapnik::image_dtype>(Nan::To<int>(init_val).FromJust());
                    }
                    else
                    {
                        Nan::ThrowTypeError("'type' option must be a valid 'mapnik.imageType'");
                        return;
                    }
                }

                if (Nan::Has(options, Nan::New("initialize").ToLocalChecked()).FromMaybe(false))
                {
                    v8::Local<v8::Value> init_val = Nan::Get(options, Nan::New("initialize").ToLocalChecked()).ToLocalChecked();
                    if (!init_val.IsEmpty() && init_val->IsBoolean())
                    {
                        initialize = Nan::To<bool>(init_val).FromJust();
                    }
                    else
                    {
                        Nan::ThrowTypeError("initialize option must be a boolean");
                        return;
                    }
                }

                if (Nan::Has(options, Nan::New("premultiplied").ToLocalChecked()).FromMaybe(false))
                {
                    v8::Local<v8::Value> pre_val = Nan::Get(options, Nan::New("premultiplied").ToLocalChecked()).ToLocalChecked();
                    if (!pre_val.IsEmpty() && pre_val->IsBoolean())
                    {
                        premultiplied = Nan::To<bool>(pre_val).FromJust();
                    }
                    else
                    {
                        Nan::ThrowTypeError("premultiplied option must be a boolean");
                        return;
                    }
                }

                if (Nan::Has(options, Nan::New("painted").ToLocalChecked()).FromMaybe(false))
                {
                    v8::Local<v8::Value> painted_val = Nan::Get(options, Nan::New("painted").ToLocalChecked()).ToLocalChecked();
                    if (!painted_val.IsEmpty() && painted_val->IsBoolean())
                    {
                        painted = Nan::To<bool>(painted_val).FromJust();
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

        try {
            Image* im = new Image(Nan::To<int>(info[0]).FromJust(),
                                  Nan::To<int>(info[1]).FromJust(),
                                  type,
                                  initialize,
                                  premultiplied,
                                  painted);
            im->Wrap(info.This());
            info.GetReturnValue().Set(info.This());
        } catch (std::exception const& ex) {
            Nan::ThrowError(ex.what());
        }
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
 * @memberof Image
 * @example
 * var img = new mapnik.Image(256, 256, {
 *   type: mapnik.imageType.gray8
 * });
 * var type = img.getType();
 * var typeCheck = mapnik.imageType.gray8;
 * console.log(type, typeCheck); // 1, 1
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
        /* LCOV_EXCL_STOP */

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
 * Get a specific pixel and its value
 * @name getPixel
 * @instance
 * @memberof Image
 * @param {number} x - position within image from top left
 * @param {number} y - position within image from top left
 * @param {Object} [options] the only valid option is `get_color`, which
 * should be a `boolean`. If set, the return is an Object with `rgba` values
 * instead of a pixel number.
 * @returns {number|Object} color number or object of rgba values
 * @example
 * // check for color after rendering image
 * var img = new mapnik.Image(4, 4);
 * var map = new mapnik.Map(4, 4);
 * map.background = new mapnik.Color('green');
 * map.render(img, {},function(err, img) {
 *   console.log(img.painted()); // false
 *   var pixel = img.getPixel(0,0);
 *   var values = img.getPixel(0,0, {get_color: true});
 *   console.log(pixel); // 4278222848
 *   console.log(values); // { premultiplied: false, a: 255, b: 0, g: 128, r: 0 }
 * });
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

        v8::Local<v8::Object> options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::Has(options, Nan::New("get_color").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("get_color").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsBoolean()) {
                Nan::ThrowTypeError("optional arg 'color' must be a boolean");
                return;
            }
            get_color = Nan::To<bool>(bind_opt).FromJust();
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
        x = Nan::To<int>(info[0]).FromJust();
        y = Nan::To<int>(info[1]).FromJust();
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
 * Set a pixels value
 * @name setPixel
 * @instance
 * @memberof Image
 * @param {number} x position within image from top left
 * @param {number} y position within image from top left
 * @param {Object|number} numeric or object representation of a color, typically used with {@link mapnik.Color}
 * @example
 * var gray = new mapnik.Image(256, 256);
 * gray.setPixel(0,0,new mapnik.Color('white'));
 * var pixel = gray.getPixel(0,0,{get_color:true});
 * console.log(pixel); // { premultiplied: false, a: 255, b: 255, g: 255, r: 255 }
 */
NAN_METHOD(Image::setPixel)
{
    if (info.Length() < 3 || (!info[0]->IsNumber() && !info[1]->IsNumber())) {
        Nan::ThrowTypeError("expects three arguments: x, y, and pixel value");
        return;
    }
    int x = Nan::To<int>(info[0]).FromJust();
    int y = Nan::To<int>(info[1]).FromJust();
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (x < 0 || x >= static_cast<int>(im->this_->width()) || y < 0 || y >= static_cast<int>(im->this_->height()))
    {
        Nan::ThrowTypeError("invalid pixel requested");
        return;
    }
    if (info[2]->IsUint32())
    {
        std::uint32_t val = Nan::To<std::uint32_t>(info[2]).FromJust();
        mapnik::set_pixel<std::uint32_t>(*im->this_,x,y,val);
    }
    else if (info[2]->IsInt32())
    {
        std::int32_t val = Nan::To<int32_t>(info[2]).FromJust();
        mapnik::set_pixel<std::int32_t>(*im->this_,x,y,val);
    }
    else if (info[2]->IsNumber())
    {
        double val = Nan::To<double>(info[2]).FromJust();
        mapnik::set_pixel<double>(*im->this_,x,y,val);
    }
    else if (info[2]->IsObject())
    {
        v8::Local<v8::Object> obj = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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
 * Compare the pixels of one image to the pixels of another. Returns the number
 * of pixels that are different. So, if the images are identical then it returns `0`.
 * And if the images share no common pixels it returns the total number of pixels
 * in an image which is equivalent to `im.width()*im.height()`.
 *
 * @name compare
 * @instance
 * @memberof Image
 * @param {mapnik.Image} image - another {@link mapnik.Image} instance to compare to
 * @param {Object} [options]
 * @param {number} [options.threshold=16] - A value that should be `0` or greater to
 * determine if the pixels match. Defaults to 16 which means that `rgba(0,0,0,0)`
 * would be considered the same as `rgba(15,15,15,0)`.
 * @param {boolean} [options.alpha=true] - `alpha` value, along with `rgb`, is considered
 * when comparing pixels
 * @returns {number} quantified visual difference between these two images in "number of
 * pixels" (i.e. `80` pixels are different);
 * @example
 * // start with the exact same images
 * var img1 = new mapnik.Image(2,2);
 * var img2 = new mapnik.Image(2,2);
 * console.log(img1.compare(img2)); // 0
 *
 * // change 1 pixel in img2
 * img2.setPixel(0,0, new mapnik.Color('green'));
 * console.log(img1.compare(img2)); // 1
 *
 * // difference in color at first pixel
 * img1.setPixel(0,0, new mapnik.Color('red'));
 * console.log(img1.compare(img2)); // 1
 *
 * // two pixels different
 * img2.setPixel(0,1, new mapnik.Color('red'));
 * console.log(img1.compare(img2)); // 2
 *
 * // all pixels different
 * img2.setPixel(1,1, new mapnik.Color('blue'));
 * img2.setPixel(1,0, new mapnik.Color('blue'));
 * console.log(img1.compare(img2)); // 4
 */
NAN_METHOD(Image::compare)
{
    if (info.Length() < 1 || !info[0]->IsObject()) {
        Nan::ThrowTypeError("first argument should be a mapnik.Image");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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

        v8::Local<v8::Object> options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::Has(options, Nan::New("threshold").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("threshold").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'threshold' must be a number");
                return;
            }
            threshold = Nan::To<int>(bind_opt).FromJust();
        }

        if (Nan::Has(options, Nan::New("alpha").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("alpha").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsBoolean()) {
                Nan::ThrowTypeError("optional arg 'alpha' must be a boolean");
                return;
            }
            alpha = Nan::To<bool>(bind_opt).FromJust();
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

/**
 * Apply a filter to this image. This changes all pixel values. (synchronous)
 *
 * @name filterSync
 * @instance
 * @memberof Image
 * @param {string} filter - can be `blur`, `emboss`, `sharpen`,
 * `sobel`, or `gray`.
 * @example
 * var img = new mapnik.Image(5, 5);
 * img.filter('blur');
 * // your custom code with `img` having blur applied
 */
NAN_METHOD(Image::filterSync)
{
    info.GetReturnValue().Set(_filterSync(info));
}

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
 * Apply a filter to this image. Changes all pixel values.
 *
 * @name filter
 * @instance
 * @memberof Image
 * @param {string} filter - can be `blur`, `emboss`, `sharpen`,
 * `sobel`, or `gray`.
 * @param {Function} callback - `function(err, img)`
 * @example
 * var img = new mapnik.Image(5, 5);
 * img.filter('sobel', function(err, img) {
 *   if (err) throw err;
 *   // your custom `img` with sobel filter
 *   // https://en.wikipedia.org/wiki/Sobel_operator
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
    Nan::AsyncResource async_resource(__func__);
    filter_image_baton_t *closure = static_cast<filter_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}


/**
 * Fill this image with a given color. Changes all pixel values. (synchronous)
 *
 * @name fillSync
 * @instance
 * @memberof Image
 * @param {mapnik.Color|number} color
 * @example
 * var img = new mapnik.Image(5,5);
 * // blue pixels
 * img.fillSync(new mapnik.Color('blue'));
 * var colors = img.getPixel(0,0, {get_color: true});
 * // blue value is filled
 * console.log(colors.b); // 255
 */
NAN_METHOD(Image::fillSync)
{
    info.GetReturnValue().Set(_fillSync(info));
}

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
            std::uint32_t val = Nan::To<std::uint32_t>(info[0]).FromJust();
            mapnik::fill<std::uint32_t>(*im->this_,val);
        }
        else if (info[0]->IsInt32())
        {
            std::int32_t val = Nan::To<int32_t>(info[0]).FromJust();
            mapnik::fill<std::int32_t>(*im->this_,val);
        }
        else if (info[0]->IsNumber())
        {
            double val = Nan::To<double>(info[0]).FromJust();
            mapnik::fill<double>(*im->this_,val);
        }
        else if (info[0]->IsObject())
        {
            v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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
 * Fill this image with a given color. Changes all pixel values.
 *
 * @name fill
 * @instance
 * @memberof Image
 * @param {mapnik.Color|number} color
 * @param {Function} callback - `function(err, img)`
 * @example
 * var img = new mapnik.Image(5,5);
 * img.fill(new mapnik.Color('blue'), function(err, img) {
 *   if (err) throw err;
 *   var colors = img.getPixel(0,0, {get_color: true});
 *   pixel is colored blue
 *   console.log(color.b); // 255
 * });
 *
 * // or fill with rgb string
 * img.fill('rgba(255,255,255,0)', function(err, img) { ... });
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
        closure->val_u32 = Nan::To<std::uint32_t>(info[0]).FromJust();
        closure->type = FILL_UINT32;
    }
    else if (info[0]->IsInt32())
    {
        closure->val_32 = Nan::To<int32_t>(info[0]).FromJust();
        closure->type = FILL_INT32;
    }
    else if (info[0]->IsNumber())
    {
        closure->val_double = Nan::To<double>(info[0]).FromJust();
        closure->type = FILL_DOUBLE;
    }
    else if (info[0]->IsObject())
    {
        v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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
    Nan::AsyncResource async_resource(__func__);
    fill_image_baton_t *closure = static_cast<fill_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Make this image transparent. (synchronous)
 *
 * @name clearSync
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image(5,5);
 * img.fillSync(1);
 * console.log(img.getPixel(0, 0)); // 1
 * img.clearSync();
 * console.log(img.getPixel(0, 0)); // 0
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
 * @memberof Image
 * @example
 * var img = new mapnik.Image(5,5);
 * img.fillSync(1);
 * console.log(img.getPixel(0, 0)); // 1
 * img.clear(function(err, result) {
 *   console.log(result.getPixel(0,0)); // 0
 * });
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
    Nan::AsyncResource async_resource(__func__);
    clear_image_baton_t *closure = static_cast<clear_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Convert all grayscale values to alpha values. Great for creating
 * a mask layer based on alpha values.
 *
 * @name setGrayScaleToAlpha
 * @memberof Image
 * @instance
 * @param {mapnik.Color} color
 * @example
 * var image = new mapnik.Image(2,2);
 * image.fillSync(new mapnik.Color('rgba(0,0,0,255)'));
 * console.log(image.getPixel(0,0, {get_color:true})); // { premultiplied: false, a: 255, b: 0, g: 0, r: 0 }
 *
 * image.setGrayScaleToAlpha();
 * // turns a black pixel into a completely transparent mask
 * console.log(image.getPixel(0,0, {get_color:true})); // { premultiplied: false, a: 0, b: 255, g: 255, r: 255 }
 */
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

        v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

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
 * https://en.wikipedia.org/wiki/Alpha_compositing
 *
 * @name premultiplied
 * @memberof Image
 * @instance
 * @returns {boolean} premultiplied `true` if the image is premultiplied
 * @example
 * var img = new mapnik.Image(5,5);
 * console.log(img.premultiplied()); // false
 * img.premultiplySync()
 * console.log(img.premultiplied()); // true
 */
NAN_METHOD(Image::premultiplied)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    bool premultiplied = im->this_->get_premultiplied();
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(premultiplied));
}

/**
 * Premultiply the pixels in this image.
 *
 * @name premultiplySync
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image(5,5);
 * img.premultiplySync();
 * console.log(img.premultiplied()); // true
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
 * @memberof Image
 * @instance
 * @param {Function} callback
 * @example
 * var img = new mapnik.Image(5,5);
 * img.premultiply(function(err, img) {
 *   if (err) throw err;
 *   // your custom code with premultiplied img
 * })
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
    Nan::AsyncResource async_resource(__func__);
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
    async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Demultiply the pixels in this image. The opposite of
 * premultiplying.
 *
 * @name demultiplySync
 * @instance
 * @memberof Image
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
 * @memberof Image
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

/**
 * Test if an image's pixels are all exactly the same
 * @name isSolid
 * @memberof Image
 * @instance
 * @returns {boolean} `true` means all pixels are exactly the same
 * @example
 * var img = new mapnik.Image(2,2);
 * console.log(img.isSolid()); // true
 *
 * // change a pixel
 * img.setPixel(0,0, new mapnik.Color('green'));
 * console.log(img.isSolid()); // false
 */
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
    Nan::AsyncResource async_resource(__func__);
    is_solid_image_baton_t *closure = static_cast<is_solid_image_baton_t *>(req->data);
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            v8::Local<v8::Value> argv[3] = { Nan::Null(),
                                     Nan::New(closure->result),
                                     mapnik::util::apply_visitor(visitor_get_pixel(0,0),*(closure->im->this_)),
            };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 3, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::New(closure->result) };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
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
 * @memberof Image
 * @example
 * var img = new mapnik.Image(256, 256);
 * var view = img.view(0, 0, 256, 256);
 * console.log(view.isSolidSync()); // true
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
    image_ptr im2;
    mapnik::image_dtype type;
    double offset;
    double scaling;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
} copy_image_baton_t;

/**
 * Copy an image into a new image by creating a clone
 * @name copy
 * @instance
 * @memberof Image
 * @param {number} type - image type to clone into, can be any mapnik.imageType number
 * @param {Object} [options={}]
 * @param {number} [options.scaling] - scale the image
 * @param {number} [options.offset] - offset this image
 * @param {Function} callback
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray16});
 * var img2 = img.copy(mapnik.imageType.gray8, function(err, img2) {
 *   if (err) throw err;
 *   // custom code with `img2` converted into gray8 type
 * });
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
            type = static_cast<mapnik::image_dtype>(Nan::To<int>(info[0]).FromJust());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Nan::ThrowTypeError("Image 'type' must be a valid image type");
                return;
            }
        }
        else if (info[0]->IsObject())
        {
            options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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
            options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as second argument");
            return;
        }
    }

    if (Nan::Has(options, Nan::New("scaling").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> scaling_val = Nan::Get(options, Nan::New("scaling").ToLocalChecked()).ToLocalChecked();
        if (scaling_val->IsNumber())
        {
            scaling = Nan::To<double>(scaling_val).FromJust();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("scaling argument must be a number");
            return;
        }
    }

    if (Nan::Has(options, Nan::New("offset").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> offset_val = Nan::Get(options, Nan::New("offset").ToLocalChecked()).ToLocalChecked();
        if (offset_val->IsNumber())
        {
            offset = Nan::To<double>(offset_val).FromJust();
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
    Nan::AsyncResource async_resource(__func__);
    copy_image_baton_t *closure = static_cast<copy_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else if (!closure->im2)
    {
        // Not quite sure if this is even required or ever can be reached, but leaving it
        // and simply removing it from coverage tests.
        /* LCOV_EXCL_START */
        v8::Local<v8::Value> argv[1] = { Nan::Error("could not render to image") };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    }
    else
    {
        Image* im = new Image(closure->im2);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            v8::Local<v8::Value> argv[1] = { Nan::Error("Could not create new Image instance") };
           async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->im1->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Copy an image into a new image by creating a clone
 * @name copySync
 * @instance
 * @memberof Image
 * @param {number} type - image type to clone into, can be any mapnik.imageType number
 * @param {Object} [options={}]
 * @param {number} [options.scaling] - scale the image
 * @param {number} [options.offset] - offset this image
 * @returns {mapnik.Image} copy
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray16});
 * var img2 = img.copy(mapnik.imageType.gray8);
 * // custom code with `img2` as a gray8 type
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
            type = static_cast<mapnik::image_dtype>(Nan::To<int>(info[0]).FromJust());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Nan::ThrowTypeError("Image 'type' must be a valid image type");
                return scope.Escape(Nan::Undefined());
            }
        }
        else if (info[0]->IsObject())
        {
            options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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
            options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as second argument");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (Nan::Has(options, Nan::New("scaling").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> scaling_val = Nan::Get(options, Nan::New("scaling").ToLocalChecked()).ToLocalChecked();
        if (scaling_val->IsNumber())
        {
            scaling = Nan::To<double>(scaling_val).FromJust();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("scaling argument must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (Nan::Has(options, Nan::New("offset").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> offset_val = Nan::Get(options, Nan::New("offset").ToLocalChecked()).ToLocalChecked();
        if (offset_val->IsNumber())
        {
            offset = Nan::To<double>(offset_val).FromJust();
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
        image_ptr imagep = std::make_shared<mapnik::image_any>(
            mapnik::image_copy(*(im->this_),
                               type,
                               offset,
                               scaling)
            );
        Image* new_im = new Image(imagep);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(new_im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        return scope.Escape(maybe_local.ToLocalChecked());
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
    image_ptr im2;
    mapnik::scaling_method_e scaling_method;
    std::size_t size_x;
    std::size_t size_y;
    double offset_x;
    double offset_y;
    double offset_width;
    double offset_height;
    double filter_factor;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
} resize_image_baton_t;

/**
 * Resize this image (makes a copy)
 *
 * @name resize
 * @instance
 * @memberof Image
 * @param {number} width - in pixels
 * @param {number} height - in pixels
 * @param {Object} [options={}]
 * @param {number} [options.offset_x=0] - offset the image horizontally in pixels
 * @param {number} [options.offset_y=0] - offset the image vertically in pixels
 * @param {number} [options.offset_width] - the width from the start of the offset_x to use from source image
 * @param {number} [options.offset_height] - the height from the start of the offset_y to use from source image
 * @param {mapnik.imageScaling} [options.scaling_method=mapnik.imageScaling.near] - scaling method
 * @param {number} [options.filter_factor=1.0]
 * @param {Function} callback - `function(err, result)`
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray8});
 * img.resize(8, 8, function(err, result) {
 *   if (err) throw err;
 *   // new image object as `result`
 * });
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
    double offset_x = 0.0;
    double offset_y = 0.0;
    double offset_width = static_cast<double>(im1->this_->width());
    double offset_height = static_cast<double>(im1->this_->height());
    double filter_factor = 1.0;
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();

    if (info.Length() >= 3)
    {
        if (info[0]->IsNumber())
        {
            auto width_tmp = Nan::To<int>(info[0]).FromJust();
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
            auto height_tmp = Nan::To<int>(info[1]).FromJust();
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
        Nan::ThrowTypeError("resize requires a width and height parameter.");
        return;
    }
    if (info.Length() >= 4)
    {
        if (info[2]->IsObject())
        {
            options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as third argument");
            return;
        }
    }
    if (Nan::Has(options, Nan::New("offset_x").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_x").ToLocalChecked()).ToLocalChecked();
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
            return;
        }
        offset_x = Nan::To<double>(bind_opt).FromJust();
    }
    if (Nan::Has(options, Nan::New("offset_y").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_y").ToLocalChecked()).ToLocalChecked();
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
            return;
        }
        offset_y = Nan::To<double>(bind_opt).FromJust();
    }
    if (Nan::Has(options, Nan::New("offset_width").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_width").ToLocalChecked()).ToLocalChecked();
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_width' must be a number");
            return;
        }
        offset_width = Nan::To<double>(bind_opt).FromJust();
        if (offset_width <= 0.0)
        {
            Nan::ThrowTypeError("optional arg 'offset_width' must be a integer greater then zero");
            return;
        }
    }
    if (Nan::Has(options, Nan::New("offset_height").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_height").ToLocalChecked()).ToLocalChecked();
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_height' must be a number");
            return;
        }
        offset_height = Nan::To<double>(bind_opt).FromJust();
        if (offset_height <= 0.0)
        {
            Nan::ThrowTypeError("optional arg 'offset_height' must be a integer greater then zero");
            return;
        }
    }
    if (Nan::Has(options, Nan::New("scaling_method").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> scaling_val = Nan::Get(options, Nan::New("scaling_method").ToLocalChecked()).ToLocalChecked();
        if (scaling_val->IsNumber())
        {
            std::int64_t scaling_int = Nan::To<int>(scaling_val).FromJust();
            if (scaling_int > mapnik::SCALING_BLACKMAN || scaling_int < 0)
            {
                Nan::ThrowTypeError("Invalid scaling_method");
                return;
            }
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_int);
        }
        else
        {
            Nan::ThrowTypeError("scaling_method argument must be an integer");
            return;
        }
    }

    if (Nan::Has(options, Nan::New("filter_factor").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> ff_val = Nan::Get(options, Nan::New("filter_factor").ToLocalChecked()).ToLocalChecked();
        if (ff_val->IsNumber())
        {
            filter_factor = Nan::To<double>(ff_val).FromJust();
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
    closure->offset_x = offset_x;
    closure->offset_y = offset_y;
    closure->offset_width = offset_width;
    closure->offset_height = offset_height;
    closure->filter_factor = filter_factor;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Resize, (uv_after_work_cb)EIO_AfterResize);
    closure->im1->Ref();
    return;
}

struct resize_visitor
{

    resize_visitor(mapnik::image_any & im1,
                   mapnik::scaling_method_e scaling_method,
                   double image_ratio_x,
                   double image_ratio_y,
                   double filter_factor,
                   double offset_x,
                   double offset_y) :
        im1_(im1),
        scaling_method_(scaling_method),
        image_ratio_x_(image_ratio_x),
        image_ratio_y_(image_ratio_y),
        filter_factor_(filter_factor),
        offset_x_(offset_x),
        offset_y_(offset_y) {}

    void operator()(mapnik::image_rgba8 & im2) const
    {
        bool remultiply = false;
        if (!im1_.get_premultiplied())
        {
            remultiply = true;
            mapnik::premultiply_alpha(im1_);
        }
        mapnik::scale_image_agg(im2,
                                mapnik::util::get<mapnik::image_rgba8>(im1_),
                                scaling_method_,
                                image_ratio_x_,
                                image_ratio_y_,
                                offset_x_,
                                offset_y_,
                                filter_factor_);
        if (remultiply) {
            mapnik::demultiply_alpha(im2);
        }
    }

    template <typename T>
    void operator()(T & im2) const
    {
        mapnik::scale_image_agg(im2,
                                mapnik::util::get<T>(im1_),
                                scaling_method_,
                                image_ratio_x_,
                                image_ratio_y_,
                                offset_x_,
                                offset_y_,
                                filter_factor_);
    }

    void operator()(mapnik::image_null &) const
    {
        // Should be caught earlier so no test coverage should reach here.
        /* LCOV_EXCL_START */
        throw std::runtime_error("Can not resize null images");
        /* LCOV_EXCL_STOP */
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
    mapnik::image_any & im1_;
    mapnik::scaling_method_e scaling_method_;
    double image_ratio_x_;
    double image_ratio_y_;
    double filter_factor_;
    int offset_x_;
    int offset_y_;

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
                                                           true,
                                                           true,
                                                           false);
        closure->im2->set_offset(offset);
        closure->im2->set_scaling(scaling);
        if (closure->offset_width <= 0 || closure->offset_height <= 0)
        {
            closure->error = true;
            closure->error_name = "Image width or height is zero or less then zero.";
            return;
        }

        double image_ratio_x = static_cast<double>(closure->size_x) / closure->offset_width;
        double image_ratio_y = static_cast<double>(closure->size_y) / closure->offset_height;
        double corrected_offset_x = closure->offset_x * image_ratio_x;
        double corrected_offset_y = closure->offset_y * image_ratio_y;
        resize_visitor visit(*(closure->im1->this_),
                             closure->scaling_method,
                             image_ratio_x,
                             image_ratio_y,
                             closure->filter_factor,
                             corrected_offset_x,
                             corrected_offset_y);
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
    Nan::AsyncResource async_resource(__func__);
    resize_image_baton_t *closure = static_cast<resize_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im2);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            v8::Local<v8::Value> argv[1] = { Nan::Error("Could not create new Image instance") };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->im1->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Resize this image (makes a copy). Synchronous version of {@link mapnik.Image.resize}.
 *
 * @name resizeSync
 * @instance
 * @memberof Image
 * @param {number} width
 * @param {number} height
 * @param {Object} [options={}]
 * @param {number} [options.offset_x=0] - offset the image horizontally in pixels
 * @param {number} [options.offset_y=0] - offset the image vertically in pixels
 * @param {number} [options.offset_width] - the width from the start of the offset_x to use from source image
 * @param {number} [options.offset_height] - the height from the start of the offset_y to use from source image
 * @param {mapnik.imageScaling} [options.scaling_method=mapnik.imageScaling.near] - scaling method
 * @param {number} [options.filter_factor=1.0]
 * @returns {mapnik.Image} copy
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray8});
 * var img2 = img.resizeSync(8, 8);
 * // new copy as `img2`
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
    double offset_x = 0.0;
    double offset_y = 0.0;
    double offset_width = static_cast<double>(im->this_->width());
    double offset_height = static_cast<double>(im->this_->height());
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() >= 2)
    {
        if (info[0]->IsNumber())
        {
            int width_tmp = Nan::To<int>(info[0]).FromJust();
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
            int height_tmp = Nan::To<int>(info[1]).FromJust();
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
            options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as third argument");
            return scope.Escape(Nan::Undefined());
        }
    }
    if (Nan::Has(options, Nan::New("offset_x").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_x").ToLocalChecked()).ToLocalChecked();
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
            return scope.Escape(Nan::Undefined());
        }
        offset_x = Nan::To<double>(bind_opt).FromJust();
    }
    if (Nan::Has(options, Nan::New("offset_y").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_y").ToLocalChecked()).ToLocalChecked();
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
            return scope.Escape(Nan::Undefined());
        }
        offset_y = Nan::To<double>(bind_opt).FromJust();
    }
    if (Nan::Has(options, Nan::New("offset_width").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_width").ToLocalChecked()).ToLocalChecked();
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_width' must be a number");
            return scope.Escape(Nan::Undefined());
        }
        offset_width = Nan::To<double>(bind_opt).FromJust();
        if (offset_width <= 0.0)
        {
            Nan::ThrowTypeError("optional arg 'offset_width' must be a integer greater then zero");
            return scope.Escape(Nan::Undefined());
        }
    }
    if (Nan::Has(options, Nan::New("offset_height").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_height").ToLocalChecked()).ToLocalChecked();
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_height' must be a number");
            return scope.Escape(Nan::Undefined());
        }
        offset_height = Nan::To<double>(bind_opt).FromJust();
        if (offset_height <= 0.0)
        {
            Nan::ThrowTypeError("optional arg 'offset_height' must be a integer greater then zero");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (Nan::Has(options, Nan::New("scaling_method").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> scaling_val = Nan::Get(options, Nan::New("scaling_method").ToLocalChecked()).ToLocalChecked();
        if (scaling_val->IsNumber())
        {
            std::int64_t scaling_int = Nan::To<int>(scaling_val).FromJust();
            if (scaling_int > mapnik::SCALING_BLACKMAN || scaling_int < 0)
            {
                Nan::ThrowTypeError("Invalid scaling_method");
                return scope.Escape(Nan::Undefined());
            }
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_int);
        }
        else
        {
            Nan::ThrowTypeError("scaling_method argument must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (Nan::Has(options, Nan::New("filter_factor").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> ff_val = Nan::Get(options, Nan::New("filter_factor").ToLocalChecked()).ToLocalChecked();
        if (ff_val->IsNumber())
        {
            filter_factor = Nan::To<double>(ff_val).FromJust();
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
    if (offset_width <= 0 || offset_height <= 0)
    {
        Nan::ThrowTypeError("Image width or height is zero or less then zero.");
        return scope.Escape(Nan::Undefined());
    }
    try
    {
        double offset = im->this_->get_offset();
        double scaling = im->this_->get_scaling();

        image_ptr imagep = std::make_shared<mapnik::image_any>(width,
                                                           height,
                                                           im->this_->get_dtype(),
                                                           true,
                                                           true,
                                                           false);
        imagep->set_offset(offset);
        imagep->set_scaling(scaling);
        double image_ratio_x = static_cast<double>(width) / offset_width;
        double image_ratio_y = static_cast<double>(height) / offset_height;
        double corrected_offset_x = offset_x * image_ratio_x;
        double corrected_offset_y = offset_y * image_ratio_y;
        resize_visitor visit(*(im->this_),
                             scaling_method,
                             image_ratio_x,
                             image_ratio_y,
                             filter_factor,
                             corrected_offset_x,
                             corrected_offset_y);
        mapnik::util::apply_visitor(visit, *imagep);
        Image* new_im = new Image(imagep);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(new_im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        return scope.Escape(maybe_local.ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}

/**
 * Check if this image is painted. "Painted" refers to if it has
 * data or not. An image created with `new mapnik.Image(4,4)` defaults to
 * `false` since we loaded a new image without rendering and have no idea
 * if it was painted or not. You can run `new mapnik.Image(4, 4, {painted: true})`
 * to manually set the `painted` value.
 *
 * @name painted
 * @instance
 * @memberof Image
 * @returns {boolean} whether it is painted or not
 * @example
 * var img = new mapnik.Image(5,5);
 * console.log(img.painted()); // false
 */
NAN_METHOD(Image::painted)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(im->this_->painted()));
}

/**
 * Get this image's width in pixels
 *
 * @name width
 * @instance
 * @memberof Image
 * @returns {number} width
 * @example
 * var img = new mapnik.Image(4,4);
 * console.log(img.width()); // 4
 */
NAN_METHOD(Image::width)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<std::int32_t>(im->this_->width())));
}

/**
 * Get this image's height in pixels
 *
 * @name height
 * @instance
 * @memberof Image
 * @returns {number} height
 * @example
 * var img = new mapnik.Image(4,4);
 * console.log(img.height()); // 4
 */
NAN_METHOD(Image::height)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<std::int32_t>(im->this_->height())));
}

/**
 * Load in a pre-existing image as an image object
 * @name openSync
 * @memberof Image
 * @instance
 * @param {string} path - path to the image you want to load
 * @returns {mapnik.Image} new image object based on existing image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.jpg');
 */
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
                image_ptr imagep = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(), reader->height()));
                if (!reader->has_alpha())
                {
                    mapnik::set_premultiplied_alpha(*imagep, true);
                }
                Image* im = new Image(imagep);
                v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
                Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
                if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
                return scope.Escape(maybe_local.ToLocalChecked());
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
    std::string error_name;
    Nan::Persistent<v8::Object> buffer;
    Nan::Persistent<v8::Function> cb;
    bool premultiply;
    std::uint32_t max_size;
    const char *data;
    size_t dataLength;
    bool error;
} image_mem_ptr_baton_t;

typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string filename;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} image_file_ptr_baton_t;

/**
 * Load in a pre-existing image as an image object
 * @name open
 * @memberof Image
 * @static
 * @param {string} path - path to the image you want to load
 * @param {Function} callback -
 * @example
 * mapnik.Image.open('./path/to/image.jpg', function(err, img) {
 *   if (err) throw err;
 *   // img is now an Image object
 * });
 */
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
                /* LCOV_EXCL_STOP */
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
    Nan::AsyncResource async_resource(__func__);
    image_file_ptr_baton_t *closure = static_cast<image_file_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            v8::Local<v8::Value> argv[1] = { Nan::Error("Could not create new Image instance") };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->cb.Reset();
    delete closure;
}

/**
 * Load image from an SVG buffer (synchronous)
 * @name fromSVGBytesSync
 * @memberof Image
 * @static
 * @param {string} path - path to SVG image
 * @param {Object} [options]
 * @param {number} [options.strict] - Throw on unhandled elements and invalid values in SVG. The default is `false`, which means that
 * unhandled elements will be ignored and invalid values will be set to defaults (and may not render correctly). If `true` then all occurances
 * of invalid value or unhandled elements will be collected and an error will be thrown reporting them all.
 * @param {number} [options.scale] - scale the image. For example passing `0.5` as scale would render
 * your SVG at 50% the original size.
 * @param {number} [options.max_size] - the maximum allowed size of the svg dimensions * scale. The default is 2048.
 * This option can be passed a smaller or larger size in order to control the final size of the image allocated for
 * rasterizing the SVG.
 * @returns {mapnik.Image} Image object
 * @example
 * var buffer = fs.readFileSync('./path/to/image.svg');
 * var img = mapnik.Image.fromSVGBytesSync(buffer);
 */
NAN_METHOD(Image::fromSVGBytesSync)
{
    info.GetReturnValue().Set(_fromSVGSync(false, info));
}

/**
 * Create a new image from an SVG file (synchronous)
 *
 * @name fromSVGSync
 * @param {string} filename
 * @param {Object} [options]
 * @param {number} [options.strict] - Throw on unhandled elements and invalid values in SVG. The default is `false`, which means that
 * unhandled elements will be ignored and invalid values will be set to defaults (and may not render correctly). If `true` then all occurances
 * of invalid value or unhandled elements will be collected and an error will be thrown reporting them all.
 * @param {number} [options.scale] - scale the image. For example passing `0.5` as scale would render
 * your SVG at 50% the original size.
 * @param {number} [options.max_size] - the maximum allowed size of the svg dimensions * scale. The default is 2048.
 * This option can be passed a smaller or larger size in order to control the final size of the image allocated for
 * rasterizing the SVG.
 * @returns {mapnik.Image} image object
 * @static
 * @memberof Image
 * @example
 * var img = mapnik.Image.fromSVG('./path/to/image.svg');
 */
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
    std::uint32_t max_size = 2048;
    bool strict = false;
    if (info.Length() >= 2)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> scale_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
            if (!scale_opt->IsNumber())
            {
                Nan::ThrowTypeError("'scale' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            scale = Nan::To<double>(scale_opt).FromJust();
            if (scale <= 0)
            {
                Nan::ThrowTypeError("'scale' must be a positive non zero number");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (Nan::Has(options, Nan::New("max_size").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("max_size").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("'max_size' must be a positive integer");
                return scope.Escape(Nan::Undefined());
            }
            auto max_size_val = Nan::To<int>(opt).FromJust();
            if (max_size_val < 0 || max_size_val > 65535) {
                Nan::ThrowTypeError("'max_size' must be a positive integer between 0 and 65535");
                return scope.Escape(Nan::Undefined());
            }
            max_size = static_cast<std::uint32_t>(max_size_val);
        }
        if (Nan::Has(options, Nan::New("strict").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("strict").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsBoolean())
            {
                Nan::ThrowTypeError("'strict' must be a boolean value");
                return scope.Escape(Nan::Undefined());
            }
            strict = Nan::To<bool>(opt).FromJust();
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
            p.parse(TOSTR(info[0]));
            if (strict && !p.err_handler().error_messages().empty())
            {
                std::ostringstream errorMessage;
                for (auto const& error : p.err_handler().error_messages()) {
                    errorMessage <<  error << std::endl;
                }
                Nan::ThrowTypeError(errorMessage.str().c_str());
                return scope.Escape(Nan::Undefined());
            }
        }
        else
        {
            v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
            {
                Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
                return scope.Escape(Nan::Undefined());
            }
            std::string svg_buffer(node::Buffer::Data(obj),node::Buffer::Length(obj));
            p.parse_from_string(svg_buffer);
            if (strict && !p.err_handler().error_messages().empty())
            {
                std::ostringstream errorMessage;
                for (auto const& error : p.err_handler().error_messages()) {
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
        double svg_width = svg.width() * scale;
        double svg_height = svg.height() * scale;

        if (svg_width <= 0 || svg_height <= 0)
        {
            Nan::ThrowTypeError("image created from svg must have a width and height greater then zero");
            return scope.Escape(Nan::Undefined());
        }

        if (svg_width > static_cast<double>(max_size) || svg_height > static_cast<double>(max_size))
        {
            std::stringstream s;
            s << "image created from svg must be " << max_size << " pixels or fewer on each side";
            Nan::ThrowTypeError(s.str().c_str());
            return scope.Escape(Nan::Undefined());
        }

        mapnik::image_rgba8 im(static_cast<int>(svg_width), static_cast<int>(svg_height), true, true);
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

        mapnik::svg::renderer_agg<mapnik::svg::svg_path_adapter,
            mapnik::svg_attribute_type,
            renderer_solid,
            agg::pixfmt_rgba32_pre > svg_renderer_this(svg_path,
                                                       marker_path->attributes());

        svg_renderer_this.render(ras_ptr, sl, renb, mtx, opacity, bbox);
        mapnik::demultiply_alpha(im);

        image_ptr imagep = std::make_shared<mapnik::image_any>(im);
        Image *im2 = new Image(imagep);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im2);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        return scope.Escape(maybe_local.ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        // There is currently no known way to make these operations throw an exception, however,
        // since the underlying agg library does possibly have some operation that might throw
        // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
}

typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string filename;
    bool error;
    double scale;
    std::uint32_t max_size;
    bool strict;
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
    std::uint32_t max_size;
    bool strict;
    std::string error_name;
    Nan::Persistent<v8::Object> buffer;
    Nan::Persistent<v8::Function> cb;
} svg_mem_ptr_baton_t;

/**
 * Create a new image from an SVG file
 *
 * @name fromSVG
 * @param {string} filename
 * @param {Object} [options]
 * @param {number} [options.strict] - Throw on unhandled elements and invalid values in SVG. The default is `false`, which means that
 * unhandled elements will be ignored and invalid values will be set to defaults (and may not render correctly). If `true` then all occurances
 * of invalid value or unhandled elements will be collected and an error will be thrown reporting them all.
 * @param {number} [options.scale] - scale the image. For example passing `0.5` as scale would render
 * your SVG at 50% the original size.
 * @param {number} [options.max_size] - the maximum allowed size of the svg dimensions * scale. The default is 2048.
 * This option can be passed a smaller or larger size in order to control the final size of the image allocated for
 * rasterizing the SVG.
 * @param {Function} callback
 * @static
 * @memberof Image
 * @example
 * mapnik.Image.fromSVG('./path/to/image.svg', {scale: 0.5}, function(err, img) {
 *   if (err) throw err;
 *   // new img object (at 50% scale)
 * });
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
    std::uint32_t max_size = 2048;
    bool strict = false;
    if (info.Length() >= 3)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> scale_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
            if (!scale_opt->IsNumber())
            {
                Nan::ThrowTypeError("'scale' must be a number");
                return;
            }
            scale = Nan::To<double>(scale_opt).FromJust();
            if (scale <= 0)
            {
                Nan::ThrowTypeError("'scale' must be a positive non zero number");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("max_size").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("max_size").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("'max_size' must be a positive integer");
                return;
            }
            auto max_size_val = Nan::To<int>(opt).FromJust();
            if (max_size_val < 0 || max_size_val > 65535) {
                Nan::ThrowTypeError("'max_size' must be a positive integer between 0 and 65535");
                return;
            }
            max_size = static_cast<std::uint32_t>(max_size_val);
        }
        if (Nan::Has(options, Nan::New("strict").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("strict").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsBoolean())
            {
                Nan::ThrowTypeError("'strict' must be a boolean value");
                return;
            }
            strict = Nan::To<bool>(opt).FromJust();
        }
    }

    svg_file_ptr_baton_t *closure = new svg_file_ptr_baton_t();
    closure->request.data = closure;
    closure->filename = TOSTR(info[0]);
    closure->error = false;
    closure->scale = scale;
    closure->max_size = max_size;
    closure->strict = strict;
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
        p.parse(closure->filename);
        if (closure->strict && !p.err_handler().error_messages().empty())
        {
            std::ostringstream errorMessage;
            for (auto const& error : p.err_handler().error_messages()) {
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

        double svg_width = svg.width() * closure->scale;
        double svg_height = svg.height() * closure->scale;

        if (svg_width <= 0 || svg_height <= 0)
        {
            closure->error = true;
            closure->error_name = "image created from svg must have a width and height greater then zero";
            return;
        }

        if (svg_width > static_cast<double>(closure->max_size) || svg_height > static_cast<double>(closure->max_size))
        {
            closure->error = true;
            std::stringstream s;
            s << "image created from svg must be " << closure->max_size << " pixels or fewer on each side";
            closure->error_name = s.str();
            return;
        }

        mapnik::image_rgba8 im(static_cast<int>(svg_width), static_cast<int>(svg_height), true, true);
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

        mapnik::svg::renderer_agg<mapnik::svg::svg_path_adapter,
            mapnik::svg_attribute_type,
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
        // LCOV_EXCL_STOP
    }
}

void Image::EIO_AfterFromSVG(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    svg_file_ptr_baton_t *closure = static_cast<svg_file_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            v8::Local<v8::Value> argv[1] = { Nan::Error("Could not create new Image instance") };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->cb.Reset();
    delete closure;
}

/**
 * Load image from an SVG buffer
 * @name fromSVGBytes
 * @memberof Image
 * @static
 * @param {string} path - path to SVG image
 * @param {Object} [options]
 * @param {number} [options.strict] - Throw on unhandled elements and invalid values in SVG. The default is `false`, which means that
 * unhandled elements will be ignored and invalid values will be set to defaults (and may not render correctly). If `true` then all occurances
 * of invalid value or unhandled elements will be collected and an error will be thrown reporting them all.
 * @param {number} [options.scale] - scale the image. For example passing `0.5` as scale would render
 * your SVG at 50% the original size.
 * @param {number} [options.max_size] - the maximum allowed size of the svg dimensions * scale. The default is 2048.
 * This option can be passed a smaller or larger size in order to control the final size of the image allocated for
 * rasterizing the SVG.
 * @param {Function} callback = `function(err, img)`
 * @example
 * var buffer = fs.readFileSync('./path/to/image.svg');
 * mapnik.Image.fromSVGBytes(buffer, function(err, img) {
 *   if (err) throw err;
 *   // your custom code with `img`
 * });
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

    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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
    std::uint32_t max_size = 2048;
    bool strict = false;
    if (info.Length() >= 3)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> scale_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
            if (!scale_opt->IsNumber())
            {
                Nan::ThrowTypeError("'scale' must be a number");
                return;
            }
            scale = Nan::To<double>(scale_opt).FromJust();
            if (scale <= 0)
            {
                Nan::ThrowTypeError("'scale' must be a positive non zero number");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("max_size").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("max_size").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("'max_size' must be a positive integer");
                return;
            }
            auto max_size_val = Nan::To<int>(opt).FromJust();
            if (max_size_val < 0 || max_size_val > 65535) {
                Nan::ThrowTypeError("'max_size' must be a positive integer between 0 and 65535");
                return;
            }
            max_size = static_cast<std::uint32_t>(max_size_val);
        }
        if (Nan::Has(options, Nan::New("strict").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("strict").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsBoolean())
            {
                Nan::ThrowTypeError("'strict' must be a boolean value");
                return;
            }
            strict = Nan::To<bool>(opt).FromJust();
        }
    }

    svg_mem_ptr_baton_t *closure = new svg_mem_ptr_baton_t();
    closure->request.data = closure;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->scale = scale;
    closure->max_size = max_size;
    closure->strict = strict;
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
        p.parse_from_string(svg_buffer);
        if (closure->strict && !p.err_handler().error_messages().empty())
        {
            std::ostringstream errorMessage;
            for (auto const& error : p.err_handler().error_messages()) {
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
        double svg_width = svg.width() * closure->scale;
        double svg_height = svg.height() * closure->scale;

        if (svg_width <= 0 || svg_height <= 0)
        {
            closure->error = true;
            closure->error_name = "image created from svg must have a width and height greater then zero";
            return;
        }

        if (svg_width > static_cast<double>(closure->max_size) || svg_height > static_cast<double>(closure->max_size))
        {
            closure->error = true;
            std::stringstream s;
            s << "image created from svg must be " << closure->max_size << " pixels or fewer on each side";
            closure->error_name = s.str();
            return;
        }

        mapnik::image_rgba8 im(static_cast<int>(svg_width), static_cast<int>(svg_height), true, true);
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

        mapnik::svg::renderer_agg<mapnik::svg::svg_path_adapter,
            mapnik::svg_attribute_type,
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
        // LCOV_EXCL_STOP
    }
}

void Image::EIO_AfterFromSVGBytes(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    svg_mem_ptr_baton_t *closure = static_cast<svg_mem_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            v8::Local<v8::Value> argv[1] = { Nan::Error("Could not create new Image instance") };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Create an image of the existing buffer.
 *
 * Note: the buffer must live as long as the image.
 * It is recommended that you do not use this method. Be warned!
 *
 * @name fromBufferSync
 * @param {number} width
 * @param {number} height
 * @param {Buffer} buffer
 * @returns {mapnik.Image} image object
 * @static
 * @memberof Image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * var buffer = img.data(); // returns data as buffer
 * var img2 = mapnik.Image.fromBufferSync(img.width(), img.height(), buffer);
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

    unsigned width = Nan::To<int>(info[0]).FromJust();
    unsigned height = Nan::To<int>(info[1]).FromJust();

    if (width <= 0 || height <= 0)
    {
        Nan::ThrowTypeError("width and height must be greater then zero");
        return scope.Escape(Nan::Undefined());
    }

    v8::Local<v8::Object> obj = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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
            if (Nan::Has(options, Nan::New("type").ToLocalChecked()).FromMaybe(false))
            {
                Nan::ThrowTypeError("'type' option not supported (only rgba images currently viable)");
                return scope.Escape(Nan::Undefined());
            }

            if (Nan::Has(options, Nan::New("premultiplied").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> pre_val = Nan::Get(options, Nan::New("premultiplied").ToLocalChecked()).ToLocalChecked();
                if (!pre_val.IsEmpty() && pre_val->IsBoolean())
                {
                    premultiplied = Nan::To<bool>(pre_val).FromJust();
                }
                else
                {
                    Nan::ThrowTypeError("premultiplied option must be a boolean");
                    return scope.Escape(Nan::Undefined());
                }
            }

            if (Nan::Has(options, Nan::New("painted").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> painted_val = Nan::Get(options, Nan::New("painted").ToLocalChecked()).ToLocalChecked();
                if (!painted_val.IsEmpty() && painted_val->IsBoolean())
                {
                    painted = Nan::To<bool>(painted_val).FromJust();
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
        image_ptr imagep = std::make_shared<mapnik::image_any>(im_wrapper);
        Image* im = new Image(imagep);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        v8::Local<v8::Object> image_obj = maybe_local.ToLocalChecked()->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        Nan::Set(image_obj, Nan::New("_buffer").ToLocalChecked(),obj);
        return scope.Escape(maybe_local.ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        // There is no known way for this exception to be reached currently.
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
}

/**
 * Create an image from a byte stream buffer. (synchronous)
 *
 * @name fromBytesSync
 * @param {Buffer} buffer - image buffer
 * @returns {mapnik.Image} image object
 * @instance
 * @memberof Image
 * @example
 * var buffer = fs.readFileSync('./path/to/image.png');
 * var img = new mapnik.Image.fromBytesSync(buffer);
 */
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

    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
        return scope.Escape(Nan::Undefined());
    }

    try
    {
        std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(node::Buffer::Data(obj),node::Buffer::Length(obj)));
        if (reader.get())
        {
            image_ptr imagep = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
            Image* im = new Image(imagep);
            v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
            Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
            if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
            return scope.Escape(maybe_local.ToLocalChecked());
        }
        // The only way this is ever reached is if the reader factory in
        // mapnik was not providing an image type it should. This should never
        // be occuring so marking this out from coverage
        /* LCOV_EXCL_START */
        Nan::ThrowTypeError("Failed to load from buffer");
        return scope.Escape(Nan::Undefined());
        /* LCOV_EXCL_STOP */
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}

/**
 * Create an image from a byte stream buffer.
 *
 * @name fromBytes
 * @param {Buffer} buffer - image buffer
 * @param {Object} [options]
 * @param {Boolean} [options.premultiply] - Default false, if true, then the image will be premultiplied before being returned
 * @param {Number} [options.max_size] - the maximum allowed size of the image dimensions. The default is 2048.
 * @param {Function} callback - `function(err, img)`
 * @static
 * @memberof Image
 * @example
 * var buffer = fs.readFileSync('./path/to/image.png');
 * mapnik.Image.fromBytes(buffer, function(err, img) {
 *   if (err) throw err;
 *   // your custom code with `img` object
 * });
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

    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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

    bool premultiply = false;
    std::uint32_t max_size = 2048;
    if (info.Length() >= 2)
    {
        if (info[1]->IsObject())
        {
            v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[1]);
            if (Nan::Has(options, Nan::New("premultiply").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("premultiply").ToLocalChecked()).ToLocalChecked();
                if (!opt.IsEmpty() && opt->IsBoolean())
                {
                    premultiply = Nan::To<bool>(opt).FromJust();
                }
                else
                {
                    Nan::ThrowTypeError("premultiply option must be a boolean");
                    return;
                }
            }
            if (Nan::Has(options, Nan::New("max_size").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("max_size").ToLocalChecked()).ToLocalChecked();
                if (opt->IsNumber())
                {
                    auto max_size_val = Nan::To<int>(opt).FromJust();
                    if (max_size_val < 0 || max_size_val > 65535) {
                        Nan::ThrowTypeError("max_size must be a positive integer between 0 and 65535");
                        return;
                    }
                    max_size = static_cast<std::uint32_t>(max_size_val);
                }
                else
                {
                    Nan::ThrowTypeError("max_size option must be a number");
                    return;
                }
            }
        }
    }

    image_mem_ptr_baton_t *closure = new image_mem_ptr_baton_t();
    closure->request.data = closure;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    closure->premultiply = premultiply;
    closure->max_size = max_size;
    closure->im = nullptr;
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
            if (reader->width() > closure->max_size || reader->height() > closure->max_size) {
                std::stringstream s;
                s << "image created from bytes must be " << closure->max_size << " pixels or fewer on each side";
                closure->error_name = s.str();
            } else {
                closure->im = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
                if (closure->premultiply) {
                    mapnik::premultiply_alpha(*closure->im);
                }
            }
        }
    }
    catch (std::exception const& ex)
    {
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterFromBytes(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    image_mem_ptr_baton_t *closure = static_cast<image_mem_ptr_baton_t *>(req->data);
    if (!closure->error_name.empty())
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else if (closure->im == nullptr)
    {
        /* LCOV_EXCL_START */
        // The only way this is ever reached is if the reader factory in
        // mapnik was not providing an image type it should. This should never
        // be occuring so marking this out from coverage
        v8::Local<v8::Value> argv[1] = { Nan::Error("Failed to load from buffer") };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    } else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            v8::Local<v8::Value> argv[1] = { Nan::Error("Could not create new Image instance") };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Encode this image into a buffer of encoded data (synchronous)
 *
 * @name encodeSync
 * @param {string} [format=png] image format
 * @param {Object} [options]
 * @param {mapnik.Palette} [options.palette] - mapnik.Palette object
 * @returns {Buffer} buffer - encoded image data
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * var buffer = img.encodeSync('png');
 * // write buffer to a new file
 * fs.writeFileSync('myimage.png', buffer);
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
        v8::Local<v8::Object> options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("palette").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("palette").ToLocalChecked()).ToLocalChecked();
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }
            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    try {
        std::unique_ptr<std::string> s;
        if (palette.get())
        {
            s = std::make_unique<std::string>(save_to_string(*(im->this_), format, *palette));
        }
        else {
            s = std::make_unique<std::string>(save_to_string(*(im->this_), format));
        }

        info.GetReturnValue().Set(node_mapnik::NewBufferFrom(std::move(s)).ToLocalChecked());
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
    std::unique_ptr<std::string> result;
} encode_image_baton_t;

/**
 * Encode this image into a buffer of encoded data
 *
 * @name encode
 * @param {string} [format=png] image format
 * @param {Object} [options]
 * @param {mapnik.Palette} [options.palette] - mapnik.Palette object
 * @param {Function} callback - `function(err, encoded)`
 * @returns {Buffer} encoded image data
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * myImage.encode('png', function(err, encoded) {
 *   if (err) throw err;
 *   // write buffer to new file
 *   fs.writeFileSync('myimage.png', encoded);
 * });
 *
 * // encoding an image object with a mapnik.Palette
 * var im = new mapnik.Image(256, 256);
 * var pal = new mapnik.Palette(new Buffer('\xff\x09\x93\xFF\x01\x02\x03\x04','ascii'));
 * im.encode('png', {palette: pal}, function(err, encode) {
 *   if (err) throw err;
 *   // your custom code with `encode` image buffer
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

        if (Nan::Has(options, Nan::New("palette").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("palette").ToLocalChecked()).ToLocalChecked();
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
            closure->result = std::make_unique<std::string>(save_to_string(*(closure->im->this_), closure->format, *closure->palette));
        }
        else
        {
            closure->result = std::make_unique<std::string>(save_to_string(*(closure->im->this_), closure->format));
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
    Nan::AsyncResource async_resource(__func__);

    encode_image_baton_t *closure = static_cast<encode_image_baton_t *>(req->data);

    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), node_mapnik::NewBufferFrom(std::move(closure->result)).ToLocalChecked() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Get a constrained view of this image given x, y, width, height parameters.
 * @memberof Image
 * @instance
 * @name view
 * @param {number} x
 * @param {number} y
 * @param {number} width
 * @param {number} height
 * @returns {mapnik.Image} an image constrained to this new view
 * @example
 * var img = new mapnik.Image(10, 10);
 * // This function says "starting from the 0/0 pixel, grab 5 pixels along
 * // the x-axis and 5 along the y-axis" which gives us a quarter of the original
 * // 10x10 pixel image
 * var img2 = img.view(0, 0, 5, 5);
 * console.log(img.width(), img2.width()); // 10, 5
 */
NAN_METHOD(Image::view)
{
    if ( (info.Length() != 4) || (!info[0]->IsNumber() && !info[1]->IsNumber() && !info[2]->IsNumber() && !info[3]->IsNumber() )) {
        Nan::ThrowTypeError("requires 4 integer arguments: x, y, width, height");
        return;
    }

    // TODO parse args
    unsigned x = Nan::To<int>(info[0]).FromJust();
    unsigned y = Nan::To<int>(info[1]).FromJust();
    unsigned w = Nan::To<int>(info[2]).FromJust();
    unsigned h = Nan::To<int>(info[3]).FromJust();

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
 * @memberof Image
 * @example
 * img.saveSync('foo.png');
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
 * @memberof Image
 * @example
 * img.save('image.png', 'png', function(err) {
 *   if (err) throw err;
 *   // your custom code
 * });
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
    Nan::AsyncResource async_resource(__func__);
    save_image_baton_t *closure = static_cast<save_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
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
 * @param {mapnik.Image} image - image to composite with
 * @param {Object} [options]
 * @param {mapnik.compositeOp} [options.comp_op] - compositing operation. Must be an integer
 * value that relates to a compositing operation.
 * @param {number} [options.opacity] - opacity must be a floating point number between 0-1
 * @param {number} [options.dx]
 * @param {number} [options.dy]
 * @param {string} [options.image_filters] - a string of filter names
 * @param {Function} callback
 * @instance
 * @memberof Image
 * @example
 * var img1 = new mapnik.Image.open('./path/to/image.png');
 * var img2 = new mapnik.Image.open('./path/to/another-image.png');
 * img1.composite(img2, {
 *   comp_op: mapnik.compositeOp['multiply'],
 *   dx: 0,
 *   dy: 0,
 *   opacity: 0.5,
 *   image_filters: 'invert agg-stack-blur(10,10)'
 * }, function(err, result) {
 *   if (err) throw err;
 *   // new image with `result`
 * });
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

        if (Nan::Has(options, Nan::New("comp_op").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("comp_op").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("comp_op must be a mapnik.compositeOp value");
                return;
            }
            int mode_int = Nan::To<int>(opt).FromJust();
            if (mode_int > static_cast<int>(mapnik::composite_mode_e::divide) || mode_int < 0)
            {
                Nan::ThrowTypeError("Invalid comp_op value");
                return;
            }
            mode = static_cast<mapnik::composite_mode_e>(mode_int);
        }

        if (Nan::Has(options, Nan::New("opacity").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("opacity").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsNumber()) {
                Nan::ThrowTypeError("opacity must be a floating point number");
                return;
            }
            opacity = Nan::To<double>(opt).FromJust();
            if (opacity < 0 || opacity > 1) {
                Nan::ThrowTypeError("opacity must be a floating point number between 0-1");
                return;
            }
        }

        if (Nan::Has(options, Nan::New("dx").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("dx").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsNumber()) {
                Nan::ThrowTypeError("dx must be an integer");
                return;
            }
            dx = Nan::To<int>(opt).FromJust();
        }

        if (Nan::Has(options, Nan::New("dy").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("dy").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsNumber()) {
                Nan::ThrowTypeError("dy must be an integer");
                return;
            }
            dy = Nan::To<int>(opt).FromJust();
        }

        if (Nan::Has(options, Nan::New("image_filters").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("image_filters").ToLocalChecked()).ToLocalChecked();
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
            // TODO: expose passing custom scale_factor: https://github.com/mapnik/mapnik/commit/b830469d2d574ac575ff24713935378894f8bdc9
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
    Nan::AsyncResource async_resource(__func__);

    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    } else {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im1->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
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
        double val = Nan::To<double>(value).FromJust();
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
        double val = Nan::To<double>(value).FromJust();
        im->this_->set_offset(val);
    }
}

/**
 * Return a copy of the pixel data in this image as a buffer
 *
 * @name data
 * @instance
 * @memberof Image
 * @returns {Buffer} pixel data as a buffer
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * var buffr = img.data();
 */
NAN_METHOD(Image::data)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    // TODO - make this zero copy
    info.GetReturnValue().Set(Nan::CopyBuffer(reinterpret_cast<const char *>(im->this_->bytes()), im->this_->size()).ToLocalChecked());
}
