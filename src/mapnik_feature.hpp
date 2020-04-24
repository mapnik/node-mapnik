#ifndef __NODE_MAPNIK_FEATURE_H__
#define __NODE_MAPNIK_FEATURE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

#include <memory>

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/feature.hpp>



class Feature : public Napi::ObjectWrap<Feature> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(const Napi::CallbackInfo& info);
    static Napi::Value NewInstance(mapnik::feature_ptr f_ptr);
    static Napi::Value fromJSON(const Napi::CallbackInfo& info);
    static Napi::Value id(const Napi::CallbackInfo& info);
    static Napi::Value extent(const Napi::CallbackInfo& info);
    static Napi::Value attributes(const Napi::CallbackInfo& info);
    static Napi::Value geometry(const Napi::CallbackInfo& info);
    static Napi::Value toJSON(const Napi::CallbackInfo& info);

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
