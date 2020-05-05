#pragma once

#include <napi.h>
#include <memory>

namespace mapnik { class datasource; }

using datasroue_ptr = std::shared_ptr<mapnik::datasource>;

class Datasource : public Napi::ObjectWrap<Datasource>
{
public:
    //initilizer
    static void Initialize(Napi::Env enc, Napi::Object exports);
    // ctor
    explicit Datasource(Napi::CallbackInfo const& info);
    static Napi::Value NewInstance(datasource_ptr ds_ptr);

    static Napi::Value parameters(Napi::CallbackInfo const& info);
    static Napi::Value describe(Napi::CallbackInfo const& info);
    static Napi::Value featureset(Napi::CallbackInfo const& info);
    static Napi::Value extent(Napi::CallbackInfo const& info);
    static Napi::Value fields(Napi::CallbackInfo const& info);

    Datasource();
    inline datasource_ptr get() { return datasource_; }

private:
    static Napi::FunctionReference constructor;
    datasource_ptr datasource_;
};
