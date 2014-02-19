#ifndef __NODE_MAPNIK_PROJECTION_H__
#define __NODE_MAPNIK_PROJECTION_H__

#include <v8.h>
#include <node_object_wrap.h>

// boost
#include "mapnik3x_compatibility.hpp"
#include MAPNIK_SHARED_INCLUDE

#include <mapnik/proj_transform.hpp>
#include <mapnik/projection.hpp>

using namespace v8;

typedef MAPNIK_SHARED_PTR<mapnik::projection> proj_ptr;

class Projection: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    static Handle<Value> inverse(const Arguments& args);
    static Handle<Value> forward(const Arguments& args);
    static bool HasInstance(Handle<Value> val);

    explicit Projection(std::string const& name);

    inline proj_ptr get() { return projection_; }

private:
    ~Projection();
    proj_ptr projection_;
};

typedef MAPNIK_SHARED_PTR<mapnik::proj_transform> proj_tr_ptr;

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

