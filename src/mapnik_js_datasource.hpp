#ifndef __NODE_MAPNIK_JSDatasource_H__
#define __NODE_MAPNIK_JSDatasource_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

using namespace v8;
using namespace node;

class JSDatasource: public node::ObjectWrap {
  public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> parameters(const Arguments &args);
    static Handle<Value> next(const Arguments &args);
    //static Handle<Value> describe(const Arguments &args);
    //static Handle<Value> features(const Arguments &args);

    //static Handle<Value> featureset(const Arguments &args);

    inline mapnik::datasource_ptr get() { return ds_ptr_; }
    //static Handle<Value> New(mapnik::datasource_ptr ds_ptr);

    JSDatasource();

  private:
    ~JSDatasource();
    mapnik::datasource_ptr ds_ptr_;
};

#endif
