#pragma once

#include <napi.h>
// mapnik
#include <mapnik/expression.hpp>

class Expression : public Napi::ObjectWrap<Expression> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);
    static Napi::Value toString(Napi::CallbackInfo const& info);
    static Napi::Value evaluate(Napi::CallbackInfo const& info);

    Expression();
    inline mapnik::expression_ptr get() { return this_; }

private:
    mapnik::expression_ptr this_;
};
