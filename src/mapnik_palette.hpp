#pragma once

#include <napi.h>
#include <mapnik/palette.hpp>
#include <memory>

using palette_ptr =  std::shared_ptr<mapnik::rgba_palette>;

class Palette : public Napi::ObjectWrap<Palette>
{
    friend class Image;
public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports);
    // ctor
    explicit Palette(Napi::CallbackInfo const& info);
    // methods
    Napi::Value toString(Napi::CallbackInfo const& info);
    Napi::Value toBuffer(Napi::CallbackInfo const& info);
private:
    static Napi::FunctionReference constructor;
    palette_ptr palette_;
};
