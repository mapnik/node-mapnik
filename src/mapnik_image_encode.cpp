// mapnik

#include <mapnik/image.hpp>      // for image types
#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc
#include <mapnik/image_copy.hpp>
#include "mapnik_image.hpp"
#include "mapnik_palette.hpp"

void Image::encode_common_args_(Napi::CallbackInfo const& info, std::string& format, palette_ptr& palette)
{
    Napi::Env env = info.Env();
    // accept custom format
    if (info.Length() >= 1)
    {
        if (!info[0].IsString())
        {
            Napi::TypeError::New(env, "first arg, 'format' must be a string").ThrowAsJavaScriptException();
            return;
        }
        format = info[0].As<Napi::String>();
    }
    // options
    if (info.Length() >= 2)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return;
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("palette"))
        {
            Napi::Value palette_opt = options.Get("palette");
            if (!palette_opt.IsObject())
            {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return;
            }

            Napi::Object obj = palette_opt.As<Napi::Object>();

            if (!obj.InstanceOf(Palette::constructor.Value()))
            {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return;
            }
            palette = Napi::ObjectWrap<Palette>::Unwrap(obj)->palette_;
        }
    }
}

namespace {

struct AsyncEncode : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    // ctor
    AsyncEncode(Image* obj, image_ptr image, palette_ptr palette, std::string const& format, Napi::Function const& callback)
        : Base(callback),
          obj_(obj),
          image_(image),
          palette_(palette),
          format_(format)
    {
    }
    ~AsyncEncode() {}
    void OnWorkComplete(Napi::Env env, napi_status status) override
    {
        if (obj_ && !obj_->IsEmpty())
        {
            obj_->Unref();
        }
        Base::OnWorkComplete(env, status);
    }
    void Execute() override
    {
        try
        {
            if (palette_)
                result_ = std::make_unique<std::string>(save_to_string(*image_, format_, *palette_));
            else
                result_ = std::make_unique<std::string>(save_to_string(*image_, format_));
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        if (result_)
        {
            std::string& str = *result_;
            auto buffer = Napi::Buffer<char>::New(
                env,
                str.empty() ? nullptr : &str[0],
                str.size(),
                [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                    if (str_ptr != nullptr)
                    {
                        Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                    }
                    delete str_ptr;
                },
                result_.release());
            Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(str.size()));
            return {env.Null(), buffer};
        }
        return Base::GetResult(env);
    }

  private:
    Image* obj_;
    image_ptr image_;
    palette_ptr palette_;
    std::string format_;
    std::unique_ptr<std::string> result_;
};

} // namespace

/**
 * Encode this image into a buffer of encoded data (synchronous)
 *
 * @name encodeSync
 * @param {string} [format=png] image format
 * @param {Object} [options]
 * @param {mapnik.Palette} [options.palette] - mapnik.Palette object
 * @returns {Buffer} buffer - encoded image data
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * var buffer = img.encodeSync('png');
 * // write buffer to a new file
 * fs.writeFileSync('myimage.png', buffer);
 */

Napi::Value Image::encodeSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::string format{"png"};
    palette_ptr palette;
    encode_common_args_(info, format, palette);
    try
    {
        std::unique_ptr<std::string> result;
        if (palette)
            result = std::make_unique<std::string>(save_to_string(*image_, format, *palette));
        else
            result = std::make_unique<std::string>(save_to_string(*image_, format));
        std::string& str = *result;
        auto buffer = Napi::Buffer<char>::New(
            env,
            str.empty() ? nullptr : &str[0],
            str.size(),
            [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                if (str_ptr != nullptr)
                {
                    Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                }
                delete str_ptr;
            },
            result.release());
        Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(str.size()));
        return scope.Escape(buffer);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

/**
 * Encode this image into a buffer of encoded data
 *
 * @name encode
 * @param {string} [format=png] image format
 * @param {Object} [options]
 * @param {mapnik.Palette} [options.palette] - mapnik.Palette object
 * @param {Function} callback - `function(err, encoded)`
 * @returns {Buffer} encoded image data
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * myImage.encode('png', function(err, encoded) {
 *   if (err) throw err;
 *   // write buffer to new file
 *   fs.writeFileSync('myimage.png', encoded);
 * });
 *
 * // encoding an image object with a mapnik.Palette
 * var im = new mapnik.Image(256, 256);
 * var pal = new mapnik.Palette(new Buffer('\xff\x09\x93\xFF\x01\x02\x03\x04','ascii'));
 * im.encode('png', {palette: pal}, function(err, encode) {
 *   if (err) throw err;
 *   // your custom code with `encode` image buffer
 * });
 */

Napi::Value Image::encode(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    std::string format{"png"};
    palette_ptr palette;
    encode_common_args_(info, format, palette);
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Function callback = callback_val.As<Napi::Function>();
    // Increment reference count here to ensure 'Image' object is not GC'ed during async op.
    // `Unref()` is called on completion in `OnWorkComplete`
    this->Ref();
    auto* worker = new AsyncEncode{this, image_, palette, format, callback};
    worker->Queue();
    return env.Undefined();
}
