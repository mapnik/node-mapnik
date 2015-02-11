#ifndef __NODE_MAPNIK_LAYER_H__
#define __NODE_MAPNIK_LAYER_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

// stl
#include <string>
#include <memory>

using namespace v8;

namespace mapnik { class layer; }
typedef std::shared_ptr<mapnik::layer> layer_ptr;

class Layer: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);

    static Handle<Value> NewInstance(mapnik::layer const& lay_ref);
    static NAN_METHOD(describe);

    static NAN_GETTER(get_prop);
    static NAN_SETTER(set_prop);

    Layer(std::string const& name);
    Layer(std::string const& name, std::string const& srs);
    Layer();
    inline layer_ptr get() { return layer_; }

private:
    ~Layer();
    layer_ptr layer_;
};

#endif
