// mapnik
#include <mapnik/image.hpp>      // for image types
#include <mapnik/image_any.hpp>  // for image_any
#include <mapnik/image_util.hpp> // for save_to_string, guess_type, etc
#include <mapnik/marker.hpp>
#include <mapnik/marker_cache.hpp>
#include <mapnik/svg/svg_parser.hpp>
#include <mapnik/svg/svg_storage.hpp>
#include <mapnik/svg/svg_converter.hpp>
#include <mapnik/svg/svg_path_adapter.hpp>
#include <mapnik/svg/svg_path_attributes.hpp>
#include <mapnik/svg/svg_renderer_agg.hpp>
#include <mapnik/svg/svg_path_attributes.hpp>

// agg
#include "agg_pixfmt_rgba.h"
#include "agg_rasterizer_scanline_aa.h"

#include "mapnik_image.hpp"

namespace detail {
struct AsyncFromSVG : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncFromSVG(std::string const& filename, double scale, std::size_t max_size,
                 bool strict, Napi::Function const& callback)
        : Base(callback),
          filename_(filename),
          scale_(scale),
          max_size_(max_size),
          strict_(strict)
    {
    }

    void Execute() override
    {
        try
        {
            using namespace mapnik::svg;
            mapnik::svg_path_ptr marker_path(std::make_shared<mapnik::svg_storage_type>());
            vertex_stl_adapter<svg_path_storage> stl_storage(marker_path->source());
            svg_path_adapter svg_path(stl_storage); // NOLINT
            svg_converter_type svg(svg_path, marker_path->attributes());
            svg_parser p(svg);
            p.parse(filename_);
            if (strict_ && !p.err_handler().error_messages().empty())
            {
                std::ostringstream errorMessage;
                for (auto const& error : p.err_handler().error_messages())
                {
                    errorMessage << error << std::endl;
                }
                SetError(errorMessage.str());
                return;
            }

            double lox, loy, hix, hiy;
            svg.bounding_rect(&lox, &loy, &hix, &hiy);
            marker_path->set_bounding_box(lox, loy, hix, hiy);
            marker_path->set_dimensions(svg.width(), svg.height());

            using pixfmt = agg::pixfmt_rgba32_pre;
            using renderer_base = agg::renderer_base<pixfmt>;
            using renderer_solid = agg::renderer_scanline_aa_solid<renderer_base>;
            using renderer_agg = mapnik::svg::renderer_agg<mapnik::svg::svg_path_adapter,
                                                           mapnik::svg_attribute_type,
                                                           renderer_solid,
                                                           agg::pixfmt_rgba32_pre>;
            agg::rasterizer_scanline_aa<> ras_ptr;
            agg::scanline_u8 sl;

            double opacity = 1.0;

            double svg_width = svg.width() * scale_;
            double svg_height = svg.height() * scale_;

            if (svg_width <= 0 || svg_height <= 0)
            {
                SetError("image created from svg must have a width and height greater than zero");
                return;
            }

            if (svg_width > static_cast<double>(max_size_) || svg_height > static_cast<double>(max_size_))
            {
                std::stringstream s;
                s << "image created from svg must be " << max_size_ << " pixels or fewer on each side";
                SetError(s.str());
                return;
            }

            mapnik::image_rgba8 im(static_cast<int>(svg_width), static_cast<int>(svg_height), true, true);
            agg::rendering_buffer buf(im.bytes(), im.width(), im.height(), im.row_size());
            pixfmt pixf(buf);
            renderer_base renb(pixf);

            mapnik::box2d<double> const& bbox = marker_path->bounding_box();
            mapnik::coord<double, 2> c = bbox.center();
            // center the svg marker on '0,0'
            agg::trans_affine mtx = agg::trans_affine_translation(-c.x, -c.y);
            // Scale the image
            mtx.scale(scale_);
            // render the marker at the center of the marker box
            mtx.translate(0.5 * im.width(), 0.5 * im.height());
            renderer_agg svg_renderer_this(svg_path, marker_path->attributes());
            // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
            svg_renderer_this.render(ras_ptr, sl, renb, mtx, opacity, bbox);

            mapnik::demultiply_alpha(im);
            image_ = std::make_shared<mapnik::image_any>(im);
        }
        catch (std::exception const& ex)
        {
            // There is currently no known way to make these operations throw an exception, however,
            // since the underlying agg library does possibly have some operation that might throw
            // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
            // LCOV_EXCL_START
            SetError(ex.what());
            // LCOV_EXCL_STOP
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
    double scale_;
    std::size_t max_size_;
    bool strict_;
    image_ptr image_;
};

struct AsyncFromSVGBytes : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncFromSVGBytes(Napi::Buffer<char> const& buffer, double scale, std::size_t max_size,
                      bool strict, Napi::Function const& callback)
        : Base(callback),
          buffer_ref{Napi::Persistent(buffer)},
          data_{buffer.Data()},
          dataLength_{buffer.Length()},
          scale_(scale),
          max_size_(max_size),
          strict_(strict)
    {
    }

    void Execute() override
    {
        try
        {
            using namespace mapnik::svg;
            mapnik::svg_path_ptr marker_path(std::make_shared<mapnik::svg_storage_type>());
            vertex_stl_adapter<svg_path_storage> stl_storage(marker_path->source());
            svg_path_adapter svg_path(stl_storage); // NOLINT(clang-analyzer-optin.cplusplus.UninitializedObject)
            svg_converter_type svg(svg_path, marker_path->attributes());
            svg_parser p(svg);
            std::string svg_buffer(data_, dataLength_);
            p.parse_from_string(svg_buffer);
            if (strict_ && !p.err_handler().error_messages().empty())
            {
                std::ostringstream errorMessage;
                for (auto const& error : p.err_handler().error_messages())
                {
                    errorMessage << error << std::endl;
                }
                SetError(errorMessage.str());
                return;
            }

            double lox, loy, hix, hiy;
            svg.bounding_rect(&lox, &loy, &hix, &hiy);
            marker_path->set_bounding_box(lox, loy, hix, hiy);
            marker_path->set_dimensions(svg.width(), svg.height());

            using pixfmt = agg::pixfmt_rgba32_pre;
            using renderer_base = agg::renderer_base<pixfmt>;
            using renderer_solid = agg::renderer_scanline_aa_solid<renderer_base>;
            using renderer_agg = mapnik::svg::renderer_agg<mapnik::svg::svg_path_adapter,
                                                           mapnik::svg_attribute_type,
                                                           renderer_solid,
                                                           agg::pixfmt_rgba32_pre>;
            agg::rasterizer_scanline_aa<> ras_ptr;
            agg::scanline_u8 sl;

            double opacity = 1;
            double svg_width = svg.width() * scale_;
            double svg_height = svg.height() * scale_;

            if (svg_width <= 0 || svg_height <= 0)
            {
                SetError("image created from svg must have a width and height greater than zero");
                return;
            }

            if (svg_width > static_cast<double>(max_size_) || svg_height > static_cast<double>(max_size_))
            {
                std::stringstream s;
                s << "image created from svg must be " << max_size_ << " pixels or fewer on each side";
                SetError(s.str());
                return;
            }

            mapnik::image_rgba8 im(static_cast<int>(svg_width), static_cast<int>(svg_height), true, true);
            agg::rendering_buffer buf(im.bytes(), im.width(), im.height(), im.row_size());
            pixfmt pixf(buf);
            renderer_base renb(pixf);

            mapnik::box2d<double> const& bbox = marker_path->bounding_box();
            mapnik::coord<double, 2> c = bbox.center();
            // center the svg marker on '0,0'
            agg::trans_affine mtx = agg::trans_affine_translation(-c.x, -c.y);
            // Scale the image
            mtx.scale(scale_);
            // render the marker at the center of the marker box
            mtx.translate(0.5 * im.width(), 0.5 * im.height());
            renderer_agg svg_ren(svg_path, marker_path->attributes());
            svg_ren.render(ras_ptr, sl, renb, mtx, opacity, bbox); // NOLINT
            mapnik::demultiply_alpha(im);
            image_ = std::make_shared<mapnik::image_any>(im);
        }
        catch (std::exception const& ex)
        {
            // There is currently no known way to make these operations throw an exception, however,
            // since the underlying agg library does possibly have some operation that might throw
            // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
            // LCOV_EXCL_START
            SetError(ex.what());
            // LCOV_EXCL_STOP
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
    double scale_;
    std::size_t max_size_;
    bool strict_;
    image_ptr image_;
};

} // namespace detail

Napi::Value Image::from_svg_sync_impl(Napi::CallbackInfo const& info, bool from_file)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (!from_file && (info.Length() < 1 || !info[0].IsObject()))
    {
        Napi::TypeError::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (from_file && (info.Length() < 1 || !info[0].IsString()))
    {
        Napi::TypeError::New(env, "must provide a filename argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    double scale = 1.0;
    std::size_t max_size = 2048;
    bool strict = false;
    if (info.Length() >= 2)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();

            return env.Undefined();
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("scale"))
        {
            Napi::Value scale_opt = options.Get("scale");
            if (!scale_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'scale' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale = scale_opt.As<Napi::Number>().DoubleValue();
            if (scale <= 0)
            {
                Napi::TypeError::New(env, "'scale' must be a positive non zero number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("max_size"))
        {
            Napi::Value opt = options.Get("max_size");
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "'max_size' must be a positive integer").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            auto max_size_val = opt.As<Napi::Number>().Int32Value();
            if (max_size_val < 0 || max_size_val > 65535)
            {
                Napi::TypeError::New(env, "'max_size' must be a positive integer between 0 and 65535").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            max_size = static_cast<std::size_t>(max_size_val);
        }
        if (options.Has("strict"))
        {
            Napi::Value opt = options.Get("strict");
            if (!opt.IsBoolean())
            {
                Napi::TypeError::New(env, "'strict' must be a boolean value").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            strict = opt.As<Napi::Boolean>();
        }
    }

    try
    {
        using namespace mapnik::svg;
        mapnik::svg_path_ptr marker_path(std::make_shared<mapnik::svg_storage_type>());
        vertex_stl_adapter<svg_path_storage> stl_storage(marker_path->source());
        svg_path_adapter svg_path(stl_storage);
        svg_converter_type svg(svg_path, marker_path->attributes());
        svg_parser p(svg);
        if (from_file)
        {
            p.parse(info[0].As<Napi::String>());
            if (strict && !p.err_handler().error_messages().empty())
            {
                std::ostringstream errorMessage;
                for (auto const& error : p.err_handler().error_messages())
                {
                    errorMessage << error << std::endl;
                }
                Napi::TypeError::New(env, errorMessage.str().c_str()).ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        else
        {
            Napi::Object obj = info[0].As<Napi::Object>();
            if (!obj.IsBuffer())
            {
                Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::string svg_buffer(obj.As<Napi::Buffer<char>>().Data(), obj.As<Napi::Buffer<char>>().Length());
            p.parse_from_string(svg_buffer);
            if (strict && !p.err_handler().error_messages().empty())
            {
                std::ostringstream errorMessage;
                for (auto const& error : p.err_handler().error_messages())
                {
                    errorMessage << error << std::endl;
                }
                Napi::TypeError::New(env, errorMessage.str().c_str()).ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        double lox, loy, hix, hiy;
        svg.bounding_rect(&lox, &loy, &hix, &hiy);
        marker_path->set_bounding_box(lox, loy, hix, hiy);
        marker_path->set_dimensions(svg.width(), svg.height());

        using pixfmt = agg::pixfmt_rgba32_pre;
        using renderer_base = agg::renderer_base<pixfmt>;
        using renderer_solid = agg::renderer_scanline_aa_solid<renderer_base>;
        using renderer_agg = mapnik::svg::renderer_agg<mapnik::svg::svg_path_adapter,
                                                       mapnik::svg_attribute_type,
                                                       renderer_solid,
                                                       agg::pixfmt_rgba32_pre>;
        agg::rasterizer_scanline_aa<> ras_ptr;
        agg::scanline_u8 sl;

        double opacity = 1;
        double svg_width = svg.width() * scale;
        double svg_height = svg.height() * scale;

        if (svg_width <= 0 || svg_height <= 0)
        {
            Napi::TypeError::New(env, "image created from svg must have a width and height greater than zero").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        if (svg_width > static_cast<double>(max_size) || svg_height > static_cast<double>(max_size))
        {
            std::stringstream s;
            s << "image created from svg must be " << max_size << " pixels or fewer on each side";
            Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        mapnik::image_rgba8 im(static_cast<int>(std::round(svg_width)),
                               static_cast<int>(std::round(svg_height)),
                               true, true);
        agg::rendering_buffer buf(im.bytes(), im.width(), im.height(), im.row_size());
        pixfmt pixf(buf);
        renderer_base renb(pixf);
        mapnik::box2d<double> const& bbox = marker_path->bounding_box();
        mapnik::coord<double, 2> c = bbox.center();
        // center the svg marker on '0,0'
        agg::trans_affine mtx = agg::trans_affine_translation(-c.x, -c.y);
        // Scale the image
        mtx.scale(scale);
        // render the marker at the center of the marker box
        mtx.translate(0.5 * im.width(), 0.5 * im.height());
        renderer_agg svg_ren(svg_path, marker_path->attributes());
        svg_ren.render(ras_ptr, sl, renb, mtx, opacity, bbox); // NOLINT
        mapnik::demultiply_alpha(im);

        image_ptr imagep = std::make_shared<mapnik::image_any>(im);
        Napi::Value arg = Napi::External<image_ptr>::New(env, &imagep);
        Napi::Object obj = Image::constructor.New({arg});
        return scope.Escape(obj);
    }
    catch (std::exception const& ex)
    {
        // There is currently no known way to make these operations throw an exception, however,
        // since the underlying agg library does possibly have some operation that might throw
        // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
        // LCOV_EXCL_STOP
    }
}

/**
 * Load image from an SVG buffer (synchronous)
 * @name fromSVGBytesSync
 * @memberof Image
 * @static
 * @param {string} path - path to SVG image
 * @param {Object} [options]
 * @param {number} [options.strict] - Throw on unhandled elements and invalid values in SVG. The default is `false`, which means that
 * unhandled elements will be ignored and invalid values will be set to defaults (and may not render correctly). If `true` then all occurances
 * of invalid value or unhandled elements will be collected and an error will be thrown reporting them all.
 * @param {number} [options.scale] - scale the image. For example passing `0.5` as scale would render
 * your SVG at 50% the original size.
 * @param {number} [options.max_size] - the maximum allowed size of the svg dimensions * scale. The default is 2048.
 * This option can be passed a smaller or larger size in order to control the final size of the image allocated for
 * rasterizing the SVG.
 * @returns {mapnik.Image} Image object
 * @example
 * var buffer = fs.readFileSync('./path/to/image.svg');
 * var img = mapnik.Image.fromSVGBytesSync(buffer);
 */

Napi::Value Image::fromSVGBytesSync(Napi::CallbackInfo const& info)
{
    return from_svg_sync_impl(info, false);
}

/**
 * Create a new image from an SVG file (synchronous)
 *
 * @name fromSVGSync
 * @param {string} filename
 * @param {Object} [options]
 * @param {number} [options.strict] - Throw on unhandled elements and invalid values in SVG. The default is `false`, which means that
 * unhandled elements will be ignored and invalid values will be set to defaults (and may not render correctly). If `true` then all occurances
 * of invalid value or unhandled elements will be collected and an error will be thrown reporting them all.
 * @param {number} [options.scale] - scale the image. For example passing `0.5` as scale would render
 * your SVG at 50% the original size.
 * @param {number} [options.max_size] - the maximum allowed size of the svg dimensions * scale. The default is 2048.
 * This option can be passed a smaller or larger size in order to control the final size of the image allocated for
 * rasterizing the SVG.
 * @returns {mapnik.Image} image object
 * @static
 * @memberof Image
 * @example
 * var img = mapnik.Image.fromSVG('./path/to/image.svg');
 */

Napi::Value Image::fromSVGSync(Napi::CallbackInfo const& info)
{
    return from_svg_sync_impl(info, true);
}

/**
 * Create a new image from an SVG file
 *
 * @name fromSVG
 * @param {string} filename
 * @param {Object} [options]
 * @param {number} [options.strict] - Throw on unhandled elements and invalid values in SVG. The default is `false`, which means that
 * unhandled elements will be ignored and invalid values will be set to defaults (and may not render correctly). If `true` then all occurances
 * of invalid value or unhandled elements will be collected and an error will be thrown reporting them all.
 * @param {number} [options.scale] - scale the image. For example passing `0.5` as scale would render
 * your SVG at 50% the original size.
 * @param {number} [options.max_size] - the maximum allowed size of the svg dimensions * scale. The default is 2048.
 * This option can be passed a smaller or larger size in order to control the final size of the image allocated for
 * rasterizing the SVG.
 * @param {Function} callback
 * @static
 * @memberof Image
 * @example
 * mapnik.Image.fromSVG('./path/to/image.svg', {scale: 0.5}, function(err, img) {
 *   if (err) throw err;
 *   // new img object (at 50% scale)
 * });
 */

Napi::Value Image::fromSVG(Napi::CallbackInfo const& info)
{
    if (info.Length() < 2)
    {
        return fromSVGSync(info);
    }

    Napi::Env env = info.Env();
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "must provide a filename argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    double scale = 1.0;
    std::size_t max_size = 2048;
    bool strict = false;
    if (info.Length() >= 3)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("scale"))
        {
            Napi::Value scale_opt = options.Get("scale");
            if (!scale_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'scale' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale = scale_opt.As<Napi::Number>().DoubleValue();
            if (scale <= 0)
            {
                Napi::TypeError::New(env, "'scale' must be a positive non zero number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("max_size"))
        {
            Napi::Value opt = options.Get("max_size");
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "'max_size' must be a positive integer").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            auto max_size_val = opt.As<Napi::Number>().Int32Value();
            if (max_size_val < 0 || max_size_val > 65535)
            {
                Napi::TypeError::New(env, "'max_size' must be a positive integer between 0 and 65535").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            max_size = static_cast<std::size_t>(max_size_val);
        }
        if (options.Has("strict"))
        {
            Napi::Value opt = options.Get("strict");
            if (!opt.IsBoolean())
            {
                Napi::TypeError::New(env, "'strict' must be a boolean value").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            strict = opt.As<Napi::Boolean>();
        }
    }
    Napi::Function callback = callback_val.As<Napi::Function>();
    auto* worker = new detail::AsyncFromSVG(info[0].As<Napi::String>(), scale, max_size, strict, callback);
    worker->Queue();
    return env.Undefined();
}

/**
 * Load image from an SVG buffer
 * @name fromSVGBytes
 * @memberof Image
 * @static
 * @param {string} path - path to SVG image
 * @param {Object} [options]
 * @param {number} [options.strict] - Throw on unhandled elements and invalid values in SVG. The default is `false`, which means that
 * unhandled elements will be ignored and invalid values will be set to defaults (and may not render correctly). If `true` then all occurances
 * of invalid value or unhandled elements will be collected and an error will be thrown reporting them all.
 * @param {number} [options.scale] - scale the image. For example passing `0.5` as scale would render
 * your SVG at 50% the original size.
 * @param {number} [options.max_size] - the maximum allowed size of the svg dimensions * scale. The default is 2048.
 * This option can be passed a smaller or larger size in order to control the final size of the image allocated for
 * rasterizing the SVG.
 * @param {Function} callback = `function(err, img)`
 * @example
 * var buffer = fs.readFileSync('./path/to/image.svg');
 * mapnik.Image.fromSVGBytes(buffer, function(err, img) {
 *   if (err) throw err;
 *   // your custom code with `img`
 * });
 */

Napi::Value Image::fromSVGBytes(Napi::CallbackInfo const& info)
{
    if (info.Length() < 2)
    {
        return from_svg_sync_impl(info, false);
    }
    Napi::Env env = info.Env();

    if (!info[0].IsObject())
    {
        Napi::Error::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
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

    double scale = 1.0;
    std::size_t max_size = 2048;
    bool strict = false;
    if (info.Length() >= 3)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("scale"))
        {
            Napi::Value scale_opt = options.Get("scale");
            if (!scale_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'scale' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale = scale_opt.As<Napi::Number>().DoubleValue();
            if (scale <= 0)
            {
                Napi::TypeError::New(env, "'scale' must be a positive non zero number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("max_size"))
        {
            Napi::Value opt = options.Get("max_size");
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "'max_size' must be a positive integer").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            auto max_size_val = opt.As<Napi::Number>().Int32Value();
            if (max_size_val < 0 || max_size_val > 65535)
            {
                Napi::TypeError::New(env, "'max_size' must be a positive integer between 0 and 65535").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            max_size = static_cast<std::size_t>(max_size_val);
        }
        if (options.Has("strict"))
        {
            Napi::Value opt = options.Get("strict");
            if (!opt.IsBoolean())
            {
                Napi::TypeError::New(env, "'strict' must be a boolean value").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            strict = opt.As<Napi::Boolean>();
        }
    }
    Napi::Buffer<char> buffer = info[0].As<Napi::Buffer<char>>();
    Napi::Function callback = callback_val.As<Napi::Function>();
    auto* worker = new detail::AsyncFromSVGBytes(buffer, scale, max_size, strict, callback);
    worker->Queue();
    return env.Undefined();
}
