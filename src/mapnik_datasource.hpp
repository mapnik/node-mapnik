#ifndef __NODE_MAPNIK_DATASOURCE_H__
#define __NODE_MAPNIK_DATASOURCE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include <memory>



namespace mapnik { class datasource; }

typedef std::shared_ptr<mapnik::datasource> datasource_ptr;

class Datasource: public Nan::ObjectWrap {
public:
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);
    static v8::Local<v8::Value> NewInstance(datasource_ptr ds_ptr);

    static NAN_METHOD(parameters);
    static NAN_METHOD(describe);
    static NAN_METHOD(featureset);
    static NAN_METHOD(extent);
    static NAN_METHOD(fields);

    Datasource();
    inline datasource_ptr get() { return datasource_; }

private:
    ~Datasource();
    datasource_ptr datasource_;
};

#endif
