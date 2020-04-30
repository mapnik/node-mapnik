#pragma once

#include <napi.h>
#include <memory>
#include "mapnik_palette.hpp"

namespace mapnik {
    struct image_any;
    enum image_dtype : std::uint8_t;
}

using image_ptr = std::shared_ptr<mapnik::image_any>;

class Image : public Napi::ObjectWrap<Image>
{
public:

    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports);
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
//static Napi::Value view(const Napi::CallbackInfo& info);

    static Napi::Value openSync(Napi::CallbackInfo const& info);
    static Napi::Value open(Napi::CallbackInfo const& info);
    static Napi::Value fromBytesSync(Napi::CallbackInfo const& info);
    static Napi::Value fromBytes(Napi::CallbackInfo const& info);
    static Napi::Value fromBufferSync(Napi::CallbackInfo const& info);
    static Napi::Value fromSVGSync(Napi::CallbackInfo const& info);
    static Napi::Value fromSVG(Napi::CallbackInfo const& info);
    static Napi::Value fromSVGBytesSync(Napi::CallbackInfo const& info);
    static Napi::Value fromSVGBytes(Napi::CallbackInfo const& info);

/*
    static Napi::Value painted(const Napi::CallbackInfo& info);
    static Napi::Value composite(const Napi::CallbackInfo& info);
    static Napi::Value _filterSync(const Napi::CallbackInfo& info);
    static Napi::Value filterSync(const Napi::CallbackInfo& info);
    static Napi::Value filter(const Napi::CallbackInfo& info);
*/
    Napi::Value fillSync(Napi::CallbackInfo const& info);
    Napi::Value fill(Napi::CallbackInfo const& info);
    /*
    static Napi::Value premultiplySync(const Napi::CallbackInfo& info);
    static Napi::Value premultiply(const Napi::CallbackInfo& info);
    */
    Napi::Value premultiplied(Napi::CallbackInfo const& info);
    /*
    static Napi::Value demultiplySync(const Napi::CallbackInfo& info);
    static Napi::Value demultiply(const Napi::CallbackInfo& info);
    static Napi::Value clearSync(const Napi::CallbackInfo& info);
    static Napi::Value clear(const Napi::CallbackInfo& info);
*/
    Napi::Value compare(Napi::CallbackInfo const& info);

    Napi::Value isSolid(Napi::CallbackInfo const& info);
    Napi::Value isSolidSync(Napi::CallbackInfo const& info);
    /*
    static Napi::Value copy(const Napi::CallbackInfo& info);
    static Napi::Value copySync(const Napi::CallbackInfo& info);
    static Napi::Value resize(const Napi::CallbackInfo& info);
    static Napi::Value resizeSync(const Napi::CallbackInfo& info);
    *static Napi::Value data(const Napi::CallbackInfo& info);
*/
    // accessors
    Napi::Value scaling(Napi::CallbackInfo const& info);
    void scaling(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value offset(Napi::CallbackInfo const& info);
    void offset(Napi::CallbackInfo const& info, Napi::Value const& value);

    static Napi::FunctionReference constructor;
private:
    static void encode_common_args_(Napi::CallbackInfo const& info, std::string& format, palette_ptr& palette);
    image_ptr image_;
};
