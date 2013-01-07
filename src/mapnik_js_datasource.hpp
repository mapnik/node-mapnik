#ifndef __NODE_MAPNIK_JSDatasource_H__
#define __NODE_MAPNIK_JSDatasource_H__

#include <v8.h>
#include <node_object_wrap.h>

using namespace v8;

class JSDatasource: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> parameters(const Arguments &args);
    static Handle<Value> next(const Arguments &args);
    inline mapnik::datasource_ptr get() { return ds_ptr_; }
    JSDatasource();

private:
    ~JSDatasource();
    mapnik::datasource_ptr ds_ptr_;
};

#endif
