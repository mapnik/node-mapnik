#pragma once

#include <napi.h>
// stl
#include <memory>

namespace mapnik {
class layer;
}

using layer_ptr = std::shared_ptr<mapnik::layer>;

class Layer : public Napi::ObjectWrap<Layer>
{
    friend class Map;

  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit Layer(Napi::CallbackInfo const& info);
    // methods
    Napi::Value describe(Napi::CallbackInfo const& info);
    // accessors
    Napi::Value name(Napi::CallbackInfo const& info);
    void name(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value styles(Napi::CallbackInfo const& info);
    void styles(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value active(Napi::CallbackInfo const& info);
    void active(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value srs(Napi::CallbackInfo const& info);
    void srs(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value datasource(Napi::CallbackInfo const& info);
    void datasource(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value minimum_scale_denominator(Napi::CallbackInfo const& info);
    void minimum_scale_denominator(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value maximum_scale_denominator(Napi::CallbackInfo const& info);
    void maximum_scale_denominator(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value queryable(Napi::CallbackInfo const& info);
    void queryable(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value clear_label_cache(Napi::CallbackInfo const& info);
    void clear_label_cache(Napi::CallbackInfo const& info, Napi::Value const& value);
    inline layer_ptr impl() const { return layer_; }

  private:
    static Napi::FunctionReference constructor;
    layer_ptr layer_;
};
