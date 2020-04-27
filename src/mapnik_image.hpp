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
    /*
    static Napi::Value setGrayScaleToAlpha(const Napi::CallbackInfo& info);
    */
    Napi::Value width(Napi::CallbackInfo const& info);
    Napi::Value height(Napi::CallbackInfo const& info);
/*
  static Napi::Value view(const Napi::CallbackInfo& info);
*/
    static Napi::Value openSync(Napi::CallbackInfo const& info);
    static Napi::Value open(Napi::CallbackInfo const& info);
/*
    static Napi::Value _fromBytesSync(const Napi::CallbackInfo& info);
    static Napi::Value _fromBufferSync(const Napi::CallbackInfo& info);
    static Napi::Value fromBytesSync(const Napi::CallbackInfo& info);
    static Napi::Value fromBufferSync(const Napi::CallbackInfo& info);
    static Napi::Value fromBytes(const Napi::CallbackInfo& info);
    static void EIO_FromBytes(uv_work_t* req);
    static void EIO_AfterFromBytes(uv_work_t* req);
    static Napi::Value _fromSVGSync(bool fromFile, const Napi::CallbackInfo& info);
    static Napi::Value fromSVGSync(const Napi::CallbackInfo& info);
    static Napi::Value fromSVG(const Napi::CallbackInfo& info);
    static Napi::Value fromSVGBytesSync(const Napi::CallbackInfo& info);
    static Napi::Value fromSVGBytes(const Napi::CallbackInfo& info);
    static void EIO_FromSVG(uv_work_t* req);
    static void EIO_AfterFromSVG(uv_work_t* req);
    static void EIO_FromSVGBytes(uv_work_t* req);
    static void EIO_AfterFromSVGBytes(uv_work_t* req);
    static Napi::Value _saveSync(const Napi::CallbackInfo& info);
    static Napi::Value saveSync(const Napi::CallbackInfo& info);
    static Napi::Value save(const Napi::CallbackInfo& info);
    static void EIO_Save(uv_work_t* req);
    static void EIO_AfterSave(uv_work_t* req);
    static Napi::Value painted(const Napi::CallbackInfo& info);
    static Napi::Value composite(const Napi::CallbackInfo& info);
    static Napi::Value _filterSync(const Napi::CallbackInfo& info);
    static Napi::Value filterSync(const Napi::CallbackInfo& info);
    static Napi::Value filter(const Napi::CallbackInfo& info);
    static void EIO_Filter(uv_work_t* req);
    static void EIO_AfterFilter(uv_work_t* req);
*/
    Napi::Value fillSync(Napi::CallbackInfo const& info);
    Napi::Value fill(Napi::CallbackInfo const& info);
    /*
    static Napi::Value _premultiplySync(const Napi::CallbackInfo& info);
    static Napi::Value premultiplySync(const Napi::CallbackInfo& info);
    static Napi::Value premultiply(const Napi::CallbackInfo& info);
    static Napi::Value premultiplied(const Napi::CallbackInfo& info);
    static void EIO_Premultiply(uv_work_t* req);
    static Napi::Value _demultiplySync(const Napi::CallbackInfo& info);
    static Napi::Value demultiplySync(const Napi::CallbackInfo& info);
    static Napi::Value demultiply(const Napi::CallbackInfo& info);
    static void EIO_Demultiply(uv_work_t* req);
    static void EIO_AfterMultiply(uv_work_t* req);
    static Napi::Value _clearSync(const Napi::CallbackInfo& info);
    static Napi::Value clearSync(const Napi::CallbackInfo& info);
    static Napi::Value clear(const Napi::CallbackInfo& info);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);
    static void EIO_Composite(uv_work_t* req);
    static void EIO_AfterComposite(uv_work_t* req);
*/
    Napi::Value compare(Napi::CallbackInfo const& info);
/*
    static Napi::Value isSolid(const Napi::CallbackInfo& info);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static Napi::Value _isSolidSync(const Napi::CallbackInfo& info);
    static Napi::Value isSolidSync(const Napi::CallbackInfo& info);
    static Napi::Value copy(const Napi::CallbackInfo& info);
    static void EIO_Copy(uv_work_t* req);
    static void EIO_AfterCopy(uv_work_t* req);
    static Napi::Value _copySync(const Napi::CallbackInfo& info);
    static Napi::Value copySync(const Napi::CallbackInfo& info);
    static Napi::Value resize(const Napi::CallbackInfo& info);
    static void EIO_Resize(uv_work_t* req);
    static void EIO_AfterResize(uv_work_t* req);
    static Napi::Value _resizeSync(const Napi::CallbackInfo& info);
    static Napi::Value resizeSync(const Napi::CallbackInfo& info);
    static Napi::Value data(const Napi::CallbackInfo& info);

    Napi::Value get_scaling(const Napi::CallbackInfo& info);
    void set_scaling(const Napi::CallbackInfo& info, const Napi::Value& value);
    Napi::Value get_offset(const Napi::CallbackInfo& info);
    void set_offset(const Napi::CallbackInfo& info, const Napi::Value& value);

    using Napi::ObjectWrap::Ref;
    using Napi::ObjectWrap::Unref;

    Image(unsigned int width, unsigned int height, mapnik::image_dtype type, bool initialized, bool premultiplied, bool painted);
    Image(image_ptr this_);
    inline image_ptr get() { return this_; }
*/

    static Napi::FunctionReference constructor;
private:
    static void encode_common_args_(Napi::CallbackInfo const& info, std::string& format, palette_ptr& palette);
    image_ptr image_;
};
