#ifndef __NODE_MAPNIK_GEOMETRY_H__
#define __NODE_MAPNIK_GEOMETRY_H__

#include <v8.h>
#include <node_object_wrap.h>

// mapnik
#include <mapnik/geometry.hpp>
#include "mapnik3x_compatibility.hpp"

// boost
#include MAPNIK_SHARED_INCLUDE

using namespace v8;

typedef MAPNIK_SHARED_PTR<mapnik::geometry_type> geometry_ptr;

class Geometry: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> fromWKT(const Arguments &args);
    static Handle<Value> extent(const Arguments &args);
    static Handle<Value> type(const Arguments &args);

    Geometry();
    inline geometry_ptr get() { return this_; }

private:
    ~Geometry();
    geometry_ptr this_;
};

#endif
