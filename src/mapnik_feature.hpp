#pragma once
// stl
#include <memory>

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/feature.hpp>
//
#include <napi.h>

class Feature : public Napi::ObjectWrap<Feature>
{
    friend class Featureset;
    friend class Expression;
public:
    // initialiser
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports);
    // ctor
    explicit Feature(Napi::CallbackInfo const& info);
    // methods
    static Napi::Value fromJSON(Napi::CallbackInfo const& info);
    Napi::Value id(Napi::CallbackInfo const& info);
    Napi::Value extent(Napi::CallbackInfo const& info);
    Napi::Value attributes(Napi::CallbackInfo const& info);
    Napi::Value geometry(Napi::CallbackInfo const& info);
    Napi::Value toJSON(Napi::CallbackInfo const& info);
    inline mapnik::feature_ptr impl() const {return feature_;}
private:
    static Napi::FunctionReference constructor;
    mapnik::feature_ptr feature_;
    mapnik::context_ptr ctx_;
};
