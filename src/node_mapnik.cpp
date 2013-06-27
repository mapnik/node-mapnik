// v8
#include <v8.h>

// node
#include <node.h>
#include <node_version.h>

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
#include "mapnik_proj_transform.hpp"
#include "mapnik_layer.hpp"
#include "mapnik_datasource.hpp"
#include "mapnik_featureset.hpp"
//#include "mapnik_js_datasource.hpp"
#include "mapnik_memory_datasource.hpp"
#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_grid.hpp"
#include "mapnik_cairo_surface.hpp"
#include "mapnik_grid_view.hpp"
#include "mapnik_expression.hpp"
#include "utils.hpp"

#ifdef MAPNIK_DEBUG
#include <libxml/parser.h>
#endif

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
#if MAPNIK_VERSION >= 200200
    mapnik::marker_cache::instance().clear();
    mapnik::mapped_memory_cache::instance().clear();
#else
#if MAPNIK_VERSION >= 200100
    mapnik::marker_cache::instance()->clear();
    mapnik::mapped_memory_cache::instance()->clear();
#endif
#endif
    return Undefined();
}

static Handle<Value> shutdown(const Arguments& args)
{
    HandleScope scope;
    google::protobuf::ShutdownProtobufLibrary();
#ifdef MAPNIK_DEBUG
    // http://lists.fedoraproject.org/pipermail/devel/2010-January/129117.html
    xmlCleanupParser();
#endif
    return Undefined();
}

extern "C" {

    static void InitMapnik (Handle<Object> target)
    {
        GOOGLE_PROTOBUF_VERIFY_VERSION;

        // module level functions
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
        Expression::Initialize(target);
        CairoSurface::Initialize(target);

        // versions of deps
        Local<Object> versions = Object::New();
        versions->Set(String::NewSymbol("node"), String::New(NODE_VERSION+1));
        versions->Set(String::NewSymbol("v8"), String::New(V8::GetVersion()));
        versions->Set(String::NewSymbol("boost"), String::New(format_version(BOOST_VERSION).c_str()));
        versions->Set(String::NewSymbol("boost_number"), Integer::New(BOOST_VERSION));
        versions->Set(String::NewSymbol("mapnik"), String::New(format_version(MAPNIK_VERSION).c_str()));
        versions->Set(String::NewSymbol("mapnik_number"), Integer::New(MAPNIK_VERSION));
#if defined(HAVE_CAIRO)
        versions->Set(String::NewSymbol("cairo"), String::New(CAIRO_VERSION_STRING));
#endif
        target->Set(String::NewSymbol("versions"), versions);

        // built in support
        Local<Object> supports = Object::New();
        supports->Set(String::NewSymbol("grid"), True());

#if defined(HAVE_CAIRO)
        supports->Set(String::NewSymbol("cairo"), True());
#else
        supports->Set(String::NewSymbol("cairo"), False());
#endif

#if defined(HAVE_JPEG)
        supports->Set(String::NewSymbol("jpeg"), True());
#else
        supports->Set(String::NewSymbol("jpeg"), False());
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
        NODE_MAPNIK_DEFINE_CONSTANT(composite_ops, "invert_rgb", mapnik::invert_rgb)

        target->Set(String::NewSymbol("compositeOp"), composite_ops);
#endif
    }

}

} // namespace node_mapnik

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

NODE_MAPNIK_MODULE(_mapnik, node_mapnik::InitMapnik)
