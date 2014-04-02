// v8
#include <v8.h>

// node
#include <node.h>
#include <node_version.h>
#include "mapnik3x_compatibility.hpp"

// node-mapnik
#include "mapnik_vector_tile.hpp"
#include "mapnik_map.hpp"
#include "mapnik_color.hpp"
#include "mapnik_geometry.hpp"
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
#ifdef NODE_MAPNIK_EXPRESSION
#include "mapnik_expression.hpp"
#endif
#include "utils.hpp"

#include <libxml/parser.h>
#include <libxml/xmlversion.h>

// mapnik
#include <mapnik/config.hpp> // for MAPNIK_DECL
#include <mapnik/version.hpp>
#if MAPNIK_VERSION >= 200100
#include <mapnik/marker_cache.hpp>
#include <mapnik/mapped_memory_cache.hpp>
#include <mapnik/image_compositing.hpp>
#endif

// boost
#include <boost/version.hpp>

// cairo
#if defined(HAVE_CAIRO)
#include <cairo.h>
#endif

namespace node_mapnik {

using namespace node;
using namespace v8;

/**
 * Optional notification that the embedder is idle.
 * V8 uses the notification to reduce memory footprint.
 * This call can be used repeatedly if the embedder remains idle.
 * Returns true if the embedder should stop calling IdleNotification
 * until real work has been done.  This indicates that V8 has done
 * as much cleanup as it will be able to do.
 */
static Handle<Value> gc(const Arguments& args)
{
    HandleScope scope;
    return scope.Close(Boolean::New(V8::IdleNotification()));
}

static std::string format_version(int version)
{
    std::ostringstream s;
    s << version/100000 << "." << version/100 % 1000  << "." << version % 100;
    return s.str();
}

static Handle<Value> clearCache(const Arguments& args)
{
    HandleScope scope;
#if MAPNIK_VERSION >= 200300
    #if defined(SHAPE_MEMORY_MAPPED_FILE)
        mapnik::marker_cache::instance().clear();
        mapnik::mapped_memory_cache::instance().clear();
    #endif
#else
    #if MAPNIK_VERSION >= 200200
        mapnik::marker_cache::instance().clear();
        mapnik::mapped_memory_cache::instance().clear();
    #else
        #if MAPNIK_VERSION >= 200100
            mapnik::marker_cache::instance()->clear();
            mapnik::mapped_memory_cache::instance()->clear();
        #endif
    #endif
#endif
    return scope.Close(Undefined());
}

static Handle<Value> shutdown(const Arguments& args)
{
    HandleScope scope;
    google::protobuf::ShutdownProtobufLibrary();
    // http://lists.fedoraproject.org/pipermail/devel/2010-January/129117.html
    xmlCleanupParser();
    return scope.Close(Undefined());
}

extern "C" {

    static void InitMapnik (Handle<Object> target)
    {
        HandleScope scope;
        GOOGLE_PROTOBUF_VERIFY_VERSION;
        // https://mail.gnome.org/archives/xml/2007-October/msg00004.html
        // calls http://xmlsoft.org/html/libxml-xmlversion.html#xmlCheckVersion
        // which internall calls http://xmlsoft.org/html/libxml-parser.html#xmlInitParser
        // see 'parserInternals.c' for details / requires xmlCleanupParser(); at exit
        LIBXML_TEST_VERSION;

        // module level functions
        NODE_SET_METHOD(target, "register_datasource", node_mapnik::register_datasource);
        NODE_SET_METHOD(target, "register_datasources", node_mapnik::register_datasources);
        NODE_SET_METHOD(target, "datasources", node_mapnik::available_input_plugins);
        NODE_SET_METHOD(target, "register_fonts", node_mapnik::register_fonts);
        NODE_SET_METHOD(target, "fonts", node_mapnik::available_font_faces);
        NODE_SET_METHOD(target, "fontFiles", node_mapnik::available_font_files);
        NODE_SET_METHOD(target, "clearCache", clearCache);
        NODE_SET_METHOD(target, "gc", gc);
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
        // Not production safe, so disabling indefinitely
        //JSDatasource::Initialize(target);
        MemoryDatasource::Initialize(target);
        #ifdef NODE_MAPNIK_EXPRESSION
        Expression::Initialize(target);
        #endif
        CairoSurface::Initialize(target);

        // versions of deps
        Local<Object> versions = Object::New();
        versions->Set(String::NewSymbol("node"), String::New(NODE_VERSION+1)); // NOTE: +1 strips the v in v0.10.26
        versions->Set(String::NewSymbol("v8"), String::New(V8::GetVersion()));
        versions->Set(String::NewSymbol("boost"), String::New(format_version(BOOST_VERSION).c_str()));
        versions->Set(String::NewSymbol("boost_number"), Integer::New(BOOST_VERSION));
        versions->Set(String::NewSymbol("mapnik"), String::New(format_version(MAPNIK_VERSION).c_str()));
        versions->Set(String::NewSymbol("mapnik_number"), Integer::New(MAPNIK_VERSION));
        versions->Set(String::NewSymbol("libxml"), String::New(LIBXML_DOTTED_VERSION));
#if defined(HAVE_CAIRO)
        versions->Set(String::NewSymbol("cairo"), String::New(CAIRO_VERSION_STRING));
#endif
        target->Set(String::NewSymbol("versions"), versions);

        Local<Object> supports = Object::New();
#if MAPNIK_VERSION >= 200300
        #ifdef GRID_RENDERER
        supports->Set(String::NewSymbol("grid"), True());
        #else
        supports->Set(String::NewSymbol("grid"), False());
        #endif
#else
        supports->Set(String::NewSymbol("grid"), True());
#endif

#ifdef SVG_RENDERER
        supports->Set(String::NewSymbol("svg"), True());
#else
        supports->Set(String::NewSymbol("svg"), False());
#endif

#if defined(HAVE_CAIRO)
        supports->Set(String::NewSymbol("cairo"), True());
        #ifdef CAIRO_HAS_PDF_SURFACE
        supports->Set(String::NewSymbol("cairo_pdf"), True());
        #else
        supports->Set(String::NewSymbol("cairo_pdf"), False());
        #endif
        #ifdef CAIRO_HAS_SVG_SURFACE
        supports->Set(String::NewSymbol("cairo_svg"), True());
        #else
        supports->Set(String::NewSymbol("cairo_svg"), False());
        #endif
#else
        supports->Set(String::NewSymbol("cairo"), False());
#endif

#if defined(HAVE_PNG)
        supports->Set(String::NewSymbol("png"), True());
#else
        supports->Set(String::NewSymbol("png"), False());
#endif

#if defined(HAVE_JPEG)
        supports->Set(String::NewSymbol("jpeg"), True());
#else
        supports->Set(String::NewSymbol("jpeg"), False());
#endif

#if defined(HAVE_TIFF)
        supports->Set(String::NewSymbol("tiff"), True());
#else
        supports->Set(String::NewSymbol("tiff"), False());
#endif

#if defined(HAVE_WEBP)
        supports->Set(String::NewSymbol("webp"), True());
#else
        supports->Set(String::NewSymbol("webp"), False());
#endif

#if defined(MAPNIK_USE_PROJ4)
        supports->Set(String::NewSymbol("proj4"), True());
#else
        supports->Set(String::NewSymbol("proj4"), False());
#endif

#if defined(MAPNIK_THREADSAFE)
        supports->Set(String::NewSymbol("threadsafe"), True());
#else
        supports->Set(String::NewSymbol("threadsafe"), False());
#endif

        target->Set(String::NewSymbol("supports"), supports);

#if MAPNIK_VERSION >= 200100
        Local<Object> composite_ops = Object::New();
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
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "grain_merge", mapnik::grain_merge)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "grain_extract", mapnik::grain_extract)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "hue", mapnik::hue)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "saturation", mapnik::saturation)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "color", mapnik::_color)
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "value", mapnik::_value)

        target->Set(String::NewSymbol("compositeOp"), composite_ops);
#endif
    }

}

} // namespace node_mapnik

// TODO - remove this hack at node v0.12.x
// https://github.com/joyent/node/commit/bd8a5755dceda415eee147bc06574f5a30abb0d0#commitcomment-5686345
#if NODE_VERSION_AT_LEAST(0, 9, 0)

#define NODE_MAPNIK_MODULE(modname, regfunc)                          \
  extern "C" {                                                        \
    MAPNIK_EXP node::node_module_struct modname ## _module =         \
    {                                                                 \
      NODE_STANDARD_MODULE_STUFF,                                     \
      (node::addon_register_func)regfunc,                             \
      NODE_STRINGIFY(modname)                                         \
    };                                                                \
  }

#else

#define NODE_MAPNIK_MODULE(modname, regfunc)                          \
  extern "C" {                                                        \
    MAPNIK_EXP node::node_module_struct modname ## _module =         \
    {                                                                 \
      NODE_STANDARD_MODULE_STUFF,                                     \
      regfunc,                             \
      NODE_STRINGIFY(modname)                                         \
    };                                                                \
  }

#endif

NODE_MAPNIK_MODULE(mapnik, node_mapnik::InitMapnik)
