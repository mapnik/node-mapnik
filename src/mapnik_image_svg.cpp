// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include <mapnik/marker.hpp>
#include <mapnik/marker_cache.hpp>
#include <mapnik/svg/svg_parser.hpp>
#include <mapnik/svg/svg_storage.hpp>
#include <mapnik/svg/svg_converter.hpp>
#include <mapnik/svg/svg_path_adapter.hpp>
#include <mapnik/svg/svg_path_attributes.hpp>
#include <mapnik/svg/svg_path_adapter.hpp>
#include <mapnik/svg/svg_renderer_agg.hpp>
#include <mapnik/svg/svg_path_attributes.hpp>

#include "mapnik_image.hpp"
#include "utils.hpp"

#include "agg_rasterizer_scanline_aa.h"
#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_renderer_base.h"
#include "agg_pixfmt_rgba.h"
#include "agg_scanline_u.h"

// std
#include <exception>

/**
 * Load image from an SVG buffer (synchronous)
 * @name fromSVGBytesSync
 * @memberof Image
 * @static
 * @param {string} path - path to SVG image
 * @param {Object} [options]
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
NAN_METHOD(Image::fromSVGBytesSync)
{
    info.GetReturnValue().Set(_fromSVGSync(false, info));
}

/**
 * Create a new image from an SVG file (synchronous)
 *
 * @name fromSVGSync
 * @param {string} filename
 * @param {Object} [options]
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
NAN_METHOD(Image::fromSVGSync)
{
    info.GetReturnValue().Set(_fromSVGSync(true, info));
}

v8::Local<v8::Value> Image::_fromSVGSync(bool fromFile, Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;

    if (!fromFile && (info.Length() < 1 || !info[0]->IsObject()))
    {
        Nan::ThrowTypeError("must provide a buffer argument");
        return scope.Escape(Nan::Undefined());
    }

    if (fromFile && (info.Length() < 1 || !info[0]->IsString()))
    {
        Nan::ThrowTypeError("must provide a filename argument");
        return scope.Escape(Nan::Undefined());
    }


    double scale = 1.0;
    std::uint32_t max_size = 2048;
    if (info.Length() >= 2)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("scale").ToLocalChecked()))
        {
            v8::Local<v8::Value> scale_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!scale_opt->IsNumber())
            {
                Nan::ThrowTypeError("'scale' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            scale = scale_opt->NumberValue();
            if (scale <= 0)
            {
                Nan::ThrowTypeError("'scale' must be a positive non zero number");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (options->Has(Nan::New("max_size").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("max_size").ToLocalChecked());
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("max_size must be a positive integer");
                return scope.Escape(Nan::Undefined());
            }
            auto max_size_val = opt->IntegerValue();
            if (max_size_val < 0 || max_size_val > 65535) {
                Nan::ThrowTypeError("max_size must be a positive integer between 0 and 65535");
                return scope.Escape(Nan::Undefined());
            }
            max_size = static_cast<std::uint32_t>(max_size_val);
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
        if (fromFile)
        {
            if (!p.parse(TOSTR(info[0])))
            {
                std::ostringstream errorMessage("");
                errorMessage << "SVG parse error:" << std::endl;
                for (auto const& error : p.err_handler().error_messages()) {
                    errorMessage <<  error << std::endl;
                }
                Nan::ThrowTypeError(errorMessage.str().c_str());
                return scope.Escape(Nan::Undefined());
            }
        }
        else
        {
            v8::Local<v8::Object> obj = info[0]->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
            {
                Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
                return scope.Escape(Nan::Undefined());
            }
            std::string svg_buffer(node::Buffer::Data(obj),node::Buffer::Length(obj));
            if (!p.parse_from_string(svg_buffer))
            {
                std::ostringstream errorMessage("");
                errorMessage << "SVG parse error:" << std::endl;
                for (auto const& error : p.err_handler().error_messages()) {
                    errorMessage <<  error << std::endl;
                }
                Nan::ThrowTypeError(errorMessage.str().c_str());
                return scope.Escape(Nan::Undefined());
            }
        }

        double lox,loy,hix,hiy;
        svg.bounding_rect(&lox, &loy, &hix, &hiy);
        marker_path->set_bounding_box(lox,loy,hix,hiy);
        marker_path->set_dimensions(svg.width(),svg.height());

        using pixfmt = agg::pixfmt_rgba32_pre;
        using renderer_base = agg::renderer_base<pixfmt>;
        using renderer_solid = agg::renderer_scanline_aa_solid<renderer_base>;
        agg::rasterizer_scanline_aa<> ras_ptr;
        agg::scanline_u8 sl;

        double opacity = 1;
        double svg_width = svg.width() * scale;
        double svg_height = svg.height() * scale;

        if (svg_width <= 0 || svg_height <= 0)
        {
            Nan::ThrowTypeError("image created from svg must have a width and height greater then zero");
            return scope.Escape(Nan::Undefined());
        }

        if (svg_width > static_cast<double>(max_size) || svg_height > static_cast<double>(max_size))
        {
            std::stringstream s;
            s << "image created from svg must be " << max_size << " pixels or fewer on each side";
            Nan::ThrowTypeError(s.str().c_str());
            return scope.Escape(Nan::Undefined());
        }

        mapnik::image_rgba8 im(static_cast<int>(svg_width), static_cast<int>(svg_height), true, true);
        agg::rendering_buffer buf(im.bytes(), im.width(), im.height(), im.row_size());
        pixfmt pixf(buf);
        renderer_base renb(pixf);

        mapnik::box2d<double> const& bbox = marker_path->bounding_box();
        mapnik::coord<double,2> c = bbox.center();
        // center the svg marker on '0,0'
        agg::trans_affine mtx = agg::trans_affine_translation(-c.x,-c.y);
        // Scale the image
        mtx.scale(scale);
        // render the marker at the center of the marker box
        mtx.translate(0.5 * im.width(), 0.5 * im.height());

        mapnik::svg::svg_renderer_agg<mapnik::svg::svg_path_adapter,
            agg::pod_bvector<mapnik::svg::path_attributes>,
            renderer_solid,
            agg::pixfmt_rgba32_pre > svg_renderer_this(svg_path,
                                                       marker_path->attributes());

        svg_renderer_this.render(ras_ptr, sl, renb, mtx, opacity, bbox);
        mapnik::demultiply_alpha(im);

        image_ptr imagep = std::make_shared<mapnik::image_any>(im);
        Image *im2 = new Image(imagep);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im2);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        return scope.Escape(maybe_local.ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        // There is currently no known way to make these operations throw an exception, however,
        // since the underlying agg library does possibly have some operation that might throw
        // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
}

typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string filename;
    bool error;
    double scale;
    std::uint32_t max_size;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} svg_file_ptr_baton_t;

typedef struct {
    uv_work_t request;
    image_ptr im;
    const char *data;
    size_t dataLength;
    bool error;
    double scale;
    std::uint32_t max_size;
    std::string error_name;
    Nan::Persistent<v8::Object> buffer;
    Nan::Persistent<v8::Function> cb;
} svg_mem_ptr_baton_t;

/**
 * Create a new image from an SVG file
 *
 * @name fromSVG
 * @param {string} filename
 * @param {Object} [options]
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
NAN_METHOD(Image::fromSVG)
{
    if (info.Length() == 1) {
        info.GetReturnValue().Set(_fromSVGSync(true, info));
        return;
    }

    if (info.Length() < 2 || !info[0]->IsString())
    {
        Nan::ThrowTypeError("must provide a filename argument");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    double scale = 1.0;
    std::uint32_t max_size = 2048;
    if (info.Length() >= 3)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("scale").ToLocalChecked()))
        {
            v8::Local<v8::Value> scale_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!scale_opt->IsNumber())
            {
                Nan::ThrowTypeError("'scale' must be a number");
                return;
            }
            scale = scale_opt->NumberValue();
            if (scale <= 0)
            {
                Nan::ThrowTypeError("'scale' must be a positive non zero number");
                return;
            }
        }
        if (options->Has(Nan::New("max_size").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("max_size").ToLocalChecked());
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("max_size must be a positive integer");
                return;
            }
            auto max_size_val = opt->IntegerValue();
            if (max_size_val < 0 || max_size_val > 65535) {
                Nan::ThrowTypeError("max_size must be a positive integer between 0 and 65535");
                return;
            }
            max_size = static_cast<std::uint32_t>(max_size_val);
        }
    }

    svg_file_ptr_baton_t *closure = new svg_file_ptr_baton_t();
    closure->request.data = closure;
    closure->filename = TOSTR(info[0]);
    closure->error = false;
    closure->scale = scale;
    closure->max_size = max_size;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromSVG, (uv_after_work_cb)EIO_AfterFromSVG);
    return;
}

void Image::EIO_FromSVG(uv_work_t* req)
{
    svg_file_ptr_baton_t *closure = static_cast<svg_file_ptr_baton_t *>(req->data);

    try
    {
        using namespace mapnik::svg;
        mapnik::svg_path_ptr marker_path(std::make_shared<mapnik::svg_storage_type>());
        vertex_stl_adapter<svg_path_storage> stl_storage(marker_path->source());
        svg_path_adapter svg_path(stl_storage);
        svg_converter_type svg(svg_path, marker_path->attributes());
        svg_parser p(svg);
        if (!p.parse(closure->filename))
        {
            std::ostringstream errorMessage("");
            errorMessage << "SVG parse error:" << std::endl;
            for (auto const& error : p.err_handler().error_messages()) {
                errorMessage <<  error << std::endl;
            }
            closure->error = true;
            closure->error_name = errorMessage.str();
            return;
        }

        double lox,loy,hix,hiy;
        svg.bounding_rect(&lox, &loy, &hix, &hiy);
        marker_path->set_bounding_box(lox,loy,hix,hiy);
        marker_path->set_dimensions(svg.width(),svg.height());

        using pixfmt = agg::pixfmt_rgba32_pre;
        using renderer_base = agg::renderer_base<pixfmt>;
        using renderer_solid = agg::renderer_scanline_aa_solid<renderer_base>;
        agg::rasterizer_scanline_aa<> ras_ptr;
        agg::scanline_u8 sl;

        double opacity = 1;

        double svg_width = svg.width() * closure->scale;
        double svg_height = svg.height() * closure->scale;

        if (svg_width <= 0 || svg_height <= 0)
        {
            closure->error = true;
            closure->error_name = "image created from svg must have a width and height greater then zero";
            return;
        }

        if (svg_width > static_cast<double>(closure->max_size) || svg_height > static_cast<double>(closure->max_size))
        {
            closure->error = true;
            std::stringstream s;
            s << "image created from svg must be " << closure->max_size << " pixels or fewer on each side";
            closure->error_name = s.str();
            return;
        }

        mapnik::image_rgba8 im(static_cast<int>(svg_width), static_cast<int>(svg_height), true, true);
        agg::rendering_buffer buf(im.bytes(), im.width(), im.height(), im.row_size());
        pixfmt pixf(buf);
        renderer_base renb(pixf);

        mapnik::box2d<double> const& bbox = marker_path->bounding_box();
        mapnik::coord<double,2> c = bbox.center();
        // center the svg marker on '0,0'
        agg::trans_affine mtx = agg::trans_affine_translation(-c.x,-c.y);
        // Scale the image
        mtx.scale(closure->scale);
        // render the marker at the center of the marker box
        mtx.translate(0.5 * im.width(), 0.5 * im.height());

        mapnik::svg::svg_renderer_agg<mapnik::svg::svg_path_adapter,
            agg::pod_bvector<mapnik::svg::path_attributes>,
            renderer_solid,
            agg::pixfmt_rgba32_pre > svg_renderer_this(svg_path,
                                                       marker_path->attributes());

        svg_renderer_this.render(ras_ptr, sl, renb, mtx, opacity, bbox);
        mapnik::demultiply_alpha(im);
        closure->im = std::make_shared<mapnik::image_any>(im);
    }
    catch (std::exception const& ex)
    {
        // There is currently no known way to make these operations throw an exception, however,
        // since the underlying agg library does possibly have some operation that might throw
        // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = "Failed to load: " + closure->filename;
        // LCOV_EXCL_STOP
    }
}

void Image::EIO_AfterFromSVG(uv_work_t* req)
{
    Nan::HandleScope scope;
    svg_file_ptr_baton_t *closure = static_cast<svg_file_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->cb.Reset();
    delete closure;
}

/**
 * Load image from an SVG buffer
 * @name fromSVGBytes
 * @memberof Image
 * @static
 * @param {string} path - path to SVG image
 * @param {Object} [options]
 * @param {number} [options.scale] - scale the image. For example passing `0.5` as scale would render
 * your SVG at 50% the original size.
 * @param {number} [options.max_size] - the maximum allowed size of the svg dimensions * scale. The default is 2048.
 * This option can be passed a smaller or larger size in order to control the final size of the image allocated for
 * rasterizing the SVG.
 * @param {Function} callback = `function(err, img)`
 * @example
 * var buffer = fs.readFileSync('./path/to/image.svg');
 * mapnik.Image.fromSVGBytesSync(buffer, function(err, img) {
 *   if (err) throw err;
 *   // your custom code with `img`
 * });
 */
