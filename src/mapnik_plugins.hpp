#ifndef __NODE_MAPNIK_PLUGINS_H__
#define __NODE_MAPNIK_PLUGINS_H__


// v8
#include <v8.h>

// node
#include <node.h>

// mapnik
#include <mapnik/datasource_cache.hpp>

// stl
#include <vector>

#include "utils.hpp"

using namespace v8;
using namespace node;

namespace node_mapnik {


static inline Handle<Value> available_input_plugins(const Arguments& args)
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


static inline Handle<Value> register_datasources(const Arguments& args)
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
        return scope.Close(True());
    return scope.Close(False());
}


}

#endif // __NODE_MAPNIK_PLUGINS_H__
