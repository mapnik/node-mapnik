#ifndef __NODE_MAPNIK_COLOR_H__
#define __NODE_MAPNIK_COLOR_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include <memory>



namespace mapnik { class color; }
typedef std::shared_ptr<mapnik::color> color_ptr;

class Color: public Nan::ObjectWrap {
public:
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);
    static v8::Local<v8::Value> NewInstance(mapnik::color const& color);
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
