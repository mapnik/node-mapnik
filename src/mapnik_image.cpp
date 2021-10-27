// mapnik
#include <mapnik/color.hpp>      // for color
#include <mapnik/image.hpp>      // for image types
#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc
#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_palette.hpp"
#include "mapnik_color.hpp"
#include "pixel_utils.hpp"

// std
#include <exception>
#include <sstream> // for basic_ostringstream, etc
#include <cstdlib>

Napi::FunctionReference Image::constructor;

Napi::Object Image::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Image", {
            InstanceAccessor<&Image::offset, &Image::offset>("offset", prop_attr),
            InstanceAccessor<&Image::scaling, &Image::scaling>("scaling", prop_attr),
            InstanceMethod<&Image::getType>("getType", prop_attr),
            InstanceMethod<&Image::encodeSync>("encodeSync", prop_attr),
            InstanceMethod<&Image::encode>("encode", prop_attr),
            InstanceMethod<&Image::saveSync>("saveSync", prop_attr),
            InstanceMethod<&Image::save>("save", prop_attr),
            InstanceMethod<&Image::fillSync>("fillSync", prop_attr),
            InstanceMethod<&Image::fill>("fill", prop_attr),
            InstanceMethod<&Image::width>("width", prop_attr),
            InstanceMethod<&Image::height>("height", prop_attr),
            InstanceMethod<&Image::setPixel>("setPixel", prop_attr),
            InstanceMethod<&Image::getPixel>("getPixel", prop_attr),
            InstanceMethod<&Image::compare>("compare", prop_attr),
            InstanceMethod<&Image::setGrayScaleToAlpha>("setGrayScaleToAlpha", prop_attr),
            InstanceMethod<&Image::premultiplied>("premultiplied", prop_attr),
            InstanceMethod<&Image::isSolidSync>("isSolidSync", prop_attr),
            InstanceMethod<&Image::isSolid>("isSolid", prop_attr),
            InstanceMethod<&Image::data>("data", prop_attr),
            InstanceMethod<&Image::buffer>("buffer", prop_attr),
            InstanceMethod<&Image::painted>("painted", prop_attr),
            InstanceMethod<&Image::premultiplySync>("premultiplySync", prop_attr),
            InstanceMethod<&Image::premultiply>("premultiply", prop_attr),
            InstanceMethod<&Image::demultiplySync>("demultiplySync", prop_attr),
            InstanceMethod<&Image::demultiply>("demultiply", prop_attr),
            InstanceMethod<&Image::clearSync>("clearSync", prop_attr),
            InstanceMethod<&Image::clear>("clear", prop_attr),
            InstanceMethod<&Image::copySync>("copySync", prop_attr),
            InstanceMethod<&Image::copy>("copy", prop_attr),
            InstanceMethod<&Image::resizeSync>("resizeSync", prop_attr),
            InstanceMethod<&Image::resize>("resize", prop_attr),
            InstanceMethod<&Image::filterSync>("filterSync", prop_attr),
            InstanceMethod<&Image::filter>("filter", prop_attr),
            InstanceMethod<&Image::composite>("composite", prop_attr),
            InstanceMethod<&Image::view>("view", prop_attr),
            StaticMethod<&Image::openSync>("openSync", prop_attr),
            StaticMethod<&Image::open>("open", prop_attr),
            StaticMethod<&Image::fromBufferSync>("fromBufferSync", prop_attr),
            StaticMethod<&Image::fromBytesSync>("fromBytesSync", prop_attr),
            StaticMethod<&Image::fromBytes>("fromBytes", prop_attr),
            StaticMethod<&Image::fromSVGSync>("fromSVGSync", prop_attr),
            StaticMethod<&Image::fromSVG>("fromSVG", prop_attr),
            StaticMethod<&Image::fromSVGBytesSync>("fromSVGBytesSync", prop_attr),
            StaticMethod<&Image::fromSVGBytes>("fromSVGBytes", prop_attr)
        });
    // clang-format off
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

