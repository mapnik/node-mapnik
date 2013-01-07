#ifndef __NODE_MAPNIK_FEATURESET_H__
#define __NODE_MAPNIK_FEATURESET_H__

#include <v8.h>
#include <node_object_wrap.h>
#include <mapnik/datasource_cache.hpp>

using namespace v8;

typedef mapnik::featureset_ptr fs_ptr;

class Featureset: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> New(mapnik::featureset_ptr fs_ptr);
    static Handle<Value> next(const Arguments &args);

    Featureset();

private:
    ~Featureset();
    fs_ptr this_;
};

#endif
