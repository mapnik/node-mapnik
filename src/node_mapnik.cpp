
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
#include "mapnik_memory_datasource.hpp"
#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_grid.hpp"
#include "mapnik_cairo_surface.hpp"
#include "mapnik_grid_view.hpp"
#include "mapnik_expression.hpp"
#include "utils.hpp"
#include "blend.hpp"

// mapnik
#include <mapnik/config.hpp> // for MAPNIK_DECL
#include <mapnik/version.hpp>
#include <mapnik/marker_cache.hpp>
#include <mapnik/mapped_memory_cache.hpp>
#include <mapnik/image_compositing.hpp>

// boost
#include <boost/version.hpp>

// cairo
#if defined(HAVE_CAIRO)
#include <cairo.h>
#endif

namespace node_mapnik {

using namespace node;
using namespace v8;

static std::string format_version(int version)
{
    std::ostringstream s;
    s << version/100000 << "." << version/100 % 1000  << "." << version % 100;
    return s.str();
}

static NAN_METHOD(clearCache)
{
    NanScope();
#if defined(SHAPE_MEMORY_MAPPED_FILE)
    mapnik::marker_cache::instance().clear();
    mapnik::mapped_memory_cache::instance().clear();
#endif
    NanReturnUndefined();
}

static NAN_METHOD(shutdown)
{
    NanScope();
    google::protobuf::ShutdownProtobufLibrary();
    NanReturnUndefined();
}

extern "C" {

    static void InitMapnik (Handle<Object> target)
    {
        NanScope();
        GOOGLE_PROTOBUF_VERIFY_VERSION;

        // module level functions
        NODE_SET_METHOD(target, "blend",node_mapnik::Blend);
        NODE_SET_METHOD(target, "rgb2hsl", rgb2hsl);
        NODE_SET_METHOD(target, "hsl2rgb", hsl2rgb);
        // back compat
        NODE_SET_METHOD(target, "registerFonts", node_mapnik::register_fonts);
        NODE_SET_METHOD(target, "registerDatasource", node_mapnik::register_datasource);
        NODE_SET_METHOD(target, "registerDatasources", node_mapnik::register_datasources);
        NODE_SET_METHOD(target, "register_fonts", node_mapnik::register_fonts);
        NODE_SET_METHOD(target, "register_datasource", node_mapnik::register_datasource);
        NODE_SET_METHOD(target, "register_datasources", node_mapnik::register_datasources);
        NODE_SET_METHOD(target, "datasources", node_mapnik::available_input_plugins);
        NODE_SET_METHOD(target, "fonts", node_mapnik::available_font_faces);
        NODE_SET_METHOD(target, "fontFiles", node_mapnik::available_font_files);
        NODE_SET_METHOD(target, "memoryFonts", node_mapnik::memory_fonts);
        NODE_SET_METHOD(target, "clearCache", clearCache);
        NODE_SET_METHOD(target, "shutdown",shutdown);

        // Classes
        VectorTile::Initialize(target);
        Map::Initialize(target);
        Color::Initialize(target);
        Geometry::Initialize(target);
        Feature::Initialize(target);
        Image::Initialize(target);
        ImageView::Initialize(target);
        Palette::Initialize(target);
        Projection::Initialize(target);
        ProjTransform::Initialize(target);
        Layer::Initialize(target);
        Grid::Initialize(target);
        GridView::Initialize(target);
        Datasource::Initialize(target);
        Featureset::Initialize(target);
        Logger::Initialize(target);
        // Not production safe, so disabling indefinitely
        //JSDatasource::Initialize(target);
        MemoryDatasource::Initialize(target);
        Expression::Initialize(target);
        CairoSurface::Initialize(target);

        // versions of deps
        Local<Object> versions = NanNew<Object>();
        versions->Set(NanNew("node"), NanNew(NODE_VERSION+1)); // NOTE: +1 strips the v in v0.10.26
        versions->Set(NanNew("v8"), NanNew(V8::GetVersion()));
        versions->Set(NanNew("boost"), NanNew(format_version(BOOST_VERSION).c_str()));
        versions->Set(NanNew("boost_number"), NanNew(BOOST_VERSION));
        versions->Set(NanNew("mapnik"), NanNew(format_version(MAPNIK_VERSION).c_str()));
        versions->Set(NanNew("mapnik_number"), NanNew(MAPNIK_VERSION));
        versions->Set(NanNew("mapnik_git_describe"), NanNew(MAPNIK_GIT_REVISION));
#if defined(HAVE_CAIRO)
        versions->Set(NanNew("cairo"), NanNew(CAIRO_VERSION_STRING));
#endif
        target->Set(NanNew("versions"), versions);

        Local<Object> supports = NanNew<Object>();
#ifdef GRID_RENDERER
        supports->Set(NanNew("grid"), NanTrue());
#else
        supports->Set(NanNew("grid"), NanFalse());
#endif

#ifdef SVG_RENDERER
        supports->Set(NanNew("svg"), NanTrue());
#else
        supports->Set(NanNew("svg"), NanFalse());
#endif

#if defined(HAVE_CAIRO)
        supports->Set(NanNew("cairo"), NanTrue());
        #ifdef CAIRO_HAS_PDF_SURFACE
        supports->Set(NanNew("cairo_pdf"), NanTrue());
        #else
        supports->Set(NanNew("cairo_pdf"), NanFalse());
        #endif
        #ifdef CAIRO_HAS_SVG_SURFACE
        supports->Set(NanNew("cairo_svg"), NanTrue());
        #else
        supports->Set(NanNew("cairo_svg"), NanFalse());
        #endif
#else
        supports->Set(NanNew("cairo"), NanFalse());
#endif

#if defined(HAVE_PNG)
        supports->Set(NanNew("png"), NanTrue());
#else
        supports->Set(NanNew("png"), NanFalse());
#endif

#if defined(HAVE_JPEG)
        supports->Set(NanNew("jpeg"), NanTrue());
#else
        supports->Set(NanNew("jpeg"), NanFalse());
#endif

#if defined(HAVE_TIFF)
        supports->Set(NanNew("tiff"), NanTrue());
#else
        supports->Set(NanNew("tiff"), NanFalse());
#endif

#if defined(HAVE_WEBP)
        supports->Set(NanNew("webp"), NanTrue());
#else
        supports->Set(NanNew("webp"), NanFalse());
#endif

#if defined(MAPNIK_USE_PROJ4)
        supports->Set(NanNew("proj4"), NanTrue());
#else
        supports->Set(NanNew("proj4"), NanFalse());
#endif

#if defined(MAPNIK_THREADSAFE)
        supports->Set(NanNew("threadsafe"), NanTrue());
#else
        supports->Set(NanNew("threadsafe"), NanFalse());
#endif

        target->Set(NanNew("supports"), supports);

        Local<Object> composite_ops = NanNew<Object>();
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "clear", mapnik::clear)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "src", mapnik::src)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "dst", mapnik::dst)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "src_over", mapnik::src_over)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "dst_over", mapnik::dst_over)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "src_in", mapnik::src_in)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "dst_in", mapnik::dst_in)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "src_out", mapnik::src_out)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "dst_out", mapnik::dst_out)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "src_atop", mapnik::src_atop)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "dst_atop", mapnik::dst_atop)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "xor", mapnik::_xor)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "plus", mapnik::plus)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "minus", mapnik::minus)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "multiply", mapnik::multiply)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "screen", mapnik::screen)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "overlay", mapnik::overlay)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "darken", mapnik::darken)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "lighten", mapnik::lighten)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "color_dodge", mapnik::color_dodge)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "color_burn", mapnik::color_burn)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "hard_light", mapnik::hard_light)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "soft_light", mapnik::soft_light)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "difference", mapnik::difference)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "exclusion", mapnik::exclusion)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "contrast", mapnik::contrast)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "invert", mapnik::invert)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "invert-rgb", mapnik::invert_rgb)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "grain_merge", mapnik::grain_merge)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "grain_extract", mapnik::grain_extract)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "hue", mapnik::hue)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "saturation", mapnik::saturation)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "color", mapnik::_color)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "linear_dodge", mapnik::linear_dodge)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "linear_burn", mapnik::linear_burn)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "divide", mapnik::divide)
        target->Set(NanNew("compositeOp"), composite_ops);
        
        Local<Object> image_types = NanNew<Object>();
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "null", mapnik::image_dtype_null)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "rgba8", mapnik::image_dtype_rgba8)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray8", mapnik::image_dtype_gray8)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray8s", mapnik::image_dtype_gray8s)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray16", mapnik::image_dtype_gray16)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray16s", mapnik::image_dtype_gray16s)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray32", mapnik::image_dtype_gray32)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray32s", mapnik::image_dtype_gray32s)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray32f", mapnik::image_dtype_gray32f)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray64", mapnik::image_dtype_gray64)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray64s", mapnik::image_dtype_gray64s)
        NODE_MAPNIK_DEFINE_CONSTANT(image_types, "gray64f", mapnik::image_dtype_gray64f)
        target->Set(NanNew("imageType"), image_types);
    }

}

} // namespace node_mapnik

// TODO - remove this hack at node v0.12.x
// https://github.com/joyent/node/commit/bd8a5755dceda415eee147bc06574f5a30abb0d0#commitcomment-5686345
#if NODE_VERSION_AT_LEAST(0, 9, 0)

#define NODE_MAPNIK_MODULE(modname, regfunc)                          \
  extern "C" {                                                        \
    MAPNIK_EXP node::node_module_struct modname ## _module =          \
    {                                                                 \
      NODE_STANDARD_MODULE_STUFF,                                     \
      (node::addon_register_func)regfunc,                             \
      NODE_STRINGIFY(modname)                                         \
    };                                                                \
  }

#else

#define NODE_MAPNIK_MODULE(modname, regfunc)                          \
  extern "C" {                                                        \
    MAPNIK_EXP node::node_module_struct modname ## _module =          \
    {                                                                 \
      NODE_STANDARD_MODULE_STUFF,                                     \
      regfunc,                                                        \
      NODE_STRINGIFY(modname)                                         \
    };                                                                \
  }

#endif

#if NODE_MODULE_VERSION > 0x000B
NODE_MODULE(mapnik, node_mapnik::InitMapnik)
#else
NODE_MAPNIK_MODULE(mapnik, node_mapnik::InitMapnik)
#endif
