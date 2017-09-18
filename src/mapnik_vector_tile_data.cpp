#include "utils.hpp"
#include "mapnik_vector_tile.hpp"

#include "vector_tile_compression.hpp"
#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_load_tile.hpp"

// std
#include <sstream>                      // for operator<<, basic_ostream, etc
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

/**
 * Add raw data to this tile as a Buffer
 *
 * @memberof VectorTile
 * @instance
 * @name addDataSync
 * @param {Buffer} buffer - raw data
 * @param {object} [options]
 * @param {boolean} [options.validate=false] - If true does validity checks mvt schema (not geometries)
 * Will throw if anything invalid or unexpected is encountered in the data
 * @param {boolean} [options.upgrade=false] - If true will upgrade v1 tiles to adhere to the v2 specification
 * @example
 * var data_buffer = fs.readFileSync('./path/to/data.mvt'); // returns a buffer
 * // assumes you have created a vector tile object already
 * vt.addDataSync(data_buffer);
 * // your custom code
 */
NAN_METHOD(VectorTile::addDataSync)
{
    info.GetReturnValue().Set(_addDataSync(info));
}

v8::Local<v8::Value> VectorTile::_addDataSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
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
    bool upgrade = false;
    bool validate = false;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 1)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("second arg must be a options object");
            return scope.Escape(Nan::Undefined());
        }
        options = info[1]->ToObject();
        if (options->Has(Nan::New<v8::String>("validate").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("validate").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'validate' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            validate = param_val->BooleanValue();
        }
        if (options->Has(Nan::New<v8::String>("upgrade").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("upgrade").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'upgrade' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            upgrade = param_val->BooleanValue();
        }
    }
    try
    {
        merge_from_compressed_buffer(*d->get_tile(), node::Buffer::Data(obj), buffer_size, validate, upgrade);
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
    const char *data;
    bool validate;
    bool upgrade;
    size_t dataLength;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    Nan::Persistent<v8::Object> buffer;
} vector_tile_adddata_baton_t;


/**
 * Add new vector tile data to an existing vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name addData
 * @param {Buffer} buffer - raw vector data
 * @param {object} [options]
 * @param {boolean} [options.validate=false] - If true does validity checks mvt schema (not geometries)
 * Will throw if anything invalid or unexpected is encountered in the data
 * @param {boolean} [options.upgrade=false] - If true will upgrade v1 tiles to adhere to the v2 specification
 * @param {Object} callback
 * @example
 * var data_buffer = fs.readFileSync('./path/to/data.mvt'); // returns a buffer
 * var vt = new mapnik.VectorTile(9,112,195);
 * vt.addData(data_buffer, function(err) {
 *   if (err) throw err;
 *   // your custom code
 * });
 */
NAN_METHOD(VectorTile::addData)
{
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length() - 1]->IsFunction())
    {
        info.GetReturnValue().Set(_addDataSync(info));
        return;
    }

    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return;
    }

    bool upgrade = false;
    bool validate = false;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 1)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("second arg must be a options object");
            return;
        }
        options = info[1]->ToObject();
        if (options->Has(Nan::New<v8::String>("validate").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("validate").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'validate' must be a boolean");
                return;
            }
            validate = param_val->BooleanValue();
        }
        if (options->Has(Nan::New<v8::String>("upgrade").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("upgrade").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'upgrade' must be a boolean");
                return;
            }
            upgrade = param_val->BooleanValue();
        }
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    vector_tile_adddata_baton_t *closure = new vector_tile_adddata_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->validate = validate;
    closure->upgrade = upgrade;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_AddData, (uv_after_work_cb)EIO_AfterAddData);
    d->Ref();
    return;
}

