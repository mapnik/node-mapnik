#include "utils.hpp"
#include "mapnik_cairo_surface.hpp"

Napi::FunctionReference CairoSurface::constructor;

Napi::Object CairoSurface::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "CairoSurface", {
            InstanceMethod<&CairoSurface::width>("width", prop_attr),
            InstanceMethod<&CairoSurface::height>("height", prop_attr),
            InstanceMethod<&CairoSurface::getData>("getData", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("CairoSurface", func);
    return exports;
}

// ctor
CairoSurface::CairoSurface(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<CairoSurface>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 3)
    {
        if (!info[0].IsString())
        {
            Napi::TypeError::New(env, "CairoSurface 'format' must be a string").ThrowAsJavaScriptException();
            return;
        }
        format_ = info[0].As<Napi::String>();
        if (!info[1].IsNumber() || !info[2].IsNumber())
        {
            Napi::TypeError::New(env, "CairoSurface 'width' and 'height' must be integers").ThrowAsJavaScriptException();
            return;
        }
        width_ = info[1].As<Napi::Number>().Int32Value();
        height_ = info[2].As<Napi::Number>().Int32Value();
    }
    else
    {
        Napi::Error::New(env, "CairoSurface requires three arguments: format, width, and height").ThrowAsJavaScriptException();
    }
}

Napi::Value CairoSurface::width(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, width_);
}

Napi::Value CairoSurface::height(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, height_);
}

Napi::Value CairoSurface::getData(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if (!data_.empty()) return scope.Escape(Napi::String::New(env, data_));
    std::string str = stream_.str();
    return scope.Escape(Napi::String::New(env, str));
}
