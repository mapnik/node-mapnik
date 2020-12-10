#include "mapnik_image.hpp"
#include "mapnik_color.hpp"
#include <mapnik/image.hpp>      // for image types
#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc

namespace detail {

// AsyncWorker
template <typename T>
struct AsyncFill : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncFill(image_ptr const& image, T const& val, Napi::Function const& callback)
        : Base(callback),
          image_(image),
          val_(val) {}

    void Execute() override
    {
        mapnik::fill(*image_, val_);
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        if (image_)
        {
            Napi::Value arg = Napi::External<image_ptr>::New(env, &image_);
            Napi::Object obj = Image::constructor.New({arg});
            return {env.Null(), napi_value(obj)};
        }
        return Base::GetResult(env);
    }
    image_ptr image_;
    T val_;
};
} // namespace detail

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

Napi::Value Image::fillSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "expects one argument: Color object or a number").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try
    {
        if (info[0].IsNumber())
        {
            double val = info[0].As<Napi::Number>().DoubleValue();
            mapnik::fill<double>(*image_, val);
        }
        else if (info[0].IsObject())
        {
            Napi::Object obj = info[0].As<Napi::Object>();
            if (!obj.InstanceOf(Color::constructor.Value()))
            {
                Napi::TypeError::New(env, "A numeric or color value is expected").ThrowAsJavaScriptException();
            }
            else
            {
                Color* color = Napi::ObjectWrap<Color>::Unwrap(obj);
                mapnik::fill(*image_, color->color_);
            }
        }
        else
        {
            Napi::TypeError::New(env, "A numeric or color value is expected").ThrowAsJavaScriptException();
        }
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

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

Napi::Value Image::fill(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2)
    {
        return fillSync(info);
    }

    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Function callback = callback_val.As<Napi::Function>();
    double val = 0.0;
    if (info[0].IsNumber())
    {
        val = info[0].As<Napi::Number>().DoubleValue();
        auto* worker = new detail::AsyncFill<double>(image_, val, callback);
        worker->Queue();
        return env.Undefined();
    }
    else if (info[0].IsObject())
    {
        Napi::Object obj = info[0].As<Napi::Object>();
        if (!obj.InstanceOf(Color::constructor.Value()))
        {
            Napi::TypeError::New(env, "A numeric or color value is expected").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        else
        {
            Color* color = Napi::ObjectWrap<Color>::Unwrap(obj);
            auto* worker = new detail::AsyncFill<mapnik::color>(image_, color->color_, callback);
            worker->Queue();
        }
    }
    else
    {
        Napi::TypeError::New(env, "A numeric or color value is expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    return env.Undefined();
}
