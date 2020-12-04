#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc
#include <mapnik/image_filter_types.hpp>
#include <mapnik/image_filter.hpp> // filter_visitor

#include "mapnik_image.hpp"

namespace detail {

// AsyncWorker

struct AsyncFilter : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncFilter(image_ptr const& image, std::string const& filter, Napi::Function const& callback)
        : Base(callback),
          image_(image),
          filter_(filter)
    {
    }

    void Execute() override
    {
        try
        {
            mapnik::filter::filter_image(*image_, filter_); // NOLINT
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
            ;
        }
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
    std::string filter_;
};
} // namespace detail

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

Napi::Value Image::filterSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "expects one argument: string filter argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "A string is expected for filter argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string filter = info[0].As<Napi::String>();
    try
    {
        mapnik::filter::filter_image(*image_, filter);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

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

Napi::Value Image::filter(Napi::CallbackInfo const& info)
{
    if (info.Length() < 2)
    {
        return filterSync(info);
    }
    Napi::Env env = info.Env();
    if (!info[info.Length() - 1].IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "A string is expected for filter argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Function callback = info[info.Length() - 1].As<Napi::Function>();
    auto* worker = new detail::AsyncFilter{image_, info[0].As<Napi::String>(), callback};
    worker->Queue();
    return env.Undefined();
}