void VectorTile::EIO_AddData(uv_work_t* req)
{
    vector_tile_adddata_baton_t *closure = static_cast<vector_tile_adddata_baton_t *>(req->data);

    if (closure->dataLength <= 0)
    {
        closure->error = true;
        closure->error_name = "cannot accept empty buffer as protobuf";
        return;
    }
    try
    {
        merge_from_compressed_buffer(*closure->d->get_tile(), closure->data, closure->dataLength, closure->validate, closure->upgrade);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterAddData(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_adddata_baton_t *closure = static_cast<vector_tile_adddata_baton_t *>(req->data);
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
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Replace the data in this vector tile with new raw data (synchronous). This function validates
 * geometry according to the [Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec).
 *
 * @memberof VectorTile
 * @instance
 * @name setDataSync
 * @param {Buffer} buffer - raw data
 * @param {object} [options]
 * @param {boolean} [options.validate=false] - If true does validity checks mvt schema (not geometries)
 * Will throw if anything invalid or unexpected is encountered in the data
 * @param {boolean} [options.upgrade=false] - If true will upgrade v1 tiles to adhere to the v2 specification
 * @example
 * var data = fs.readFileSync('./path/to/data.mvt');
 * vectorTile.setDataSync(data);
 * // your custom code
 */
NAN_METHOD(VectorTile::setDataSync)
{
    info.GetReturnValue().Set(_setDataSync(info));
}

v8::Local<v8::Value> VectorTile::_setDataSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
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
    bool upgrade = false;
    bool validate = false;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 1)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("second arg must be a options object");
            return scope.Escape(Nan::Undefined());
        }
        options = info[1]->ToObject();
        if (options->Has(Nan::New<v8::String>("validate").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("validate").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'validate' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            validate = param_val->BooleanValue();
        }
        if (options->Has(Nan::New<v8::String>("upgrade").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("upgrade").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'upgrade' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            upgrade = param_val->BooleanValue();
        }
    }
    try
    {
        d->clear();
        merge_from_compressed_buffer(*d->get_tile(), node::Buffer::Data(obj), buffer_size, validate, upgrade);
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
    const char *data;
    bool validate;
    bool upgrade;
    size_t dataLength;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    Nan::Persistent<v8::Object> buffer;
} vector_tile_setdata_baton_t;


/**
 * Replace the data in this vector tile with new raw data
 *
 * @memberof VectorTile
 * @instance
 * @name setData
 * @param {Buffer} buffer - raw data
 * @param {object} [options]
 * @param {boolean} [options.validate=false] - If true does validity checks mvt schema (not geometries)
 * Will throw if anything invalid or unexpected is encountered in the data
 * @param {boolean} [options.upgrade=false] - If true will upgrade v1 tiles to adhere to the v2 specification
 * @param {Function} callback
 * @example
 * var data = fs.readFileSync('./path/to/data.mvt');
 * vectorTile.setData(data, function(err) {
 *   if (err) throw err;
 *   // your custom code
 * });
 */
NAN_METHOD(VectorTile::setData)
{
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length() - 1]->IsFunction())
    {
        info.GetReturnValue().Set(_setDataSync(info));
        return;
    }

    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return;
    }

    bool upgrade = false;
    bool validate = false;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 1)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("second arg must be a options object");
            return;
        }
        options = info[1]->ToObject();
        if (options->Has(Nan::New<v8::String>("validate").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("validate").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'validate' must be a boolean");
                return;
            }
            validate = param_val->BooleanValue();
        }
        if (options->Has(Nan::New<v8::String>("upgrade").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("upgrade").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'upgrade' must be a boolean");
                return;
            }
            upgrade = param_val->BooleanValue();
        }
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    vector_tile_setdata_baton_t *closure = new vector_tile_setdata_baton_t();
    closure->request.data = closure;
    closure->validate = validate;
    closure->upgrade = upgrade;
    closure->d = d;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_SetData, (uv_after_work_cb)EIO_AfterSetData);
    d->Ref();
    return;
}

