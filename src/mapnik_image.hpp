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
    static Handle<Value> toString(const Arguments &args);
    static Handle<Value> open(const Arguments &args);

    Image(unsigned int width, unsigned int height);
    Image(image_ptr this_);

  private:
    ~Image();
    image_ptr this_;
};

#endif
