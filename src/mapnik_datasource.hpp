#ifndef __NODE_MAPNIK_DATASOURCE_H__
#define __NODE_MAPNIK_DATASOURCE_H__

#include <v8.h>
#include <node_object_wrap.h>

#include <boost/shared_ptr.hpp>

using namespace v8;

namespace mapnik { class datasource; }

typedef boost::shared_ptr<mapnik::datasource> datasource_ptr;

class Datasource: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> New(datasource_ptr ds_ptr);

    static Handle<Value> parameters(const Arguments &args);
    static Handle<Value> describe(const Arguments &args);
    static Handle<Value> features(const Arguments &args);
    static Handle<Value> featureset(const Arguments &args);
    static Handle<Value> extent(const Arguments &args);

    Datasource();
    inline datasource_ptr get() { return datasource_; }

private:
    ~Datasource();
    datasource_ptr datasource_;
};

#endif
