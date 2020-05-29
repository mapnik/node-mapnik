#include "mapnik_vector_tile.hpp"
//#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_load_tile.hpp"

namespace {

struct AsyncSetData : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncSetData(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                 Napi::Buffer<char> const& buffer,
                 bool validate,
                 bool upgrade,
                 Napi::Function const& callback)
        : Base(callback),
          tile_(tile),
          buffer_ref{Napi::Persistent(buffer)},
          data_{buffer.Data()},
          length_{buffer.Length()},
          validate_(validate),
          upgrade_(upgrade) {}

    void Execute() override
    {
        if (length_ <= 0)
        {
            SetError("cannot accept empty buffer as protobuf");
            return;
        }
        try
        {
            tile_->clear();
            merge_from_compressed_buffer(*tile_, data_, length_, validate_, upgrade_);
        }
        catch(std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        return Base::GetResult(env);
    }

private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    Napi::Reference<Napi::Buffer<char>> buffer_ref;
    char const* data_;
    std::size_t length_;
    bool validate_;
    bool upgrade_;
};
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
Napi::Value VectorTile::setDataSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "first argument must be a buffer object").ThrowAsJavaScriptException();

        return env.Undefined();
    }
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

    bool upgrade = false;
    bool validate = false;
    Napi::Object options = Napi::Object::New(env);
    if (info.Length() > 1)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "second arg must be a options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        options = info[1].As<Napi::Object>();
        if (options.Has("validate"))
        {
            Napi::Value param_val = options.Get("validate");
            if (!param_val.IsBoolean())
            {
                Napi::TypeError::New(env, "option 'validate' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            validate = param_val.As<Napi::Boolean>();
        }
        if (options.Has("upgrade"))
        {
            Napi::Value param_val = options.Get("upgrade");
            if (!param_val.IsBoolean())
            {
                Napi::TypeError::New(env, "option 'upgrade' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            upgrade = param_val.As<Napi::Boolean>();
        }
    }
    try
    {
        tile_->clear();
        merge_from_compressed_buffer(*tile_, obj.As<Napi::Buffer<char>>().Data(), buffer_size, validate, upgrade);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

/*
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
    Napi::FunctionReference cb;
    Napi::Persistent<v8::Object> buffer;
} vector_tile_setdata_baton_t;
*/

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
Napi::Value VectorTile::setData(Napi::CallbackInfo const& info)
{
    // ensure callback is a function
    if (info.Length() > 0 && !info[info.Length() - 1].IsFunction())
    {
        return setDataSync(info);
    }

    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "first argument must be a buffer object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.IsBuffer())
    {
        Napi::TypeError::New(env, "first arg must be a buffer object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    bool upgrade = false;
    bool validate = false;
    Napi::Object options = Napi::Object::New(env);
    if (info.Length() > 1)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "second arg must be a options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        options = info[1].As<Napi::Object>();
        if (options.Has("validate"))
        {
            Napi::Value param_val = options.Get("validate");
            if (!param_val.IsBoolean())
            {
                Napi::TypeError::New(env, "option 'validate' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            validate = param_val.As<Napi::Boolean>();
        }
        if (options.Has("upgrade"))
        {
            Napi::Value param_val = options.Get("upgrade");
            if (!param_val.IsBoolean())
            {
                Napi::TypeError::New(env, "option 'upgrade' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            upgrade = param_val.As<Napi::Boolean>();
        }
    }
    Napi::Function callback = info[info.Length() - 1].As<Napi::Function>();
    auto * worker = new AsyncSetData(tile_, obj.As<Napi::Buffer<char>>(), validate, upgrade, callback);
    worker->Queue();
    return env.Undefined();
}

/**
 * Get the data in this vector tile as a buffer (synchronous)
 *
 * @memberof VectorTile
 * @instance
 * @name getDataSync
 * @param {Object} [options]
 * @param {string} [options.compression=none] - can also be `gzip`
 * @param {boolean} [options.release=false] releases VT buffer
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
Napi::Value VectorTile::getDataSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    bool compress = false;
    bool release = false;
    int level = Z_DEFAULT_COMPRESSION;
    int strategy = Z_DEFAULT_STRATEGY;

    Napi::Object options = Napi::Object::New(env);

    if (info.Length() > 0)
    {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "first arg must be a options object").ThrowAsJavaScriptException();
            return scope.Escape(env.Undefined());
        }

        options = info[0].As<Napi::Object>();

        if (options.Has("compression"))
        {
            Napi::Value param_val = options.Get("compression");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'compression' must be a string, either 'gzip', or 'none' (default)").ThrowAsJavaScriptException();
                return scope.Escape(env.Undefined());
            }
            compress = (std::string("gzip") == param_val.As<Napi::String>().Utf8Value());
        }
        if (options.Has("release"))
        {
            Napi::Value param_val = options.Get("release");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'release' must be a boolean").ThrowAsJavaScriptException();
                return scope.Escape(env.Undefined());
            }
            release = param_val.As<Napi::Boolean>();
        }
        if (options.Has("level"))
        {
            Napi::Value param_val = options.Get("level");
            if (!param_val.IsNumber())
            {
                Napi::TypeError::New(env, "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive").ThrowAsJavaScriptException();
                return scope.Escape(env.Undefined());
            }
            level = param_val.As<Napi::Number>().Int32Value();
            if (level < 0 || level > 9)
            {
                Napi::TypeError::New(env, "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive").ThrowAsJavaScriptException();
                return scope.Escape(env.Undefined());
            }
        }
        if (options.Has("strategy"))
        {
            Napi::Value param_val = options.Get("strategy");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT").ThrowAsJavaScriptException();
                return scope.Escape(env.Undefined());
            }
            std::string str =  param_val.As<Napi::String>().Utf8Value();
            if (std::string("FILTERED") == str)
            {
                strategy = Z_FILTERED;
            }
            else if (std::string("HUFFMAN_ONLY") == str)
            {
                strategy = Z_HUFFMAN_ONLY;
            }
            else if (std::string("RLE") == str)
            {
                strategy = Z_RLE;
            }
            else if (std::string("FIXED") == str)
            {
                strategy = Z_FIXED;
            }
            else if (std::string("DEFAULT") == str)
            {
                strategy = Z_DEFAULT_STRATEGY;
            }
            else
            {
                Napi::TypeError::New(env, "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT").ThrowAsJavaScriptException();
                return scope.Escape(env.Undefined());
            }
        }
    }

    try
    {
        std::size_t raw_size = tile_->size();
        if (raw_size <= 0)
        {
            return scope.Escape(Napi::Buffer<char>::New(env, 0));
        }
        else
        {
            /*
            if (raw_size >= node::Buffer::kMaxLength)
            {
                // This is a valid test path, but I am excluding it from test coverage due to the
                // requirement of loading a very large object in memory in order to test it.
                // LCOV_EXCL_START
                std::ostringstream s;
                s << "Data is too large to convert to a node::Buffer ";
                s << "(" << raw_size << " raw bytes >= node::Buffer::kMaxLength)";
                Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
                // LCOV_EXCL_STOP
            }
            */
            if (!compress)
            {
                if (release)
                {
                    std::unique_ptr<std::string> ptr = tile_->release_buffer();
                    std::string& data = *ptr;
                    auto buffer = Napi::Buffer<char>::New(
                        Env(),
                        &data[0],
                        data.size(),
                        [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                            if (str_ptr != nullptr) {
                                Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                            }
                            delete str_ptr;
                        },
                        ptr.release());
                    Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(data.size()));
                    return scope.Escape(buffer);
                }
                else
                {
                    return scope.Escape(Napi::Buffer<char>::Copy(env, (char*)tile_->data(), raw_size));
                }
            }
            else
            {
                std::unique_ptr<std::string> compressed = std::make_unique<std::string>();
                mapnik::vector_tile_impl::zlib_compress(tile_->data(), raw_size, *compressed, true, level, strategy);
                if (release)
                {
                    // To keep the same behaviour as a non compression release, we want to clear the VT buffer
                    tile_->clear();
                }

                std::string& data = *compressed;
                auto buffer = Napi::Buffer<char>::New(
                    Env(),
                    &data[0],
                    data.size(),
                    [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                        if (str_ptr != nullptr) {
                            Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                        }
                        delete str_ptr;
                    },
                    compressed.release());
                Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(data.size()));
                return scope.Escape(buffer);
            }
        }
    }
    catch (std::exception const& ex)
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        Napi::TypeError::New(env, ex.what()).ThrowAsJavaScriptException();
        return scope.Escape(env.Undefined());
        // LCOV_EXCL_STOP
    }
    return scope.Escape(env.Undefined());
}
/*
typedef struct
{
    uv_work_t request;
    VectorTile* d;
    bool error;
    std::unique_ptr<std::string> data;
    bool compress;
    bool release;
    int level;
    int strategy;
    std::string error_name;
    Napi::FunctionReference cb;
} vector_tile_get_data_baton_t;
*/

/**
 * Get the data in this vector tile as a buffer (asynchronous)
 * @memberof VectorTile
 * @instance
 * @name getData
 * @param {Object} [options]
 * @param {string} [options.compression=none] compression type can also be `gzip`
 * @param {boolean} [options.release=false] releases VT buffer
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
Napi::Value VectorTile::getData(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    ///Napi::EscapableHandleScope scope(env);
    return env.Undefined();
    /*
    if (info.Length() == 0 || !info[info.Length()-1]->IsFunction())
    {
        return _getDataSync(info);
        return;
    }

    Napi::Value callback = info[info.Length()-1];
    bool compress = false;
    bool release = false;
    int level = Z_DEFAULT_COMPRESSION;
    int strategy = Z_DEFAULT_STRATEGY;

    Napi::Object options = Napi::Object::New(env);

    if (info.Length() > 1)
    {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "first arg must be a options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        options = info[0].ToObject(Napi::GetCurrentContext());

        if ((options).Has(Napi::String::New(env, "compression")).FromMaybe(false))
        {
            Napi::Value param_val = (options).Get(Napi::String::New(env, "compression"));
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'compression' must be a string, either 'gzip', or 'none' (default)").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            compress = std::string("gzip") == (TOSTR(param_val->ToString(Napi::GetCurrentContext())));
        }
        if ((options).Has(Napi::String::New(env, "release")).FromMaybe(false))
        {
            Napi::Value param_val = (options).Get(Napi::String::New(env, "release"));
            if (!param_val->IsBoolean())
            {
                Napi::TypeError::New(env, "option 'release' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            release = param_val.As<Napi::Boolean>().Value();
        }
        if ((options).Has(Napi::String::New(env, "level")).FromMaybe(false))
        {
            Napi::Value param_val = (options).Get(Napi::String::New(env, "level"));
            if (!param_val.IsNumber())
            {
                Napi::TypeError::New(env, "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            level = param_val.As<Napi::Number>().Int32Value();
            if (level < 0 || level > 9)
            {
                Napi::TypeError::New(env, "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if ((options).Has(Napi::String::New(env, "strategy")).FromMaybe(false))
        {
            Napi::Value param_val = (options).Get(Napi::String::New(env, "strategy"));
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            else if (std::string("FILTERED") == TOSTR(param_val->ToString(Napi::GetCurrentContext())))
            {
                strategy = Z_FILTERED;
            }
            else if (std::string("HUFFMAN_ONLY") == TOSTR(param_val->ToString(Napi::GetCurrentContext())))
            {
                strategy = Z_HUFFMAN_ONLY;
            }
            else if (std::string("RLE") == TOSTR(param_val->ToString(Napi::GetCurrentContext())))
            {
                strategy = Z_RLE;
            }
            else if (std::string("FIXED") == TOSTR(param_val->ToString(Napi::GetCurrentContext())))
            {
                strategy = Z_FIXED;
            }
            else if (std::string("DEFAULT") == TOSTR(param_val->ToString(Napi::GetCurrentContext())))
            {
                strategy = Z_DEFAULT_STRATEGY;
            }
            else
            {
                Napi::TypeError::New(env, "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
    }

    VectorTile* d = info.Holder().Unwrap<VectorTile>();
    vector_tile_get_data_baton_t *closure = new vector_tile_get_data_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->compress = compress;
    closure->release = release;
    closure->data = std::make_unique<std::string>();
    closure->level = level;
    closure->strategy = strategy;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, get_data, (uv_after_work_cb)after_get_data);
    d->Ref();
    return;
    */
}
/*
void VectorTile::get_data(uv_work_t* req)
{
    vector_tile_get_data_baton_t *closure = static_cast<vector_tile_get_data_baton_t *>(req->data);
    try
    {
        // compress if requested
        if (closure->compress)
        {
            mapnik::vector_tile_impl::zlib_compress(closure->d->tile_->data(), closure->d->tile_->size(), *(closure->data), true, closure->level, closure->strategy);
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    vector_tile_get_data_baton_t *closure = static_cast<vector_tile_get_data_baton_t *>(req->data);
    if (closure->error)
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else if (!closure->data->empty())
    {
        if (closure->release)
        {
            // To keep the same behaviour as a non compression release, we want to clear the VT buffer
            closure->d->tile_->clear();
        }
        Napi::Value argv[2] = { env.Undefined(),
                                         node_mapnik::NewBufferFrom(std::move(closure->data)) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }
    else
    {
        std::size_t raw_size = closure->d->tile_->size();
        if (raw_size <= 0)
        {
            Napi::Value argv[2] = { env.Undefined(), Napi::Buffer<char>::New(env, 0) };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
        else if (raw_size >= node::Buffer::kMaxLength)
        {
            // This is a valid test path, but I am excluding it from test coverage due to the
            // requirement of loading a very large object in memory in order to test it.
            // LCOV_EXCL_START
            std::ostringstream s;
            s << "Data is too large to convert to a node::Buffer ";
            s << "(" << raw_size << " raw bytes >= node::Buffer::kMaxLength)";
            Napi::Value argv[1] = { Napi::Error::New(env, s.str().c_str()) };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
            // LCOV_EXCL_STOP
        }
        else
        {
            if (closure->release)
            {
                Napi::Value argv[2] = { env.Undefined(),
                                                 node_mapnik::NewBufferFrom(closure->d->tile_->release_buffer()) };
                async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
            }
            else
            {
                Napi::Value argv[2] = { env.Undefined(), Napi::Buffer::Copy(env, (char*)closure->d->tile_->data(),raw_size) };
                async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
            }
        }
    }

    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}
*/



/*
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    vector_tile_setdata_baton_t *closure = static_cast<vector_tile_setdata_baton_t *>(req->data);
    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Napi::Value argv[1] = { env.Undefined() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}
*/
