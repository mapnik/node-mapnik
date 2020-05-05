#ifndef __NODE_MAPNIK_GEOMETRY_H__
#define __NODE_MAPNIK_GEOMETRY_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

// mapnik
#include <mapnik/feature.hpp>



class Geometry : public Napi::ObjectWrap<Geometry> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);
    static Napi::Value NewInstance(mapnik::feature_ptr f);
    static Napi::Value type(Napi::CallbackInfo const& info);
    static Napi::Value extent(Napi::CallbackInfo const& info);
    static Napi::Value toWKB(Napi::CallbackInfo const& info);
    static Napi::Value toWKT(Napi::CallbackInfo const& info);
    static Napi::Value _toJSONSync(Napi::CallbackInfo const& info);
    static Napi::Value toJSON(Napi::CallbackInfo const& info);
    static Napi::Value toJSONSync(Napi::CallbackInfo const& info);
    static void to_json(uv_work_t* req);
    static void after_to_json(uv_work_t* req);
    Geometry(mapnik::feature_ptr f);
private:
    ~Geometry();
    mapnik::feature_ptr feat_;
};

#endif
