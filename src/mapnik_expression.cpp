#include "utils.hpp"
#include "mapnik_expression.hpp"
#include "mapnik_feature.hpp"
#include "utils.hpp"
#include "object_to_container.hpp"

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/attribute.hpp>
#include <mapnik/expression_string.hpp>
#include <mapnik/expression_evaluator.hpp>

// stl
#include <exception>                    // for exception

Persistent<FunctionTemplate> Expression::constructor;

void Expression::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Expression::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Expression"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "toString", toString);
    NODE_SET_PROTOTYPE_METHOD(lcons, "evaluate", evaluate);

    target->Set(NanNew("Expression"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Expression::Expression() :
    node::ObjectWrap(),
    this_() {}

Expression::~Expression()
{
}

NAN_METHOD(Expression::New)
{
    NanScope();
    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    mapnik::expression_ptr e_ptr;
    try
    {
        if (args.Length() == 1 && args[0]->IsString()) {
            e_ptr = mapnik::parse_expression(TOSTR(args[0]));
        } else {
            NanThrowTypeError("invalid arguments: accepts a single argument of string type");
            NanReturnUndefined();
        }
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }

    Expression* e = new Expression();
    e->Wrap(args.This());
    e->this_ = e_ptr;
    NanReturnValue(args.This());
}

NAN_METHOD(Expression::toString)
{
    NanScope();

    Expression* e = node::ObjectWrap::Unwrap<Expression>(args.Holder());
    NanReturnValue(NanNew(mapnik::to_expression_string(*e->get()).c_str()));
}

NAN_METHOD(Expression::evaluate)
{
    NanScope();

    if (args.Length() < 1) {
        NanThrowError("requires a mapnik.Feature as an argument");
        NanReturnUndefined();
    }

    Local<Object> obj = args[0].As<Object>();
    if (obj->IsNull() || obj->IsUndefined() || !NanNew(Feature::constructor)->HasInstance(obj)) {
        NanThrowTypeError("first argument is invalid, must be a mapnik.Feature");
        NanReturnUndefined();
    }

    Feature* f = node::ObjectWrap::Unwrap<Feature>(obj);

    Expression* e = node::ObjectWrap::Unwrap<Expression>(args.Holder());
    Local<Object> options = NanNew<Object>();
    mapnik::attributes vars;
    if (args.Length() > 1)
    {
        if (!args[1]->IsObject())
        {
            NanThrowTypeError("optional second argument must be an options object");
            NanReturnUndefined();
        }
        options = args[1]->ToObject();

        if (options->Has(NanNew("variables")))
        {
            Local<Value> bind_opt = options->Get(NanNew("variables"));
            if (!bind_opt->IsObject())
            {
                NanThrowTypeError("optional arg 'variables' must be an object");
                NanReturnUndefined();
            }
            object_to_container(vars,bind_opt->ToObject());
        }
    }
    mapnik::value value_obj = mapnik::util::apply_visitor(mapnik::evaluate<mapnik::Feature,mapnik::value,mapnik::attributes>(*(f->get()),vars),*(e->get()));
    NanReturnValue(mapnik::util::apply_visitor(node_mapnik::value_converter(),value_obj));
}
