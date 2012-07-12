#ifndef __NODE_MAPNIK_IMAGE_VIEW_H__
#define __NODE_MAPNIK_IMAGE_VIEW_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <mapnik/graphics.hpp>
#include <mapnik/image_view.hpp>
#include <mapnik/graphics.hpp>
#include <boost/shared_ptr.hpp>

#include "mapnik_image.hpp"

using namespace v8;
using namespace node;

typedef boost::shared_ptr<mapnik::image_view<mapnik::image_data_32> > image_view_ptr;

class ImageView: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> New(Image * JSImage,
                             unsigned x,unsigned y, unsigned w, unsigned h);

    static Handle<Value> encodeSync(const Arguments &args);
    static Handle<Value> encode(const Arguments &args);
    static void EIO_Encode(uv_work_t* req);
    static void EIO_AfterEncode(uv_work_t* req);

    //static Handle<Value> view(const Arguments &args);
    static Handle<Value> width(const Arguments &args);
    static Handle<Value> height(const Arguments &args);
    //static Handle<Value> open(const Arguments &args);
    static Handle<Value> save(const Arguments &args);
    static Handle<Value> isSolid(const Arguments &args);
    static Handle<Value> getPixel(const Arguments &args);

    ImageView(Image * JSImage);
    inline image_view_ptr get() { return this_; }

private:
    ~ImageView();
    image_view_ptr this_;
    Image * JSImage_;
};

#endif
