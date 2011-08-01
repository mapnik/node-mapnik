#ifndef __NODE_MAPNIK_IMAGE_H__
#define __NODE_MAPNIK_IMAGE_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <mapnik/graphics.hpp>
#include <boost/shared_ptr.hpp>

using namespace v8;
using namespace node;

typedef boost::shared_ptr<mapnik::image_32> image_ptr;

class Image: public node::ObjectWrap {
  public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    static Handle<Value> encodeSync(const Arguments &args);
    static Handle<Value> encode(const Arguments &args);
    static int EIO_Encode(eio_req *req);
    static int EIO_AfterEncode(eio_req *req);

    static Handle<Value> width(const Arguments &args);
    static Handle<Value> height(const Arguments &args);
    static Handle<Value> view(const Arguments &args);
    static Handle<Value> open(const Arguments &args);
    static Handle<Value> save(const Arguments &args);

    static Handle<Value> get_prop(Local<String> property,
                         const AccessorInfo& info);
    static void set_prop(Local<String> property,
                         Local<Value> value,
                         const AccessorInfo& info);
    void _ref() { Ref(); }
    void _unref() { Unref(); }

    Image(unsigned int width, unsigned int height);
    Image(image_ptr this_);
    inline image_ptr get() { return this_; }

  private:
    ~Image();
    image_ptr this_;
};

#endif
