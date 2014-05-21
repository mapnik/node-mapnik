#ifndef __NODE_MAPNIK_PLUGINS_H__
#define __NODE_MAPNIK_PLUGINS_H__


// mapnik
#include <mapnik/datasource_cache.hpp>
#include <mapnik/version.hpp>

// stl
#include <vector>
#include "mapnik3x_compatibility.hpp"
#include "utils.hpp"

using namespace v8;

namespace node_mapnik {


static inline NAN_METHOD(available_input_plugins)
{
    NanScope();
#if MAPNIK_VERSION >= 200200
    std::vector<std::string> names = mapnik::datasource_cache::instance().plugin_names();
#else
    std::vector<std::string> names = mapnik::datasource_cache::instance()->plugin_names();
#endif
    Local<Array> a = NanNew<Array>(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        a->Set(i, NanNew(names[i].c_str()));
    }
    NanReturnValue(a);
}

static inline NAN_METHOD(register_datasource)
{
    NanScope();
    if (args.Length() != 1 || !args[0]->IsString())
    {
        NanThrowTypeError("first argument must be a path to a mapnik input plugin (.input)");
        NanReturnUndefined();
    }
#if MAPNIK_VERSION >= 200200
    std::vector<std::string> names_before = mapnik::datasource_cache::instance().plugin_names();
#else
    std::vector<std::string> names_before = mapnik::datasource_cache::instance()->plugin_names();
#endif
    std::string path = TOSTR(args[0]);
#if MAPNIK_VERSION >= 200200
    mapnik::datasource_cache::instance().register_datasource(path);
    std::vector<std::string> names_after = mapnik::datasource_cache::instance().plugin_names();
#else
    mapnik::datasource_cache::instance()->register_datasource(path);
    std::vector<std::string> names_after = mapnik::datasource_cache::instance()->plugin_names();
#endif
    if (names_after.size() > names_before.size())
        NanReturnValue(NanTrue());
    NanReturnValue(NanFalse());
}

static inline NAN_METHOD(register_datasources)
{
    NanScope();
    if (args.Length() != 1 || !args[0]->IsString())
    {
        NanThrowTypeError("first argument must be a path to a directory of mapnik input plugins");
        NanReturnUndefined();
    }
#if MAPNIK_VERSION >= 200200
    std::vector<std::string> names_before = mapnik::datasource_cache::instance().plugin_names();
#else
    std::vector<std::string> names_before = mapnik::datasource_cache::instance()->plugin_names();
#endif
    std::string path = TOSTR(args[0]);
#if MAPNIK_VERSION >= 200200
    mapnik::datasource_cache::instance().register_datasources(path);
    std::vector<std::string> names_after = mapnik::datasource_cache::instance().plugin_names();
#else
    mapnik::datasource_cache::instance()->register_datasources(path);
    std::vector<std::string> names_after = mapnik::datasource_cache::instance()->plugin_names();
#endif
    if (names_after.size() > names_before.size())
        NanReturnValue(NanTrue());
    NanReturnValue(NanFalse());
}


}

#endif // __NODE_MAPNIK_PLUGINS_H__
