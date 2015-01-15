#ifndef __NODE_MAPNIK_GEOMETRY_H__
#define __NODE_MAPNIK_GEOMETRY_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

// mapnik
#include <mapnik/feature.hpp>

using namespace v8;

class Geometry: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static Handle<Value> NewInstance(mapnik::feature_ptr f);
    static NAN_METHOD(extent);
    static NAN_METHOD(toWKB);
    static NAN_METHOD(toWKT);
    static Local<Value> _toJSONSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(toJSON);
    static NAN_METHOD(toJSONSync);
    static void to_json(uv_work_t* req);
    static void after_to_json(uv_work_t* req);
    Geometry(mapnik::feature_ptr f);
private:
    ~Geometry();
    mapnik::feature_ptr feat_;
};

#endif
