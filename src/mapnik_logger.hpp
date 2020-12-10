#pragma once
#include <napi.h>

namespace mapnik {
class logger;
}

class Logger : public Napi::ObjectWrap<Logger>
{
  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);

    // ctor
    explicit Logger(Napi::CallbackInfo const& info);

    // Get and set functions
    // Are these the only methods available in logger?
    static Napi::Value get_severity(Napi::CallbackInfo const& info);
    static Napi::Value set_severity(Napi::CallbackInfo const& info);

  private:
    static Napi::FunctionReference constructor;
};
