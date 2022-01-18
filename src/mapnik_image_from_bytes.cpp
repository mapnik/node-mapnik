#include <mapnik/image.hpp>        // for image types
#include <mapnik/image_any.hpp>    // for image_any
#include <mapnik/image_util.hpp>   // for save_to_string, guess_type, etc
#include <mapnik/image_reader.hpp> // for get_image_reader, etc

#include "mapnik_image.hpp"
#include <sstream>
namespace detail {

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
    {
    }

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

} // namespace detail

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
        return env.Undefined();
    }

    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.IsBuffer())
    {
        Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();
        return env.Undefined();
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
            return scope.Escape(obj);
        }
        // The only way this is ever reached is if the reader factory in
        // mapnik was not providing an image type it should. This should never
        // be occuring so marking this out from coverage
        Napi::TypeError::New(env, "Failed to load from buffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
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
        return env.Undefined();
    }

    if (!info[0].IsObject())
    {
        Napi::TypeError::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.IsBuffer())
    {
        Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
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
                    return env.Undefined();
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
                        return env.Undefined();
                    }
                    max_size = static_cast<std::size_t>(max_size_val);
                }
                else
                {
                    Napi::TypeError::New(env, "max_size option must be a number").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
        }
    }
    Napi::Function callback = callback_val.As<Napi::Function>();
    Napi::Buffer<char> buffer = info[0].As<Napi::Buffer<char>>();
    auto* worker = new detail::AsyncFromBytes(buffer, max_size, premultiply, callback);
    worker->Queue();
    return env.Undefined();
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

Napi::Value Image::fromBufferSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsObject())
    {
        Napi::TypeError::New(env, "must provide a width, height, and buffer argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::int32_t width = info[0].As<Napi::Number>().Int32Value();
    std::int32_t height = info[1].As<Napi::Number>().Int32Value();

    if (width <= 0 || height <= 0)
    {
        Napi::TypeError::New(env, "width and height must be greater than zero").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object obj = info[2].As<Napi::Object>();
    if (!obj.IsBuffer())
    {
        Napi::TypeError::New(env, "third argument is invalid, must be a Buffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // TODO - support other image types?
    auto im_size = mapnik::image_rgba8::pixel_size * width * height;
    if (im_size != obj.As<Napi::Buffer<char>>().Length())
    {
        Napi::TypeError::New(env, "invalid image size").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    bool premultiplied = false;
    bool painted = false;

    if (info.Length() >= 4)
    {
        if (info[3].IsObject())
        {
            Napi::Object options = info[3].As<Napi::Object>();
            if (options.Has("type"))
            {
                Napi::TypeError::New(env, "'type' option not supported (only rgba images currently viable)").ThrowAsJavaScriptException();
                return env.Undefined();
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
                    return env.Undefined();
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
                    return env.Undefined();
                }
            }
        }
        else
        {
            Napi::TypeError::New(env, "Options parameter must be an object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    try
    {
        Napi::Object image_obj = constructor.New({obj,
                                                  Napi::Number::New(env, width),
                                                  Napi::Number::New(env, height),
                                                  Napi::Boolean::New(env, premultiplied),
                                                  Napi::Boolean::New(env, painted)});
        return scope.Escape(image_obj);
    }
    catch (std::exception const& ex)
    {
        // There is no known way for this exception to be reached currently.
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
        // LCOV_EXCL_STOP
    }
}
