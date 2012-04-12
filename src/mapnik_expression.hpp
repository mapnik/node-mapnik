#ifndef __NODE_MAPNIK_EXPRESSION_H__
#define __NODE_MAPNIK_EXPRESSION_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

// mapnik
#include <mapnik/expression_node.hpp>
#include <mapnik/expression.hpp>
#include <mapnik/expression_string.hpp>
#include <mapnik/expression_evaluator.hpp>


// boost
#include <boost/shared_ptr.hpp>

using namespace v8;
using namespace node;

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
