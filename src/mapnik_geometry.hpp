#pragma once

// mapnik
#include <mapnik/feature.hpp>
//
#include <napi.h>

class Geometry : public Napi::ObjectWrap<Geometry>
{
    friend class Feature;

  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit Geometry(Napi::CallbackInfo const& info);
    // methods
    Napi::Value type(Napi::CallbackInfo const& info);
    Napi::Value extent(Napi::CallbackInfo const& info);
    Napi::Value toWKB(Napi::CallbackInfo const& info);
    Napi::Value toWKT(Napi::CallbackInfo const& info);
    Napi::Value toJSON(Napi::CallbackInfo const& info);
    Napi::Value toJSONSync(Napi::CallbackInfo const& info);
    inline mapnik::geometry::geometry<double> const& geometry()
    {
        return feature_->get_geometry();
    }

  private:
    static Napi::FunctionReference constructor;
    mapnik::feature_ptr feature_;
};
