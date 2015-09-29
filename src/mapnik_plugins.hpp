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
