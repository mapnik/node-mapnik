#include <mapnik/image.hpp>      // for image types
#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"

namespace detail {

// AsyncWorker

struct AsyncClear : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncClear(image_ptr const& image, Napi::Function const& callback)
        : Base(callback),
          image_(image)
    {
    }

    void Execute() override
    {
        mapnik::fill(*image_, 0);
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

Napi::Value Image::clearSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    try
    {
        mapnik::fill(*image_, 0);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

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

Napi::Value Image::clear(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return clearSync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto* worker = new detail::AsyncClear{image_, callback_val.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}
