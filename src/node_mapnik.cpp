// Vector Tile
#define _USE_MATH_DEFINES
#include <cmath> // M_PI
#include "vector_tile_config.hpp"

// node-mapnik
#include "mapnik_vector_tile.hpp"
#include "mapnik_map.hpp"
#include "mapnik_color.hpp"
#include "mapnik_geometry.hpp"
#include "mapnik_logger.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_fonts.hpp"
#include "mapnik_plugins.hpp"
#include "mapnik_palette.hpp"
#include "mapnik_projection.hpp"
#include "mapnik_layer.hpp"
#include "mapnik_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_cairo_surface.hpp"
#if defined(GRID_RENDERER)
#include "mapnik_grid.hpp"
#include "mapnik_grid_view.hpp"
#endif
#include "mapnik_expression.hpp"
#include "blend.hpp"

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

namespace node_mapnik {

// napi property attributes
napi_property_attributes const prop_attr = napi_writable;

static std::string format_version(int version)
{
    std::ostringstream s;
    s << version / 100000 << "." << version / 100 % 1000 << "." << version % 100;
    return s.str();
}

static Napi::Value clearCache(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
#if defined(MAPNIK_MEMORY_MAPPED_FILE)
    mapnik::marker_cache::instance().clear();
    mapnik::mapped_memory_cache::instance().clear();
#endif
    return env.Undefined();
}
} // namespace node_mapnik
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
 * Composite operation constants
 *
 * @property {number} clear
 * @property {number} src
 * @property {number} dst
 * @property {number} src_over
 * @property {number} dst_over
 * @property {number} src_in
 * @property {number} dst_in
 * @property {number} src_out
 * @property {number} dst_out
 * @property {number} src_atop
 * @property {number} dst_atop
 * @property {number} xor
 * @property {number} plus
 * @property {number} minus
 * @property {number} multiply
 * @property {number} screen
 * @property {number} overlay
 * @property {number} darken
 * @property {number} lighten
 * @property {number} color_dodge
 * @property {number} color_burn
 * @property {number} hard_light
 * @property {number} soft_light
 * @property {number} difference
 * @property {number} exclusion
 * @property {number} contrast
 * @property {number} invert
 * @property {number} invert-rgb
 * @property {number} grain_merge
 * @property {number} grain_extract
 * @property {number} hue
 * @property {number} saturation
 * @property {number} color
 * @property {number} linear_dodge
 * @property {number} linear_burn
 * @property {number} divide
 * @name compositeOp
 * @memberof mapnik
 * @static
 * @class
 */

void init_image_comp_op(Napi::Env env, Napi::Object exports)
{
    Napi::Object composite_ops = Napi::Object::New(env);
    composite_ops.Set("clear", Napi::Number::New(env, mapnik::clear));
    composite_ops.Set("src", Napi::Number::New(env, mapnik::src));
    composite_ops.Set("dst", Napi::Number::New(env, mapnik::dst));
    composite_ops.Set("src_over", Napi::Number::New(env, mapnik::src_over));
    composite_ops.Set("dst_over", Napi::Number::New(env, mapnik::dst_over));
    composite_ops.Set("src_in", Napi::Number::New(env, mapnik::src_in));
    composite_ops.Set("dst_in", Napi::Number::New(env, mapnik::dst_in));
    composite_ops.Set("src_out", Napi::Number::New(env, mapnik::src_out));
    composite_ops.Set("dst_out", Napi::Number::New(env, mapnik::dst_out));
    composite_ops.Set("src_atop", Napi::Number::New(env, mapnik::src_atop));
    composite_ops.Set("dst_atop", Napi::Number::New(env, mapnik::dst_atop));
    composite_ops.Set("xor", Napi::Number::New(env, mapnik::_xor));
    composite_ops.Set("plus", Napi::Number::New(env, mapnik::plus));
    composite_ops.Set("minus", Napi::Number::New(env, mapnik::minus));
    composite_ops.Set("multiply", Napi::Number::New(env, mapnik::multiply));
    composite_ops.Set("screen", Napi::Number::New(env, mapnik::screen));
    composite_ops.Set("overlay", Napi::Number::New(env, mapnik::overlay));
    composite_ops.Set("darken", Napi::Number::New(env, mapnik::darken));
    composite_ops.Set("lighten", Napi::Number::New(env, mapnik::lighten));
    composite_ops.Set("color_dodge", Napi::Number::New(env, mapnik::color_dodge));
    composite_ops.Set("color_burn", Napi::Number::New(env, mapnik::color_burn));
    composite_ops.Set("hard_light", Napi::Number::New(env, mapnik::hard_light));
    composite_ops.Set("soft_light", Napi::Number::New(env, mapnik::soft_light));
    composite_ops.Set("difference", Napi::Number::New(env, mapnik::difference));
    composite_ops.Set("exclusion", Napi::Number::New(env, mapnik::exclusion));
    composite_ops.Set("contrast", Napi::Number::New(env, mapnik::contrast));
    composite_ops.Set("invert", Napi::Number::New(env, mapnik::invert));
    composite_ops.Set("invert-rgb", Napi::Number::New(env, mapnik::invert_rgb));
    composite_ops.Set("grain_merge", Napi::Number::New(env, mapnik::grain_merge));
    composite_ops.Set("grain_extract", Napi::Number::New(env, mapnik::grain_extract));
    composite_ops.Set("hue", Napi::Number::New(env, mapnik::hue));
    composite_ops.Set("saturation", Napi::Number::New(env, mapnik::saturation));
    composite_ops.Set("color", Napi::Number::New(env, mapnik::_color));
    composite_ops.Set("linear_dodge", Napi::Number::New(env, mapnik::linear_dodge));
    composite_ops.Set("linear_burn", Napi::Number::New(env, mapnik::linear_burn));
    composite_ops.Set("divide", Napi::Number::New(env, mapnik::divide));
    exports.Set("compositeOp", composite_ops);
}

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
    image_types.Set("null", Napi::Number::New(env, mapnik::image_dtype_null));
    image_types.Set("rgba8", Napi::Number::New(env, mapnik::image_dtype_rgba8));
    image_types.Set("gray8", Napi::Number::New(env, mapnik::image_dtype_gray8));
    image_types.Set("gray8s", Napi::Number::New(env, mapnik::image_dtype_gray8s));
    image_types.Set("gray16", Napi::Number::New(env, mapnik::image_dtype_gray16));
    image_types.Set("gray16s", Napi::Number::New(env, mapnik::image_dtype_gray16s));
    image_types.Set("gray32", Napi::Number::New(env, mapnik::image_dtype_gray32));
    image_types.Set("gray32s", Napi::Number::New(env, mapnik::image_dtype_gray32s));
    image_types.Set("gray32f", Napi::Number::New(env, mapnik::image_dtype_gray32f));
    image_types.Set("gray64", Napi::Number::New(env, mapnik::image_dtype_gray64));
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
    image_scaling_types.Set("mitchell", Napi::Number::New(env, mapnik::SCALING_MITCHELL));
    image_scaling_types.Set("sinc", Napi::Number::New(env, mapnik::SCALING_SINC));
    image_scaling_types.Set("lanczos", Napi::Number::New(env, mapnik::SCALING_LANCZOS));
    image_scaling_types.Set("blackman", Napi::Number::New(env, mapnik::SCALING_BLACKMAN));
    exports.Set("imageScaling", image_scaling_types);
}

