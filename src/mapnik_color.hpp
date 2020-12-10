#pragma once

#include <napi.h>
#include <mapnik/color.hpp>

class Color : public Napi::ObjectWrap<Color>
{
    friend class Image;
    friend class ImageView;
    friend class Map;

  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit Color(Napi::CallbackInfo const& info);
    // methods
    Napi::Value toString(Napi::CallbackInfo const& info);
    Napi::Value hex(Napi::CallbackInfo const& info);
    // accessors
    Napi::Value red(Napi::CallbackInfo const& info);
    void red(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value green(Napi::CallbackInfo const& info);
    void green(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value blue(Napi::CallbackInfo const& info);
    void blue(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value alpha(Napi::CallbackInfo const& info);
    void alpha(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value premultiplied(Napi::CallbackInfo const& info);
    void premultiplied(Napi::CallbackInfo const& info, Napi::Value const& value);

  private:
    static Napi::FunctionReference constructor;
    mapnik::color color_;
};
