#ifndef __NODE_MAPNIK_MEMORY_DATASOURCE_H__
#define __NODE_MAPNIK_MEMORY_DATASOURCE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/datasource.hpp>

using namespace v8;

namespace mapnik { class transcoder; }

class MemoryDatasource: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static Handle<Value> NewInstance(mapnik::datasource_ptr ds_ptr);
    static NAN_METHOD(parameters);
    static NAN_METHOD(describe);
    static NAN_METHOD(features);
    static NAN_METHOD(featureset);
    static NAN_METHOD(add);

    MemoryDatasource();
    inline mapnik::datasource_ptr get() { return datasource_; }

private:
    ~MemoryDatasource();
    mapnik::datasource_ptr datasource_;
    unsigned int feature_id_;
    mapnik::transcoder tr_;
};

#endif
