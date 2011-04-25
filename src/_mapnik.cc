// v8
#include <v8.h>

// node
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

/* dlopen(), dlsym() */
#include <dlfcn.h>

// boost
#include "utils.hpp"

// node-mapnik
#include "mapnik_map.hpp"
#include "mapnik_projection.hpp"
#include "mapnik_layer.hpp"
#include "mapnik_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "mapnik_js_datasource.hpp"
#include "mapnik_memory_datasource.hpp"

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/font_engine_freetype.hpp>

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

static Handle<Value> make_mapnik_symbols_visible(const Arguments& args)
{
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a path to a directory holding _mapnik.node")));
  String::Utf8Value filename(args[0]->ToString());
  void *handle = dlopen(*filename, RTLD_NOW|RTLD_GLOBAL);
  if (handle == NULL) {
      return False();
  }
  else
  {
      dlclose(handle);
      return True();
  }
}

static Handle<Value> register_datasources(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() != 1 || !args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a path to a directory of mapnik input plugins")));

    std::vector<std::string> const names_before = mapnik::datasource_cache::plugin_names();
    std::string const& path = TOSTR(args[0]);
    mapnik::datasource_cache::instance()->register_datasources(path);
    std::vector<std::string> const& names_after = mapnik::datasource_cache::plugin_names();
    if (names_after.size() > names_before.size())
        return scope.Close(Boolean::New(true));
    return scope.Close(Boolean::New(false));
}

static Handle<Value> available_input_plugins(const Arguments& args)
{
    HandleScope scope;
    std::vector<std::string> const& names = mapnik::datasource_cache::plugin_names();
    Local<Array> a = Array::New(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        a->Set(i, String::New(names[i].c_str()));
    }
    return scope.Close(a);
}

static Handle<Value> register_fonts(const Arguments& args)
{
  HandleScope scope;

  if (!args.Length() >= 1 || !args[0]->IsString())
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a path to a directory of fonts")));

  bool found = false;

  std::vector<std::string> const names_before = mapnik::freetype_engine::face_names();

  // option hash
  if (args.Length() == 2){
    if (!args[1]->IsObject())
      return ThrowException(Exception::TypeError(
        String::New("second argument is optional, but if provided must be an object, eg. { recurse:Boolean }")));

      Local<Object> options = args[1]->ToObject();
      if (options->Has(String::New("recurse")))
      {
          Local<Value> recurse_opt = options->Get(String::New("recurse"));
          if (!recurse_opt->IsBoolean())
            return ThrowException(Exception::TypeError(
              String::New("'recurse' must be a Boolean")));

          bool recurse = recurse_opt->BooleanValue();
          std::string const& path = TOSTR(args[0]);
          found = mapnik::freetype_engine::register_fonts(path,recurse);
      }
  }
  else
  {
      std::string const& path = TOSTR(args[0]);
      found = mapnik::freetype_engine::register_fonts(path);
  }

  std::vector<std::string> const& names_after = mapnik::freetype_engine::face_names();
  if (names_after.size() == names_before.size())
      found = false;

  return scope.Close(Boolean::New(found));
}

static Handle<Value> available_font_faces(const Arguments& args)
{
    HandleScope scope;
    std::vector<std::string> const& names = mapnik::freetype_engine::face_names();
    Local<Array> a = Array::New(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        a->Set(i, String::New(names[i].c_str()));
    }
    return scope.Close(a);
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
    NODE_SET_METHOD(target, "make_mapnik_symbols_visible", make_mapnik_symbols_visible);
    NODE_SET_METHOD(target, "register_datasources", register_datasources);
    NODE_SET_METHOD(target, "datasources", available_input_plugins);
    NODE_SET_METHOD(target, "register_fonts", register_fonts);
    NODE_SET_METHOD(target, "fonts", available_font_faces);
    NODE_SET_METHOD(target, "gc", gc);

    // Map
    Map::Initialize(target);

    // Projection
    Projection::Initialize(target);

    // Layer
    Layer::Initialize(target);

    // Datasource
    Datasource::Initialize(target);

    // Featureset
    Featureset::Initialize(target);

    // JSDatasource
    JSDatasource::Initialize(target);

    // MemoryDatasource
    MemoryDatasource::Initialize(target);

    // node-mapnik version
    target->Set(String::NewSymbol("version"), String::New("0.2.13"));

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
    target->Set(String::NewSymbol("supports"), supports);

  }

  NODE_MODULE(_mapnik, init)
}
