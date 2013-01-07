#ifndef __NODE_MAPNIK_FEATURE_H__
#define __NODE_MAPNIK_FEATURE_H__

#include <v8.h>
#include <node_object_wrap.h>

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/datasource.hpp> // feature_ptr and featureset_ptr

// boost
#include <boost/shared_ptr.hpp>

using namespace v8;

class Feature: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> New(mapnik::feature_ptr f_ptr);
    static Handle<Value> id(const Arguments &args);
    static Handle<Value> extent(const Arguments &args);
    static Handle<Value> attributes(const Arguments &args);
    static Handle<Value> addGeometry(const Arguments &args);
    static Handle<Value> addAttributes(const Arguments &args);
    static Handle<Value> numGeometries(const Arguments &args);
    static Handle<Value> toString(const Arguments &args);
    static Handle<Value> toJSON(const Arguments &args);

    // todo
    // how to allow altering of attributes
    // expose get_geometry
    Feature(mapnik::feature_ptr f);
    Feature(int id);
    inline mapnik::feature_ptr get() { return this_; }

private:
    ~Feature();
    mapnik::feature_ptr this_;
#if MAPNIK_VERSION >= 200100
    mapnik::context_ptr ctx_;
#endif
};

#endif
