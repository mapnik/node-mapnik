#ifndef __NODE_MAPNIK_DATASOURCE_H__
#define __NODE_MAPNIK_DATASOURCE_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

#include <mapnik/datasource.hpp>

using namespace v8;
using namespace node;

class Datasource: public node::ObjectWrap {
  public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> New(mapnik::datasource_ptr ds_ptr);
    static Handle<Value> parameters(const Arguments &args);
    static Handle<Value> describe(const Arguments &args);
    static Handle<Value> features(const Arguments &args);

    static Handle<Value> get_featureset(const Arguments &args);

    Datasource();
    inline mapnik::datasource_ptr get() { return datasource_; }

  private:
    ~Datasource();
    mapnik::datasource_ptr datasource_;
};

#endif
