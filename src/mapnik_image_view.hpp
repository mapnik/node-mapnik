#pragma once

#include <napi.h>
// mapnik
#include <mapnik/image.hpp> // for image_rgba8
#include <mapnik/image_view_any.hpp>

class Image;
namespace mapnik {
template <typename T>
class image_view;
}

typedef std::shared_ptr<mapnik::image_view_any> image_view_ptr;

class ImageView : public Napi::ObjectWrap<ImageView>
{
    friend class Image;

  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit ImageView(Napi::CallbackInfo const& info);
    // methods
    Napi::Value encodeSync(Napi::CallbackInfo const& info);
    Napi::Value encode(Napi::CallbackInfo const& info);
    Napi::Value width(Napi::CallbackInfo const& info);
    Napi::Value height(Napi::CallbackInfo const& info);
    void saveSync(Napi::CallbackInfo const& info);
    Napi::Value isSolid(Napi::CallbackInfo const& info);
    Napi::Value isSolidSync(Napi::CallbackInfo const& info);
    Napi::Value getPixel(Napi::CallbackInfo const& info);

  private:
    static void encode_common_args_(Napi::CallbackInfo const& info, std::string& format, palette_ptr& palette);
    static Napi::FunctionReference constructor;
    image_view_ptr image_view_;
    image_ptr image_;
    Napi::Reference<Napi::Buffer<unsigned char>> buf_ref_;
};
