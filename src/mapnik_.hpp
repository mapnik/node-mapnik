#ifndef __NODE_MAPNIK_•_H__
#define __NODE_MAPNIK_•_H__

#include <v8.h>
#include <node_object_wrap.h>

// mapnik
#include "mapnik3x_compatibility.hpp"
#include <mapnik/•.hpp>

// boost
#include MAPNIK_SHARED_INCLUDE

using namespace v8;

typedef MAPNIK_SHARED_PTR<mapnik::•> •_ptr;

class •: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    •(std::string const& name);
    •(std::string const& name, std::string const& srs);

private:
    ~•();
    •_ptr this_;
};

#endif
