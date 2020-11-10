#include "mapnik_map.hpp"

#include <mapnik/load_map.hpp> // for load_map, load_map_string
#include <mapnik/map.hpp>      // for Map, etc

namespace detail {

struct AsyncMapLoad : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncMapLoad(map_ptr const& map, std::string const& stylesheet,
                 std::string const& base_path, bool strict, Napi::Function const& callback)
        : Base(callback),
          map_(map),
          stylesheet_(stylesheet),
          base_path_(base_path),
          strict_(strict) {}

    void Execute() override
    {
        try
        {
            mapnik::load_map(*map_, stylesheet_, strict_, base_path_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        if (map_)
        {
            Napi::Value arg = Napi::External<map_ptr>::New(env, &map_);
            Napi::Object obj = Map::constructor.New({arg});
            return {env.Null(), napi_value(obj)};
        }
        return Base::GetResult(env);
    }

  private:
    map_ptr map_;
    std::string stylesheet_;
    std::string base_path_;
    bool strict_;
};

} // namespace detail

/**
 * Load styles, layers, and other information for this map from a Mapnik
 * XML stylesheet.
 *
 * @memberof Map
 * @instance
 * @name load
 * @param {string} stylesheet path
 * @param {Object} [options={}]
 * @param {Function} callback
 */

Napi::Value Map::load(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2)
    {
        Napi::Error::New(env, "please provide a stylesheet path, options, and callback").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // ensure stylesheet path is a string
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to a mapnik stylesheet").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // ensure options object
    if (!info[1].IsObject())
    {
        Napi::TypeError::New(env, "options must be an object, eg {strict: true}").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object options = info[1].As<Napi::Object>();

    bool strict = false;
    if (options.Has("strict"))
    {
        Napi::Value strict_val = options.Get("strict");
        if (!strict_val.IsBoolean())
        {
            Napi::TypeError::New(env, "'strict' must be a Boolean").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        strict = strict_val.As<Napi::Boolean>();
    }

    std::string base_path = "";
    if (options.Has("base"))
    {
        Napi::Value base_val = options.Get("base");
        if (!base_val.IsString())
        {
            Napi::TypeError::New(env, "'base' must be a string representing a filesystem path").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        base_path = base_val.As<Napi::String>();
    }

    auto* worker = new detail::AsyncMapLoad(map_, info[0].As<Napi::String>(),
                                            base_path, strict, callback_val.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

/**
 * Load styles, layers, and other information for this map from a Mapnik
 * XML stylesheet.
 *
 * @memberof Map
 * @instance
 * @name loadSync
 * @param {string} stylesheet path
 * @param {Object} [options={}]
 * @example
 * map.loadSync('./style.xml');
 */

Napi::Value Map::loadSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to a mapnik stylesheet").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string stylesheet = info[0].As<Napi::String>();
    bool strict = false;
    std::string base_path;

    if (info.Length() > 2)
    {
        Napi::Error::New(env, "only accepts two arguments: a path to a mapnik stylesheet and an optional options object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    else if (info.Length() == 2)
    {
        // ensure options object
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "options must be an object, eg {strict: true}").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("strict"))
        {
            Napi::Value strict_val = options.Get("strict");
            if (!strict_val.IsBoolean())
            {
                Napi::TypeError::New(env, "'strict' must be a Boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            strict = strict_val.As<Napi::Boolean>();
        }

        if (options.Has("base"))
        {
            Napi::Value base_val = options.Get("base");
            if (!base_val.IsString())
            {
                Napi::TypeError::New(env, "'base' must be a string representing a filesystem path").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            base_path = base_val.As<Napi::String>();
        }
    }

    try
    {
        mapnik::load_map(*map_, stylesheet, strict, base_path);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
    return env.Undefined();
}