/**
 * Constants representing fill types understood by [Clipper during vector tile encoding](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/PolyFillType.htm).
 *
 * @name polygonFillType
 * @memberof mapnik
 * @static
 * @class
 * @property {number} evenOdd
 * @property {number} nonZero
 * @property {number} positive
 * @property {number} negative
 */
void init_polygon_fill_types(Napi::Env env, Napi::Object exports)
{
    Napi::Object polygon_fill_types = Napi::Object::New(env);
    polygon_fill_types.Set("evenOdd", Napi::Number::New(env, mapnik::vector_tile_impl::even_odd_fill));
    polygon_fill_types.Set("nonZero", Napi::Number::New(env, mapnik::vector_tile_impl::non_zero_fill));
    polygon_fill_types.Set("positive", Napi::Number::New(env, mapnik::vector_tile_impl::positive_fill));
    polygon_fill_types.Set("negative", Napi::Number::New(env, mapnik::vector_tile_impl::negative_fill));
    exports.Set("polygonFillType", polygon_fill_types);
}

/**
 * Constants representing `std::async` threading mode (aka [launch policy](http://en.cppreference.com/w/cpp/thread/launch)).
 *
 * @name threadingMode
 * @memberof mapnik
 * @static
 * @class
 * @property {number} async
 * @property {number} deferred
 */
void init_threading_mode_types(Napi::Env env, Napi::Object exports)
{
    Napi::Object threading_mode_types = Napi::Object::New(env);
    threading_mode_types.Set("async", static_cast<unsigned>(std::launch::async));
    threading_mode_types.Set("deferred", static_cast<unsigned>(std::launch::deferred));
    threading_mode_types.Set("auto", static_cast<unsigned>(std::launch::async | std::launch::deferred));
    exports.Set("threadingMode", threading_mode_types);
}

