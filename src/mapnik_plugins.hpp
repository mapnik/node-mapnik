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
 * Mapnik relies on a set of datasource input plugins that must be configured prior to using the API.
 * These plugins are either built into Mapnik, such as the "geojson" plugin
 * or rely on exeternal dependencies, such as GDAL. All plugin methods exist on the `mapnik` 
 * class level.
 *
 * Plugins are referenced based on the location of the bindings on your system. These paths are generated
 * in the lib/binding/{build}/mapnik_settings.js file. This file, depending on your system architecture
 * looks something like this:
 * 
 * ```
 * var path = require('path');
 * module.exports.paths = {
 *   'fonts': path.join(__dirname, 'mapnik/fonts'),
 *   'input_plugins': path.join(__dirname, 'mapnik/input')
 * };
 * module.exports.env = {
 *   'ICU_DATA': path.join(__dirname, 'share/mapnik/icu'),
 *   'GDAL_DATA': path.join(__dirname, 'share/mapnik/gdal'),
 *   'PROJ_LIB': path.join(__dirname, 'share/mapnik/proj')
 * };
 * ```
 *
 * These settings can be referenced by the `mapnik.settings` object. We recommend using the `require('path')`
 * module when building these paths. Here's what this looks like if you are needing the geojson input.
 * 
 * ```
 * path.resolve(mapnik.settings.paths.input_plugins, 'geojson.input')
 * ```
 * 
 * @class Plugins
 * 
 */

/**
 * Register all plugins available. This is not recommend in environments where high-performance is priority.
 * Consider registering plugins on a per-need basis.
 * 
 * @memberof Plugins
 * @name register_default_input_plugins
 * @example
 * var mapnik = require('mapnik');
 * mapnik.register_default_input_plugins();
 */

/**
 * List all plugins that are currently available.
 *
 * @memberof Plugins
 * @name datasources
 * @returns {Array<String>} list of plugins available to use
 */
static inline NAN_METHOD(available_input_plugins)
{
    std::vector<std::string> names = mapnik::datasource_cache::instance().plugin_names();
    v8::Local<v8::Array> a = Nan::New<v8::Array>(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        a->Set(i, Nan::New<v8::String>(names[i].c_str()).ToLocalChecked());
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
 * @memberof Plugins
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
 * @memberof Plugins
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
