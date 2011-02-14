#ifndef __NODE_MAPNIK_PROJECTION_H__
#define __NODE_MAPNIK_PROJECTION_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

// boost
#include <boost/shared_ptr.hpp>

#include <mapnik/projection.hpp>

using namespace v8;
using namespace node;

typedef boost::shared_ptr<mapnik::projection> proj_ptr;

class Projection: public node::ObjectWrap {
  public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    static Handle<Value> inverse(const Arguments& args);
    static Handle<Value> forward(const Arguments& args);

    explicit Projection(std::string const& name);
    explicit Projection(std::string const& name, std::string const& srs);

  private:
    ~Projection();
    proj_ptr projection_;
};

#endif