Napi::Object init(Napi::Env env, Napi::Object exports)
{
    // methods
    exports.Set("registerDatasource", Napi::Function::New(env, node_mapnik::register_datasource));
    exports.Set("register_datasource", Napi::Function::New(env, node_mapnik::register_datasource));
    exports.Set("registerDatasources", Napi::Function::New(env, node_mapnik::register_datasources));
    exports.Set("register_datasources", Napi::Function::New(env, node_mapnik::register_datasources));
    exports.Set("datasources", Napi::Function::New(env, node_mapnik::available_input_plugins));
    exports.Set("registerFonts", Napi::Function::New(env, node_mapnik::register_fonts));
    exports.Set("register_fonts", Napi::Function::New(env, node_mapnik::register_fonts));
    exports.Set("fonts", Napi::Function::New(env, node_mapnik::available_font_faces));
    exports.Set("fontFiles", Napi::Function::New(env, node_mapnik::available_font_files));
    exports.Set("memoryFonts", Napi::Function::New(env, node_mapnik::memory_fonts));
    exports.Set("clearCache", Napi::Function::New(env, node_mapnik::clearCache));
    exports.Set("blend", Napi::Function::New(env, node_mapnik::blend));
    exports.Set("rgb2hsl", Napi::Function::New(env, node_mapnik::rgb2hsl));
    exports.Set("hsl2rgb", Napi::Function::New(env, node_mapnik::hsl2rgb));
    // classes
    Color::Initialize(env, exports, node_mapnik::prop_attr);
    Image::Initialize(env, exports, node_mapnik::prop_attr);
    ImageView::Initialize(env, exports, node_mapnik::prop_attr);
    Palette::Initialize(env, exports, node_mapnik::prop_attr);
    Datasource::Initialize(env, exports, node_mapnik::prop_attr);
    Projection::Initialize(env, exports, node_mapnik::prop_attr);
    ProjTransform::Initialize(env, exports, node_mapnik::prop_attr);
    Geometry::Initialize(env, exports, node_mapnik::prop_attr);
    Feature::Initialize(env, exports, node_mapnik::prop_attr);
    Featureset::Initialize(env, exports, node_mapnik::prop_attr);
    Layer::Initialize(env, exports, node_mapnik::prop_attr);
    Map::Initialize(env, exports, node_mapnik::prop_attr);
    Expression::Initialize(env, exports, node_mapnik::prop_attr);
    Logger::Initialize(env, exports, node_mapnik::prop_attr);
    CairoSurface::Initialize(env, exports, node_mapnik::prop_attr);
#if defined(GRID_RENDERER)
    Grid::Initialize(env, exports, node_mapnik::prop_attr);
    GridView::Initialize(env, exports, node_mapnik::prop_attr);
#endif
    VectorTile::Initialize(env, exports, node_mapnik::prop_attr);
    // enums
    init_image_types(env, exports);
    init_image_scalings(env, exports);
    init_image_comp_op(env, exports);
    init_polygon_fill_types(env, exports);
    init_threading_mode_types(env, exports);

    // versions
    // versions of deps
    Napi::Object versions = Napi::Object::New(env);
    versions.Set("boost", node_mapnik::format_version(BOOST_VERSION));
    versions.Set("boost_number", BOOST_VERSION);
    versions.Set("mapnik", node_mapnik::format_version(MAPNIK_VERSION));
    versions.Set("mapnik_number", MAPNIK_VERSION);
    versions.Set("mapnik_git_describe", MAPNIK_GIT_REVISION);
#if defined(HAVE_CAIRO)
    versions.Set("cairo", CAIRO_VERSION_STRING);
#endif
    exports.Set("versions", versions);

    // supports
    Napi::Object supports = Napi::Object::New(env);
#ifdef GRID_RENDERER
    supports.Set("grid", Napi::Boolean::New(env, true));
#else
    supports.Set("grid", Napi::Boolean::New(env, false));
#endif

#ifdef SVG_RENDERER
    supports.Set("svg", Napi::Boolean::New(env, true));
#else
    supports.Set("svg", Napi::Boolean::New(env, false));
#endif

#if defined(HAVE_CAIRO)
    supports.Set("cairo", Napi::Boolean::New(env, true));
#ifdef CAIRO_HAS_PDF_SURFACE
    supports.Set("cairo_pdf", Napi::Boolean::New(env, true));
#else
    supports.Set("cairo_pdf", Napi::Boolean::New(env, false));
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
    supports.Set("cairo_svg", Napi::Boolean::New(env, true));
#else
    supports.Set("cairo_svg", Napi::Boolean::New(env, false));
#endif
#else
    supports.Set("cairo", Napi::Boolean::New(env, false));
#endif

#if defined(HAVE_PNG)
    supports.Set("png", Napi::Boolean::New(env, true));
#else
    supports.Set("png", Napi::Boolean::New(env, false));
#endif

#if defined(HAVE_JPEG)
    supports.Set("jpeg", Napi::Boolean::New(env, true));
#else
    supports.Set("jpeg", Napi::Boolean::New(env, false));
#endif

#if defined(HAVE_TIFF)
    supports.Set("tiff", Napi::Boolean::New(env, true));
#else
    supports.Set("tiff", Napi::Boolean::New(env, false));
#endif

#if defined(HAVE_WEBP)
    supports.Set("webp", Napi::Boolean::New(env, true));
#else
    supports.Set("webp", Napi::Boolean::New(env, false));
#endif

#if defined(MAPNIK_USE_PROJ)
    supports.Set("proj4", Napi::Boolean::New(env, true));
#else
    supports.Set("proj4", Napi::Boolean::New(env, false));
#endif

#if defined(MAPNIK_THREADSAFE)
    supports.Set("threadsafe", Napi::Boolean::New(env, true));
#else
    supports.Set("threadsafe", Napi::Boolean::New(env, false));
#endif
    exports.Set("supports", supports);
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, init)
