#include "mapnik_vector_tile.hpp"
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
        catch (std::exception const& ex)
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

struct AsyncGetData : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncGetData(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                 bool compress,
                 bool release,
                 int level,
                 int strategy,
                 Napi::Function const& callback)
        : Base(callback),
          tile_(tile),
          compress_(compress),
          release_(release),
          level_(level),
          strategy_(strategy)
    {
    }

    void Execute() override
    {
        try
        {
            // compress if requested
            if (compress_)
            {
                data_ = std::make_unique<std::string>();
                mapnik::vector_tile_impl::zlib_compress(tile_->data(), tile_->size(), *data_, true, level_, strategy_);
            }
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        std::size_t raw_size = tile_->size();
        if (raw_size == 0)
        {
            return {env.Undefined(), Napi::Buffer<char>::New(env, 0)};
        }
        else if (compress_ && data_)
        {
            if (release_) tile_->clear();
            std::string& data = *data_;
            auto buffer = Napi::Buffer<char>::New(
                Env(),
                data.empty() ? nullptr : &data[0],
                data.size(),
                [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                    if (str_ptr != nullptr)
                    {
                        Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                    }
                    delete str_ptr;
                },
                data_.release());
            Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(data.size()));
            return {env.Undefined(), buffer};
        }
        else
        {
            if (release_)
            {
                std::unique_ptr<std::string> ptr = tile_->release_buffer();
                std::string& data = *ptr;
                auto buffer = Napi::Buffer<char>::New(
                    Env(),
                    data.empty() ? nullptr : &data[0],
                    data.size(),
                    [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                        if (str_ptr != nullptr)
                        {
                            Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                        }
                        delete str_ptr;
                    },
                    ptr.release());
                Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(data.size()));
                return {env.Undefined(), buffer};
            }
            else
            {
                return {env.Undefined(), Napi::Buffer<char>::Copy(env, (char*)tile_->data(), raw_size)};
            }
        }
        return Base::GetResult(env);
    }

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    bool compress_;
    bool release_;
    int level_;
    int strategy_;
    std::unique_ptr<std::string> data_;
};

struct AsyncAddData : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncAddData(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
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
            merge_from_compressed_buffer(*tile_, data_, length_, validate_, upgrade_);
        }
        catch (std::exception const& ex)
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

} // namespace
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
    auto* worker = new AsyncSetData(tile_, obj.As<Napi::Buffer<char>>(), validate, upgrade, callback);
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
            return env.Undefined();
        }

        options = info[0].As<Napi::Object>();

        if (options.Has("compression"))
        {
            Napi::Value param_val = options.Get("compression");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'compression' must be a string, either 'gzip', or 'none' (default)").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            compress = (std::string("gzip") == param_val.As<Napi::String>().Utf8Value());
        }
        if (options.Has("release"))
        {
            Napi::Value param_val = options.Get("release");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'release' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            release = param_val.As<Napi::Boolean>();
        }
        if (options.Has("level"))
        {
            Napi::Value param_val = options.Get("level");
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
        if (options.Has("strategy"))
        {
            Napi::Value param_val = options.Get("strategy");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::string str = param_val.As<Napi::String>().Utf8Value();
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
                return env.Undefined();
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

                return env.Undefined();
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
                        data.empty() ? nullptr : &data[0],
                        data.size(),
                        [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                            if (str_ptr != nullptr)
                            {
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
                    data.empty() ? nullptr : &data[0],
                    data.size(),
                    [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                        if (str_ptr != nullptr)
                        {
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
    if (info.Length() == 0 || !info[info.Length() - 1].IsFunction())
    {
        return getDataSync(info);
    }
    Napi::Env env = info.Env();
    Napi::Value callback = info[info.Length() - 1];
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

        options = info[0].As<Napi::Object>();

        if (options.Has("compression"))
        {
            Napi::Value param_val = options.Get("compression");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'compression' must be a string, either 'gzip', or 'none' (default)")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
            compress = (std::string("gzip") == param_val.As<Napi::String>().Utf8Value());
        }
        if (options.Has("release"))
        {
            Napi::Value param_val = options.Get("release");
            if (!param_val.IsBoolean())
            {
                Napi::TypeError::New(env, "option 'release' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            release = param_val.As<Napi::Boolean>();
        }
        if (options.Has("level"))
        {
            Napi::Value param_val = options.Get("level");
            if (!param_val.IsNumber())
            {
                Napi::TypeError::New(env, "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
            level = param_val.As<Napi::Number>().Int32Value();
            if (level < 0 || level > 9)
            {
                Napi::TypeError::New(env, "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("strategy"))
        {
            Napi::Value param_val = options.Get("strategy");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::string param_str = param_val.As<Napi::String>();

            if (std::string("FILTERED") == param_str)
            {
                strategy = Z_FILTERED;
            }
            else if (std::string("HUFFMAN_ONLY") == param_str)
            {
                strategy = Z_HUFFMAN_ONLY;
            }
            else if (std::string("RLE") == param_str)
            {
                strategy = Z_RLE;
            }
            else if (std::string("FIXED") == param_str)
            {
                strategy = Z_FIXED;
            }
            else if (std::string("DEFAULT") == param_str)
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

    auto* worker = new AsyncGetData(tile_, compress, release, level, strategy, callback.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

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

Napi::Value VectorTile::addDataSync(Napi::CallbackInfo const& info)
{
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
        merge_from_compressed_buffer(*tile_, obj.As<Napi::Buffer<char>>().Data(), buffer_size, validate, upgrade);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

        return env.Undefined();
    }
    return env.Undefined();
}

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

Napi::Value VectorTile::addData(Napi::CallbackInfo const& info)
{
    // ensure callback is a function
    if (info.Length() == 0 || !info[info.Length() - 1].IsFunction())
    {
        return addDataSync(info);
    }
    Napi::Env env = info.Env();

    if (!info[0].IsObject())
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
    if (info.Length() > 2)
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
    auto* worker = new AsyncAddData(tile_, obj.As<Napi::Buffer<char>>(), validate, upgrade, callback);
    worker->Queue();
    return env.Undefined();
}
