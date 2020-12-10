#include <mapnik/image.hpp>      // for image types
#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"

namespace detail {

// AsyncWorker
template <bool pre = true>
struct AsyncMultiply : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncMultiply(image_ptr const& image, Napi::Function const& callback)
        : Base(callback),
          image_(image)
    {
    }

    void Execute() override
    {
        if (pre)
            mapnik::premultiply_alpha(*image_);
        else
            mapnik::demultiply_alpha(*image_);
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
};

} // namespace detail

using AsyncPremultiply = detail::AsyncMultiply<true>;
using AsyncDemultiply = detail::AsyncMultiply<false>;

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

Napi::Value Image::premultiplySync(Napi::CallbackInfo const& info)
{
    mapnik::premultiply_alpha(*image_);
    return info.Env().Undefined();
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

Napi::Value Image::premultiply(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return premultiplySync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto* worker = new AsyncPremultiply{image_, callback_val.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}

/**
 * Demultiply the pixels in this image. The opposite of
 * premultiplying.
 *
 * @name demultiplySync
 * @instance
 * @memberof Image
 */

Napi::Value Image::demultiplySync(Napi::CallbackInfo const& info)
{
    mapnik::demultiply_alpha(*image_);
    return info.Env().Undefined();
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

Napi::Value Image::demultiply(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return demultiplySync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto* worker = new AsyncDemultiply{image_, callback_val.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}
