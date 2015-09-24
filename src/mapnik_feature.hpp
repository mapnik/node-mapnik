#ifndef __NODE_MAPNIK_FEATURE_H__
#define __NODE_MAPNIK_FEATURE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include <memory>

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/feature.hpp>



class Feature: public Nan::ObjectWrap {
public:
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);
    static v8::Local<v8::Value> NewInstance(mapnik::feature_ptr f_ptr);
    static NAN_METHOD(fromJSON);
    static NAN_METHOD(id);
    static NAN_METHOD(extent);
    static NAN_METHOD(attributes);
    static NAN_METHOD(geometry);
    static NAN_METHOD(toJSON);

    // todo
    // how to allow altering of attributes
    // expose get_geometry
    Feature(mapnik::feature_ptr f);
    Feature(int id);
    inline mapnik::feature_ptr get() { return this_; }

private:
    ~Feature();
    mapnik::feature_ptr this_;
    mapnik::context_ptr ctx_;
};

#endif
