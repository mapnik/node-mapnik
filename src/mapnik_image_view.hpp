#ifndef __NODE_MAPNIK_IMAGE_VIEW_H__
#define __NODE_MAPNIK_IMAGE_VIEW_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/image.hpp>        // for image_rgba8
#include <mapnik/image_view_any.hpp>
#include <memory>

class Image;
namespace mapnik { template <typename T> class image_view; }

using namespace v8;

typedef std::shared_ptr<mapnik::image_view_any> image_view_ptr;

class ImageView: public Nan::ObjectWrap {
public:
    static Nan::Persistent<FunctionTemplate> constructor;
    static void Initialize(Local<Object> target);
    static NAN_METHOD(New);
    static Local<Value> NewInstance(Image * JSImage,
                             unsigned x,unsigned y, unsigned w, unsigned h);

    static NAN_METHOD(encodeSync);
    static NAN_METHOD(encode);
    static void EIO_Encode(uv_work_t* req);
    static void EIO_AfterEncode(uv_work_t* req);

    //static NAN_METHOD(view);
    static NAN_METHOD(width);
    static NAN_METHOD(height);
    //static NAN_METHOD(open);
    static NAN_METHOD(save);
    static NAN_METHOD(isSolid);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static Local<Value> _isSolidSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(isSolidSync);
    static NAN_METHOD(getPixel);

    ImageView(Image * JSImage);

private:
    ~ImageView();
    image_view_ptr this_;
    Image * JSImage_;
};

#endif
