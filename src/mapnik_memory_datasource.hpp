#ifndef __NODE_MAPNIK_MEMORY_DATASOURCE_H__
#define __NODE_MAPNIK_MEMORY_DATASOURCE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/datasource.hpp>



namespace mapnik { class transcoder; }

class MemoryDatasource: public Nan::ObjectWrap {
public:
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);
    static v8::Local<v8::Value> NewInstance(mapnik::datasource_ptr ds_ptr);
    static NAN_METHOD(parameters);
    static NAN_METHOD(describe);
    static NAN_METHOD(features);
    static NAN_METHOD(featureset);
    static NAN_METHOD(add);
    static NAN_METHOD(fields);

    MemoryDatasource();
    inline mapnik::datasource_ptr get() { return datasource_; }

private:
    ~MemoryDatasource();
    mapnik::datasource_ptr datasource_;
    unsigned int feature_id_;
    mapnik::transcoder tr_;
};

#endif
