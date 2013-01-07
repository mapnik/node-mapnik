#ifndef __NODE_MAPNIK_PROJECTION_H__
#define __NODE_MAPNIK_PROJECTION_H__

#include <v8.h>
#include <node_object_wrap.h>

// boost
#include <boost/shared_ptr.hpp>

#include <mapnik/projection.hpp>

using namespace v8;

typedef boost::shared_ptr<mapnik::projection> proj_ptr;

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

#endif

