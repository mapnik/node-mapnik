#include <mapnik/image_any.hpp>     // NOLINT for image_any
#include <mapnik/image_util.hpp>    // NOLINT for save_to_string, guess_type, etc
#include <mapnik/image_scaling.hpp> // NOLINT
#include "mapnik_image.hpp"         // NOLINT

namespace {

struct resize_visitor
{

    resize_visitor(mapnik::image_any& im1,
                   mapnik::scaling_method_e scaling_method,
                   double image_ratio_x,
                   double image_ratio_y,
                   double filter_factor,
                   double offset_x,
                   double offset_y) : im1_(im1),
                                      scaling_method_(scaling_method),
                                      image_ratio_x_(image_ratio_x),
                                      image_ratio_y_(image_ratio_y),
                                      filter_factor_(filter_factor),
                                      offset_x_(offset_x),
                                      offset_y_(offset_y) {}

    void operator()(mapnik::image_rgba8& im2) const
    {
        bool remultiply = false;
        if (!im1_.get_premultiplied())
        {
            remultiply = true;
            mapnik::premultiply_alpha(im1_);
        }
        // NOLINTNEXTLINE
        mapnik::scale_image_agg(im2,
                                mapnik::util::get<mapnik::image_rgba8>(im1_),
                                scaling_method_,
                                image_ratio_x_,
                                image_ratio_y_,
                                offset_x_,
                                offset_y_,
                                filter_factor_);
        if (remultiply)
        {
            mapnik::demultiply_alpha(im2);
        }
    }

    template <typename T>
    void operator()(T& im2) const
    {
        // NOLINTNEXTLINE
        mapnik::scale_image_agg(im2,
                                mapnik::util::get<T>(im1_),
                                scaling_method_,
                                image_ratio_x_,
                                image_ratio_y_,
                                offset_x_,
                                offset_y_,
                                filter_factor_);
    }

    void operator()(mapnik::image_null&) const
    {
        // Should be caught earlier so no test coverage should reach here.
        throw std::runtime_error("Can not resize null images");
    }

    void operator()(mapnik::image_gray8s&) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 8 bit integer rasters");
    }

    void operator()(mapnik::image_gray16s&) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 16 bit integer rasters");
    }

    void operator()(mapnik::image_gray32&) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing unsigned 32 bit integer rasters");
    }

    void operator()(mapnik::image_gray32s&) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 32 bit integer rasters");
    }

    void operator()(mapnik::image_gray64&) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing unsigned 64 bit integer rasters");
    }

    void operator()(mapnik::image_gray64s&) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 64 bit integer rasters");
    }

    void operator()(mapnik::image_gray64f&) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing 64 bit floating point rasters");
    }

  private:
    mapnik::image_any& im1_;
    mapnik::scaling_method_e scaling_method_;
    double image_ratio_x_;
    double image_ratio_y_;
    double filter_factor_;
    int offset_x_;
    int offset_y_;
};
} // namespace

namespace detail {
struct AsyncResize : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncResize(image_ptr const& image, mapnik::scaling_method_e scaling_method,
                std::size_t width, std::size_t height,
                double offset_x, double offset_y, double offset_width, double offset_height,
                double filter_factor, Napi::Function const& callback)
        : Base(callback),
          image_in_(image),
          scaling_method_{scaling_method},
          width_{width},
          height_{height},
          offset_x_{offset_x},
          offset_y_{offset_y},
          offset_width_{offset_width},
          offset_height_{offset_height},
          filter_factor_{filter_factor}
    {
    }

    void Execute() override
    {
        if (image_in_->is<mapnik::image_null>())
        {
            SetError("Can not resize a null image.");
            return;
        }
        try
        {
            double offset = image_in_->get_offset();
            double scaling = image_in_->get_scaling();

            image_out_ = std::make_shared<mapnik::image_any>(width_,
                                                             height_,
                                                             image_in_->get_dtype(),
                                                             true,
                                                             true,
                                                             false);
            image_out_->set_offset(offset);
            image_out_->set_scaling(scaling);
            if (offset_width_ <= 0 || offset_height_ <= 0)
            {
                SetError("Image width or height is zero or less than zero.");
                return;
            }

            double image_ratio_x = static_cast<double>(width_) / offset_width_;
            double image_ratio_y = static_cast<double>(height_) / offset_height_;
            double corrected_offset_x = offset_x_ * image_ratio_x;
            double corrected_offset_y = offset_y_ * image_ratio_y;
            resize_visitor visit(*image_in_, scaling_method_, image_ratio_x, image_ratio_y,
                                 filter_factor_, corrected_offset_x, corrected_offset_y);
            mapnik::util::apply_visitor(visit, *image_out_);
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
            return {env.Null(), napi_value(obj)};
        }
        return Base::GetResult(env);
    }

  private:
    image_ptr image_in_;
    image_ptr image_out_;
    mapnik::scaling_method_e scaling_method_;
    std::size_t width_;
    std::size_t height_;
    double offset_x_;
    double offset_y_;
    double offset_width_;
    double offset_height_;
    double filter_factor_;
};

} // namespace detail

