#pragma once

#include <napi.h>
// stl
#include <string>
#include <memory>

namespace mapnik {
class proj_transform;
}
namespace mapnik {
class projection;
}

using proj_ptr = std::shared_ptr<mapnik::projection>;

class Projection : public Napi::ObjectWrap<Projection>
{
    friend class ProjTransform;

  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctorx
    explicit Projection(Napi::CallbackInfo const& info);
    // methods
    Napi::Value inverse(Napi::CallbackInfo const& info);
    Napi::Value forward(Napi::CallbackInfo const& info);

  private:
    static Napi::FunctionReference constructor;
    proj_ptr projection_;
};

using proj_tr_ptr = std::shared_ptr<mapnik::proj_transform>;

class ProjTransform : public Napi::ObjectWrap<ProjTransform>
{
    friend class Geometry;

  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit ProjTransform(Napi::CallbackInfo const& info);
    // methods
    Napi::Value forward(Napi::CallbackInfo const& info);
    Napi::Value backward(Napi::CallbackInfo const& info);
    inline proj_tr_ptr impl() { return proj_transform_; }

  private:
    static Napi::FunctionReference constructor;
    proj_tr_ptr proj_transform_;
};
