// Vector Tile
//#include "vector_tile_config.hpp"

// node-mapnik
//#include "mapnik_vector_tile.hpp"
//#include "mapnik_map.hpp"
#include "mapnik_color.hpp"
//#include "mapnik_geometry.hpp"
//#include "mapnik_logger.hpp"
//#include "mapnik_feature.hpp"
//#include "mapnik_fonts.hpp"
//#include "mapnik_plugins.hpp"
#include "mapnik_palette.hpp"
//#include "mapnik_projection.hpp"
//#include "mapnik_layer.hpp"
//#include "mapnik_datasource.hpp"
//#include "mapnik_featureset.hpp"
//#include "mapnik_memory_datasource.hpp"
#include "mapnik_image.hpp"
//#include "mapnik_image_view.hpp"
//#include "mapnik_cairo_surface.hpp"
#if defined(GRID_RENDERER)
//#include "mapnik_grid.hpp"
//#include "mapnik_grid_view.hpp"
#endif
//#include "mapnik_expression.hpp"
//#include "utils.hpp"
//#include "blend.hpp"

// mapnik
#include <mapnik/config.hpp> // for MAPNIK_DECL
#include <mapnik/version.hpp>
#include <mapnik/marker_cache.hpp>
#include <mapnik/mapped_memory_cache.hpp>
#include <mapnik/image_compositing.hpp>
#include <mapnik/image_scaling.hpp>

// boost
#include <boost/version.hpp>

// cairo
#if defined(HAVE_CAIRO)
#include <cairo.h>
#endif

// std
#include <future>

//namespace node_mapnik {

/*
static std::string format_version(int version)
{
    std::ostringstream s;
    s << version/100000 << "." << version/100 % 1000  << "." << version % 100;
    return s.str();
}

static Napi::Value clearCache(const Napi::CallbackInfo& info)
{
    Napi::HandleScope scope(env);
#if defined(MAPNIK_MEMORY_MAPPED_FILE)
    mapnik::marker_cache::instance().clear();
    mapnik::mapped_memory_cache::instance().clear();
#endif
    return;
}
*/
/**
 * Mapnik is the core of cartographic design and processing. `node-mapnik` provides a
 * set of bindings to `mapnik` for node.js.
 *
 * ### Plugins
 *
 * Node Mapnik relies on a set of datasource input plugins that must be configured prior to using the API.
 * These plugins are either built into Mapnik, such as the "geojson" plugin
 * or rely on exeternal dependencies, such as GDAL. All plugin methods exist on the `mapnik`
 * class level.
 *
 * Plugins are referenced based on the location of the bindings on your system. These paths are generated
 * in the lib/binding/{build}/mapnik_settings.js file. These settings can be referenced by the `mapnik.settings` object. We recommend using the `require('path')`
 * object when building these paths:
 *
 * ```
 * path.resolve(mapnik.settings.paths.input_plugins, 'geojson.input')
 * ```
 *
 * @name mapnik
 * @class mapnik
 * @property {string} version current version of mapnik
 * @property {string} module_path path to native mapnik binding
 * @property {Object} supports indicates which of the following are supported:
 * grid, svg, cairo, cairo_pdf, cairo_svg, png, jpeg, tiff, webp, proj4, threadsafe
 * @property {Object} versions diagnostic object with versions of
 * node, v8, boost, boost_number, mapnik, mapnik_number, mapnik_git_describe, cairo
 * @property {Object} settings - object that defines local paths for particular plugins and addons.
 *
 * ```
 * // mapnik.settings on OSX
 * {
 *   paths: {
 *     fonts: '/Users/username/mapnik/node_modules/mapnik/lib/binding/node-v46-darwin-x64/mapnik/fonts',
 *     input_plugins: '/Users/username/mapnik/node_modules/mapnik/lib/binding/node-v46-darwin-x64/mapnik/input'
 *   },
 *   env: {
 *     ICU_DATA: '/Users/username/mapnik/node_modules/mapnik/lib/binding/node-v46-darwin-x64/share/mapnik/icu',
 *     GDAL_DATA: '/Users/username/mapnik/node_modules/mapnik/lib/binding/node-v46-darwin-x64/share/mapnik/gdal',
 *     PROJ_LIB: '/Users/username/mapnik/node_modules/mapnik/lib/binding/node-v46-darwin-x64/share/mapnik/proj'
 *   }
 * }
 * ```
 * @example
 * var mapnik = require('mapnik');
 */



