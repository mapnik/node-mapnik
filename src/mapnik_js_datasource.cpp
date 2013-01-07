/*
#include <mapnik/datasource_cache.hpp>

#include "mapnik_js_datasource.hpp"
#include "utils.hpp"
#include "mem_datasource.hpp"

Persistent<FunctionTemplate> JSDatasource::constructor;

void JSDatasource::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(JSDatasource::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("JSDatasource"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(constructor, "next", next);

    target->Set(String::NewSymbol("JSDatasource"),constructor->GetFunction());
}

JSDatasource::JSDatasource() :
    ObjectWrap(),
    ds_ptr_() {}

JSDatasource::~JSDatasource()
{
}

Handle<Value> JSDatasource::New(const Arguments& args)
{
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        JSDatasource* d =  static_cast<JSDatasource*>(ptr);
        d->Wrap(args.This());
        return args.This();
    }

    if (!args.Length() == 2){
        return ThrowException(Exception::TypeError(
                                  String::New("two argument required: an object of key:value datasource options and a callback function for features")));
    }

    if (!args[0]->IsObject())
        return ThrowException(Exception::TypeError(
                                  String::New("Must provide an object, eg {extent: '-180,-90,180,90'}")));

    Local<Object> options = args[0]->ToObject();

    // function callback
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    // TODO - maybe validate in js?

    bool bind=true;
    if (options->Has(String::New("bind")))
    {
        Local<Value> bind_opt = options->Get(String::New("bind"));
        if (!bind_opt->IsBoolean())
            return ThrowException(Exception::TypeError(
                                      String::New("'bind' must be a Boolean")));

        bind = bind_opt->BooleanValue();
    }

    mapnik::parameters params;
    params["type"] = "js";
    Local<Array> names = options->GetPropertyNames();
    unsigned int i = 0;
    unsigned int a_length = names->Length();
    while (i < a_length) {
        Local<Value> name = names->Get(i)->ToString();
        Local<Value> value = options->Get(name);
        params[TOSTR(name)] = TOSTR(value);
        i++;
    }

    mapnik::datasource_ptr ds;
    try
    {

        ds = mapnik::datasource_ptr(new js_datasource(params,bind,args[args.Length()-1]));

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
    if (ds)
    {
        JSDatasource* d = new JSDatasource();
        d->Wrap(args.This());
        d->ds_ptr_ = ds;
        return args.This();
    }

    return Undefined();
}

Handle<Value> JSDatasource::next(const Arguments& args)
{
    HandleScope scope;

    JSDatasource* d = ObjectWrap::Unwrap<JSDatasource>(args.This());
    js_datasource *js = dynamic_cast<js_datasource *>(d->ds_ptr_.get());
    return scope.Close((*js->cb_)->Call(Context::GetCurrent()->Global(), 0, NULL));
}
*/
