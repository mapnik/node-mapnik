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
    static Napi::Value New(const Napi::CallbackInfo& info);
    static Napi::Value NewInstance(mapnik::feature_ptr f);
    static Napi::Value type(const Napi::CallbackInfo& info);
    static Napi::Value extent(const Napi::CallbackInfo& info);
    static Napi::Value toWKB(const Napi::CallbackInfo& info);
    static Napi::Value toWKT(const Napi::CallbackInfo& info);
    static Napi::Value _toJSONSync(const Napi::CallbackInfo& info);
    static Napi::Value toJSON(const Napi::CallbackInfo& info);
    static Napi::Value toJSONSync(const Napi::CallbackInfo& info);
    static void to_json(uv_work_t* req);
    static void after_to_json(uv_work_t* req);
    Geometry(mapnik::feature_ptr f);
private:
    ~Geometry();
    mapnik::feature_ptr feat_;
};

#endif
