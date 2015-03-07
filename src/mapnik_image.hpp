#ifndef __NODE_MAPNIK_IMAGE_H__
#define __NODE_MAPNIK_IMAGE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <memory>

using namespace v8;

namespace mapnik { 
    struct image_any; 
    enum image_dtype : std::uint8_t;
}

typedef std::shared_ptr<mapnik::image_any> image_ptr;

class Image: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);

    static NAN_METHOD(getPixel);
    static NAN_METHOD(setPixel);
    static NAN_METHOD(encodeSync);
    static NAN_METHOD(encode);
    static void EIO_Encode(uv_work_t* req);
    static void EIO_AfterEncode(uv_work_t* req);

    static NAN_METHOD(setGrayScaleToAlpha);
    static NAN_METHOD(width);
    static NAN_METHOD(height);
    static NAN_METHOD(view);
    static Local<Value> _openSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(openSync);
    static NAN_METHOD(open);
    static void EIO_Open(uv_work_t* req);
    static void EIO_AfterOpen(uv_work_t* req);
    static Local<Value> _fromBytesSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(fromBytesSync);
    static NAN_METHOD(fromBytes);
    static void EIO_FromBytes(uv_work_t* req);
    static void EIO_AfterFromBytes(uv_work_t* req);
    static NAN_METHOD(save);
    static NAN_METHOD(painted);
    static NAN_METHOD(composite);
    static Local<Value> _fillSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(fillSync);
    static NAN_METHOD(fill);
    static void EIO_Fill(uv_work_t* req);
    static void EIO_AfterFill(uv_work_t* req);
    static Local<Value> _premultiplySync(_NAN_METHOD_ARGS);
    static NAN_METHOD(premultiplySync);
    static NAN_METHOD(premultiply);
    static NAN_METHOD(premultiplied);
    static void EIO_Premultiply(uv_work_t* req);
    static Local<Value> _demultiplySync(_NAN_METHOD_ARGS);
    static NAN_METHOD(demultiplySync);
    static NAN_METHOD(demultiply);
    static void EIO_Demultiply(uv_work_t* req);
    static void EIO_AfterMultiply(uv_work_t* req);
    static Local<Value> _clearSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(clearSync);
    static NAN_METHOD(clear);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);
    static void EIO_Composite(uv_work_t* req);
    static void EIO_AfterComposite(uv_work_t* req);
    static NAN_METHOD(compare);
    static NAN_METHOD(isSolid);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static Local<Value> _isSolidSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(isSolidSync);
    static NAN_METHOD(copy);
    static void EIO_Copy(uv_work_t* req);
    static void EIO_AfterCopy(uv_work_t* req);
    static Local<Value> _copySync(_NAN_METHOD_ARGS);
    static NAN_METHOD(copySync);
    
    static NAN_GETTER(get_scaling);
    static NAN_SETTER(set_scaling);
    static NAN_GETTER(get_offset);
    static NAN_SETTER(set_offset);

    void _ref() { Ref(); }
    void _unref() { Unref(); }

    Image(unsigned int width, unsigned int height, mapnik::image_dtype type, bool initialized, bool premultiplied, bool painted);
    Image(image_ptr this_);
    inline image_ptr get() { return this_; }

private:
    ~Image();
    image_ptr this_;
};

#endif
