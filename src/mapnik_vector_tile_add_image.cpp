#include "utils.hpp"
#include "mapnik_image.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_vector_tile.hpp"

#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_processor.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_load_tile.hpp"

// mapnik
#include <mapnik/datasource_cache.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/raster.hpp>

// std
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception

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
NAN_METHOD(VectorTile::addImageSync)
{
    info.GetReturnValue().Set(_addImageSync(info));
}

v8::Local<v8::Value> VectorTile::_addImageSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowError("first argument must be an Image object");
        return scope.Escape(Nan::Undefined());
    }
    if (info.Length() < 2 || !info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return scope.Escape(Nan::Undefined());
    }
    std::string layer_name = TOSTR(info[1]);
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() ||
        obj->IsUndefined() ||
        !Nan::New(Image::constructor)->HasInstance(obj))
    {
        Nan::ThrowError("first argument must be an Image object");
        return scope.Escape(Nan::Undefined());
    }
    Image *im = Nan::ObjectWrap::Unwrap<Image>(obj);
    if (im->get()->width() <= 0 || im->get()->height() <= 0)
    {
        Nan::ThrowError("Image width and height must be greater then zero");
        return scope.Escape(Nan::Undefined());
    }

    std::string image_format = "webp";
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_BILINEAR;

    if (info.Length() > 2)
    {
        // options object
        if (!info[2]->IsObject())
        {
            Nan::ThrowError("optional third argument must be an options object");
            return scope.Escape(Nan::Undefined());
        }

        v8::Local<v8::Object> options = info[2]->ToObject();
        if (options->Has(Nan::New("image_scaling").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("image_scaling").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'image_scaling' must be a string");
                return scope.Escape(Nan::Undefined());
            }
            std::string image_scaling = TOSTR(param_val);
            boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
            if (!method)
            {
                Nan::ThrowTypeError("option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')");
                return scope.Escape(Nan::Undefined());
            }
            scaling_method = *method;
        }

        if (options->Has(Nan::New("image_format").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("image_format").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'image_format' must be a string");
                return scope.Escape(Nan::Undefined());
            }
            image_format = TOSTR(param_val);
        }
    }
    mapnik::image_any im_copy = *im->get();
    std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(mapnik::parameters());
    mapnik::raster_ptr ras = std::make_shared<mapnik::raster>(d->get_tile()->extent(), im_copy, 1.0);
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    feature->set_raster(ras);
    ds->push(feature);
    ds->envelope(); // can be removed later, currently doesn't work with out this.
    ds->set_envelope(d->get_tile()->extent());
    try
    {
        // create map object
        mapnik::Map map(d->tile_size(),d->tile_size(),"+init=epsg:3857");
        mapnik::layer lyr(layer_name,"+init=epsg:3857");
        lyr.set_datasource(ds);
        map.add_layer(lyr);

        mapnik::vector_tile_impl::processor ren(map);
        ren.set_scaling_method(scaling_method);
        ren.set_image_format(image_format);
        ren.update_tile(*d->get_tile());
        info.GetReturnValue().Set(Nan::True());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    Image* im;
    std::string layer_name;
    std::string image_format;
    mapnik::scaling_method_e scaling_method;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_add_image_baton_t;

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
NAN_METHOD(VectorTile::addImage)
{
    // If last param is not a function assume sync
    if (info.Length() < 2)
    {
        Nan::ThrowError("addImage requires at least two parameters: an Image and a layer name");
        return;
    }
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length() - 1]->IsFunction())
    {
        info.GetReturnValue().Set(_addImageSync(info));
        return;
    }
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.This());
    if (!info[0]->IsObject())
    {
        Nan::ThrowError("first argument must be an Image object");
        return;
    }
    if (!info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return;
    }
    std::string layer_name = TOSTR(info[1]);
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() ||
        obj->IsUndefined() ||
        !Nan::New(Image::constructor)->HasInstance(obj))
    {
        Nan::ThrowError("first argument must be an Image object");
        return;
    }
    Image *im = Nan::ObjectWrap::Unwrap<Image>(obj);
    if (im->get()->width() <= 0 || im->get()->height() <= 0)
    {
        Nan::ThrowError("Image width and height must be greater then zero");
        return;
    }

    std::string image_format = "webp";
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_BILINEAR;

    if (info.Length() > 3)
    {
        // options object
        if (!info[2]->IsObject())
        {
            Nan::ThrowError("optional third argument must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[2]->ToObject();
        if (options->Has(Nan::New("image_scaling").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("image_scaling").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'image_scaling' must be a string");
                return;
            }
            std::string image_scaling = TOSTR(param_val);
            boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
            if (!method)
            {
                Nan::ThrowTypeError("option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')");
                return;
            }
            scaling_method = *method;
        }

        if (options->Has(Nan::New("image_format").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("image_format").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'image_format' must be a string");
                return;
            }
            image_format = TOSTR(param_val);
        }
    }
    vector_tile_add_image_baton_t *closure = new vector_tile_add_image_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->im = im;
    closure->scaling_method = scaling_method;
    closure->image_format = image_format;
    closure->layer_name = layer_name;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_AddImage, (uv_after_work_cb)EIO_AfterAddImage);
    d->Ref();
    im->_ref();
    return;
}

