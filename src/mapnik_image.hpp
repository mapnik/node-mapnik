#pragma once

#include <napi.h>
#include "mapnik_palette.hpp"

namespace mapnik {
struct image_any;
enum image_dtype : std::uint8_t;
} // namespace mapnik

using image_ptr = std::shared_ptr<mapnik::image_any>;

class Image : public Napi::ObjectWrap<Image>
{
  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes attr);
    // ctor
    explicit Image(Napi::CallbackInfo const& info);
    // methods
    Napi::Value getType(Napi::CallbackInfo const& info);

    Napi::Value getPixel(Napi::CallbackInfo const& info);
    void setPixel(Napi::CallbackInfo const& info);

    Napi::Value encodeSync(Napi::CallbackInfo const& info);
    Napi::Value encode(Napi::CallbackInfo const& info);
    Napi::Value setGrayScaleToAlpha(Napi::CallbackInfo const& info);
    Napi::Value width(Napi::CallbackInfo const& info);
    Napi::Value height(Napi::CallbackInfo const& info);
    void saveSync(Napi::CallbackInfo const& info);
    void save(Napi::CallbackInfo const& info);
    Napi::Value data(Napi::CallbackInfo const& info);
    Napi::Value buffer(Napi::CallbackInfo const& info);
    static Napi::Value openSync(Napi::CallbackInfo const& info);
    static Napi::Value open(Napi::CallbackInfo const& info);
    static Napi::Value fromBytesSync(Napi::CallbackInfo const& info);
    static Napi::Value fromBytes(Napi::CallbackInfo const& info);
    static Napi::Value fromBufferSync(Napi::CallbackInfo const& info);
    static Napi::Value fromSVGSync(Napi::CallbackInfo const& info);
    static Napi::Value fromSVG(Napi::CallbackInfo const& info);
    static Napi::Value fromSVGBytesSync(Napi::CallbackInfo const& info);
    static Napi::Value fromSVGBytes(Napi::CallbackInfo const& info);

    Napi::Value painted(Napi::CallbackInfo const& info);
    Napi::Value view(Napi::CallbackInfo const& info);
    Napi::Value composite(Napi::CallbackInfo const& info);
    Napi::Value filterSync(Napi::CallbackInfo const& info);
    Napi::Value filter(Napi::CallbackInfo const& info);
    Napi::Value fillSync(Napi::CallbackInfo const& info);
    Napi::Value fill(Napi::CallbackInfo const& info);
    Napi::Value premultiplySync(Napi::CallbackInfo const& info);
    Napi::Value premultiply(Napi::CallbackInfo const& info);
    Napi::Value demultiplySync(Napi::CallbackInfo const& info);
    Napi::Value demultiply(Napi::CallbackInfo const& info);
    Napi::Value premultiplied(Napi::CallbackInfo const& info);
    Napi::Value clearSync(Napi::CallbackInfo const& info);
    Napi::Value clear(Napi::CallbackInfo const& info);
    Napi::Value compare(Napi::CallbackInfo const& info);
    Napi::Value isSolid(Napi::CallbackInfo const& info);
    Napi::Value isSolidSync(Napi::CallbackInfo const& info);
    Napi::Value copy(Napi::CallbackInfo const& info);
    Napi::Value copySync(Napi::CallbackInfo const& info);
    Napi::Value resize(Napi::CallbackInfo const& info);
    Napi::Value resizeSync(Napi::CallbackInfo const& info);

    // accessors
    Napi::Value scaling(Napi::CallbackInfo const& info);
    void scaling(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value offset(Napi::CallbackInfo const& info);
    void offset(Napi::CallbackInfo const& info, Napi::Value const& value);
    inline image_ptr impl() const { return image_; }
    static Napi::FunctionReference constructor;

  private:
    static void encode_common_args_(Napi::CallbackInfo const& info, std::string& format, palette_ptr& palette);
    static Napi::Value from_svg_sync_impl(Napi::CallbackInfo const& info, bool from_file);
    image_ptr image_;
    Napi::Reference<Napi::Buffer<unsigned char>> buf_ref_;
};