NAN_METHOD(Image::fromSVGBytes)
{
    if (info.Length() == 1) {
        info.GetReturnValue().Set(_fromSVGSync(false, info));
        return;
    }

    if (info.Length() < 2 || !info[0]->IsObject()) {
        Nan::ThrowError("must provide a buffer argument");
        return;
    }

    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    double scale = 1.0;
    std::uint32_t max_size = 2048;
    if (info.Length() >= 3)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("scale").ToLocalChecked()))
        {
            v8::Local<v8::Value> scale_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!scale_opt->IsNumber())
            {
                Nan::ThrowTypeError("'scale' must be a number");
                return;
            }
            scale = scale_opt->NumberValue();
            if (scale <= 0)
            {
                Nan::ThrowTypeError("'scale' must be a positive non zero number");
                return;
            }
        }
        if (options->Has(Nan::New("max_size").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("max_size").ToLocalChecked());
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("max_size must be a positive integer");
                return;
            }
            auto max_size_val = opt->IntegerValue();
            if (max_size_val < 0 || max_size_val > 65535) {
                Nan::ThrowTypeError("max_size must be a positive integer between 0 and 65535");
                return;
            }
            max_size = static_cast<std::uint32_t>(max_size_val);
        }
    }

    svg_mem_ptr_baton_t *closure = new svg_mem_ptr_baton_t();
    closure->request.data = closure;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->scale = scale;
    closure->max_size = max_size;
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromSVGBytes, (uv_after_work_cb)EIO_AfterFromSVGBytes);
    return;
}

