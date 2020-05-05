#include "utils.hpp"
#include "mapnik_cairo_surface.hpp"


Napi::FunctionReference CairoSurface::constructor;

void CairoSurface::Initialize(Napi::Object target) {
    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, CairoSurface::New);

    lcons->SetClassName(Napi::String::New(env, "CairoSurface"));
    InstanceMethod("width", &width),
    InstanceMethod("height", &height),
    InstanceMethod("getData", &getData),
    (target).Set(Napi::String::New(env, "CairoSurface"), Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

CairoSurface::CairoSurface(std::string const& format, unsigned int width, unsigned int height) : Napi::ObjectWrap<CairoSurface>(),
    ss_(),
    width_(width),
    height_(height),
    format_(format)
{
}

CairoSurface::~CairoSurface()
{
}

Napi::Value CairoSurface::New(Napi::CallbackInfo const& info)
{
    if (!info.IsConstructCall())
    {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info[0].IsExternal())
    {
        // Currently there is no C++ that executes this call
        /* LCOV_EXCL_START */
        Napi::External ext = info[0].As<Napi::External>();
        void* ptr = ext->Value();
        CairoSurface* im =  static_cast<CairoSurface*>(ptr);
        im->Wrap(info.This());
        return info.This();
        return;
        /* LCOV_EXCL_STOP */
    }

    if (info.Length() == 3)
    {
        if (!info[0].IsString())
        {
            Napi::TypeError::New(env, "CairoSurface 'format' must be a string").ThrowAsJavaScriptException();
            return env.Null();
        }
        std::string format = TOSTR(info[0]);
        if (!info[1].IsNumber() || !info[2].IsNumber())
        {
            Napi::TypeError::New(env, "CairoSurface 'width' and 'height' must be a integers").ThrowAsJavaScriptException();
            return env.Null();
        }
        CairoSurface* im = new CairoSurface(format, info[1].As<Napi::Number>().Int32Value(), info[2].As<Napi::Number>().Int32Value());
        im->Wrap(info.This());
        return info.This();
        return;
    }
    else
    {
        Napi::Error::New(env, "CairoSurface requires three arguments: format, width, and height").ThrowAsJavaScriptException();
        return env.Null();
    }
    return;
}

Napi::Value CairoSurface::width(Napi::CallbackInfo const& info)
{
    CairoSurface* im = info.Holder().Unwrap<CairoSurface>();
    return Napi::New(env, im->width());
}

Napi::Value CairoSurface::height(Napi::CallbackInfo const& info)
{
    CairoSurface* im = info.Holder().Unwrap<CairoSurface>();
    return Napi::New(env, im->height());
}

Napi::Value CairoSurface::getData(Napi::CallbackInfo const& info)
{
    CairoSurface* surface = info.Holder().Unwrap<CairoSurface>();
    std::string s = surface->ss_.str();
    return Napi::Buffer::Copy(env, (char*)s.data(), s.size());
}
