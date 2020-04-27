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
//#include "mapnik_image_view.hpp"
#include "mapnik_palette.hpp"
#include "mapnik_color.hpp"

//#include "utils.hpp"
// agg
#include "agg_rasterizer_scanline_aa.h"
#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_renderer_base.h"
#include "agg_pixfmt_rgba.h"
#include "agg_scanline_u.h"

// boost
//#include <boost/optional/optional.hpp>

// std
#include <exception>
//#include <ostream>                      // for operator<<, basic_ostream
#include <sstream>                      // for basic_ostringstream, etc
#include <cstdlib>

Napi::FunctionReference Image::constructor;

Napi::Object Image::Initialize(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "Image", {
            InstanceMethod<&Image::getType>("getType"),
            InstanceMethod<&Image::encodeSync>("encodeSync"),
            InstanceMethod<&Image::encode>("encode"),
            InstanceMethod<&Image::fillSync>("fillSync"),
            InstanceMethod<&Image::fill>("fill"),
            InstanceMethod<&Image::width>("width"),
            InstanceMethod<&Image::height>("height"),
            InstanceMethod<&Image::setPixel>("setPixel"),
            InstanceMethod<&Image::getPixel>("getPixel"),
            InstanceMethod<&Image::compare>("compare"),
            StaticMethod<&Image::openSync>("openSync"),
            StaticMethod<&Image::open>("open")
        });
/*

    *InstanceMethod("getType", &getType),
    *InstanceMethod("getPixel", &getPixel),
    *InstanceMethod("setPixel", &setPixel),
    *InstanceMethod("encodeSync", &encodeSync),
    *InstanceMethod("encode", &encode),
    InstanceMethod("view", &view),
    InstanceMethod("saveSync", &saveSync),
    InstanceMethod("save", &save),
    InstanceMethod("setGrayScaleToAlpha", &setGrayScaleToAlpha),
    InstanceMethod("width", &width),
    InstanceMethod("height", &height),
    InstanceMethod("painted", &painted),
    InstanceMethod("composite", &composite),
    InstanceMethod("filter", &filter),
    InstanceMethod("filterSync", &filterSync),
    *InstanceMethod("fillSync", &fillSync),
    !InstanceMethod("fill", &fill),
    InstanceMethod("premultiplySync", &premultiplySync),
    InstanceMethod("premultiply", &premultiply),
    InstanceMethod("premultiplied", &premultiplied),
    InstanceMethod("demultiplySync", &demultiplySync),
    InstanceMethod("demultiply", &demultiply),
    InstanceMethod("clear", &clear),
    InstanceMethod("clearSync", &clearSync),
    *InstanceMethod("compare", &compare),
    InstanceMethod("isSolid", &isSolid),
    InstanceMethod("isSolidSync", &isSolidSync),
    InstanceMethod("copy", &copy),
    InstanceMethod("copySync", &copySync),
    InstanceMethod("resize", &resize),
    InstanceMethod("resizeSync", &resizeSync),
    InstanceMethod("data", &data),

    // properties
    ATTR(lcons, "scaling", get_scaling, set_scaling);
    ATTR(lcons, "offset", get_offset, set_offset);

    // This *must* go after the ATTR setting
    *Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
    *                "open",
    *                Image::open);

    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
                    "fromBytes",
                    Image::fromBytes);
    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
                    "openSync",
                    Image::openSync);
    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
                    "fromBytesSync",
                    Image::fromBytesSync);
    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
                    "fromBufferSync",
                    Image::fromBufferSync);
    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
                    "fromSVG",
                    Image::fromSVG);
    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
                    "fromSVGSync",
                    Image::fromSVGSync);
    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
                    "fromSVGBytes",
                    Image::fromSVGBytes);
    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
                    "fromSVGBytesSync",
                    Image::fromSVGBytesSync);
    (target).Set(Napi::String::New(env, "Image"),Napi::GetFunction(lcons));
    constructor.Reset(lcons);
*/

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Image", func);
    return exports;
}

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


//Image::Image(unsigned int width, unsigned int height, mapnik::image_dtype type, bool initialized, bool premultiplied, bool painted) : Napi::ObjectWrap<Image>(),
//    image_( std::make_shared<mapnik::image_any>(width,height,type,initialized,premultiplied,painted))
//{
//}

//Image::Image(image_ptr _this) : Napi::ObjectWrap<Image>(),
//    this_(_this)
//{
//}

Image::Image(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Image>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 1 && info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<image_ptr>>();
        image_ = *ext.Data();
        return;
    }
    if (info.Length() >= 2)
    {
        mapnik::image_dtype type = mapnik::image_dtype_rgba8;
        bool initialize = true;
        bool premultiplied = false;
        bool painted = false;
        if (!info[0].IsNumber() || !info[1].IsNumber())
        {
            Napi::TypeError::New(env, "Image 'width' and 'height' must be a integers").ThrowAsJavaScriptException();
            return;
        }
        if (info.Length() >= 3)
        {
            if (info[2].IsObject())
            {
                Napi::Object options = info[2].As<Napi::Object>();
                if (options.Has("type"))
                {
                    Napi::Value init_val = options.Get("type");

                    if (!init_val.IsEmpty() && init_val.IsNumber())
                    {
                        int int_val = init_val.As<Napi::Number>().Int32Value();
                        if (int_val >= mapnik::image_dtype::IMAGE_DTYPE_MAX || int_val < 0)
                        {
                            Napi::TypeError::New(env, "Image 'type' must be a valid image type")
                                .ThrowAsJavaScriptException();
                            return;
                        }
                        type = static_cast<mapnik::image_dtype>(init_val.As<Napi::Number>().Int32Value());
                    }
                    else
                    {
                        Napi::TypeError::New(env, "'type' option must be a valid 'mapnik.imageType'")
                            .ThrowAsJavaScriptException();
                        return;
                    }
                }

                if (options.Has("initialize"))
                {
                    Napi::Value init_val = options.Get("initialize");
                    if (!init_val.IsEmpty() && init_val.IsBoolean())
                    {
                        initialize = init_val.As<Napi::Boolean>();
                    }
                    else
                    {
                        Napi::TypeError::New(env, "initialize option must be a boolean").ThrowAsJavaScriptException();
                        return;
                    }
                }

                if (options.Has("premultiplied"))
                {
                    Napi::Value pre_val = options.Get("premultiplied");
                    if (!pre_val.IsEmpty() && pre_val.IsBoolean())
                    {
                        premultiplied = pre_val.As<Napi::Boolean>();
                    }
                    else
                    {
                        Napi::TypeError::New(env, "premultiplied option must be a boolean").ThrowAsJavaScriptException();
                        return;
                    }
                }

                if (options.Has("painted"))
                {
                    Napi::Value painted_val = options.Get("painted");
                    if (!painted_val.IsEmpty() && painted_val.IsBoolean())
                    {
                        painted = painted_val.As<Napi::Boolean>();
                    }
                    else
                    {
                        Napi::TypeError::New(env, "painted option must be a boolean").ThrowAsJavaScriptException();
                        return;
                    }
                }
            }
            else
            {
                Napi::TypeError::New(env, "Options parameter must be an object").ThrowAsJavaScriptException();
                return;
            }
        }

        try {
            int width = info[0].As<Napi::Number>().Int32Value();
            int height = info[1].As<Napi::Number>().Int32Value();
            image_ = std::make_shared<mapnik::image_any>(width, height, type, initialize, premultiplied, painted);
        }
        catch (std::exception const& ex)
        {
            Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        }
    }
    else
    {
        Napi::Error::New(env, "please provide at least Image width and height").ThrowAsJavaScriptException();
    }
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

