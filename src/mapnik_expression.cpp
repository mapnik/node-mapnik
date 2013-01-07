#include <node.h>
#include "utils.hpp"
#include "mapnik_expression.hpp"
#include "mapnik_feature.hpp"
#include "utils.hpp"

// mapnik
#include <mapnik/expression_string.hpp>
#include <mapnik/expression_evaluator.hpp>

// boost
#include <boost/make_shared.hpp>

// stl
#include <exception>                    // for exception

Persistent<FunctionTemplate> Expression::constructor;

void Expression::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Expression::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Expression"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "toString", toString);
    NODE_SET_PROTOTYPE_METHOD(constructor, "evaluate", evaluate);

    target->Set(String::NewSymbol("Expression"),constructor->GetFunction());
}

Expression::Expression() :
    ObjectWrap(),
    this_() {}

Expression::~Expression()
{
}

Handle<Value> Expression::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Expression* e = static_cast<Expression*>(ptr);
        e->Wrap(args.This());
        return args.This();
    }

    mapnik::expression_ptr e_ptr;
    try
    {
        if (args.Length() == 1 && args[0]->IsString()){
            e_ptr = mapnik::parse_expression(TOSTR(args[0]),"utf8");

        } else {
            return ThrowException(Exception::Error(
                                      String::New("invalid arguments: accepts a single argument of string type")));
        }
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::Error(
                                  String::New("unknown exception happened, please file bug")));
    }

    if (e_ptr)
    {
        Expression* e = new Expression();
        e->Wrap(args.This());
        e->this_ = e_ptr;
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
                                  String::New("unknown exception happened, please file bug")));
    }

    return Undefined();
}

Handle<Value> Expression::toString(const Arguments& args)
{
    HandleScope scope;

    Expression* e = ObjectWrap::Unwrap<Expression>(args.This());
    return scope.Close(String::New( mapnik::to_expression_string(*e->get()).c_str() ));
}

Handle<Value> Expression::evaluate(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() > 0) {
        return ThrowException(Exception::Error(
                                  String::New("requires a mapnik.Feature as an argument")));
    }

    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined()) {
        return ThrowException(Exception::TypeError(String::New("first argument is invalid, must be a mapnik.Feature not null/undefined")));
    }

    if (!Feature::constructor->HasInstance(obj)) {
        return ThrowException(Exception::TypeError(String::New("first argument is invalid, must be a mapnik.Feature")));
    }

    Feature* f = ObjectWrap::Unwrap<Feature>(obj);

    Expression* e = ObjectWrap::Unwrap<Expression>(args.This());
    mapnik::value value_obj = boost::apply_visitor(mapnik::evaluate<mapnik::Feature,mapnik::value>(*(f->get())),*(e->get()));
    return scope.Close(boost::apply_visitor(node_mapnik::value_converter(),value_obj.base()));
}
