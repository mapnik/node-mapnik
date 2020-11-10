#pragma once

#include <napi.h>
// mapnik
#include <mapnik/expression.hpp>

class Expression : public Napi::ObjectWrap<Expression>
{
  public:
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    explicit Expression(Napi::CallbackInfo const& info);
    Napi::Value toString(Napi::CallbackInfo const& info);
    Napi::Value evaluate(Napi::CallbackInfo const& info);

  private:
    static Napi::FunctionReference constructor;
    mapnik::expression_ptr expression_;
};
