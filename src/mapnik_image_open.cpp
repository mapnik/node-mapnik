// mapnik
#include <mapnik/image.hpp>        // for image types
#include <mapnik/image_any.hpp>    // for image_any
#include <mapnik/image_reader.hpp> // for get_image_reader, etc
#include <mapnik/image_util.hpp>   // for save_to_string, guess_type, etc
//
#include "mapnik_image.hpp"

namespace detail {

struct AsyncOpen : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncOpen(std::string const& filename, Napi::Function const& callback)
        : Base(callback),
          filename_(filename) {}

    void Execute() override
    {
        boost::optional<std::string> type = mapnik::type_from_filename(filename_);
        if (type)
        {
            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(filename_, *type));
            if (reader.get())
            {
                try
                {
                    image_ = std::make_shared<mapnik::image_any>(reader->read(0, 0, reader->width(), reader->height()));
                    if (!reader->has_alpha()) mapnik::set_premultiplied_alpha(*image_, true);
                }
                catch (std::exception const& ex)
                {
                    SetError(ex.what());
                }
            }
            else
                SetError("Can't create image reader");
        }
        else
        {
            SetError("Can't determine image type from filename");
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

  private:
    std::string filename_;
    image_ptr image_;
};

} // namespace detail

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

Napi::Value Image::openSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if (info.Length() < 1)
    {
        Napi::Error::New(env, "must provide a string argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "Argument must be a string").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try
    {
        std::string filename = info[0].As<Napi::String>();
        boost::optional<std::string> type = mapnik::type_from_filename(filename);
        if (type)
        {
            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(filename, *type));
            if (reader.get())
            {
                image_ptr imagep = std::make_shared<mapnik::image_any>(reader->read(0, 0, reader->width(), reader->height()));
                if (!reader->has_alpha())
                {
                    mapnik::set_premultiplied_alpha(*imagep, true);
                }
                Napi::Value arg = Napi::External<image_ptr>::New(env, &imagep);
                Napi::Object obj = constructor.New({arg});
                return scope.Escape(obj);
            }
        }
        Napi::TypeError::New(env, ("Unsupported image format:" + filename)).ThrowAsJavaScriptException();

        return env.Undefined();
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

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

Napi::Value Image::open(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();

    if (info.Length() == 1)
    {
        return openSync(info);
    }

    if (info.Length() < 2)
    {
        Napi::Error::New(env, "must provide a string argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "Argument must be a string").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Function callback = callback_val.As<Napi::Function>();
    std::string filename = info[0].As<Napi::String>();
    auto* worker = new detail::AsyncOpen(filename, callback);
    worker->Queue();
    return env.Undefined();
}
