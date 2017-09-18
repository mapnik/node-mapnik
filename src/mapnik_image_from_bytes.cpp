// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_reader.hpp>      // for get_image_reader, etc
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc
#include <mapnik/image_copy.hpp>

#include <mapnik/image_compositing.hpp>
#include <mapnik/image_filter_types.hpp>
#include <mapnik/image_filter.hpp>      // filter_visitor
#include <mapnik/image_scaling.hpp>

#include <mapnik/marker.hpp>
#include <mapnik/marker_cache.hpp>

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_palette.hpp"
#include "mapnik_color.hpp"
#include "visitor_get_pixel.hpp"

#include "utils.hpp"

#include "agg_rasterizer_scanline_aa.h"
#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_renderer_base.h"
#include "agg_pixfmt_rgba.h"
#include "agg_scanline_u.h"

// boost
#include <boost/optional/optional.hpp>

// std
#include <exception>
#include <ostream>                      // for operator<<, basic_ostream
#include <sstream>                      // for basic_ostringstream, etc
#include <cstdlib>


typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string error_name;
    Nan::Persistent<v8::Object> buffer;
    Nan::Persistent<v8::Function> cb;
    bool premultiply;
    std::uint32_t max_size;
    const char *data;
    size_t dataLength;
    bool error;
} image_mem_ptr_baton_t;


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
NAN_METHOD(Image::fromBytes)
{
    if (info.Length() == 1) {
        info.GetReturnValue().Set(_fromBytesSync(info));
        return;
    }

    if (info.Length() < 2) {
        Nan::ThrowError("must provide a buffer argument");
        return;
    }

    if (!info[0]->IsObject()) {
        Nan::ThrowTypeError("must provide a buffer argument");
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

    bool premultiply = false;
    std::uint32_t max_size = 2048;
    if (info.Length() >= 2)
    {
        if (info[1]->IsObject())
        {
            v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[1]);
            if (options->Has(Nan::New("premultiply").ToLocalChecked()))
            {
                v8::Local<v8::Value> opt = options->Get(Nan::New("premultiply").ToLocalChecked());
                if (!opt.IsEmpty() && opt->IsBoolean())
                {
                    premultiply = opt->BooleanValue();
                }
                else
                {
                    Nan::ThrowTypeError("premultiply option must be a boolean");
                    return;
                }
            }
            if (options->Has(Nan::New("max_size").ToLocalChecked()))
            {
                v8::Local<v8::Value> opt = options->Get(Nan::New("max_size").ToLocalChecked());
                if (opt->IsNumber())
                {
                    auto max_size_val = opt->IntegerValue();
                    if (max_size_val < 0 || max_size_val > 65535) {
                        Nan::ThrowTypeError("max_size must be a positive integer between 0 and 65535");
                        return;
                    }
                    max_size = static_cast<std::uint32_t>(max_size_val);
                }
                else
                {
                    Nan::ThrowTypeError("max_size option must be a number");
                    return;
                }
            }
        }
    }

    image_mem_ptr_baton_t *closure = new image_mem_ptr_baton_t();
    closure->request.data = closure;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    closure->premultiply = premultiply;
    closure->max_size = max_size;
    closure->im = nullptr;
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromBytes, (uv_after_work_cb)EIO_AfterFromBytes);
    return;
}

void Image::EIO_FromBytes(uv_work_t* req)
{
    image_mem_ptr_baton_t *closure = static_cast<image_mem_ptr_baton_t *>(req->data);

    try
    {
        std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(closure->data,closure->dataLength));
        if (reader.get())
        {
            if (reader->width() > closure->max_size || reader->height() > closure->max_size) {
                std::stringstream s;
                s << "image created from bytes must be " << closure->max_size << " pixels or fewer on each side";
                closure->error_name = s.str();
            } else {
                closure->im = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
                if (closure->premultiply) {
                    mapnik::premultiply_alpha(*closure->im);
                }
            }
        }
    }
    catch (std::exception const& ex)
    {
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterFromBytes(uv_work_t* req)
{
    Nan::HandleScope scope;
    image_mem_ptr_baton_t *closure = static_cast<image_mem_ptr_baton_t *>(req->data);
    if (!closure->error_name.empty())
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else if (closure->im == nullptr)
    {
        /* LCOV_EXCL_START */
        // The only way this is ever reached is if the reader factory in
        // mapnik was not providing an image type it should. This should never
        // be occuring so marking this out from coverage
        v8::Local<v8::Value> argv[1] = { Nan::Error("Failed to load from buffer") };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    } else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
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
NAN_METHOD(Image::fromBytesSync)
{
    info.GetReturnValue().Set(_fromBytesSync(info));
}

v8::Local<v8::Value> Image::_fromBytesSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;

    if (info.Length() < 1 || !info[0]->IsObject()) {
        Nan::ThrowTypeError("must provide a buffer argument");
        return scope.Escape(Nan::Undefined());
    }

    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
        return scope.Escape(Nan::Undefined());
    }

    try
    {
        std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(node::Buffer::Data(obj),node::Buffer::Length(obj)));
        if (reader.get())
        {
            image_ptr imagep = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
            Image* im = new Image(imagep);
            v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
            Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
            if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
            return scope.Escape(maybe_local.ToLocalChecked());
        }
        // The only way this is ever reached is if the reader factory in
        // mapnik was not providing an image type it should. This should never
        // be occuring so marking this out from coverage
        /* LCOV_EXCL_START */
        Nan::ThrowTypeError("Failed to load from buffer");
        return scope.Escape(Nan::Undefined());
        /* LCOV_EXCL_STOP */
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}