/**
 * Resize this image (makes a copy)
 *
 * @name resize
 * @instance
 * @memberof Image
 * @param {number} width - in pixels
 * @param {number} height - in pixels
 * @param {Object} [options={}]
 * @param {number} [options.offset_x=0] - offset the image horizontally in pixels
 * @param {number} [options.offset_y=0] - offset the image vertically in pixels
 * @param {number} [options.offset_width] - the width from the start of the offset_x to use from source image
 * @param {number} [options.offset_height] - the height from the start of the offset_y to use from source image
 * @param {mapnik.imageScaling} [options.scaling_method=mapnik.imageScaling.near] - scaling method
 * @param {number} [options.filter_factor=1.0]
 * @param {Function} callback - `function(err, result)`
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray8});
 * img.resize(8, 8, function(err, result) {
 *   if (err) throw err;
 *   // new image object as `result`
 * });
 */

Napi::Value Image::resize(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return resizeSync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        return resizeSync(info);
    }
    std::size_t width = 0;
    std::size_t height = 0;
    double offset_x = 0.0;
    double offset_y = 0.0;
    double offset_width = static_cast<double>(image_->width());
    double offset_height = static_cast<double>(image_->height());
    double filter_factor = 1.0;
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    Napi::Object options = Napi::Object::New(env);

    if (info.Length() >= 3)
    {
        if (info[0].IsNumber())
        {
            auto width_tmp = info[0].As<Napi::Number>().Int32Value();
            if (width_tmp <= 0)
            {
                Napi::TypeError::New(env, "Width must be an integer greater than zero").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            width = static_cast<std::size_t>(width_tmp);
        }
        else
        {
            Napi::TypeError::New(env, "Width must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        if (info[1].IsNumber())
        {
            auto height_tmp = info[1].As<Napi::Number>().Int32Value();
            if (height_tmp <= 0)
            {
                Napi::TypeError::New(env, "Height must be an integer greater than zero").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            height = static_cast<std::size_t>(height_tmp);
        }
        else
        {
            Napi::TypeError::New(env, "Height must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    else
    {
        Napi::TypeError::New(env, "resize requires a width and height parameter.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (info.Length() >= 4)
    {
        if (info[2].IsObject())
        {
            options = info[2].As<Napi::Object>();
        }
        else
        {
            Napi::TypeError::New(env, "Expected options object as third argument").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    if (options.Has("offset_x"))
    {
        Napi::Value bind_opt = options.Get("offset_x");
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_x' must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        offset_x = bind_opt.As<Napi::Number>().DoubleValue();
    }
    if (options.Has("offset_y"))
    {
        Napi::Value bind_opt = options.Get("offset_y");
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_y' must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        offset_y = bind_opt.As<Napi::Number>().DoubleValue();
    }
    if (options.Has("offset_width"))
    {
        Napi::Value bind_opt = options.Get("offset_width");
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_width' must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        offset_width = bind_opt.As<Napi::Number>().DoubleValue();
        if (offset_width <= 0.0)
        {
            Napi::TypeError::New(env, "optional arg 'offset_width' must be an integer greater than zero").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    if (options.Has("offset_height"))
    {
        Napi::Value bind_opt = options.Get("offset_height");
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_height' must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        offset_height = bind_opt.As<Napi::Number>().DoubleValue();
        if (offset_height <= 0.0)
        {
            Napi::TypeError::New(env, "optional arg 'offset_height' must be an integer greater than zero").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    if (options.Has("scaling_method"))
    {
        Napi::Value scaling_val = options.Get("scaling_method");
        if (scaling_val.IsNumber())
        {
            std::int64_t scaling_int = scaling_val.As<Napi::Number>().Int32Value();
            if (scaling_int > mapnik::SCALING_BLACKMAN || scaling_int < 0)
            {
                Napi::TypeError::New(env, "Invalid scaling_method").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_int);
        }
        else
        {
            Napi::TypeError::New(env, "scaling_method argument must be an integer").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (options.Has("filter_factor"))
    {
        Napi::Value ff_val = options.Get("filter_factor");
        if (ff_val.IsNumber())
        {
            filter_factor = ff_val.As<Napi::Number>().DoubleValue();
        }
        else
        {
            Napi::TypeError::New(env, "filter_factor argument must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    auto* worker = new detail::AsyncResize{image_, scaling_method, width, height,
                                           offset_x, offset_y, offset_width, offset_height,
                                           filter_factor, callback_val.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}

/**
 * Resize this image (makes a copy). Synchronous version of {@link mapnik.Image.resize}.
 *
 * @name resizeSync
 * @instance
 * @memberof Image
 * @param {number} width
 * @param {number} height
 * @param {Object} [options={}]
 * @param {number} [options.offset_x=0] - offset the image horizontally in pixels
 * @param {number} [options.offset_y=0] - offset the image vertically in pixels
 * @param {number} [options.offset_width] - the width from the start of the offset_x to use from source image
 * @param {number} [options.offset_height] - the height from the start of the offset_y to use from source image
 * @param {mapnik.imageScaling} [options.scaling_method=mapnik.imageScaling.near] - scaling method
 * @param {number} [options.filter_factor=1.0]
 * @returns {mapnik.Image} copy
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray8});
 * var img2 = img.resizeSync(8, 8);
 * // new copy as `img2`
 */

Napi::Value Image::resizeSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::size_t width = 0;
    std::size_t height = 0;
    double filter_factor = 1.0;
    double offset_x = 0.0;
    double offset_y = 0.0;
    double offset_width = static_cast<double>(image_->width());
    double offset_height = static_cast<double>(image_->height());
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    Napi::Object options = Napi::Object::New(env);
    if (info.Length() >= 2)
    {
        if (info[0].IsNumber())
        {
            int width_tmp = info[0].As<Napi::Number>().Int32Value();
            if (width_tmp <= 0)
            {
                Napi::TypeError::New(env, "Width parameter must be an integer greater than zero").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            width = static_cast<std::size_t>(width_tmp);
        }
        else
        {
            Napi::TypeError::New(env, "Width must be a number").ThrowAsJavaScriptException();

            return env.Undefined();
        }
        if (info[1].IsNumber())
        {
            int height_tmp = info[1].As<Napi::Number>().Int32Value();
            if (height_tmp <= 0)
            {
                Napi::TypeError::New(env, "Height parameter must be an integer greater than zero").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            height = static_cast<std::size_t>(height_tmp);
        }
        else
        {
            Napi::TypeError::New(env, "Height must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    else
    {
        Napi::TypeError::New(env, "Resize requires at least a width and height parameter").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() >= 3)
    {
        if (info[2].IsObject())
        {
            options = info[2].As<Napi::Object>();
        }
        else
        {
            Napi::TypeError::New(env, "Expected options object as third argument").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    if (options.Has("offset_x"))
    {
        Napi::Value bind_opt = options.Get("offset_x");
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_x' must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        offset_x = bind_opt.As<Napi::Number>().DoubleValue();
    }
    if (options.Has("offset_y"))
    {
        Napi::Value bind_opt = options.Get("offset_y");
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_y' must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        offset_y = bind_opt.As<Napi::Number>().DoubleValue();
    }
    if (options.Has("offset_width"))
    {
        Napi::Value bind_opt = options.Get("offset_width");
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_width' must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        offset_width = bind_opt.As<Napi::Number>().DoubleValue();
        if (offset_width <= 0.0)
        {
            Napi::TypeError::New(env, "optional arg 'offset_width' must be an integer greater than zero").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    if (options.Has("offset_height"))
    {
        Napi::Value bind_opt = options.Get("offset_height");
        if (!bind_opt.IsNumber())
        {
            Napi::TypeError::New(env, "optional arg 'offset_height' must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        offset_height = bind_opt.As<Napi::Number>().DoubleValue();
        if (offset_height <= 0.0)
        {
            Napi::TypeError::New(env, "optional arg 'offset_height' must be an integer greater than zero").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (options.Has("scaling_method"))
    {
        Napi::Value scaling_val = options.Get("scaling_method");
        if (scaling_val.IsNumber())
        {
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_val.As<Napi::Number>().Int32Value());
            if (scaling_method > mapnik::SCALING_BLACKMAN || scaling_method < 0)
            {
                Napi::TypeError::New(env, "Invalid scaling_method").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        else
        {
            Napi::TypeError::New(env, "scaling_method argument must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (options.Has("filter_factor"))
    {
        Napi::Value ff_val = options.Get("filter_factor");
        if (ff_val.IsNumber())
        {
            filter_factor = ff_val.As<Napi::Number>().DoubleValue();
        }
        else
        {
            Napi::TypeError::New(env, "filter_factor argument must be a number").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    if (image_->is<mapnik::image_null>())
    {
        Napi::TypeError::New(env, "Can not resize a null image").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (offset_width <= 0 || offset_height <= 0)
    {
        Napi::TypeError::New(env, "Image width or height is zero or less than zero.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try
    {
        double offset = image_->get_offset();
        double scaling = image_->get_scaling();

        image_ptr image_out = std::make_shared<mapnik::image_any>(width,
                                                                  height,
                                                                  image_->get_dtype(),
                                                                  true,
                                                                  true,
                                                                  false);
        image_out->set_offset(offset);
        image_out->set_scaling(scaling);
        double image_ratio_x = static_cast<double>(width) / offset_width;
        double image_ratio_y = static_cast<double>(height) / offset_height;
        double corrected_offset_x = offset_x * image_ratio_x;
        double corrected_offset_y = offset_y * image_ratio_y;
        resize_visitor visit(*image_,
                             scaling_method,
                             image_ratio_x,
                             image_ratio_y,
                             filter_factor,
                             corrected_offset_x,
                             corrected_offset_y);
        mapnik::util::apply_visitor(visit, *image_out);
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
