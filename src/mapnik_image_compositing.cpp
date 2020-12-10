#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc
#include <mapnik/image_filter_types.hpp>
#include <mapnik/image_filter.hpp> // filter_visitor
#include <mapnik/image_compositing.hpp>
#include "mapnik_image.hpp"

namespace detail {

struct AsyncComposite : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncComposite(image_ptr const& src, image_ptr const& dst,
                   mapnik::composite_mode_e mode,
                   int dx, int dy, float opacity,
                   std::vector<mapnik::filter::filter_type> const& filters,
                   Napi::Function const& callback)
        : Base(callback),
          src_(src),
          dst_(dst),
          mode_{mode},
          dx_(dx),
          dy_(dy),
          opacity_(opacity),
          filters_(filters)
    {
    }

    void Execute() override
    {
        try
        {
            if (filters_.size() > 0) // NOLINT
            {
                // TODO: expose passing custom scale_factor: https://github.com/mapnik/mapnik/commit/b830469d2d574ac575ff24713935378894f8bdc9
                mapnik::filter::filter_visitor<mapnik::image_any> visitor(*src_);
                for (mapnik::filter::filter_type const& filter_tag : filters_)
                {
                    mapnik::util::apply_visitor(visitor, filter_tag); // NOLINT
                }
                mapnik::premultiply_alpha(*src_);
            }
            mapnik::composite(*dst_, *src_, mode_, opacity_, dx_, dy_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        Napi::Value arg = Napi::External<image_ptr>::New(env, &dst_);
        Napi::Object obj = Image::constructor.New({arg});
        return {env.Undefined(), napi_value(obj)};
    }

  private:
    image_ptr src_;
    image_ptr dst_;
    mapnik::composite_mode_e mode_;
    int dx_;
    int dy_;
    float opacity_;
    std::vector<mapnik::filter::filter_type> filters_;
};

} // namespace detail

/**
 * Overlay this image with another image, creating a layered composite as
 * a new image
 *
 * @name composite
 * @param {mapnik.Image} image - image to composite with
 * @param {Object} [options]
 * @param {mapnik.compositeOp} [options.comp_op] - compositing operation. Must be an integer
 * value that relates to a compositing operation.
 * @param {number} [options.opacity] - opacity must be a floating point number between 0-1
 * @param {number} [options.dx]
 * @param {number} [options.dy]
 * @param {string} [options.image_filters] - a string of filter names
 * @param {Function} callback
 * @instance
 * @memberof Image
 * @example
 * var img1 = new mapnik.Image.open('./path/to/image.png');
 * var img2 = new mapnik.Image.open('./path/to/another-image.png');
 * img1.composite(img2, {
 *   comp_op: mapnik.compositeOp['multiply'],
 *   dx: 0,
 *   dy: 0,
 *   opacity: 0.5,
 *   image_filters: 'invert agg-stack-blur(10,10)'
 * }, function(err, result) {
 *   if (err) throw err;
 *   // new image with `result`
 * });
 */

Napi::Value Image::composite(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "requires at least one argument: an image mask").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (!info[0].IsObject())
    {
        Napi::TypeError::New(env, "first argument must be an image mask").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.InstanceOf(Image::constructor.Value()))
    {
        Napi::TypeError::New(env, "mapnik.Image expected as first arg").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    image_ptr source_image = Napi::ObjectWrap<Image>::Unwrap(obj)->image_;
    if (!image_->get_premultiplied())
    {
        Napi::TypeError::New(env, "destination image must be premultiplied").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (!source_image->get_premultiplied())
    {
        Napi::TypeError::New(env, "source image must be premultiplied").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    mapnik::composite_mode_e mode = mapnik::src_over;
    float opacity = 1.0;
    std::vector<mapnik::filter::filter_type> filters;
    int dx = 0;
    int dy = 0;
    if (info.Length() >= 2)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        if (options.Has("comp_op"))
        {
            Napi::Value opt = options.Get("comp_op");
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "comp_op must be a mapnik.compositeOp value").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            int mode_int = opt.As<Napi::Number>().Int32Value();
            if (mode_int > static_cast<int>(mapnik::composite_mode_e::divide) || mode_int < 0)
            {
                Napi::TypeError::New(env, "Invalid comp_op value").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            mode = static_cast<mapnik::composite_mode_e>(mode_int);
        }

        if (options.Has("opacity"))
        {
            Napi::Value opt = options.Get("opacity");
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "opacity must be a floating point number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            opacity = opt.As<Napi::Number>().DoubleValue();
            if (opacity < 0 || opacity > 1)
            {
                Napi::TypeError::New(env, "opacity must be a floating point number between 0-1").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }

        if (options.Has("dx"))
        {
            Napi::Value opt = options.Get("dx");
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "dx must be an integer").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            dx = opt.As<Napi::Number>().Int32Value();
        }

        if (options.Has("dy"))
        {
            Napi::Value opt = options.Get("dy");
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "dy must be an integer").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            dy = opt.As<Napi::Number>().Int32Value();
        }

        if (options.Has("image_filters"))
        {
            Napi::Value opt = options.Get("image_filters");
            if (!opt.IsString())
            {
                Napi::TypeError::New(env, "image_filters argument must string of filter names").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::string filter_str = opt.As<Napi::String>();
            bool result = mapnik::filter::parse_image_filters(filter_str, filters);
            if (!result)
            {
                Napi::TypeError::New(env, "could not parse image_filters").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
    }

    auto* worker = new detail::AsyncComposite(source_image, image_, mode, dx, dy, opacity,
                                              filters, callback_val.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}
