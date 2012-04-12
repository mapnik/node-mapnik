#ifndef __NODE_MAPNIK_LAYER_H__
#define __NODE_MAPNIK_LAYER_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

// boost
#include <boost/shared_ptr.hpp>

#include <mapnik/layer.hpp>

using namespace v8;
using namespace node;

typedef boost::shared_ptr<mapnik::layer> layer_ptr;

class Layer: public node::ObjectWrap {
  public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    static Handle<Value> New(mapnik::layer const& lay_ref);
    static Handle<Value> describe(const Arguments &args);

    static Handle<Value> get_prop(Local<String> property,
                         const AccessorInfo& info);
    static void set_prop(Local<String> property,
                         Local<Value> value,
                         const AccessorInfo& info);
	

    Layer(std::string const& name);
    Layer(std::string const& name, std::string const& srs);
    Layer();
    inline layer_ptr get() { return layer_; }

  private:
    ~Layer();
    layer_ptr layer_;
};

#endif