Napi::Value Image::getType(Napi::CallbackInfo const& info)
{
    auto type = image_->get_dtype();
    return Napi::Number::New(info.Env(), type);
}

struct visitor_get_pixel
{
    visitor_get_pixel(Napi::Env env, int x, int y)
        : env_(env), x_(x), y_(y) {}

    Napi::Value operator() (mapnik::image_null const&)
    {
        // This should never be reached because the width and height of 0 for a null
        // image will prevent the visitor from being called.

        Napi::EscapableHandleScope scope(env_);
        return scope.Escape(env_.Undefined());
    }

    Napi::Value operator() (mapnik::image_gray8 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray8s const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray16 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray16s const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray32 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray32s const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray32f const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray64 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint64_t val = mapnik::get_pixel<std::uint64_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray64s const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::int64_t val = mapnik::get_pixel<std::int64_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray64f const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_rgba8 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

  private:
    Napi::Env env_;
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

Napi::Value Image::getPixel(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    int x = 0;
    int y = 0;
    bool get_color = false;
    if (info.Length() >= 3)
    {
        if (!info[2].IsObject())
        {
            Napi::TypeError::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[2].As<Napi::Object>();

        if (options.Has("get_color"))
        {
            Napi::Value bind_opt = options.Get("get_color");
            if (!bind_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "optional arg 'color' must be a boolean").ThrowAsJavaScriptException();
                return env.Null();
            }
            get_color = bind_opt.As<Napi::Boolean>();
        }
    }

    if (info.Length() >= 2)
    {
        if (!info[0].IsNumber())
        {
            Napi::TypeError::New(env, "first arg, 'x' must be an integer").ThrowAsJavaScriptException();
            return env.Null();
        }
        if (!info[1].IsNumber())
        {
            Napi::TypeError::New(env, "second arg, 'y' must be an integer").ThrowAsJavaScriptException();
            return env.Null();
        }
        x = info[0].As<Napi::Number>().Int32Value();
        y = info[1].As<Napi::Number>().Int32Value();
    }
    else
    {
        Napi::Error::New(env, "must supply x,y to query pixel color").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (x >= 0 && x < static_cast<int>(image_->width())
        && y >= 0 && y < static_cast<int>(image_->height()))
    {
        if (get_color)
        {
            Napi::EscapableHandleScope scope(env);
            using color_ptr = std::shared_ptr<mapnik::color>;
            color_ptr val = std::make_shared<mapnik::color>(mapnik::get_pixel<mapnik::color>(*image_, x, y));
            Napi::Value arg = Napi::External<color_ptr>::New(env, &val);
            Napi::Object obj = constructor.New({arg});
            return scope.Escape(napi_value(obj)).ToObject();
        }
        else
        {
            visitor_get_pixel visitor(env, x, y);
            return mapnik::util::apply_visitor(visitor, *image_);
        }
    }
    return env.Null();
}

/**
 * Set a pixels value
 * @name setPixel
 * @instance
 * @memberof Image
 * @param {number} x position within334 image from top left
 * @param {number} y position within image from top left
 * @param {Object|number} numeric or object representation of a color, typically used with {@link mapnik.Color}
 * @example
 * var gray = new mapnik.Image(256, 256);
 * gray.setPixel(0,0,new mapnik.Color('white'));
 * var pixel = gray.getPixel(0,0,{get_color:true});
 * console.log(pixel); // { premultiplied: false, a: 255, b: 255, g: 255, r: 255 }
 */

void Image::setPixel(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3 || (!info[0].IsNumber() && !info[1].IsNumber())) {
        Napi::TypeError::New(env, "expects three arguments: x, y, and pixel value").ThrowAsJavaScriptException();
        return;
    }
    int x = info[0].As<Napi::Number>().Int32Value();
    int y = info[1].As<Napi::Number>().Int32Value();

    if (x < 0 || x >= static_cast<int>(image_->width()) || y < 0 || y >= static_cast<int>(image_->height()))
    {
        Napi::TypeError::New(env, "invalid pixel requested").ThrowAsJavaScriptException();
        return;
    }
    if (info[2].IsNumber())
    {
        std::uint32_t val = info[2].As<Napi::Number>().Int32Value();
        mapnik::set_pixel<std::uint32_t>(*image_, x, y, val);
    }
    else if (info[2].IsObject())
    {
        Napi::Object obj = info[2].As<Napi::Object>();
        if (obj.IsNull() || obj.IsUndefined() || !obj.InstanceOf(Color::constructor.Value()))
        {
            Napi::TypeError::New(env, "A numeric or color value is expected as third arg").ThrowAsJavaScriptException();
        }
        else
        {
            Color * color = Napi::ObjectWrap<Color>::Unwrap(obj);
            mapnik::set_pixel(*image_, x, y, color->color_);
        }
    }
    else
    {
        Napi::TypeError::New(env, "A numeric or color value is expected as third arg").ThrowAsJavaScriptException();
    }
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

Napi::Value Image::compare(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "first argument should be a mapnik.Image").ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::Object obj = info[0].As<Napi::Object>();
    if (obj.IsNull() || obj.IsUndefined() || !obj.InstanceOf(Image::constructor.Value()))
    {
        Napi::TypeError::New(env, "mapnik.Image expected as first arg").ThrowAsJavaScriptException();
        return env.Null();
    }

    int threshold = 0;
    unsigned alpha = true;

    if (info.Length() > 1)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        if (options.Has("threshold"))
        {
            Napi::Value bind_opt = options.Get("threshold");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'threshold' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }
            threshold = bind_opt.As<Napi::Number>().Int32Value();
        }

        if (options.Has("alpha"))
        {
            Napi::Value bind_opt = options.Get("alpha");
            if (!bind_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "optional arg 'alpha' must be a boolean").ThrowAsJavaScriptException();
                return env.Null();
            }
            alpha = bind_opt.As<Napi::Boolean>();
        }
    }
    image_ptr image2 = Napi::ObjectWrap<Image>::Unwrap(obj)->image_;

    if (image_->width() != image2->width() ||
        image_->height() != image2->height())
    {
            Napi::TypeError::New(env, "image dimensions do not match").ThrowAsJavaScriptException();
            return env.Null();
    }
    unsigned difference = mapnik::compare(*image_, *image2, threshold, alpha);
    return Napi::Number::New(env, difference);
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
   /*
Napi::Value Image::filterSync(const Napi::CallbackInfo& info)
{
    return _filterSync(info);
}

Napi::Value Image::_filterSync(const Napi::CallbackInfo& info) {
    Napi::EscapableHandleScope scope(env);
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "expects one argument: string filter argument").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }
    Image* im = info.Holder().Unwrap<Image>();
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "A string is expected for filter argument").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }
    std::string filter = TOSTR(info[0]);
    try
    {
        mapnik::filter::filter_image(*im->this_,filter);
    }
    catch(std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

    }
    return scope.Escape(env.Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string filter;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
} filter_image_baton_t;
   */
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
  /*
Napi::Value Image::filter(const Napi::CallbackInfo& info)
{
    if (info.Length() <= 1) {
        return _filterSync(info);
        return;
    }

    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    Image* im = info.Holder().Unwrap<Image>();
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "A string is expected for filter argument").ThrowAsJavaScriptException();
        return env.Null();
    }
    filter_image_baton_t *closure = new filter_image_baton_t();
    closure->filter = TOSTR(info[0]);

    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    filter_image_baton_t *closure = static_cast<filter_image_baton_t *>(req->data);
    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Napi::Value argv[2] = { env.Null(), closure->im->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

  */

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
/*
Napi::Value Image::clearSync(const Napi::CallbackInfo& info)
{
    return _clearSync(info);
}

Napi::Value Image::_clearSync(const Napi::CallbackInfo& info) {
    Napi::EscapableHandleScope scope(env);
    Image* im = info.Holder().Unwrap<Image>();
    try
    {
        mapnik::fill(*im->this_, 0);
    }
    catch(std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

    }
    return scope.Escape(env.Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    //std::string format;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
} clear_image_baton_t;
*/
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
  /*
Napi::Value Image::clear(const Napi::CallbackInfo& info)
{
    Image* im = info.Holder().Unwrap<Image>();

    if (info.Length() == 0) {
        return _clearSync(info);
        return;
    }
    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }
    clear_image_baton_t *closure = new clear_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    clear_image_baton_t *closure = static_cast<clear_image_baton_t *>(req->data);
    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Napi::Value argv[2] = { env.Null(), closure->im->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}
  */
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
/*
Napi::Value Image::setGrayScaleToAlpha(const Napi::CallbackInfo& info)
{
    Image* im = info.Holder().Unwrap<Image>();
    if (info.Length() == 0) {
        mapnik::set_grayscale_to_alpha(*im->this_);
    } else {
        if (!info[0].IsObject()) {
            Napi::TypeError::New(env, "optional first arg must be a mapnik.Color").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object obj = info[0].ToObject(Napi::GetCurrentContext());

        if (obj->IsNull() || obj->IsUndefined() || !Napi::New(env, Color::constructor)->HasInstance(obj)) {
            Napi::TypeError::New(env, "mapnik.Color expected as first arg").ThrowAsJavaScriptException();
            return env.Null();
        }

        Color * color = obj.Unwrap<Color>();
        mapnik::set_grayscale_to_alpha(*im->this_, *color->get());
    }

    return;
}

typedef struct {
    uv_work_t request;
    Image* im;
    Napi::FunctionReference cb;
} image_op_baton_t;
*/
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
  /*
Napi::Value Image::premultiplied(const Napi::CallbackInfo& info)
{
    Image* im = info.Holder().Unwrap<Image>();
    bool premultiplied = im->this_->get_premultiplied();
    return Napi::Boolean::New(env, premultiplied);
}
  */

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
   /*
Napi::Value Image::premultiplySync(const Napi::CallbackInfo& info)
{
    return _premultiplySync(info);
}

Napi::Value Image::_premultiplySync(const Napi::CallbackInfo& info) {
    Napi::EscapableHandleScope scope(env);
    Image* im = info.Holder().Unwrap<Image>();
    mapnik::premultiply_alpha(*im->this_);
    return scope.Escape(env.Undefined());
}
   */
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
/*
Napi::Value Image::premultiply(const Napi::CallbackInfo& info)
{
    if (info.Length() == 0) {
        return _premultiplySync(info);
        return;
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    Image* im = info.Holder().Unwrap<Image>();
    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    Napi::Value argv[2] = { env.Null(), closure->im->handle() };
    async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}
*/
/**
 * Demultiply the pixels in this image. The opposite of
 * premultiplying.
 *
 * @name demultiplySync
 * @instance
 * @memberof Image
 */
