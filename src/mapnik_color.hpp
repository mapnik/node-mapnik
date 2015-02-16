#ifndef __NODE_MAPNIK_COLOR_H__
#define __NODE_MAPNIK_COLOR_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <memory>

using namespace v8;

namespace mapnik { class color; }
typedef std::shared_ptr<mapnik::color> color_ptr;

class Color: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static Handle<Value> NewInstance(mapnik::color const& color);
    static NAN_METHOD(toString);
    static NAN_METHOD(hex);

    static NAN_GETTER(get_prop);
    static NAN_SETTER(set_prop);
    static NAN_GETTER(get_premultiplied);
    static NAN_SETTER(set_premultiplied);
    Color();
    inline color_ptr get() { return this_; }

private:
    ~Color();
    color_ptr this_;
};

#endif
