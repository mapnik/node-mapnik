#include "mapnik_vector_tile.hpp"

namespace {

struct AsyncClear : Napi::AsyncWorker
{
    AsyncClear(mapnik::vector_tile_impl::merc_tile_ptr const& tile, Napi::Function const& callback)
        : Napi::AsyncWorker(callback),
          tile_(tile) {}

    void Execute() override
    {
        try
        {
            tile_->clear();
        }
        catch (std::exception const& ex)
        {
            // No reason this should ever throw an exception, not currently testable.
            // LCOV_EXCL_START
            SetError(ex.what());
            // LCOV_EXCL_STOP
        }
    }

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
};
} // namespace

/**
 * Remove all data from this vector tile (synchronously)
 * @name clearSync
 * @memberof VectorTile
 * @instance
 * @example
 * vt.clearSync();
 * console.log(vt.getData().length); // 0
 */
Napi::Value VectorTile::clearSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    tile_->clear();
    return env.Undefined();
}

/**
 * Remove all data from this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name clear
 * @param {Function} callback
 * @example
 * vt.clear(function(err) {
 *   if (err) throw err;
 *   console.log(vt.getData().length); // 0
 * });
 */

Napi::Value VectorTile::clear(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return clearSync(info);
    }
    Napi::Env env = info.Env();

    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!callback.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto* worker = new AsyncClear(tile_, callback.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}
