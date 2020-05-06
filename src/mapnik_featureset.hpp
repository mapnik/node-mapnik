#pragma once

#include <mapnik/featureset.hpp>
#include <memory>

#include <napi.h>


using featureset_ptr =  mapnik::featureset_ptr;

class Featureset : public Napi::ObjectWrap<Featureset>
{
public:
    // initialiser
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports);
    // ctor
    explicit Featureset(Napi::CallbackInfo const& info);
    // methods
    Napi::Value next(Napi::CallbackInfo const& info);
private:
    static Napi::FunctionReference constructor;
    featureset_ptr featureset_;
};
