#include "mapnik_map.hpp"

#include <mapnik/load_map.hpp> // for load_map, load_map_string
#include <mapnik/map.hpp>      // for Map, etc

namespace detail {

struct AsyncMapFromString : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncMapFromString(map_ptr const& map, std::string const& stylesheet,
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
            mapnik::load_map_string(*map_, stylesheet_, strict_, base_path_);
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
 * XML stylesheet given as a string.
 *
 * @memberof Map
 * @instance
 * @name fromStringSync
 * @param {string} stylesheet contents
 * @param {Object} [options={}]
 * @example
 * var fs = require('fs');
 * map.fromStringSync(fs.readFileSync('./style.xml', 'utf8'));
 */

Napi::Value Map::fromStringSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1)
    {
        Napi::Error::New(env, "Accepts 2 arguments: stylesheet string and an optional options").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "first argument must be a mapnik stylesheet string").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // defaults
    bool strict = false;
    std::string base_path("");

    if (info.Length() >= 2)
    {
        // ensure options object
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "options must be an object, eg {strict: true, base: \".\"'}").ThrowAsJavaScriptException();
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

    std::string stylesheet = info[0].As<Napi::String>();
    try
    {
        mapnik::load_map_string(*map_, stylesheet, strict, base_path);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

/**
 * Load styles, layers, and other information for this map from a Mapnik
 * XML stylesheet given as a string.
 *
 * @memberof Map
 * @instance
 * @name fromString
 * @param {string} stylesheet contents
 * @param {Object} [options={}]
 * @param {Function} callback
 * @example
 * var fs = require('fs');
 * map.fromString(fs.readFileSync('./style.xml', 'utf8'), function(err, res) {
 *   // details loaded
 * });
 */

Napi::Value Map::fromString(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2)
    {
        Napi::Error::New(env, "please provide a stylesheet string, options, and callback").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // ensure stylesheet path is a string
    Napi::Value stylesheet = info[0];
    if (!stylesheet.IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to a mapnik stylesheet string").ThrowAsJavaScriptException();
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
        Napi::TypeError::New(env, "options must be an object, eg {strict: true, base: \".\"'}").ThrowAsJavaScriptException();
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

    auto* worker = new detail::AsyncMapFromString(map_, stylesheet.As<Napi::String>(), base_path, strict, callback_val.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}
