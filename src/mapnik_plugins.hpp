#ifndef __NODE_MAPNIK_PLUGINS_H__
#define __NODE_MAPNIK_PLUGINS_H__


// mapnik
#include <mapnik/datasource_cache.hpp>
#include <mapnik/version.hpp>

// stl
#include <vector>
#include <string>
#include "utils.hpp"



namespace node_mapnik {

/**
 * Register all plugins available. This is not recommend in environments where high-performance is priority.
 * Consider registering plugins on a per-need basis.
 *
 * @memberof mapnik
 * @name register_default_input_plugins
 * @example
 * var mapnik = require('mapnik');
 * mapnik.register_default_input_plugins();
 */

/**
 * List all plugins that are currently available.
 *
 * @memberof mapnik
 * @name datasources
 * @returns {Array<String>} list of plugins available to use
 */
static inline Napi::Value available_input_plugins(Napi::CallbackInfo const& info)
{
    std::vector<std::string> names = mapnik::datasource_cache::instance().plugin_names();
    Napi::Array a = Napi::Array::New(env, names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        (a).Set(i, Napi::String::New(env, names[i].c_str()));
    }
    return a;
}

/**
 * Register a single datasource input plugin. The available plugins are:
 *
 * * `'csv.input'`
 * * `'gdal.input'`
 * * `'geojson.input'`
 * * `'ogr.input'`
 * * `'pgraster.input'`
 * * `'postgis.input'`
 * * `'raster.input'`
 * * `'shape.input'`
 * * `'sqlite.input'`
 * * `'topojson.input'`
 *
 * @memberof mapnik
 * @name registerDatasource
 * @param {String} path to a datasource to register.
 * @example
 * mapnik.registerDatasource(path.join(mapnik.settings.paths.input_plugins, 'geojson.input'));
 */
static inline Napi::Value register_datasource(Napi::CallbackInfo const& info)
{
    if (info.Length() != 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to a mapnik input plugin (.input)").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::vector<std::string> names_before = mapnik::datasource_cache::instance().plugin_names();
    std::string path = TOSTR(info[0]);
    mapnik::datasource_cache::instance().register_datasource(path);
    std::vector<std::string> names_after = mapnik::datasource_cache::instance().plugin_names();
    if (names_after.size() > names_before.size())
    {
        return env.True();
        return;
    }
    return env.False();
}

/**
 * Register multiple datasources.
 *
 * @memberof mapnik
 * @name registerDatasources
 * @param {Array<String>} list of paths to their respective datasources
 */
static inline Napi::Value register_datasources(Napi::CallbackInfo const& info)
{
    if (info.Length() != 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to a directory of mapnik input plugins").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::vector<std::string> names_before = mapnik::datasource_cache::instance().plugin_names();
    std::string path = TOSTR(info[0]);
    mapnik::datasource_cache::instance().register_datasources(path);
    std::vector<std::string> names_after = mapnik::datasource_cache::instance().plugin_names();
    if (names_after.size() > names_before.size())
    {
        return env.True();
        return;
    }
    return env.False();
}


}

#endif // __NODE_MAPNIK_PLUGINS_H__