void VectorTile::EIO_SetData(uv_work_t* req)
{
    vector_tile_setdata_baton_t *closure = static_cast<vector_tile_setdata_baton_t *>(req->data);

    if (closure->dataLength <= 0)
    {
        closure->error = true;
        closure->error_name = "cannot accept empty buffer as protobuf";
        return;
    }

    try
    {
        closure->d->clear();
        merge_from_compressed_buffer(*closure->d->get_tile(), closure->data, closure->dataLength, closure->validate, closure->upgrade);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterSetData(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_setdata_baton_t *closure = static_cast<vector_tile_setdata_baton_t *>(req->data);
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
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Get the data in this vector tile as a buffer (synchronous)
 *
 * @memberof VectorTile
 * @instance
 * @name getDataSync
 * @param {Object} [options]
 * @param {string} [options.compression=none] - can also be `gzip`
 * @param {int} [options.level=0] a number `0` (no compression) to `9` (best compression)
 * @param {string} options.strategy must be `FILTERED`, `HUFFMAN_ONLY`, `RLE`, `FIXED`, `DEFAULT`
 * @returns {Buffer} raw data
 * @example
 * var data = vt.getData({
 *   compression: 'gzip',
 *   level: 9,
 *   strategy: 'FILTERED'
 * });
 */
NAN_METHOD(VectorTile::getDataSync)
{
    info.GetReturnValue().Set(_getDataSync(info));
}

v8::Local<v8::Value> VectorTile::_getDataSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    bool compress = false;
    int level = Z_DEFAULT_COMPRESSION;
    int strategy = Z_DEFAULT_STRATEGY;

    v8::Local<v8::Object> options = Nan::New<v8::Object>();

    if (info.Length() > 0)
    {
        if (!info[0]->IsObject())
        {
            Nan::ThrowTypeError("first arg must be a options object");
            return scope.Escape(Nan::Undefined());
        }

        options = info[0]->ToObject();

        if (options->Has(Nan::New<v8::String>("compression").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("compression").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'compression' must be a string, either 'gzip', or 'none' (default)");
                return scope.Escape(Nan::Undefined());
            }
            compress = std::string("gzip") == (TOSTR(param_val->ToString()));
        }

        if (options->Has(Nan::New<v8::String>("level").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("level").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return scope.Escape(Nan::Undefined());
            }
            level = param_val->IntegerValue();
            if (level < 0 || level > 9)
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (options->Has(Nan::New<v8::String>("strategy").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("strategy").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return scope.Escape(Nan::Undefined());
            }
            else if (std::string("FILTERED") == TOSTR(param_val->ToString()))
            {
                strategy = Z_FILTERED;
            }
            else if (std::string("HUFFMAN_ONLY") == TOSTR(param_val->ToString()))
            {
                strategy = Z_HUFFMAN_ONLY;
            }
            else if (std::string("RLE") == TOSTR(param_val->ToString()))
            {
                strategy = Z_RLE;
            }
            else if (std::string("FIXED") == TOSTR(param_val->ToString()))
            {
                strategy = Z_FIXED;
            }
            else if (std::string("DEFAULT") == TOSTR(param_val->ToString()))
            {
                strategy = Z_DEFAULT_STRATEGY;
            }
            else
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return scope.Escape(Nan::Undefined());
            }
        }
    }

    try
    {
        std::size_t raw_size = d->tile_->size();
        if (raw_size <= 0)
        {
            return scope.Escape(Nan::NewBuffer(0).ToLocalChecked());
        }
        else
        {
            if (raw_size >= node::Buffer::kMaxLength) {
                // This is a valid test path, but I am excluding it from test coverage due to the
                // requirement of loading a very large object in memory in order to test it.
                // LCOV_EXCL_START
                std::ostringstream s;
                s << "Data is too large to convert to a node::Buffer ";
                s << "(" << raw_size << " raw bytes >= node::Buffer::kMaxLength)";
                Nan::ThrowTypeError(s.str().c_str());
                return scope.Escape(Nan::Undefined());
                // LCOV_EXCL_STOP
            }
            if (!compress)
            {
                return scope.Escape(Nan::CopyBuffer((char*)d->tile_->data(),raw_size).ToLocalChecked());
            }
            else
            {
                std::string compressed;
                mapnik::vector_tile_impl::zlib_compress(d->tile_->data(), raw_size, compressed, true, level, strategy);
                return scope.Escape(Nan::CopyBuffer((char*)compressed.data(),compressed.size()).ToLocalChecked());
            }
        }
    }
    catch (std::exception const& ex)
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        Nan::ThrowTypeError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    bool error;
    std::string data;
    bool compress;
    int level;
    int strategy;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_get_data_baton_t;

/**
 * Get the data in this vector tile as a buffer (asynchronous)
 * @memberof VectorTile
 * @instance
 * @name getData
 * @param {Object} [options]
 * @param {string} [options.compression=none] compression type can also be `gzip`
 * @param {int} [options.level=0] a number `0` (no compression) to `9` (best compression)
 * @param {string} options.strategy must be `FILTERED`, `HUFFMAN_ONLY`, `RLE`, `FIXED`, `DEFAULT`
 * @param {Function} callback
 * @example
 * vt.getData({
 *   compression: 'gzip',
 *   level: 9,
 *   strategy: 'FILTERED'
 * }, function(err, data) {
 *   if (err) throw err;
 *   console.log(data); // buffer
 * });
 */
NAN_METHOD(VectorTile::getData)
{
    if (info.Length() == 0 || !info[info.Length()-1]->IsFunction())
    {
        info.GetReturnValue().Set(_getDataSync(info));
        return;
    }

    v8::Local<v8::Value> callback = info[info.Length()-1];
    bool compress = false;
    int level = Z_DEFAULT_COMPRESSION;
    int strategy = Z_DEFAULT_STRATEGY;

    v8::Local<v8::Object> options = Nan::New<v8::Object>();

    if (info.Length() > 1)
    {
        if (!info[0]->IsObject())
        {
            Nan::ThrowTypeError("first arg must be a options object");
            return;
        }

        options = info[0]->ToObject();

        if (options->Has(Nan::New("compression").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("compression").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'compression' must be a string, either 'gzip', or 'none' (default)");
                return;
            }
            compress = std::string("gzip") == (TOSTR(param_val->ToString()));
        }

        if (options->Has(Nan::New("level").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("level").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return;
            }
            level = param_val->IntegerValue();
            if (level < 0 || level > 9)
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return;
            }
        }
        if (options->Has(Nan::New("strategy").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("strategy").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return;
            }
            else if (std::string("FILTERED") == TOSTR(param_val->ToString()))
            {
                strategy = Z_FILTERED;
            }
            else if (std::string("HUFFMAN_ONLY") == TOSTR(param_val->ToString()))
            {
                strategy = Z_HUFFMAN_ONLY;
            }
            else if (std::string("RLE") == TOSTR(param_val->ToString()))
            {
                strategy = Z_RLE;
            }
            else if (std::string("FIXED") == TOSTR(param_val->ToString()))
            {
                strategy = Z_FIXED;
            }
            else if (std::string("DEFAULT") == TOSTR(param_val->ToString()))
            {
                strategy = Z_DEFAULT_STRATEGY;
            }
            else
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return;
            }
        }
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    vector_tile_get_data_baton_t *closure = new vector_tile_get_data_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->compress = compress;
    closure->level = level;
    closure->strategy = strategy;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, get_data, (uv_after_work_cb)after_get_data);
    d->Ref();
    return;
}

void VectorTile::get_data(uv_work_t* req)
{
    vector_tile_get_data_baton_t *closure = static_cast<vector_tile_get_data_baton_t *>(req->data);
    try
    {
        // compress if requested
        if (closure->compress)
        {
            mapnik::vector_tile_impl::zlib_compress(closure->d->tile_->data(), closure->d->tile_->size(), closure->data, true, closure->level, closure->strategy);
        }
    }
    catch (std::exception const& ex)
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::after_get_data(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_get_data_baton_t *closure = static_cast<vector_tile_get_data_baton_t *>(req->data);
    if (closure->error)
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else if (!closure->data.empty())
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::CopyBuffer((char*)closure->data.data(),closure->data.size()).ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    else
    {
        std::size_t raw_size = closure->d->tile_->size();
        if (raw_size <= 0)
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::NewBuffer(0).ToLocalChecked() };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
        else if (raw_size >= node::Buffer::kMaxLength)
        {
            // This is a valid test path, but I am excluding it from test coverage due to the
            // requirement of loading a very large object in memory in order to test it.
            // LCOV_EXCL_START
            std::ostringstream s;
            s << "Data is too large to convert to a node::Buffer ";
            s << "(" << raw_size << " raw bytes >= node::Buffer::kMaxLength)";
            v8::Local<v8::Value> argv[1] = { Nan::Error(s.str().c_str()) };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
            // LCOV_EXCL_STOP
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::CopyBuffer((char*)closure->d->tile_->data(),raw_size).ToLocalChecked() };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }

    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

