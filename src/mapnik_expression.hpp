#ifndef __NODE_MAPNIK_EXPRESSION_H__
#define __NODE_MAPNIK_EXPRESSION_H__

#include <v8.h>
#include <node_object_wrap.h>

// mapnik
#include <mapnik/version.hpp>
#if MAPNIK_VERSION >= 200100
#include <mapnik/expression.hpp>
#else
#include <mapnik/filter_factory.hpp>
#endif

// boost
#include <boost/shared_ptr.hpp>

using namespace v8;

class Expression: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> toString(const Arguments &args);
    static Handle<Value> evaluate(const Arguments &args);

    Expression();
    inline mapnik::expression_ptr get() { return this_; }

private:
    ~Expression();
    mapnik::expression_ptr this_;
};

#endif