void VectorTile::EIO_AddImage(uv_work_t* req)
{
    vector_tile_add_image_baton_t *closure = static_cast<vector_tile_add_image_baton_t *>(req->data);

    try
    {
        mapnik::image_any im_copy = *closure->im->get();
        std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(mapnik::parameters());
        mapnik::raster_ptr ras = std::make_shared<mapnik::raster>(closure->d->get_tile()->extent(), im_copy, 1.0);
        mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
        feature->set_raster(ras);
        ds->push(feature);
        ds->envelope(); // can be removed later, currently doesn't work with out this.
        ds->set_envelope(closure->d->get_tile()->extent());
        // create map object
        mapnik::Map map(closure->d->tile_size(),closure->d->tile_size(),"+init=epsg:3857");
        mapnik::layer lyr(closure->layer_name,"+init=epsg:3857");
        lyr.set_datasource(ds);
        map.add_layer(lyr);

        mapnik::vector_tile_impl::processor ren(map);
        ren.set_scaling_method(closure->scaling_method);
        ren.set_image_format(closure->image_format);
        ren.update_tile(*closure->d->get_tile());
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterAddImage(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_add_image_baton_t *closure = static_cast<vector_tile_add_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }

    closure->d->Unref();
    closure->im->_unref();
    closure->cb.Reset();
    delete closure;
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
NAN_METHOD(VectorTile::addImageBufferSync)
{
    info.GetReturnValue().Set(_addImageBufferSync(info));
}

v8::Local<v8::Value> VectorTile::_addImageBufferSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    if (info.Length() < 2 || !info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return scope.Escape(Nan::Undefined());
    }
    std::string layer_name = TOSTR(info[1]);
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        Nan::ThrowError("cannot accept empty buffer as protobuf");
        return scope.Escape(Nan::Undefined());
    }
    try
    {
        add_image_buffer_as_tile_layer(*d->get_tile(), layer_name, node::Buffer::Data(obj), buffer_size);
    }
    catch (std::exception const& ex)
    {
        // no obvious way to get this to throw in JS under obvious conditions
        // but keep the standard exeption cache in C++
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    const char *data;
    size_t dataLength;
    std::string layer_name;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    Nan::Persistent<v8::Object> buffer;
} vector_tile_addimagebuffer_baton_t;


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
NAN_METHOD(VectorTile::addImageBuffer)
{
    if (info.Length() < 3)
    {
        info.GetReturnValue().Set(_addImageBufferSync(info));
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length() - 1]->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return;
    }
    if (info.Length() < 2 || !info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return;
    }
    std::string layer_name = TOSTR(info[1]);
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return;
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    vector_tile_addimagebuffer_baton_t *closure = new vector_tile_addimagebuffer_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->layer_name = layer_name;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_AddImageBuffer, (uv_after_work_cb)EIO_AfterAddImageBuffer);
    d->Ref();
    return;
}

void VectorTile::EIO_AddImageBuffer(uv_work_t* req)
{
    vector_tile_addimagebuffer_baton_t *closure = static_cast<vector_tile_addimagebuffer_baton_t *>(req->data);

    try
    {
        add_image_buffer_as_tile_layer(*closure->d->get_tile(), closure->layer_name, closure->data, closure->dataLength);
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::EIO_AfterAddImageBuffer(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_addimagebuffer_baton_t *closure = static_cast<vector_tile_addimagebuffer_baton_t *>(req->data);
    if (closure->error)
    {
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}
