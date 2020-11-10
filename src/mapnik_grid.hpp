#pragma once

#if defined(GRID_RENDERER)

#include <napi.h>
// mapnik
#include <mapnik/grid/grid.hpp>
// stl
#include <memory>

using grid_ptr = std::shared_ptr<mapnik::grid>;

class Grid : public Napi::ObjectWrap<Grid>
{
  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit Grid(Napi::CallbackInfo const& info);
    // methods
    Napi::Value encodeSync(Napi::CallbackInfo const& info);
    Napi::Value encode(Napi::CallbackInfo const& info);
    Napi::Value addField(Napi::CallbackInfo const& info);
    Napi::Value fields(Napi::CallbackInfo const& info);
    Napi::Value view(Napi::CallbackInfo const& info);
    Napi::Value width(Napi::CallbackInfo const& info);
    Napi::Value height(Napi::CallbackInfo const& info);
    Napi::Value painted(Napi::CallbackInfo const& info);
    Napi::Value clearSync(Napi::CallbackInfo const& info);
    Napi::Value clear(Napi::CallbackInfo const& info);
    Napi::Value key(Napi::CallbackInfo const& info);
    void key(Napi::CallbackInfo const& info, const Napi::Value& value);
    inline grid_ptr impl() const { return grid_; }
    static Napi::FunctionReference constructor;

  private:
    grid_ptr grid_;
};

#endif