void Image::EIO_FromSVGBytes(uv_work_t* req)
{
    svg_mem_ptr_baton_t *closure = static_cast<svg_mem_ptr_baton_t *>(req->data);

    try
    {
        using namespace mapnik::svg;
        mapnik::svg_path_ptr marker_path(std::make_shared<mapnik::svg_storage_type>());
        vertex_stl_adapter<svg_path_storage> stl_storage(marker_path->source());
        svg_path_adapter svg_path(stl_storage);
        svg_converter_type svg(svg_path, marker_path->attributes());
        svg_parser p(svg);

        std::string svg_buffer(closure->data,closure->dataLength);
        if (!p.parse_from_string(svg_buffer))
        {
            std::ostringstream errorMessage("");
            errorMessage << "SVG parse error:" << std::endl;
            for (auto const& error : p.err_handler().error_messages()) {
                errorMessage <<  error << std::endl;
            }
            closure->error = true;
            closure->error_name = errorMessage.str();
            return;
        }

        double lox,loy,hix,hiy;
        svg.bounding_rect(&lox, &loy, &hix, &hiy);
        marker_path->set_bounding_box(lox,loy,hix,hiy);
        marker_path->set_dimensions(svg.width(),svg.height());

        using pixfmt = agg::pixfmt_rgba32_pre;
        using renderer_base = agg::renderer_base<pixfmt>;
        using renderer_solid = agg::renderer_scanline_aa_solid<renderer_base>;
        agg::rasterizer_scanline_aa<> ras_ptr;
        agg::scanline_u8 sl;

        double opacity = 1;
        double svg_width = svg.width() * closure->scale;
        double svg_height = svg.height() * closure->scale;

        if (svg_width <= 0 || svg_height <= 0)
        {
            closure->error = true;
            closure->error_name = "image created from svg must have a width and height greater then zero";
            return;
        }

        if (svg_width > static_cast<double>(closure->max_size) || svg_height > static_cast<double>(closure->max_size))
        {
            closure->error = true;
            std::stringstream s;
            s << "image created from svg must be " << closure->max_size << " pixels or fewer on each side";
            closure->error_name = s.str();
            return;
        }

        mapnik::image_rgba8 im(static_cast<int>(svg_width), static_cast<int>(svg_height), true, true);
        agg::rendering_buffer buf(im.bytes(), im.width(), im.height(), im.row_size());
        pixfmt pixf(buf);
        renderer_base renb(pixf);

        mapnik::box2d<double> const& bbox = marker_path->bounding_box();
        mapnik::coord<double,2> c = bbox.center();
        // center the svg marker on '0,0'
        agg::trans_affine mtx = agg::trans_affine_translation(-c.x,-c.y);
        // Scale the image
        mtx.scale(closure->scale);
        // render the marker at the center of the marker box
        mtx.translate(0.5 * im.width(), 0.5 * im.height());

        mapnik::svg::svg_renderer_agg<mapnik::svg::svg_path_adapter,
            agg::pod_bvector<mapnik::svg::path_attributes>,
            renderer_solid,
            agg::pixfmt_rgba32_pre > svg_renderer_this(svg_path,
                                                       marker_path->attributes());

        svg_renderer_this.render(ras_ptr, sl, renb, mtx, opacity, bbox);
        mapnik::demultiply_alpha(im);
        closure->im = std::make_shared<mapnik::image_any>(im);
    }
    catch (std::exception const& ex)
    {
        // There is currently no known way to make these operations throw an exception, however,
        // since the underlying agg library does possibly have some operation that might throw
        // it is a good idea to keep this. Therefore, any exceptions thrown will fail gracefully.
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = ex.what();
        // LCOV_EXCL_STOP
    }
}

void Image::EIO_AfterFromSVGBytes(uv_work_t* req)
{
    Nan::HandleScope scope;
    svg_mem_ptr_baton_t *closure = static_cast<svg_mem_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        v8::Local<v8::Object> image_obj = Nan::New(constructor)->GetFunction()->NewInstance(1, &ext);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), image_obj };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

