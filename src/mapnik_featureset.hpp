#ifndef __NODE_MAPNIK_FEATURESET_H__
#define __NODE_MAPNIK_FEATURESET_H__

#include <nan.h>
#include <mapnik/datasource_cache.hpp>
#include "mapnik3x_compatibility.hpp"

using namespace v8;

typedef mapnik::featureset_ptr fs_ptr;

class Featureset: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static Handle<Value> New(mapnik::featureset_ptr fs_ptr);
    static NAN_METHOD(next);

    Featureset();

private:
    ~Featureset();
    fs_ptr this_;
};

#endif
