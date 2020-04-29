#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc
#include <mapnik/image_reader.hpp>      // for get_image_reader, etc

#include "mapnik_image.hpp"

namespace {

struct AsyncFromBytes : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncFromBytes(Napi::Buffer<char> const& buffer, std::size_t max_size, bool premultiply, Napi::Function const& callback)
        : Base(callback),
          buffer_ref{Napi::Persistent(buffer)},
          data_{buffer.Data()},
          dataLength_{buffer.Length()},
          max_size_{max_size},
          premultiply_{premultiply}
    {}

    void Execute() override
    {
        try
        {
            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(data_, dataLength_));
            if (reader.get())
            {
                if (reader->width() > max_size_ || reader->height() > max_size_)
                {
                    std::stringstream s;
                    s << "image created from bytes must be " << max_size_ << " pixels or fewer on each side";
                    SetError(s.str());
                    return;
                }
                else
                {
                    image_ = std::make_shared<mapnik::image_any>(reader->read(0, 0, reader->width(), reader->height()));
                    if (premultiply_) mapnik::premultiply_alpha(*image_);
                }
            }
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
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
    Napi::Reference<Napi::Buffer<char>> buffer_ref;
    char const* data_;
    std::size_t dataLength_;
    std::size_t max_size_;
    bool premultiply_;
    image_ptr image_;
};

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

Napi::Value Image::fromBytesSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
        return scope.Escape(env.Undefined());
    }

    Napi::Object obj = info[0].As<Napi::Object>();
    if (obj.IsNull() || obj.IsUndefined() || !obj.IsBuffer())
    {
        Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();
        return scope.Escape(env.Undefined());
    }

    try
    {
        std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(obj.As<Napi::Buffer<char>>().Data(),
                                                                              obj.As<Napi::Buffer<char>>().Length()));
        if (reader.get())
        {
            image_ptr imagep = std::make_shared<mapnik::image_any>(reader->read(0, 0, reader->width(), reader->height()));
            if (!reader->has_alpha())
            {
                mapnik::set_premultiplied_alpha(*imagep, true);
            }
            Napi::Value arg = Napi::External<image_ptr>::New(env, &imagep);
            Napi::Object obj = constructor.New({arg});
            return scope.Escape(napi_value(obj)).ToObject();
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

Napi::Value Image::fromBytes(Napi::CallbackInfo const& info)
{
    if (info.Length() == 1)
    {
        return fromBytesSync(info);
    }

    Napi::Env env = info.Env();

    if (info.Length() < 2)
    {
        Napi::Error::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsObject()) {
        Napi::TypeError::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object obj = info[0].As<Napi::Object>();
    if (obj.IsNull() || obj.IsUndefined() || !obj.IsBuffer())
    {
        Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool premultiply = false;
    std::size_t max_size = 2048;
    if (info.Length() >= 2)
    {
        if (info[1].IsObject())
        {
            Napi::Object options = info[1].As<Napi::Object>();
            if (options.Has("premultiply"))
            {
                Napi::Value opt = options.Get("premultiply");
                if (!opt.IsEmpty() && opt.IsBoolean())
                {
                    premultiply = opt.As<Napi::Boolean>();
                }
                else
                {
                    Napi::TypeError::New(env, "premultiply option must be a boolean").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }
            if (options.Has("max_size"))
            {
                Napi::Value opt = options.Get("max_size");
                if (opt.IsNumber())
                {
                    auto max_size_val = opt.As<Napi::Number>().Int32Value();
                    if (max_size_val < 0 || max_size_val > 65535)
                    {
                        Napi::TypeError::New(env, "max_size must be a positive integer between 0 and 65535").ThrowAsJavaScriptException();
                        return env.Null();
                    }
                    max_size = static_cast<std::size_t>(max_size_val);
                }
                else
                {
                    Napi::TypeError::New(env, "max_size option must be a number").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }
        }
    }
    Napi::Function callback = callback_val.As<Napi::Function>();
    Napi::Buffer<char> buffer = info[0].As<Napi::Buffer<char>>();
    auto * worker = new AsyncFromBytes(buffer, max_size, premultiply, callback);
    worker->Queue();
    return env.Undefined();
}
