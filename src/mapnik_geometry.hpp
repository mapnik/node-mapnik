#ifndef __NODE_MAPNIK_GEOMETRY_H__
#define __NODE_MAPNIK_GEOMETRY_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

// mapnik
#include <mapnik/feature.hpp>



class Geometry: public Nan::ObjectWrap {
public:
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);
    static v8::Local<v8::Value> NewInstance(mapnik::feature_ptr f);
    static NAN_METHOD(type);
    static NAN_METHOD(extent);
    static NAN_METHOD(toWKB);
    static NAN_METHOD(toWKT);
    static v8::Local<v8::Value> _toJSONSync(Nan::NAN_METHOD_ARGS_TYPE info);
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
