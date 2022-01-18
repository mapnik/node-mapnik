#include "mapnik_vector_tile.hpp"
#include "mapnik_image.hpp"

//mapnik
#include <mapnik/image_any.hpp>
#include <mapnik/image_scaling.hpp>
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/geometry/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/raster.hpp>
// mapnik-vector-tile
#include "vector_tile_processor.hpp"
#include "vector_tile_load_tile.hpp"

/**
 * Add a {@link Image} as a tile layer (synchronous)
 *
 * @memberof VectorTile
 * @instance
 * @name addImageSync
 * @param {mapnik.Image} image
 * @param {string} name of the layer to be added
 * @param {Object} options
 * @param {string} [options.image_scaling=bilinear] can be any
 * of the <mapnik.imageScaling> methods
 * @param {string} [options.image_format=webp] or `jpeg`, `png`, `tiff`
 * @example
 * var vt = new mapnik.VectorTile(1, 0, 0, {
 *   tile_size:256
 * });
 * var im = new mapnik.Image(256, 256);
 * vt.addImageSync(im, 'layer-name', {
 *   image_format: 'jpeg',
 *   image_scaling: 'gaussian'
 * });
 */

Napi::Value VectorTile::addImageSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::Error::New(env, "first argument must be an Image object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 2 || !info[1].IsString())
    {
        Napi::Error::New(env, "second argument must be a layer name (string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string layer_name = info[1].As<Napi::String>();
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.InstanceOf(Image::constructor.Value()))
    {
        Napi::Error::New(env, "first argument must be an Image object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Image* im = Napi::ObjectWrap<Image>::Unwrap(obj);
    if (im->impl()->width() <= 0 || im->impl()->height() <= 0)
    {
        Napi::Error::New(env, "Image width and height must be greater than zero").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string image_format = "webp";
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_BILINEAR;

    if (info.Length() > 2)
    {
        // options object
        if (!info[2].IsObject())
        {
            Napi::Error::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[2].As<Napi::Object>();
        if (options.Has("image_scaling"))
        {
            Napi::Value param_val = options.Get("image_scaling");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'image_scaling' must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::string image_scaling = param_val.As<Napi::String>();
            boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
            if (!method)
            {
                Napi::TypeError::New(env, "option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scaling_method = *method;
        }

        if (options.Has("image_format"))
        {
            Napi::Value param_val = options.Get("image_format");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'image_format' must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            image_format = param_val.As<Napi::String>();
        }
    }
    mapnik::image_any im_copy = *im->impl();
    std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(mapnik::parameters());
    mapnik::raster_ptr ras = std::make_shared<mapnik::raster>(tile_->extent(), im_copy, 1.0);
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx, 1));
    feature->set_raster(ras);
    ds->push(feature);
    ds->envelope(); // can be removed later, currently doesn't work with out this.
    ds->set_envelope(tile_->extent());
    try
    {
        // create map object
        mapnik::Map map(tile_->size(), tile_->size(), "epsg:3857");
        mapnik::layer lyr(layer_name, "epsg:3857");
        lyr.set_datasource(ds);
        map.add_layer(lyr);

        mapnik::vector_tile_impl::processor ren(map);
        ren.set_scaling_method(scaling_method);
        ren.set_image_format(image_format);
        ren.update_tile(*tile_);
        return scope.Escape(Napi::Boolean::New(env, true));
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

namespace {

struct AsyncAddImage : Napi::AsyncWorker
{
    AsyncAddImage(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                  image_ptr const& image,
                  std::string const& layer_name,
                  std::string const& image_format,
                  mapnik::scaling_method_e scaling_method,
                  Napi::Function const& callback)
        : Napi::AsyncWorker(callback),
          tile_(tile),
          image_(image),
          layer_name_(layer_name),
          image_format_(image_format),
          scaling_method_(scaling_method)
    {
    }

    void Execute() override
    {
        try
        {
            mapnik::image_any im_copy = *image_;
            std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(mapnik::parameters());
            mapnik::raster_ptr ras = std::make_shared<mapnik::raster>(tile_->extent(), im_copy, 1.0);
            mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
            mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx, 1));
            feature->set_raster(ras);
            ds->push(feature);
            ds->envelope(); // can be removed later, currently doesn't work with out this.
            ds->set_envelope(tile_->extent());
            // create map object
            mapnik::Map map(tile_->size(), tile_->size(), "epsg:3857");
            mapnik::layer lyr(layer_name_, "epsg:3857");
            lyr.set_datasource(ds);
            map.add_layer(lyr);

            mapnik::vector_tile_impl::processor ren(map);
            ren.set_scaling_method(scaling_method_);
            ren.set_image_format(image_format_);
            ren.update_tile(*tile_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    image_ptr image_;
    std::string layer_name_;
    std::string image_format_;
    mapnik::scaling_method_e scaling_method_;
};
} // namespace
/**
 * Add a <mapnik.Image> as a tile layer (asynchronous)
 *
 * @memberof VectorTile
 * @instance
 * @name addImage
 * @param {mapnik.Image} image
 * @param {string} name of the layer to be added
 * @param {Object} [options]
 * @param {string} [options.image_scaling=bilinear] can be any
 * of the <mapnik.imageScaling> methods
 * @param {string} [options.image_format=webp] other options include `jpeg`, `png`, `tiff`
 * @example
 * var vt = new mapnik.VectorTile(1, 0, 0, {
 *   tile_size:256
 * });
 * var im = new mapnik.Image(256, 256);
 * vt.addImage(im, 'layer-name', {
 *   image_format: 'jpeg',
 *   image_scaling: 'gaussian'
 * }, function(err) {
 *   if (err) throw err;
 *   // your custom code using `vt`
 * });
 */
Napi::Value VectorTile::addImage(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    // If last param is not a function assume sync
    if (info.Length() < 2)
    {
        Napi::Error::New(env, "addImage requires at least two parameters: an Image and a layer name").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Value callback = info[info.Length() - 1];
    if (!callback.IsFunction())
    {
        return addImageSync(info);
    }

    if (!info[0].IsObject())
    {
        Napi::Error::New(env, "first argument must be an Image object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!info[1].IsString())
    {
        Napi::Error::New(env, "second argument must be a layer name (string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string layer_name = info[1].As<Napi::String>();
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.InstanceOf(Image::constructor.Value()))
    {
        Napi::Error::New(env, "first argument must be an Image object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Image* im = Napi::ObjectWrap<Image>::Unwrap(obj);
    if (im->impl()->width() <= 0 || im->impl()->height() <= 0)
    {
        Napi::Error::New(env, "Image width and height must be greater than zero").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string image_format = "webp";
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_BILINEAR;

    if (info.Length() > 3)
    {
        // options object
        if (!info[2].IsObject())
        {
            Napi::Error::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[2].As<Napi::Object>();
        if (options.Has("image_scaling"))
        {
            Napi::Value param_val = options.Get("image_scaling");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'image_scaling' must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::string image_scaling = param_val.As<Napi::String>();
            boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
            if (!method)
            {
                Napi::TypeError::New(env, "option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scaling_method = *method;
        }

        if (options.Has("image_format"))
        {
            Napi::Value param_val = options.Get("image_format");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'image_format' must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            image_format = param_val.As<Napi::String>();
        }
    }
    auto* worker = new AsyncAddImage{tile_, im->impl(), layer_name, image_format,
                                     scaling_method, callback.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}

/**
 * Add raw image buffer as a new tile layer (synchronous)
 *
 * @memberof VectorTile
 * @instance
 * @name addImageBufferSync
 * @param {Buffer} buffer - raw data
 * @param {string} name - name of the layer to be added
 * @example
 * var vt = new mapnik.VectorTile(1, 0, 0, {
 *   tile_size: 256
 * });
 * var image_buffer = fs.readFileSync('./path/to/image.jpg');
 * vt.addImageBufferSync(image_buffer, 'layer-name');
 */

Napi::Value VectorTile::addImageBufferSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "first argument must be a buffer object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 2 || !info[1].IsString())
    {
        Napi::Error::New(env, "second argument must be a layer name (string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string layer_name = info[1].As<Napi::String>();
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.IsBuffer())
    {
        Napi::TypeError::New(env, "first arg must be a buffer object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::size_t buffer_size = obj.As<Napi::Buffer<char>>().Length();
    if (buffer_size <= 0)
    {
        Napi::Error::New(env, "cannot accept empty buffer as protobuf").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try
    {
        add_image_buffer_as_tile_layer(*tile_, layer_name, obj.As<Napi::Buffer<char>>().Data(), buffer_size);
    }
    catch (std::exception const& ex)
    {
        // no obvious way to get this to throw in JS under obvious conditions
        // but keep the standard exeption cache in C++
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
        // LCOV_EXCL_STOP
    }
    return env.Undefined();
}

/*
typedef struct
{
    uv_work_t request;
    VectorTile* d;
    const char *data;
    size_t dataLength;
    std::string layer_name;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
    Napi::Persistent<v8::Object> buffer;
} vector_tile_addimagebuffer_baton_t;
*/

namespace {

struct AsyncAddImageBuffer : Napi::AsyncWorker
{
    AsyncAddImageBuffer(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                        Napi::Buffer<char> const& buffer,
                        std::string const& layer_name,
                        Napi::Function const& callback)
        : Napi::AsyncWorker(callback),
          tile_(tile),
          buffer_ref{Napi::Persistent(buffer)},
          data_{buffer.Data()},
          dataLength_{buffer.Length()},
          layer_name_(layer_name)
    {
    }

    void Execute() override
    {
        try
        {
            add_image_buffer_as_tile_layer(*tile_, layer_name_, data_, dataLength_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    Napi::Reference<Napi::Buffer<char>> buffer_ref;
    char const* data_;
    std::size_t dataLength_;
    std::string layer_name_;
};

} // namespace

/**
 * Add an encoded image buffer as a layer
 *
 * @memberof VectorTile
 * @instance
 * @name addImageBuffer
 * @param {Buffer} buffer - raw image data
 * @param {string} name - name of the layer to be added
 * @param {Function} callback
 * @example
 * var vt = new mapnik.VectorTile(1, 0, 0, {
 *   tile_size: 256
 * });
 * var image_buffer = fs.readFileSync('./path/to/image.jpg'); // returns a buffer
 * vt.addImageBufferSync(image_buffer, 'layer-name', function(err) {
 *   if (err) throw err;
 *   // your custom code
 * });
 */
Napi::Value VectorTile::addImageBuffer(Napi::CallbackInfo const& info)
{
    if (info.Length() < 3)
    {
        return addImageBufferSync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!callback.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "first argument must be a buffer object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (info.Length() < 2 || !info[1].IsString())
    {
        Napi::Error::New(env, "second argument must be a layer name (string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string layer_name = info[1].As<Napi::String>();
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.IsBuffer())
    {
        Napi::TypeError::New(env, "first arg must be a buffer object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto* worker = new AsyncAddImageBuffer{tile_, obj.As<Napi::Buffer<char>>(), layer_name, callback.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}
