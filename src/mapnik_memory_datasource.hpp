#ifndef __NODE_MAPNIK_MEMORY_DATASOURCE_H__
#define __NODE_MAPNIK_MEMORY_DATASOURCE_H__

#include <v8.h>
#include <node_object_wrap.h>

#include <boost/scoped_ptr.hpp>
#include <mapnik/memory_datasource.hpp>

using namespace v8;

class MemoryDatasource: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> New(mapnik::datasource_ptr ds_ptr);
    static Handle<Value> parameters(const Arguments &args);
    static Handle<Value> describe(const Arguments &args);
    static Handle<Value> features(const Arguments &args);
    static Handle<Value> featureset(const Arguments &args);
    static Handle<Value> add(const Arguments &args);

    MemoryDatasource();
    inline mapnik::datasource_ptr get() { return datasource_; }

private:
    ~MemoryDatasource();
    mapnik::datasource_ptr datasource_;
    unsigned int feature_id_;
    boost::scoped_ptr<mapnik::transcoder> tr_;
};

#endif
