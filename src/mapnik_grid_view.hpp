#pragma once

#if defined(GRID_RENDERER)

#include <napi.h>
// mapnik
#include <mapnik/grid/grid_view.hpp>
// stl
#include <memory>
#include "mapnik_grid.hpp"

class Grid;

typedef std::shared_ptr<mapnik::grid_view> grid_view_ptr;

class GridView : public Napi::ObjectWrap<GridView>
{
    friend class Grid;

  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit GridView(Napi::CallbackInfo const& info);
    // methods
    Napi::Value encodeSync(Napi::CallbackInfo const& info);
    Napi::Value encode(Napi::CallbackInfo const& info);
    Napi::Value fields(Napi::CallbackInfo const& info);
    Napi::Value width(Napi::CallbackInfo const& info);
    Napi::Value height(Napi::CallbackInfo const& info);
    Napi::Value isSolid(Napi::CallbackInfo const& info);
    Napi::Value isSolidSync(Napi::CallbackInfo const& info);
    Napi::Value getPixel(Napi::CallbackInfo const& info);
    inline grid_view_ptr impl() const { return grid_view_; }

  private:
    static Napi::FunctionReference constructor;
    grid_view_ptr grid_view_;
    grid_ptr grid_;
};

#endif