/*
Napi::Value Image::demultiplySync(const Napi::CallbackInfo& info)
{
    Napi::HandleScope scope(env);
    return _demultiplySync(info);
}

Napi::Value Image::_demultiplySync(const Napi::CallbackInfo& info) {
    Napi::EscapableHandleScope scope(env);
    Image* im = info.Holder().Unwrap<Image>();
    mapnik::demultiply_alpha(*im->this_);
    return scope.Escape(env.Undefined());
}
*/
/**
 * Demultiply the pixels in this image, asynchronously. The opposite of
 * premultiplying
 *
 * @name demultiply
 * @param {Function} callback
 * @instance
 * @memberof Image
 */
/*
Napi::Value Image::demultiply(const Napi::CallbackInfo& info)
{
    if (info.Length() == 0) {
        return _demultiplySync(info);
        return;
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    Image* im = info.Holder().Unwrap<Image>();
    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::FunctionReference cb;
    bool error;
    std::string error_name;
    bool result;
} is_solid_image_baton_t;
*/
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
  /*
Napi::Value Image::isSolid(const Napi::CallbackInfo& info)
{
    Image* im = info.Holder().Unwrap<Image>();

    if (info.Length() == 0) {
        return _isSolidSync(info);
        return;
    }
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    is_solid_image_baton_t *closure = new is_solid_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->result = true;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    is_solid_image_baton_t *closure = static_cast<is_solid_image_baton_t *>(req->data);
    if (closure->error) {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            Napi::Value argv[3] = { env.Null(),
                                     Napi::New(env, closure->result),
                                     mapnik::util::apply_visitor(visitor_get_pixel(0,0),*(closure->im->this_)),
            };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 3, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), Napi::New(env, closure->result) };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}
*/
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
/*
Napi::Value Image::isSolidSync(const Napi::CallbackInfo& info)
{
    return _isSolidSync(info);
}

Napi::Value Image::_isSolidSync(const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);
    Image* im = info.Holder().Unwrap<Image>();
    if (im->this_->width() > 0 && im->this_->height() > 0)
    {
        return scope.Escape(Napi::Boolean::New(env, mapnik::is_solid(*(im->this_))));
    }
    Napi::Error::New(env, "image does not have valid dimensions").ThrowAsJavaScriptException();

    return scope.Escape(env.Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im1;
    image_ptr im2;
    mapnik::image_dtype type;
    double offset;
    double scaling;
    Napi::FunctionReference cb;
    bool error;
    std::string error_name;
} copy_image_baton_t;
*/

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
  /*
Napi::Value Image::copy(const Napi::CallbackInfo& info)
{
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        return _copySync(info);
        return;
    }

    Image* im1 = info.Holder().Unwrap<Image>();
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = im1->this_->get_dtype();
    Napi::Object options = Napi::Object::New(env);

    if (info.Length() >= 2)
    {
        if (info[0].IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(info[0].As<Napi::Number>().Int32Value());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Napi::TypeError::New(env, "Image 'type' must be a valid image type").ThrowAsJavaScriptException();
                return env.Null();
            }
        }
        else if (info[0].IsObject())
        {
            options = info[0].ToObject(Napi::GetCurrentContext());
        }
        else
        {
            Napi::TypeError::New(env, "Unknown parameters passed").ThrowAsJavaScriptException();
            return env.Null();
        }
    }
    if (info.Length() >= 3)
    {
        if (info[1].IsObject())
        {
            options = info[1].ToObject(Napi::GetCurrentContext());
        }
        else
        {
            Napi::TypeError::New(env, "Expected options object as second argument").ThrowAsJavaScriptException();
            return env.Null();
        }
    }

    if ((options).Has(Napi::String::New(env, "scaling")).FromMaybe(false))
    {
        Napi::Value scaling_val = (options).Get(Napi::String::New(env, "scaling"));
        if (scaling_val.IsNumber())
        {
            scaling = scaling_val.As<Napi::Number>().DoubleValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Napi::TypeError::New(env, "scaling argument must be a number").ThrowAsJavaScriptException();
            return env.Null();
        }
    }

    if ((options).Has(Napi::String::New(env, "offset")).FromMaybe(false))
    {
        Napi::Value offset_val = (options).Get(Napi::String::New(env, "offset"));
        if (offset_val.IsNumber())
        {
            offset = offset_val.As<Napi::Number>().DoubleValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Napi::TypeError::New(env, "offset argument must be a number").ThrowAsJavaScriptException();
            return env.Null();
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
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    copy_image_baton_t *closure = static_cast<copy_image_baton_t *>(req->data);
    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else if (!closure->im2)
    {
        // Not quite sure if this is even required or ever can be reached, but leaving it
        // and simply removing it from coverage tests.

        Napi::Value argv[1] = { Napi::Error::New(env, "could not render to image") };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im2);
        Napi::Value ext = Napi::External::New(env, im);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            Napi::Value argv[1] = { Napi::Error::New(env, "Could not create new Image instance") };
           async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), maybe_local };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }
    closure->im1->Unref();
    closure->cb.Reset();
    delete closure;
}
*/
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
/*
Napi::Value Image::copySync(const Napi::CallbackInfo& info)
{
    return _copySync(info);
}

Napi::Value Image::_copySync(const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);
    Image* im = info.Holder().Unwrap<Image>();
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = im->this_->get_dtype();
    Napi::Object options = Napi::Object::New(env);
    if (info.Length() >= 1)
    {
        if (info[0].IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(info[0].As<Napi::Number>().Int32Value());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Napi::TypeError::New(env, "Image 'type' must be a valid image type").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
        }
        else if (info[0].IsObject())
        {
            options = info[0].ToObject(Napi::GetCurrentContext());
        }
        else
        {
            Napi::TypeError::New(env, "Unknown parameters passed").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }
    if (info.Length() >= 2)
    {
        if (info[1].IsObject())
        {
            options = info[1].ToObject(Napi::GetCurrentContext());
        }
        else
        {
            Napi::TypeError::New(env, "Expected options object as second argument").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }

    if ((options).Has(Napi::String::New(env, "scaling")).FromMaybe(false))
    {
        Napi::Value scaling_val = (options).Get(Napi::String::New(env, "scaling"));
        if (scaling_val.IsNumber())
        {
            scaling = scaling_val.As<Napi::Number>().DoubleValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Napi::TypeError::New(env, "scaling argument must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }

    if ((options).Has(Napi::String::New(env, "offset")).FromMaybe(false))
    {
        Napi::Value offset_val = (options).Get(Napi::String::New(env, "offset"));
        if (offset_val.IsNumber())
        {
            offset = offset_val.As<Napi::Number>().DoubleValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Napi::TypeError::New(env, "offset argument must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
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
        Napi::Value ext = Napi::External::New(env, new_im);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Image instance").ThrowAsJavaScriptException();

        return scope.Escape(maybe_local);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
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
    Napi::FunctionReference cb;
    bool error;
    std::string error_name;
} //resize_image_baton_t;
*/
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

