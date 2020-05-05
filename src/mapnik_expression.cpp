#include "utils.hpp"
#include "mapnik_expression.hpp"
#include "mapnik_feature.hpp"
#include "utils.hpp"
#include "object_to_container.hpp"

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/attribute.hpp>
#include <mapnik/expression_string.hpp>
#include <mapnik/expression_evaluator.hpp>

// stl
#include <exception>                    // for exception

Napi::FunctionReference Expression::constructor;

void Expression::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, Expression::New);

    lcons->SetClassName(Napi::String::New(env, "Expression"));

    InstanceMethod("toString", &toString),
    InstanceMethod("evaluate", &evaluate),

    (target).Set(Napi::String::New(env, "Expression"), Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

Expression::Expression() : Napi::ObjectWrap<Expression>(),
    this_() {}

Expression::~Expression()
{
}

Napi::Value Expression::New(Napi::CallbackInfo const& info)
{
    if (!info.IsConstructCall())
    {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    mapnik::expression_ptr e_ptr;
    try
    {
        if (info.Length() == 1 && info[0].IsString()) {
            e_ptr = mapnik::parse_expression(TOSTR(info[0]));
        } else {
            Napi::TypeError::New(env, "invalid arguments: accepts a single argument of string type").ThrowAsJavaScriptException();
            return env.Null();
        }
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    Expression* e = new Expression();
    e->Wrap(info.This());
    e->this_ = e_ptr;
    return info.This();
}

Napi::Value Expression::toString(Napi::CallbackInfo const& info)
{
    Expression* e = info.Holder().Unwrap<Expression>();
    return Napi::New(env, mapnik::to_expression_string(*e->get()));
}

Napi::Value Expression::evaluate(Napi::CallbackInfo const& info)
{
    if (info.Length() < 1) {
        Napi::Error::New(env, "requires a mapnik.Feature as an argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsObject())
    {
        Napi::TypeError::New(env, "first argument is invalid, must be a mapnik.Feature").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!Napi::New(env, Feature::constructor)->HasInstance(info[0])) {
        Napi::TypeError::New(env, "first argument is invalid, must be a mapnik.Feature").ThrowAsJavaScriptException();
        return env.Null();
    }

    Feature* f = info[0].As<Napi::Object>().Unwrap<Feature>();

    Expression* e = info.Holder().Unwrap<Expression>();
    mapnik::attributes vars;
    if (info.Length() > 1)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }
        Napi::Object options = info[1].As<Napi::Object>();

        if ((options).Has(Napi::String::New(env, "variables")).FromMaybe(false))
        {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "variables"));
            if (!bind_opt.IsObject())
            {
                Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
                return env.Null();
            }
            object_to_container(vars,bind_opt->ToObject(Napi::GetCurrentContext()));
        }
    }
    mapnik::value value_obj = mapnik::util::apply_visitor(mapnik::evaluate<mapnik::feature_impl,mapnik::value,mapnik::attributes>(*(f->get()),vars),*(e->get()));
    return mapnik::util::apply_visitor(node_mapnik::value_converter(),value_obj);
}
