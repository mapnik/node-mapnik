#ifndef __NODE_MAPNIK_LAYER_H__
#define __NODE_MAPNIK_LAYER_H__

#include <node_object_wrap.h>           // for ObjectWrap
#include <v8.h>                         // for Handle, AccessorInfo, etc

// stl
#include <string>

// boost
#include <boost/shared_ptr.hpp>

using namespace v8;

namespace mapnik { class layer; }
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
