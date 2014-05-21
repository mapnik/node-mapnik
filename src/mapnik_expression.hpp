#ifndef __NODE_MAPNIK_EXPRESSION_H__
#define __NODE_MAPNIK_EXPRESSION_H__

#include <nan.h>
#include "mapnik3x_compatibility.hpp"

// mapnik
#include <mapnik/version.hpp>
#if MAPNIK_VERSION >= 200100
#include <mapnik/expression.hpp>
#else
#include <mapnik/filter_factory.hpp>
#endif

// boost
#include MAPNIK_SHARED_INCLUDE

using namespace v8;

class Expression: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static NAN_METHOD(toString);
    static NAN_METHOD(evaluate);

    Expression();
    inline mapnik::expression_ptr get() { return this_; }

private:
    ~Expression();
    mapnik::expression_ptr this_;
};

#endif
