#ifndef __NODE_MAPNIK_DATASOURCE_H__
#define __NODE_MAPNIK_DATASOURCE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <memory>

using namespace v8;

namespace mapnik { class datasource; }

typedef std::shared_ptr<mapnik::datasource> datasource_ptr;

class Datasource: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static Handle<Value> NewInstance(datasource_ptr ds_ptr);

    static NAN_METHOD(parameters);
    static NAN_METHOD(describe);
    static NAN_METHOD(featureset);
    static NAN_METHOD(extent);

    Datasource();
    inline datasource_ptr get() { return datasource_; }

private:
    ~Datasource();
    datasource_ptr datasource_;
};

#endif