/*
Napi::Value Image::resize(const Napi::CallbackInfo& info)
{
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        return _resizeSync(info);
        return;
    }
    Image* im1 = info.Holder().Unwrap<Image>();
    std::size_t width = 0;
    std::size_t height = 0;
    double offset_x = 0.0;
    double offset_y = 0.0;
    double offset_width = static_cast<double>(im1->this_->width());
    double offset_height = static_cast<double>(im1->this_->height());
    double filter_factor = 1.0;
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    Napi::Object options = Napi::Object::New(env);

    if (info.Length() >= 3)
    {
        if (info[0].IsNumber())
        {
            auto width_tmp = info[0].As<Napi::Number>().Int32Value();
            if (width_tmp <= 0)
            {
                Napi::TypeError::New(env, "Width must be a integer greater then zero").ThrowAsJavaScriptException();
                return env.Null();
            }
            width = static_cast<std::size_t>(width_tmp);
        }
        else
        {
            Napi::TypeError::New(env, "Width must be a number").ThrowAsJavaScriptException();
            return env.Null();
        }
        if (info[1].IsNumber())
        {
            auto height_tmp = info[1].As<Napi::Number>().Int32Value();
            if (height_tmp <= 0)
            {
                Napi::TypeError::New(env, "Height must be a integer greater then zero").ThrowAsJavaScriptException();
                return env.Null();
            }
            height = static_cast<std::size_t>(height_tmp);
        }
        else
        {
            Napi::TypeError::New(env, "Height must be a number").ThrowAsJavaScriptException();
            return env.Null();
        }
    }
    else
    {
        Napi::TypeError::New(env, "resize requires a width and height parameter.").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (info.Length() >= 4)
    {
        if (info[2].IsObject())
        {
            options = info[2].ToObject(Napi::GetCurrentContext());
        }
        else
        {
            Napi::TypeError::New(env, "Expected options object as third argument").ThrowAsJavaScriptException();
            return env.Null();
        }
    }
    if ((options).Has(Napi::String::New(env, "offset_x")).FromMaybe(false))
    {
        Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_x"));
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_x' must be a number").ThrowAsJavaScriptException();
            return env.Null();
        }
        offset_x = bind_opt.As<Napi::Number>().DoubleValue();
    }
    if ((options).Has(Napi::String::New(env, "offset_y")).FromMaybe(false))
    {
        Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_y"));
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_y' must be a number").ThrowAsJavaScriptException();
            return env.Null();
        }
        offset_y = bind_opt.As<Napi::Number>().DoubleValue();
    }
    if ((options).Has(Napi::String::New(env, "offset_width")).FromMaybe(false))
    {
        Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_width"));
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_width' must be a number").ThrowAsJavaScriptException();
            return env.Null();
        }
        offset_width = bind_opt.As<Napi::Number>().DoubleValue();
        if (offset_width <= 0.0)
        {
            Napi::TypeError::New(env, "optional arg 'offset_width' must be a integer greater then zero").ThrowAsJavaScriptException();
            return env.Null();
        }
    }
    if ((options).Has(Napi::String::New(env, "offset_height")).FromMaybe(false))
    {
        Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_height"));
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_height' must be a number").ThrowAsJavaScriptException();
            return env.Null();
        }
        offset_height = bind_opt.As<Napi::Number>().DoubleValue();
        if (offset_height <= 0.0)
        {
            Napi::TypeError::New(env, "optional arg 'offset_height' must be a integer greater then zero").ThrowAsJavaScriptException();
            return env.Null();
        }
    }
    if ((options).Has(Napi::String::New(env, "scaling_method")).FromMaybe(false))
    {
        Napi::Value scaling_val = (options).Get(Napi::String::New(env, "scaling_method"));
        if (scaling_val.IsNumber())
        {
            std::int64_t scaling_int = scaling_val.As<Napi::Number>().Int32Value();
            if (scaling_int > mapnik::SCALING_BLACKMAN || scaling_int < 0)
            {
                Napi::TypeError::New(env, "Invalid scaling_method").ThrowAsJavaScriptException();
                return env.Null();
            }
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_int);
        }
        else
        {
            Napi::TypeError::New(env, "scaling_method argument must be an integer").ThrowAsJavaScriptException();
            return env.Null();
        }
    }

    if ((options).Has(Napi::String::New(env, "filter_factor")).FromMaybe(false))
    {
        Napi::Value ff_val = (options).Get(Napi::String::New(env, "filter_factor"));
        if (ff_val.IsNumber())
        {
            filter_factor = ff_val.As<Napi::Number>().DoubleValue();
        }
        else
        {
            Napi::TypeError::New(env, "filter_factor argument must be a number").ThrowAsJavaScriptException();
            return env.Null();
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
    closure->cb.Reset(callback.As<Napi::Function>());
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
        throw std::runtime_error("Can not resize null images");
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    resize_image_baton_t *closure = static_cast<resize_image_baton_t *>(req->data);
    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im2);
        Napi::Value ext = Napi::External::New(env, im);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            Napi::Value argv[1] = { Napi::Error::New(env, "Could not create new Image instance") };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), maybe_local };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }
    closure->im1->Unref();
    closure->cb.Reset();
    delete closure;
}
*/
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
/*
Napi::Value Image::resizeSync(const Napi::CallbackInfo& info)
{
    return _resizeSync(info);
}

Napi::Value Image::_resizeSync(const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);
    Image* im = info.Holder().Unwrap<Image>();
    std::size_t width = 0;
    std::size_t height = 0;
    double filter_factor = 1.0;
    double offset_x = 0.0;
    double offset_y = 0.0;
    double offset_width = static_cast<double>(im->this_->width());
    double offset_height = static_cast<double>(im->this_->height());
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    Napi::Object options = Napi::Object::New(env);
    if (info.Length() >= 2)
    {
        if (info[0].IsNumber())
        {
            int width_tmp = info[0].As<Napi::Number>().Int32Value();
            if (width_tmp <= 0)
            {
                Napi::TypeError::New(env, "Width parameter must be an integer greater then zero").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
            width = static_cast<std::size_t>(width_tmp);
        }
        else
        {
            Napi::TypeError::New(env, "Width must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
        if (info[1].IsNumber())
        {
            int height_tmp = info[1].As<Napi::Number>().Int32Value();
            if (height_tmp <= 0)
            {
                Napi::TypeError::New(env, "Height parameter must be an integer greater then zero").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
            height = static_cast<std::size_t>(height_tmp);
        }
        else
        {
            Napi::TypeError::New(env, "Height must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }
    else
    {
        Napi::TypeError::New(env, "Resize requires at least a width and height parameter").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }
    if (info.Length() >= 3)
    {
        if (info[2].IsObject())
        {
            options = info[2].ToObject(Napi::GetCurrentContext());
        }
        else
        {
            Napi::TypeError::New(env, "Expected options object as third argument").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }
    if ((options).Has(Napi::String::New(env, "offset_x")).FromMaybe(false))
    {
        Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_x"));
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_x' must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
        offset_x = bind_opt.As<Napi::Number>().DoubleValue();
    }
    if ((options).Has(Napi::String::New(env, "offset_y")).FromMaybe(false))
    {
        Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_y"));
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_y' must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
        offset_y = bind_opt.As<Napi::Number>().DoubleValue();
    }
    if ((options).Has(Napi::String::New(env, "offset_width")).FromMaybe(false))
    {
        Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_width"));
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_width' must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
        offset_width = bind_opt.As<Napi::Number>().DoubleValue();
        if (offset_width <= 0.0)
        {
            Napi::TypeError::New(env, "optional arg 'offset_width' must be a integer greater then zero").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }
    if ((options).Has(Napi::String::New(env, "offset_height")).FromMaybe(false))
    {
        Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_height"));
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_height' must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
        offset_height = bind_opt.As<Napi::Number>().DoubleValue();
        if (offset_height <= 0.0)
        {
            Napi::TypeError::New(env, "optional arg 'offset_height' must be a integer greater then zero").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }

    if ((options).Has(Napi::String::New(env, "scaling_method")).FromMaybe(false))
    {
        Napi::Value scaling_val = (options).Get(Napi::String::New(env, "scaling_method"));
        if (scaling_val.IsNumber())
        {
            std::int64_t scaling_int = scaling_val.As<Napi::Number>().Int32Value();
            if (scaling_int > mapnik::SCALING_BLACKMAN || scaling_int < 0)
            {
                Napi::TypeError::New(env, "Invalid scaling_method").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_int);
        }
        else
        {
            Napi::TypeError::New(env, "scaling_method argument must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }

    if ((options).Has(Napi::String::New(env, "filter_factor")).FromMaybe(false))
    {
        Napi::Value ff_val = (options).Get(Napi::String::New(env, "filter_factor"));
        if (ff_val.IsNumber())
        {
            filter_factor = ff_val.As<Napi::Number>().DoubleValue();
        }
        else
        {
            Napi::TypeError::New(env, "filter_factor argument must be a number").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }

    if (im->this_->is<mapnik::image_null>())
    {
        Napi::TypeError::New(env, "Can not resize a null image").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }
    if (offset_width <= 0 || offset_height <= 0)
    {
        Napi::TypeError::New(env, "Image width or height is zero or less then zero.").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
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
        Napi::Value ext = Napi::External::New(env, new_im);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Image instance").ThrowAsJavaScriptException();

        return scope.Escape(maybe_local);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }
}
*/
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
/*
Napi::Value Image::painted(const Napi::CallbackInfo& info)
{
    Image* im = info.Holder().Unwrap<Image>();
    return Napi::Boolean::New(env, im->this_->painted());
}
*/

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

