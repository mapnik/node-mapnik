#ifndef __NODE_MAPNIK_QUERY_H__
#define __NODE_MAPNIK_QUERY_H__

#include <v8.h>
#include <node_object_wrap.h>

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/query.hpp>

// boost
#include <boost/shared_ptr.hpp>

using namespace v8;

typedef boost::shared_ptr<mapnik::query> query_ptr;

class Query: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    Query(mapnik::box2d<double> const& box);

private:
    ~Query();
    query_ptr this_;
};

#endif