Image::Image(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Image>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 1 && info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<image_ptr>>();
        if (ext) image_ = *ext.Data();
        return;
    }

    if (info.Length() == 5 && info[0].IsBuffer() && info[1].IsNumber() && info[2].IsNumber() && info[3].IsBoolean() && info[4].IsBoolean())
    {
        auto buf = info[0].As<Napi::Buffer<unsigned char>>();
        int width = info[1].As<Napi::Number>().Int32Value();
        int height = info[2].As<Napi::Number>().Int32Value();
        bool premultiplied = info[3].As<Napi::Boolean>();
        bool painted = info[4].As<Napi::Boolean>();
        mapnik::image_rgba8 im_wrapper(width, height, buf.Data(), premultiplied, painted);
        image_ = std::make_shared<mapnik::image_any>(im_wrapper);
        buf_ref_ = Napi::Persistent(buf);
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
            Napi::TypeError::New(env, "Image 'width' and 'height' must be integers").ThrowAsJavaScriptException();
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
            return env.Undefined();
        }

        Napi::Object options = info[2].As<Napi::Object>();
        if (options.Has("get_color"))
        {
            Napi::Value bind_opt = options.Get("get_color");
            if (!bind_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "optional arg 'color' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            get_color = bind_opt.As<Napi::Boolean>();
        }
    }

    if (info.Length() >= 2)
    {
        if (!info[0].IsNumber())
        {
            Napi::TypeError::New(env, "first arg, 'x' must be an integer").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        if (!info[1].IsNumber())
        {
            Napi::TypeError::New(env, "second arg, 'y' must be an integer").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        x = info[0].As<Napi::Number>().Int32Value();
        y = info[1].As<Napi::Number>().Int32Value();
    }
    else
    {
        Napi::Error::New(env, "must supply x,y to query pixel color").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (x >= 0 && x < static_cast<int>(image_->width())
        && y >= 0 && y < static_cast<int>(image_->height()))
    {
        if (get_color)
        {
            Napi::EscapableHandleScope scope(env);
            mapnik::color col = mapnik::get_pixel<mapnik::color>(*image_, x, y);
            Napi::Value arg = Napi::External<mapnik::color>::New(env, &col);
            Napi::Object obj = Color::constructor.New({arg});
            return scope.Escape(obj);
        }
        else
        {
            detail::visitor_get_pixel<mapnik::image_any> visitor{env, x, y};
            return mapnik::util::apply_visitor(visitor, *image_);
        }
    }
    return env.Undefined();
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
        auto num = info[2].As<Napi::Number>();
        detail::visitor_set_pixel visitor(num, x, y);
        mapnik::util::apply_visitor(visitor, *image_);
    }
    else if (info[2].IsObject())
    {
        Napi::Object obj = info[2].As<Napi::Object>();
        if (!obj.InstanceOf(Color::constructor.Value()))
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
        return env.Undefined();
    }
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.InstanceOf(Image::constructor.Value()))
    {
        Napi::TypeError::New(env, "mapnik.Image expected as first arg").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int threshold = 0;
    unsigned alpha = true;

    if (info.Length() > 1)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        if (options.Has("threshold"))
        {
            Napi::Value bind_opt = options.Get("threshold");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'threshold' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            threshold = bind_opt.As<Napi::Number>().Int32Value();
        }

        if (options.Has("alpha"))
        {
            Napi::Value bind_opt = options.Get("alpha");
            if (!bind_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "optional arg 'alpha' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            alpha = bind_opt.As<Napi::Boolean>();
        }
    }
    image_ptr image2 = Napi::ObjectWrap<Image>::Unwrap(obj)->image_;

    if (image_->width() != image2->width() ||
        image_->height() != image2->height())
    {
            Napi::TypeError::New(env, "image dimensions do not match").ThrowAsJavaScriptException();
            return env.Undefined();
    }
    unsigned difference = mapnik::compare(*image_, *image2, threshold, alpha);
    return Napi::Number::New(env, difference);
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

Napi::Value Image::setGrayScaleToAlpha(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 0)
    {
        mapnik::set_grayscale_to_alpha(*image_);
        return env.Undefined();
    }
    else if (!info[0].IsObject())
    {
        Napi::TypeError::New(env, "optional first arg must be a mapnik.Color").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.InstanceOf(Color::constructor.Value()))
    {
        Napi::TypeError::New(env, "mapnik.Color expected as first arg").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Color * color = Napi::ObjectWrap<Color>::Unwrap(obj);
    mapnik::set_grayscale_to_alpha(*image_, color->color_);
    return env.Undefined();
}

/*
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

Napi::Value Image::premultiplied(Napi::CallbackInfo const& info)
{
    bool premultiplied = image_->get_premultiplied();
    return Napi::Boolean::New(info.Env(), premultiplied);
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

Napi::Value Image::painted(Napi::CallbackInfo const& info)
{
    return Napi::Boolean::New(info.Env(), image_->painted());
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

Napi::Value Image::width(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), image_->width());
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
    return Napi::Number::New(info.Env(), image_->height());
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

Napi::Value Image::view(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if ( (info.Length() != 4) ||
         (!info[0].IsNumber() && !info[1].IsNumber() && !info[2].IsNumber() && !info[3].IsNumber() ))
    {
        Napi::TypeError::New(env, "requires 4 integer arguments: x, y, width, height").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Number x = info[0].As<Napi::Number>();
    Napi::Number y = info[1].As<Napi::Number>();
    Napi::Number w = info[2].As<Napi::Number>();
    Napi::Number h = info[3].As<Napi::Number>();
    Napi::Value image_obj = Napi::External<image_ptr>::New(env, &image_);
    if (buf_ref_.IsEmpty())
    {
        return scope.Escape(ImageView::constructor.New({image_obj, x, y, w, h}));
    }
    Napi::Object obj = ImageView::constructor.New({image_obj, x, y, w, h, buf_ref_.Value()});
    return scope.Escape(obj);
}

Napi::Value Image::offset(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), image_->get_offset());
}

void Image::offset(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsNumber())
    {
        Napi::Error::New(env, "Must provide a number").ThrowAsJavaScriptException();
    }
    else
    {
        double val = value.As<Napi::Number>().DoubleValue();
        image_->set_offset(val);
    }
}

Napi::Value Image::scaling(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), image_->get_scaling());
}

void Image::scaling(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
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
            return;
        }
        image_->set_scaling(val);
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

Napi::Value Image::data(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if (image_) return scope.Escape(Napi::Buffer<unsigned char>::Copy(env, image_->bytes(), image_->size()));
    return info.Env().Null();
}


/**
 * Return pixel data in this image as a buffer
 * (NOTE: Caller must ensure original Image is alive
 * while buffer is used as ownership is neither transferred or
 * assumed)
 *
 * @name data
 * @instance
 * @memberof Image
 * @returns {Buffer} pixel data as a buffer
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * var buff = img.buffer();
 */

Napi::Value Image::buffer(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if (image_) return scope.Escape(Napi::Buffer<unsigned char>::New(env, image_->bytes(), image_->size()));
    return info.Env().Null();
}
