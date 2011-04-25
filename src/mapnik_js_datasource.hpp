#ifndef __NODE_MAPNIK_JSDatasource_H__
#define __NODE_MAPNIK_JSDatasource_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

using namespace v8;
using namespace node;

/*
#include <mapnik/datasource.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/query.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/feature_factory.hpp> // TODO remove
#include <vector>
#include <boost/utility.hpp>
#include <boost/scoped_ptr.hpp>
#include <algorithm>
*/

// linking errors
//#include "mem_datasource.hpp"


// js object

/*
class js_datasource : public mapnik::datasource
{
    friend class js_featureset;
public:
    js_datasource();
    virtual ~js_datasource();
    //void push(mapnik::feature_ptr feature);
    int type() const;
    mapnik::featureset_ptr features(const mapnik::query& q) const;
    mapnik::featureset_ptr features_at_point(mapnik::coord2d const& pt) const;
    mapnik::box2d<double> envelope() const;
    mapnik::layer_descriptor get_descriptor() const;
    size_t size() const;
private:
    //std::vector<mapnik::feature_ptr> features_;
    mapnik::box2d<double> ext_;
}; 
*/

//typedef boost::shared_ptr<js_datasource> js_datasource_ptr;

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
    //JSDatasource(std::string const& name, std::string const& srs);

  private:
    ~JSDatasource();
    mapnik::datasource_ptr ds_ptr_;
    //Persistent<Function> cb_;
    //Value* cb_;
};

#endif