Napi::Value Image::width(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), static_cast<std::int32_t>(image_->width()));
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

Napi::Value Image::height(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), static_cast<std::int32_t>(image_->height()));
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
/*
Napi::Value Image::fromSVGBytesSync(const Napi::CallbackInfo& info)
{
    return _fromSVGSync(false, info);
}
*/
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
 /*
Napi::Value Image::fromSVGSync(const Napi::CallbackInfo& info)
{
    return _fromSVGSync(true, info);
}

Napi::Value Image::_fromSVGSync(bool fromFile, const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);

    if (!fromFile && (info.Length() < 1 || !info[0].IsObject()))
    {
        Napi::TypeError::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }

    if (fromFile && (info.Length() < 1 || !info[0].IsString()))
    {
        Napi::TypeError::New(env, "must provide a filename argument").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }


    double scale = 1.0;
    std::uint32_t max_size = 2048;
    bool strict = false;
    if (info.Length() >= 2)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
        Napi::Object options = info[1].ToObject(Napi::GetCurrentContext());
        if ((options).Has(Napi::String::New(env, "scale")).FromMaybe(false))
        {
            Napi::Value scale_opt = (options).Get(Napi::String::New(env, "scale"));
            if (!scale_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'scale' must be a number").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
            scale = scale_opt.As<Napi::Number>().DoubleValue();
            if (scale <= 0)
            {
                Napi::TypeError::New(env, "'scale' must be a positive non zero number").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
        }
        if ((options).Has(Napi::String::New(env, "max_size")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "max_size"));
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "'max_size' must be a positive integer").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
            auto max_size_val = opt.As<Napi::Number>().Int32Value();
            if (max_size_val < 0 || max_size_val > 65535) {
                Napi::TypeError::New(env, "'max_size' must be a positive integer between 0 and 65535").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
            max_size = static_cast<std::uint32_t>(max_size_val);
        }
        if ((options).Has(Napi::String::New(env, "strict")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "strict"));
            if (!opt->IsBoolean())
            {
                Napi::TypeError::New(env, "'strict' must be a boolean value").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
            strict = opt.As<Napi::Boolean>().Value();
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
                Napi::TypeError::New(env, errorMessage.str().c_str()).ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
        }
        else
        {
            Napi::Object obj = info[0].ToObject(Napi::GetCurrentContext());
            if (obj->IsNull() || obj->IsUndefined() || !obj.IsBuffer())
            {
                Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
            std::string svg_buffer(obj.As<Napi::Buffer<char>>().Data(),obj.As<Napi::Buffer<char>>().Length());
            p.parse_from_string(svg_buffer);
            if (strict && !p.err_handler().error_messages().empty())
            {
                std::ostringstream errorMessage;
                for (auto const& error : p.err_handler().error_messages()) {
                    errorMessage <<  error << std::endl;
                }
                Napi::TypeError::New(env, errorMessage.str().c_str()).ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
        }

        double lox,loy,hix,hiy;
        svg.bounding_rect(&lox, &loy, &hix, &hiy);
        marker_path->set_bounding_box(lox,loy,hix,hiy);
        marker_path->set_dimensions(svg.width(), svg.height());

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
            Napi::TypeError::New(env, "image created from svg must have a width and height greater then zero").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }

        if (svg_width > static_cast<double>(max_size) || svg_height > static_cast<double>(max_size))
        {
            std::stringstream s;
            s << "image created from svg must be " << max_size << " pixels or fewer on each side";
            Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }

        mapnik::image_rgba8 im(static_cast<int>(std::round(svg_width)),
                               static_cast<int>(std::round(svg_height)),
                               true, true);
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
        Napi::Value ext = Napi::External::New(env, im2);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Image instance").ThrowAsJavaScriptException();

        return scope.Escape(maybe_local);
    }
    catch (std::exception const& ex)
    {
        // There is currently no known way to make these operations throw an exception, however,
        // since the underlying agg library does possibly have some operation that might throw
        // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
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
    Napi::FunctionReference cb;
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
    Napi::Persistent<v8::Object> buffer;
    Napi::FunctionReference cb;
} svg_mem_ptr_baton_t;
*/

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
  /*
Napi::Value Image::fromSVG(const Napi::CallbackInfo& info)
{
    if (info.Length() == 1) {
        return _fromSVGSync(true, info);
        return;
    }

    if (info.Length() < 2 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "must provide a filename argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    double scale = 1.0;
    std::uint32_t max_size = 2048;
    bool strict = false;
    if (info.Length() >= 3)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }
        Napi::Object options = info[1].ToObject(Napi::GetCurrentContext());
        if ((options).Has(Napi::String::New(env, "scale")).FromMaybe(false))
        {
            Napi::Value scale_opt = (options).Get(Napi::String::New(env, "scale"));
            if (!scale_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'scale' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }
            scale = scale_opt.As<Napi::Number>().DoubleValue();
            if (scale <= 0)
            {
                Napi::TypeError::New(env, "'scale' must be a positive non zero number").ThrowAsJavaScriptException();
                return env.Null();
            }
        }
        if ((options).Has(Napi::String::New(env, "max_size")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "max_size"));
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "'max_size' must be a positive integer").ThrowAsJavaScriptException();
                return env.Null();
            }
            auto max_size_val = opt.As<Napi::Number>().Int32Value();
            if (max_size_val < 0 || max_size_val > 65535) {
                Napi::TypeError::New(env, "'max_size' must be a positive integer between 0 and 65535").ThrowAsJavaScriptException();
                return env.Null();
            }
            max_size = static_cast<std::uint32_t>(max_size_val);
        }
        if ((options).Has(Napi::String::New(env, "strict")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "strict"));
            if (!opt->IsBoolean())
            {
                Napi::TypeError::New(env, "'strict' must be a boolean value").ThrowAsJavaScriptException();
                return env.Null();
            }
            strict = opt.As<Napi::Boolean>().Value();
        }
    }

    svg_file_ptr_baton_t *closure = new svg_file_ptr_baton_t();
    closure->request.data = closure;
    closure->filename = TOSTR(info[0]);
    closure->error = false;
    closure->scale = scale;
    closure->max_size = max_size;
    closure->strict = strict;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    svg_file_ptr_baton_t *closure = static_cast<svg_file_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        Napi::Value ext = Napi::External::New(env, im);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            Napi::Value argv[1] = { Napi::Error::New(env, "Could not create new Image instance") };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), maybe_local };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }
    closure->cb.Reset();
    delete closure;
}
*/
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
/*
Napi::Value Image::fromSVGBytes(const Napi::CallbackInfo& info)
{
    if (info.Length() == 1) {
        return _fromSVGSync(false, info);
        return;
    }

    if (info.Length() < 2 || !info[0].IsObject()) {
        Napi::Error::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object obj = info[0].ToObject(Napi::GetCurrentContext());
    if (obj->IsNull() || obj->IsUndefined() || !obj.IsBuffer()) {
        Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    double scale = 1.0;
    std::uint32_t max_size = 2048;
    bool strict = false;
    if (info.Length() >= 3)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }
        Napi::Object options = info[1].ToObject(Napi::GetCurrentContext());
        if ((options).Has(Napi::String::New(env, "scale")).FromMaybe(false))
        {
            Napi::Value scale_opt = (options).Get(Napi::String::New(env, "scale"));
            if (!scale_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'scale' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }
            scale = scale_opt.As<Napi::Number>().DoubleValue();
            if (scale <= 0)
            {
                Napi::TypeError::New(env, "'scale' must be a positive non zero number").ThrowAsJavaScriptException();
                return env.Null();
            }
        }
        if ((options).Has(Napi::String::New(env, "max_size")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "max_size"));
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "'max_size' must be a positive integer").ThrowAsJavaScriptException();
                return env.Null();
            }
            auto max_size_val = opt.As<Napi::Number>().Int32Value();
            if (max_size_val < 0 || max_size_val > 65535) {
                Napi::TypeError::New(env, "'max_size' must be a positive integer between 0 and 65535").ThrowAsJavaScriptException();
                return env.Null();
            }
            max_size = static_cast<std::uint32_t>(max_size_val);
        }
        if ((options).Has(Napi::String::New(env, "strict")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "strict"));
            if (!opt->IsBoolean())
            {
                Napi::TypeError::New(env, "'strict' must be a boolean value").ThrowAsJavaScriptException();
                return env.Null();
            }
            strict = opt.As<Napi::Boolean>().Value();
        }
    }

    svg_mem_ptr_baton_t *closure = new svg_mem_ptr_baton_t();
    closure->request.data = closure;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
    closure->buffer.Reset(obj.As<Napi::Object>());
    closure->data = obj.As<Napi::Buffer<char>>().Data();
    closure->scale = scale;
    closure->max_size = max_size;
    closure->strict = strict;
    closure->dataLength = obj.As<Napi::Buffer<char>>().Length();
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    svg_mem_ptr_baton_t *closure = static_cast<svg_mem_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        Napi::Value ext = Napi::External::New(env, im);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            Napi::Value argv[1] = { Napi::Error::New(env, "Could not create new Image instance") };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), maybe_local };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}
*/
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
/*
Napi::Value Image::fromBufferSync(const Napi::CallbackInfo& info)
{
    return _fromBufferSync(info);
}

Napi::Value Image::_fromBufferSync(const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);

    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsObject()) {
        Napi::TypeError::New(env, "must provide a width, height, and buffer argument").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }

    unsigned width = info[0].As<Napi::Number>().Int32Value();
    unsigned height = info[1].As<Napi::Number>().Int32Value();

    if (width <= 0 || height <= 0)
    {
        Napi::TypeError::New(env, "width and height must be greater then zero").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }

    Napi::Object obj = info[2].ToObject(Napi::GetCurrentContext());
    if (obj->IsNull() || obj->IsUndefined() || !obj.IsBuffer()) {
        Napi::TypeError::New(env, "third argument is invalid, must be a Buffer").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }

    // TODO - support other image types?
    auto im_size = mapnik::image_rgba8::pixel_size * width * height;
    if (im_size != obj.As<Napi::Buffer<char>>().Length()) {
        Napi::TypeError::New(env, "invalid image size").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }

    bool premultiplied = false;
    bool painted = false;

    if (info.Length() >= 4)
    {
        if (info[3].IsObject())
        {
            Napi::Object options = info[3].As<Napi::Object>();
            if ((options).Has(Napi::String::New(env, "type")).FromMaybe(false))
            {
                Napi::TypeError::New(env, "'type' option not supported (only rgba images currently viable)").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }

            if ((options).Has(Napi::String::New(env, "premultiplied")).FromMaybe(false))
            {
                Napi::Value pre_val = (options).Get(Napi::String::New(env, "premultiplied"));
                if (!pre_val.IsEmpty() && pre_val->IsBoolean())
                {
                    premultiplied = pre_val.As<Napi::Boolean>().Value();
                }
                else
                {
                    Napi::TypeError::New(env, "premultiplied option must be a boolean").ThrowAsJavaScriptException();

                    return scope.Escape(env.Undefined());
                }
            }

            if ((options).Has(Napi::String::New(env, "painted")).FromMaybe(false))
            {
                Napi::Value painted_val = (options).Get(Napi::String::New(env, "painted"));
                if (!painted_val.IsEmpty() && painted_val->IsBoolean())
                {
                    painted = painted_val.As<Napi::Boolean>().Value();
                }
                else
                {
                    Napi::TypeError::New(env, "painted option must be a boolean").ThrowAsJavaScriptException();

                    return scope.Escape(env.Undefined());
                }
            }
        }
        else
        {
            Napi::TypeError::New(env, "Options parameter must be an object").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }

    try
    {
        mapnik::image_rgba8 im_wrapper(width, height, reinterpret_cast<unsigned char*>(obj.As<Napi::Buffer<char>>().Data()), premultiplied, painted);
        image_ptr imagep = std::make_shared<mapnik::image_any>(im_wrapper);
        Image* im = new Image(imagep);
        Napi::Value ext = Napi::External::New(env, im);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Image instance").ThrowAsJavaScriptException();

        Napi::Object image_obj = maybe_local->ToObject(Napi::GetCurrentContext());
        (image_obj).Set(Napi::String::New(env, "_buffer"),obj);
        return scope.Escape(maybe_local);
    }
    catch (std::exception const& ex)
    {
        // There is no known way for this exception to be reached currently.
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
        // LCOV_EXCL_STOP
    }
}
*/
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
/*
Napi::Value Image::fromBytesSync(const Napi::CallbackInfo& info)
{
    return _fromBytesSync(info);
}

Napi::Value Image::_fromBytesSync(const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }

    Napi::Object obj = info[0].ToObject(Napi::GetCurrentContext());
    if (obj->IsNull() || obj->IsUndefined() || !obj.IsBuffer()) {
        Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }

    try
    {
        std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(obj.As<Napi::Buffer<char>>().Data(),obj.As<Napi::Buffer<char>>().Length()));
        if (reader.get())
        {
            image_ptr imagep = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
            Image* im = new Image(imagep);
            Napi::Value ext = Napi::External::New(env, im);
            Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
            if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Image instance").ThrowAsJavaScriptException();

            return scope.Escape(maybe_local);
        }
        // The only way this is ever reached is if the reader factory in
        // mapnik was not providing an image type it should. This should never
        // be occuring so marking this out from coverage

        Napi::TypeError::New(env, "Failed to load from buffer").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }
}
*/
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
/*
Napi::Value Image::fromBytes(const Napi::CallbackInfo& info)
{
    if (info.Length() == 1) {
        return _fromBytesSync(info);
        return;
    }

    if (info.Length() < 2) {
        Napi::Error::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsObject()) {
        Napi::TypeError::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object obj = info[0].ToObject(Napi::GetCurrentContext());
    if (obj->IsNull() || obj->IsUndefined() || !obj.IsBuffer()) {
        Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool premultiply = false;
    std::uint32_t max_size = 2048;
    if (info.Length() >= 2)
    {
        if (info[1].IsObject())
        {
            Napi::Object options = info[1].As<Napi::Object>();
            if ((options).Has(Napi::String::New(env, "premultiply")).FromMaybe(false))
            {
                Napi::Value opt = (options).Get(Napi::String::New(env, "premultiply"));
                if (!opt.IsEmpty() && opt->IsBoolean())
                {
                    premultiply = opt.As<Napi::Boolean>().Value();
                }
                else
                {
                    Napi::TypeError::New(env, "premultiply option must be a boolean").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }
            if ((options).Has(Napi::String::New(env, "max_size")).FromMaybe(false))
            {
                Napi::Value opt = (options).Get(Napi::String::New(env, "max_size"));
                if (opt.IsNumber())
                {
                    auto max_size_val = opt.As<Napi::Number>().Int32Value();
                    if (max_size_val < 0 || max_size_val > 65535) {
                        Napi::TypeError::New(env, "max_size must be a positive integer between 0 and 65535").ThrowAsJavaScriptException();
                        return env.Null();
                    }
                    max_size = static_cast<std::uint32_t>(max_size_val);
                }
                else
                {
                    Napi::TypeError::New(env, "max_size option must be a number").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }
        }
    }

    image_mem_ptr_baton_t *closure = new image_mem_ptr_baton_t();
    closure->request.data = closure;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
    closure->buffer.Reset(obj.As<Napi::Object>());
    closure->data = obj.As<Napi::Buffer<char>>().Data();
    closure->dataLength = obj.As<Napi::Buffer<char>>().Length();
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    image_mem_ptr_baton_t *closure = static_cast<image_mem_ptr_baton_t *>(req->data);
    if (!closure->error_name.empty())
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else if (closure->im == nullptr)
    {
        // The only way this is ever reached is if the reader factory in
        // mapnik was not providing an image type it should. This should never
        // be occuring so marking this out from coverage
        Napi::Value argv[1] = { Napi::Error::New(env, "Failed to load from buffer") };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else
    {
        Image* im = new Image(closure->im);
        Napi::Value ext = Napi::External::New(env, im);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            Napi::Value argv[1] = { Napi::Error::New(env, "Could not create new Image instance") };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), maybe_local };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}
*/


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
/*
Napi::Value Image::view(const Napi::CallbackInfo& info)
{
    if ( (info.Length() != 4) || (!info[0].IsNumber() && !info[1].IsNumber() && !info[2].IsNumber() && !info[3].IsNumber() )) {
        Napi::TypeError::New(env, "requires 4 integer arguments: x, y, width, height").ThrowAsJavaScriptException();
        return env.Null();
    }

    // TODO parse args
    unsigned x = info[0].As<Napi::Number>().Int32Value();
    unsigned y = info[1].As<Napi::Number>().Int32Value();
    unsigned w = info[2].As<Napi::Number>().Int32Value();
    unsigned h = info[3].As<Napi::Number>().Int32Value();

    Image* im = info.Holder().Unwrap<Image>();
    return ImageView::NewInstance(im,x,y,w,h);
}
*/
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
 /*
Napi::Value Image::saveSync(const Napi::CallbackInfo& info)
{
    return _saveSync(info);
}

Napi::Value Image::_saveSync(const Napi::CallbackInfo& info) {
    Napi::EscapableHandleScope scope(env);
    Image* im = info.Holder().Unwrap<Image>();

    if (info.Length() == 0 || !info[0].IsString()){
        Napi::TypeError::New(env, "filename required to save file").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }

    std::string filename = TOSTR(info[0]);
    std::string format("");

    if (info.Length() >= 2) {
        if (!info[1].IsString()) {
            Napi::TypeError::New(env, "both 'filename' and 'format' arguments must be strings").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
        format = TOSTR(info[1]);
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
    }

    try
    {
        mapnik::save_to_file(*(im->this_),filename, format);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

    }
    return scope.Escape(env.Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string format;
    std::string filename;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
} save_image_baton_t;
 */
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
  /*
Napi::Value Image::save(const Napi::CallbackInfo& info)
{
    Image* im = info.Holder().Unwrap<Image>();

    if (info.Length() == 0 || !info[0].IsString()){
        Napi::TypeError::New(env, "filename required to save file").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[info.Length()-1]->IsFunction()) {
        return _saveSync(info);
        return;
    }
    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];

    std::string filename = TOSTR(info[0]);
    std::string format("");

    if (info.Length() >= 3) {
        if (!info[1].IsString()) {
            Napi::TypeError::New(env, "both 'filename' and 'format' arguments must be strings").ThrowAsJavaScriptException();
            return env.Null();
        }
        format = TOSTR(info[1]);
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return env.Null();
        }
    }

    save_image_baton_t *closure = new save_image_baton_t();
    closure->request.data = closure;
    closure->format = format;
    closure->filename = filename;
    closure->im = im;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    save_image_baton_t *closure = static_cast<save_image_baton_t *>(req->data);
    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Napi::Value argv[1] = { env.Null() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
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
    Napi::FunctionReference cb;
} composite_image_baton_t;
  */
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
  /*
Napi::Value Image::composite(const Napi::CallbackInfo& info)
{
    if (info.Length() < 1){
        Napi::TypeError::New(env, "requires at least one argument: an image mask").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsObject()) {
        Napi::TypeError::New(env, "first argument must be an image mask").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object im2 = info[0].As<Napi::Object>();
    if (im2->IsNull() || im2->IsUndefined() || !Napi::New(env, Image::constructor)->HasInstance(im2))
    {
        Napi::TypeError::New(env, "mapnik.Image expected as first arg").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    Image * dest_image = info.Holder().Unwrap<Image>();
    Image * source_image = im2.Unwrap<Image>();

    if (!dest_image->this_->get_premultiplied())
    {
        Napi::TypeError::New(env, "destination image must be premultiplied").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!source_image->this_->get_premultiplied())
    {
        Napi::TypeError::New(env, "source image must be premultiplied").ThrowAsJavaScriptException();
        return env.Null();
    }

    mapnik::composite_mode_e mode = mapnik::src_over;
    float opacity = 1.0;
    std::vector<mapnik::filter::filter_type> filters;
    int dx = 0;
    int dy = 0;
    if (info.Length() >= 2) {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        if ((options).Has(Napi::String::New(env, "comp_op")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "comp_op"));
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "comp_op must be a mapnik.compositeOp value").ThrowAsJavaScriptException();
                return env.Null();
            }
            int mode_int = opt.As<Napi::Number>().Int32Value();
            if (mode_int > static_cast<int>(mapnik::composite_mode_e::divide) || mode_int < 0)
            {
                Napi::TypeError::New(env, "Invalid comp_op value").ThrowAsJavaScriptException();
                return env.Null();
            }
            mode = static_cast<mapnik::composite_mode_e>(mode_int);
        }

        if ((options).Has(Napi::String::New(env, "opacity")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "opacity"));
            if (!opt.IsNumber()) {
                Napi::TypeError::New(env, "opacity must be a floating point number").ThrowAsJavaScriptException();
                return env.Null();
            }
            opacity = opt.As<Napi::Number>().DoubleValue();
            if (opacity < 0 || opacity > 1) {
                Napi::TypeError::New(env, "opacity must be a floating point number between 0-1").ThrowAsJavaScriptException();
                return env.Null();
            }
        }

        if ((options).Has(Napi::String::New(env, "dx")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "dx"));
            if (!opt.IsNumber()) {
                Napi::TypeError::New(env, "dx must be an integer").ThrowAsJavaScriptException();
                return env.Null();
            }
            dx = opt.As<Napi::Number>().Int32Value();
        }

        if ((options).Has(Napi::String::New(env, "dy")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "dy"));
            if (!opt.IsNumber()) {
                Napi::TypeError::New(env, "dy must be an integer").ThrowAsJavaScriptException();
                return env.Null();
            }
            dy = opt.As<Napi::Number>().Int32Value();
        }

        if ((options).Has(Napi::String::New(env, "image_filters")).FromMaybe(false))
        {
            Napi::Value opt = (options).Get(Napi::String::New(env, "image_filters"));
            if (!opt.IsString()) {
                Napi::TypeError::New(env, "image_filters argument must string of filter names").ThrowAsJavaScriptException();
                return env.Null();
            }
            std::string filter_str = TOSTR(opt);
            bool result = mapnik::filter::parse_image_filters(filter_str, filters);
            if (!result)
            {
                Napi::TypeError::New(env, "could not parse image_filters").ThrowAsJavaScriptException();
                return env.Null();
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
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);

    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else {
        Napi::Value argv[2] = { env.Null(), closure->im1->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }

    closure->im1->Unref();
    closure->im2->Unref();
    closure->cb.Reset();
    delete closure;
}

Napi::Value Image::get_scaling(const Napi::CallbackInfo& info)
{
    Image* im = info.Holder().Unwrap<Image>();
    return Napi::Number::New(env, im->this_->get_scaling());
}

Napi::Value Image::get_offset(const Napi::CallbackInfo& info)
{
    Image* im = info.Holder().Unwrap<Image>();
    return Napi::Number::New(env, im->this_->get_offset());
}

void Image::set_scaling(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    Image* im = info.Holder().Unwrap<Image>();
    if (!value.IsNumber())
    {
        Napi::Error::New(env, "Must provide a number").ThrowAsJavaScriptException();

    }
    else
    {
        double val = value.As<Napi::Number>().DoubleValue();
        if (val == 0.0)
        {
            Napi::Error::New(env, "Scaling value can not be zero").ThrowAsJavaScriptException();
            return env.Null();
        }
        im->this_->set_scaling(val);
    }
}

void Image::set_offset(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    Image* im = info.Holder().Unwrap<Image>();
    if (!value.IsNumber())
    {
        Napi::Error::New(env, "Must provide a number").ThrowAsJavaScriptException();

    }
    else
    {
        double val = value.As<Napi::Number>().DoubleValue();
        im->this_->set_offset(val);
    }
}
*/
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
/*
Napi::Value Image::data(const Napi::CallbackInfo& info)
{
    Image* im = info.Holder().Unwrap<Image>();
    // TODO - make this zero copy
    return Napi::Buffer::Copy(env, reinterpret_cast<const char *>(im->this_->bytes()), im->this_->size());
}
*/
