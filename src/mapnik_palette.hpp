#pragma once

#include <napi.h>
#include <mapnik/palette.hpp>
#include <memory>

using palette_ptr = std::shared_ptr<mapnik::rgba_palette>;

class Palette : public Napi::ObjectWrap<Palette>
{
    friend class Image;
    friend class ImageView;
    friend class Map;

  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit Palette(Napi::CallbackInfo const& info);
    // methods
    Napi::Value toString(Napi::CallbackInfo const& info);
    Napi::Value toBuffer(Napi::CallbackInfo const& info);
    inline palette_ptr palette() { return palette_; }

  private:
    static Napi::FunctionReference constructor;
    palette_ptr palette_;
};
