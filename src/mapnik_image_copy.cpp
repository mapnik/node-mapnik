#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc
#include <mapnik/image_copy.hpp>

#include "mapnik_image.hpp"

namespace detail {

struct AsyncCopy : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncCopy(image_ptr const& image, double offset, double scaling, mapnik::image_dtype type, Napi::Function const& callback)
        : Base(callback),
          image_in_(image),
          offset_{offset},
          scaling_{scaling},
          type_{type}
    {
    }

    void Execute() override
    {
        try
        {
            image_out_ = std::make_shared<mapnik::image_any>(
                mapnik::image_copy(*image_in_, type_, offset_, scaling_));
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        if (image_out_)
        {
            Napi::Value arg = Napi::External<image_ptr>::New(env, &image_out_);
            Napi::Object obj = Image::constructor.New({arg});
            return {env.Undefined(), napi_value(obj)};
        }
        return Base::GetResult(env);
    }

  private:
    image_ptr image_in_;
    image_ptr image_out_;
    double offset_;
    double scaling_;
    mapnik::image_dtype type_;
};

} // namespace detail

/**
 * Copy an image into a new image by creating a clone
 * @name copy
 * @instance
 * @memberof Image
 * @param {number} type - image type to clone into, can be any mapnik.imageType number
 * @param {Object} [options={}]
 * @param {number} [options.scaling] - scale the image
 * @param {number} [options.offset] - offset this image
 * @param {Function} callback
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray16});
 * var img2 = img.copy(mapnik.imageType.gray8, function(err, img2) {
 *   if (err) throw err;
 *   // custom code with `img2` converted into gray8 type
 * });
 */

Napi::Value Image::copy(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return copySync(info);
    }
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        return copySync(info);
    }
    Napi::Env env = info.Env();
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = image_->get_dtype();
    Napi::Object options = Napi::Object::New(env);

    if (info.Length() >= 2)
    {
        if (info[0].IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(info[0].As<Napi::Number>().Int32Value());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Napi::TypeError::New(env, "Image 'type' must be a valid image type").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        else if (info[0].IsObject())
        {
            options = info[0].As<Napi::Object>();
        }
        else
        {
            Napi::TypeError::New(env, "Unknown parameters passed").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    if (info.Length() >= 3)
    {
        if (info[1].IsObject())
        {
            options = info[1].As<Napi::Object>();
        }
        else
        {
            Napi::TypeError::New(env, "Expected options object as second argument").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (options.Has("scaling"))
    {
        Napi::Value scaling_val = options.Get("scaling");
        if (scaling_val.IsNumber())
        {
            scaling = scaling_val.As<Napi::Number>().DoubleValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Napi::TypeError::New(env, "scaling argument must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (options.Has("offset"))
    {
        Napi::Value offset_val = options.Get("offset");
        if (offset_val.IsNumber())
        {
            offset = offset_val.As<Napi::Number>().DoubleValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Napi::TypeError::New(env, "offset argument must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (!scaling_or_offset_set && type == image_->get_dtype())
    {
        scaling = image_->get_scaling();
        offset = image_->get_offset();
    }

    auto* worker = new detail::AsyncCopy{image_, offset, scaling, type, callback_val.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}

/**
 * Copy an image into a new image by creating a clone
 * @name copySync
 * @instance
 * @memberof Image
 * @param {number} type - image type to clone into, can be any mapnik.imageType number
 * @param {Object} [options={}]
 * @param {number} [options.scaling] - scale the image
 * @param {number} [options.offset] - offset this image
 * @returns {mapnik.Image} copy
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray16});
 * var img2 = img.copy(mapnik.imageType.gray8);
 * // custom code with `img2` as a gray8 type
 */

Napi::Value Image::copySync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = image_->get_dtype();
    Napi::Object options = Napi::Object::New(env);
    if (info.Length() >= 1)
    {
        if (info[0].IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(info[0].As<Napi::Number>().Int32Value());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Napi::TypeError::New(env, "Image 'type' must be a valid image type").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        else if (info[0].IsObject())
        {
            options = info[0].As<Napi::Object>();
        }
        else
        {
            Napi::TypeError::New(env, "Unknown parameters passed").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    if (info.Length() >= 2)
    {
        if (info[1].IsObject())
        {
            options = info[1].As<Napi::Object>();
        }
        else
        {
            Napi::TypeError::New(env, "Expected options object as second argument").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (options.Has("scaling"))
    {
        Napi::Value scaling_val = options.Get("scaling");
        if (scaling_val.IsNumber())
        {
            scaling = scaling_val.As<Napi::Number>().DoubleValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Napi::TypeError::New(env, "scaling argument must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (options.Has("offset"))
    {
        Napi::Value offset_val = options.Get("offset");
        if (offset_val.IsNumber())
        {
            offset = offset_val.As<Napi::Number>().DoubleValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Napi::TypeError::New(env, "offset argument must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (!scaling_or_offset_set && type == image_->get_dtype())
    {
        scaling = image_->get_scaling();
        offset = image_->get_offset();
    }

    try
    {
        image_ptr image_out = std::make_shared<mapnik::image_any>(
            mapnik::image_copy(*image_, type, offset, scaling));
        Napi::Value arg = Napi::External<image_ptr>::New(env, &image_out);
        Napi::Object obj = Image::constructor.New({arg});
        return scope.Escape(obj);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}
