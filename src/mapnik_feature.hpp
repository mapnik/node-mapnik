#ifndef __NODE_MAPNIK_FEATURE_H__
#define __NODE_MAPNIK_FEATURE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <memory>

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/feature.hpp>

using namespace v8;

class Feature: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static Handle<Value> NewInstance(mapnik::feature_ptr f_ptr);
    static NAN_METHOD(fromJSON);
    static NAN_METHOD(id);
    static NAN_METHOD(extent);
    static NAN_METHOD(attributes);
    static NAN_METHOD(geometry);
    static NAN_METHOD(toJSON);

    // todo
    // how to allow altering of attributes
    // expose get_geometry
    Feature(mapnik::feature_ptr f);
    Feature(int id);
    inline mapnik::feature_ptr get() { return this_; }

private:
    ~Feature();
    mapnik::feature_ptr this_;
    mapnik::context_ptr ctx_;
};

#endif
