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
