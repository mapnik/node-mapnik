#ifndef __NODE_MAPNIK_EXPRESSION_H__
#define __NODE_MAPNIK_EXPRESSION_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <memory>

// mapnik
#include <mapnik/expression.hpp>

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
