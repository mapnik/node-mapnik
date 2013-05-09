#ifndef __NODE_MAPNIK_SVG_H__
#define __NODE_MAPNIK_SVG_H__

#include <v8.h>
#include <node_object_wrap.h>

// mapnik
#include <mapnik/marker.hpp>

using namespace v8;

class SVG: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static SVG * New(mapnik::svg_path_ptr this_);
    static Handle<Value> open(const Arguments &args);
    static Handle<Value> fromString(const Arguments &args);
    static Handle<Value> width(const Arguments &args);
    static Handle<Value> height(const Arguments &args);
    static Handle<Value> extent(const Arguments &args);

    SVG();
    inline mapnik::svg_path_ptr get() { return this_; }

private:
    ~SVG();
    mapnik::svg_path_ptr this_;
};

#endif
