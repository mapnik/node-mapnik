#ifndef __NODE_MAPNIK_PROJ_TRANSFORM_H__
#define __NODE_MAPNIK_PROJ_TRANSFORM_H__

#include <v8.h>
#include <node_object_wrap.h>

// mapnik
#include <mapnik/proj_transform.hpp>

// boost
#include <boost/shared_ptr.hpp>

using namespace v8;

typedef boost::shared_ptr<mapnik::proj_transform> proj_tr_ptr;

class ProjTransform: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    static Handle<Value> forward(const Arguments& args);
    static Handle<Value> backward(const Arguments& args);

    ProjTransform(mapnik::projection const& src,
                  mapnik::projection const& dest);

private:
    ~ProjTransform();
    proj_tr_ptr this_;
};

#endif
