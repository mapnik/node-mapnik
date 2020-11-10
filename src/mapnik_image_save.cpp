// mapnik
#include <mapnik/color.hpp>      // for color
#include <mapnik/image.hpp>      // for image types
#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"

namespace {
struct AsyncSave : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    // ctor
    AsyncSave(image_ptr const& image, std::string const& filename, std::string const& format, Napi::Function const& callback)
        : Base(callback),
          image_(image),
          filename_(filename),
          format_(format)
    {
    }

    void Execute() override
    {
        try
        {
            mapnik::save_to_file(*image_, filename_, format_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

  private:
    image_ptr image_;
    std::string filename_;
    std::string format_;
};
} // namespace

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

void Image::saveSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 0 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "filename required to save file").ThrowAsJavaScriptException();
        return;
    }

    std::string filename = info[0].As<Napi::String>();
    std::string format("");

    if (info.Length() >= 2)
    {
        if (!info[1].IsString())
        {
            Napi::TypeError::New(env, "both 'filename' and 'format' arguments must be strings").ThrowAsJavaScriptException();
            return;
        }
        format = info[1].As<Napi::String>();
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>")
        {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return;
        }
    }
    try
    {
        mapnik::save_to_file(*image_, filename, format);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
}

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

void Image::save(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 0 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "filename required to save file").ThrowAsJavaScriptException();
        return;
    }
    if (!info[info.Length() - 1].IsFunction())
    {
        return saveSync(info);
    }
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];

    std::string filename = info[0].As<Napi::String>();
    std::string format("");

    if (info.Length() >= 3)
    {
        if (!info[1].IsString())
        {
            Napi::TypeError::New(env, "both 'filename' and 'format' arguments must be strings").ThrowAsJavaScriptException();
        }
        format = info[1].As<Napi::String>();
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>")
        {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return;
        }
    }

    Napi::Function callback = callback_val.As<Napi::Function>();
    auto* worker = new AsyncSave(image_, filename, format, callback);
    worker->Queue();
    return;
}
