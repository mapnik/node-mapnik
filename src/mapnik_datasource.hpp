#pragma once

#include <napi.h>
#include <memory>

namespace mapnik { class datasource; }

using datasource_ptr = std::shared_ptr<mapnik::datasource>;

class Datasource : public Napi::ObjectWrap<Datasource>
{
public:
    //initilizer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports);
    // ctor
    explicit Datasource(Napi::CallbackInfo const& info);
    // methods
    Napi::Value parameters(Napi::CallbackInfo const& info);
    Napi::Value describe(Napi::CallbackInfo const& info);
    //Napi::Value featureset(Napi::CallbackInfo const& info);
    Napi::Value extent(Napi::CallbackInfo const& info);
    Napi::Value fields(Napi::CallbackInfo const& info);
    //inline datasource_ptr get() { return datasource_; }
private:
    static Napi::FunctionReference constructor;
    datasource_ptr datasource_;
};
