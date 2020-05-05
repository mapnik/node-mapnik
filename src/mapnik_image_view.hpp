#ifndef __NODE_MAPNIK_IMAGE_VIEW_H__
#define __NODE_MAPNIK_IMAGE_VIEW_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

#include <mapnik/image.hpp>        // for image_rgba8
#include <mapnik/image_view_any.hpp>
#include <memory>

class Image;
namespace mapnik { template <typename T> class image_view; }



typedef std::shared_ptr<mapnik::image_view_any> image_view_ptr;

class ImageView : public Napi::ObjectWrap<ImageView> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);
    static Napi::Value NewInstance(Image * JSImage,
                             unsigned x,unsigned y, unsigned w, unsigned h);

    static Napi::Value encodeSync(Napi::CallbackInfo const& info);
    static Napi::Value encode(Napi::CallbackInfo const& info);
    static void AsyncEncode(uv_work_t* req);
    static void AfterEncode(uv_work_t* req);

    //static Napi::Value view(Napi::CallbackInfo const& info);
    static Napi::Value width(Napi::CallbackInfo const& info);
    static Napi::Value height(Napi::CallbackInfo const& info);
    //static Napi::Value open(Napi::CallbackInfo const& info);
    static Napi::Value save(Napi::CallbackInfo const& info);
    static Napi::Value isSolid(Napi::CallbackInfo const& info);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static Napi::Value _isSolidSync(Napi::CallbackInfo const& info);
    static Napi::Value isSolidSync(Napi::CallbackInfo const& info);
    static Napi::Value getPixel(Napi::CallbackInfo const& info);

    ImageView(Image * JSImage);

private:
    ~ImageView();
    image_view_ptr this_;
    Image * JSImage_;
};

#endif
