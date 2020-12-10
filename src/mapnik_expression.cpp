#include "utils.hpp"
#include "mapnik_expression.hpp"
#include "mapnik_feature.hpp"
#include "object_to_container.hpp"

// mapnik
#include <mapnik/attribute.hpp>
#include <mapnik/expression_string.hpp>
#include <mapnik/expression_evaluator.hpp>

Napi::FunctionReference Expression::constructor;

Napi::Object Expression::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Expression", {
            InstanceMethod<&Expression::evaluate>("evaluate", prop_attr),
            InstanceMethod<&Expression::toString>("toString", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Expression", func);
    return exports;
}

Expression::Expression(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Expression>(info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "invalid arguments: accepts a single argument of string type").ThrowAsJavaScriptException();
        return;
    }
    try
    {
        expression_ = mapnik::parse_expression(info[0].As<Napi::String>());
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
}

Napi::Value Expression::toString(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    Napi::String str = Napi::String::New(env, mapnik::to_expression_string(*expression_));
    return scope.Escape(str);
}

Napi::Value Expression::evaluate(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() < 1)
    {
        Napi::Error::New(env, "requires a mapnik.Feature as an argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (!info[0].IsObject())
    {
        Napi::TypeError::New(env, "first argument is invalid, must be a mapnik.Feature").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.InstanceOf(Feature::constructor.Value()))
    {
        Napi::TypeError::New(env, "first argument is invalid, must be a mapnik.Feature").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Feature* f = Napi::ObjectWrap<Feature>::Unwrap(obj);

    mapnik::attributes vars;
    if (info.Length() > 1)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[1].As<Napi::Object>();

        if (options.Has("variables"))
        {
            Napi::Value bind_opt = options.Get("variables");
            if (!bind_opt.IsObject())
            {
                Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            object_to_container(vars, bind_opt.As<Napi::Object>());
        }
    }
    using namespace mapnik;
    value val = util::apply_visitor(mapnik::evaluate<feature_impl, value, attributes>(*f->impl(), vars), *expression_);
    return scope.Escape(util::apply_visitor(node_mapnik::value_converter(env), val));
}