/**
 * Image type constants representing color and grayscale encodings.
 *
 * @name imageType
 * @memberof mapnik
 * @static
 * @class
 * @property {number} rgba8
 * @property {number} gray8
 * @property {number} gray8s
 * @property {number} gray16
 * @property {number} gray16s
 * @property {number} gray32
 * @property {number} gray32s
 * @property {number} gray32f
 * @property {number} gray64
 * @property {number} gray64s
 * @property {number} gray64f
 */

void init_image_types(Napi::Env env, Napi::Object exports)
{
    Napi::Object image_types = Napi::Object::New(env);
    image_types.Set("null",    Napi::Number::New(env, mapnik::image_dtype_null));
    image_types.Set("rgba8",   Napi::Number::New(env, mapnik::image_dtype_rgba8));
    image_types.Set("gray8",   Napi::Number::New(env, mapnik::image_dtype_gray8));
    image_types.Set("gray8s",  Napi::Number::New(env, mapnik::image_dtype_gray8s));
    image_types.Set("gray16",  Napi::Number::New(env, mapnik::image_dtype_gray16));
    image_types.Set("gray16s", Napi::Number::New(env, mapnik::image_dtype_gray16s));
    image_types.Set("gray32",  Napi::Number::New(env, mapnik::image_dtype_gray32));
    image_types.Set("gray32s", Napi::Number::New(env, mapnik::image_dtype_gray32s));
    image_types.Set("gray32f", Napi::Number::New(env, mapnik::image_dtype_gray32f));
    image_types.Set("gray64",  Napi::Number::New(env, mapnik::image_dtype_gray64));
    image_types.Set("gray64s", Napi::Number::New(env, mapnik::image_dtype_gray64s));
    image_types.Set("gray64f", Napi::Number::New(env, mapnik::image_dtype_gray64f));
    exports.Set("imageType", image_types);
}


/**
 * Image scaling type constants representing color and grayscale encodings.
 *
 * @name imageScaling
 * @memberof mapnik
 * @static
 * @class
 * @property {number} near
 * @property {number} bilinear
 * @property {number} bicubic
 * @property {number} spline16
 * @property {number} spline36
 * @property {number} hanning
 * @property {number} hamming
 * @property {number} hermite
 * @property {number} kaiser
 * @property {number} quadric
 * @property {number} catrom
 * @property {number} gaussian
 * @property {number} bessel
 * @property {number} mitchell
 * @property {number} sinc
 * @property {number} lanczos
 * @property {number} blackman
 */

void init_image_scalings(Napi::Env env, Napi::Object exports)
{
    Napi::Object image_scaling_types = Napi::Object::New(env);
    image_scaling_types.Set("near", Napi::Number::New(env, mapnik::SCALING_NEAR));
    image_scaling_types.Set("bilinear", Napi::Number::New(env, mapnik::SCALING_BILINEAR));
    image_scaling_types.Set("bicubic", Napi::Number::New(env, mapnik::SCALING_BICUBIC));
    image_scaling_types.Set("spline16", Napi::Number::New(env, mapnik::SCALING_SPLINE16));
    image_scaling_types.Set("spline36", Napi::Number::New(env, mapnik::SCALING_SPLINE36));
    image_scaling_types.Set("hanning", Napi::Number::New(env, mapnik::SCALING_HANNING));
    image_scaling_types.Set("hamming", Napi::Number::New(env, mapnik::SCALING_HAMMING));
    image_scaling_types.Set("hermite", Napi::Number::New(env, mapnik::SCALING_HERMITE));
    image_scaling_types.Set("kaiser", Napi::Number::New(env, mapnik::SCALING_KAISER));
    image_scaling_types.Set("quadric", Napi::Number::New(env, mapnik::SCALING_QUADRIC));
    image_scaling_types.Set("catrom", Napi::Number::New(env, mapnik::SCALING_CATROM));
    image_scaling_types.Set("gaussian", Napi::Number::New(env, mapnik::SCALING_GAUSSIAN));
    image_scaling_types.Set("bessel", Napi::Number::New(env, mapnik::SCALING_BESSEL));
    image_scaling_types.Set("mitchell",Napi::Number::New(env, mapnik::SCALING_MITCHELL));
    image_scaling_types.Set("sinc", Napi::Number::New(env, mapnik::SCALING_SINC));
    image_scaling_types.Set("lanczos", Napi::Number::New(env, mapnik::SCALING_LANCZOS));
    image_scaling_types.Set("blackman", Napi::Number::New(env, mapnik::SCALING_BLACKMAN));
    exports.Set("imageScaling", image_scaling_types);
}

Napi::Object init(Napi::Env env, Napi::Object exports)
{
    Color::Initialize(env, exports);
    Image::Initialize(env, exports);
    Palette::Initialize(env, exports);
    init_image_types(env, exports);
    init_image_scalings(env, exports);
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, init)
