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

Nan::Persistent<v8::FunctionTemplate> Expression::constructor;

void Expression::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Expression::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Expression").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "toString", toString);
    Nan::SetPrototypeMethod(lcons, "evaluate", evaluate);

    Nan::Set(target, Nan::New("Expression").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

Expression::Expression() :
    Nan::ObjectWrap(),
    this_() {}

Expression::~Expression()
{
}

NAN_METHOD(Expression::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    mapnik::expression_ptr e_ptr;
    try
    {
        if (info.Length() == 1 && info[0]->IsString()) {
            e_ptr = mapnik::parse_expression(TOSTR(info[0]));
        } else {
            Nan::ThrowTypeError("invalid arguments: accepts a single argument of string type");
            return;
        }
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }

    Expression* e = new Expression();
    e->Wrap(info.This());
    e->this_ = e_ptr;
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Expression::toString)
{
    Expression* e = Nan::ObjectWrap::Unwrap<Expression>(info.Holder());
    info.GetReturnValue().Set(Nan::New(mapnik::to_expression_string(*e->get())).ToLocalChecked());
}

NAN_METHOD(Expression::evaluate)
{
    if (info.Length() < 1) {
        Nan::ThrowError("requires a mapnik.Feature as an argument");
        return;
    }

    if (!info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument is invalid, must be a mapnik.Feature");
        return;
    }

    if (info[0]->IsNull() || info[0]->IsUndefined() || !Nan::New(Feature::constructor)->HasInstance(info[0])) {
        Nan::ThrowTypeError("first argument is invalid, must be a mapnik.Feature");
        return;
    }

    Feature* f = Nan::ObjectWrap::Unwrap<Feature>(info[0].As<v8::Object>());

    Expression* e = Nan::ObjectWrap::Unwrap<Expression>(info.Holder());
    mapnik::attributes vars;
    if (info.Length() > 1)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second argument must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1].As<v8::Object>();

        if (Nan::Has(options, Nan::New("variables").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("variables").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsObject())
            {
                Nan::ThrowTypeError("optional arg 'variables' must be an object");
                return;
            }
            object_to_container(vars,bind_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
        }
    }
    mapnik::value value_obj = mapnik::util::apply_visitor(mapnik::evaluate<mapnik::feature_impl,mapnik::value,mapnik::attributes>(*(f->get()),vars),*(e->get()));
    info.GetReturnValue().Set(mapnik::util::apply_visitor(node_mapnik::value_converter(),value_obj));
}
