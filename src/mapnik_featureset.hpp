#ifndef __NODE_MAPNIK_FEATURESET_H__
#define __NODE_MAPNIK_FEATURESET_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/featureset.hpp>
#include <memory>

using namespace v8;

typedef mapnik::featureset_ptr fs_ptr;

class Featureset: public Nan::ObjectWrap {
public:
    static Nan::Persistent<FunctionTemplate> constructor;
    static void Initialize(Local<Object> target);
    static NAN_METHOD(New);
    static Local<Value> NewInstance(mapnik::featureset_ptr fs_ptr);
    static NAN_METHOD(next);

    Featureset();

private:
    ~Featureset();
    fs_ptr this_;
};

#endif
