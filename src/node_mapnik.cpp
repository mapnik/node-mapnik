// v8
#include <v8.h>

// node
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

// node-mapnik
#include "mapnik_map.hpp"
#include "mapnik_color.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_fonts.hpp"
#include "mapnik_plugins.hpp"
#include "mapnik_projection.hpp"
#include "mapnik_layer.hpp"
#include "mapnik_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "mapnik_js_datasource.hpp"
#include "mapnik_memory_datasource.hpp"
#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_grid.hpp"
#include "mapnik_grid_view.hpp"
#include "utils.hpp"

// mapnik
#include <mapnik/version.hpp>

// provides MAPNIK_SUPPORTS_GRID_RENDERER
#include <mapnik/config.hpp>

// boost
#include <boost/version.hpp>

// cairo
#if defined(HAVE_CAIRO)
#include <cairo-version.h>
#endif

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

extern "C" {

  static void init (Handle<Object> target)
  {
    // module level functions
    NODE_SET_METHOD(target, "make_mapnik_symbols_visible", node_mapnik::make_mapnik_symbols_visible);
    NODE_SET_METHOD(target, "register_datasources", node_mapnik::register_datasources);
    NODE_SET_METHOD(target, "datasources", node_mapnik::available_input_plugins);
    NODE_SET_METHOD(target, "register_fonts", node_mapnik::register_fonts);
    NODE_SET_METHOD(target, "fonts", node_mapnik::available_font_faces);
    NODE_SET_METHOD(target, "gc", gc);

    // Classes
    Map::Initialize(target);
    Color::Initialize(target);
    Feature::Initialize(target);
    Image::Initialize(target);
    ImageView::Initialize(target);
    Projection::Initialize(target);
    Layer::Initialize(target);
    Grid::Initialize(target);
    GridView::Initialize(target);
    Datasource::Initialize(target);
    Featureset::Initialize(target);
    JSDatasource::Initialize(target);
    MemoryDatasource::Initialize(target);

    // node-mapnik version
    target->Set(String::NewSymbol("version"), String::New("0.4.0"));

    // versions of deps
    Local<Object> versions = Object::New();
    versions->Set(String::NewSymbol("node"), String::New(NODE_VERSION+1));
    versions->Set(String::NewSymbol("v8"), String::New(V8::GetVersion()));
    versions->Set(String::NewSymbol("boost"), String::New(format_version(BOOST_VERSION).c_str()));
    versions->Set(String::NewSymbol("boost_number"), Integer::New(BOOST_VERSION));
    versions->Set(String::NewSymbol("mapnik"), String::New(format_version(MAPNIK_VERSION).c_str()));
    versions->Set(String::NewSymbol("mapnik_number"), Integer::New(MAPNIK_VERSION));
    #if defined(HAVE_CAIRO)
      std::ostringstream s;
      s << CAIRO_VERSION_MAJOR << "." << CAIRO_VERSION_MINOR << "." << CAIRO_VERSION_MICRO;
      versions->Set(String::NewSymbol("cairo"), String::New(s.str().c_str()));
    #endif
    target->Set(String::NewSymbol("versions"), versions);

    // built in support
    Local<Object> supports = Object::New();
    #if defined(HAVE_CAIRO)
      supports->Set(String::NewSymbol("cairo"), Boolean::New(true));
    #else
      supports->Set(String::NewSymbol("cairo"), Boolean::New(false));
    #endif
    #if defined(HAVE_JPEG)
      supports->Set(String::NewSymbol("jpeg"), Boolean::New(true));
    #else
      supports->Set(String::NewSymbol("jpeg"), Boolean::New(false));
    #endif
    #if defined(MAPNIK_SUPPORTS_GRID_RENDERER)
      supports->Set(String::NewSymbol("grid"), Boolean::New(true));
    #else
      #warning("you are building against a version of mapnik2 without grid_renderer support, upgrading to at least >= r2898 of mapnik trunk/mapnik2 is required for grid rendering support")
      supports->Set(String::NewSymbol("grid"), Boolean::New(false));
    #endif
    target->Set(String::NewSymbol("supports"), supports);

  }

  NODE_MODULE(_mapnik, init)
}
