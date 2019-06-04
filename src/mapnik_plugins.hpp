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
static inline NAN_METHOD(available_input_plugins)
{
    std::vector<std::string> names = mapnik::datasource_cache::instance().plugin_names();
    v8::Local<v8::Array> a = Nan::New<v8::Array>(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        Nan::Set(a, i, Nan::New<v8::String>(names[i].c_str()).ToLocalChecked());
    }
    info.GetReturnValue().Set(a);
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
static inline NAN_METHOD(register_datasource)
{
    if (info.Length() != 1 || !info[0]->IsString())
    {
        Nan::ThrowTypeError("first argument must be a path to a mapnik input plugin (.input)");
        return;
    }
    std::vector<std::string> names_before = mapnik::datasource_cache::instance().plugin_names();
    std::string path = TOSTR(info[0]);
    mapnik::datasource_cache::instance().register_datasource(path);
    std::vector<std::string> names_after = mapnik::datasource_cache::instance().plugin_names();
    if (names_after.size() > names_before.size())
    {
        info.GetReturnValue().Set(Nan::True());
        return;
    }
    info.GetReturnValue().Set(Nan::False());
}

/**
 * Register multiple datasources.
 *
 * @memberof mapnik
 * @name registerDatasources
 * @param {Array<String>} list of paths to their respective datasources
 */
static inline NAN_METHOD(register_datasources)
{
    if (info.Length() != 1 || !info[0]->IsString())
    {
        Nan::ThrowTypeError("first argument must be a path to a directory of mapnik input plugins");
        return;
    }
    std::vector<std::string> names_before = mapnik::datasource_cache::instance().plugin_names();
    std::string path = TOSTR(info[0]);
    mapnik::datasource_cache::instance().register_datasources(path);
    std::vector<std::string> names_after = mapnik::datasource_cache::instance().plugin_names();
    if (names_after.size() > names_before.size())
    {
        info.GetReturnValue().Set(Nan::True());
        return;
    }
    info.GetReturnValue().Set(Nan::False());
}


}

#endif // __NODE_MAPNIK_PLUGINS_H__
