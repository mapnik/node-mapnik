
#include <mapnik/datasource_cache.hpp>

#include "mapnik_datasource.hpp"
#include "utils.hpp"
#include "json_emitter.hpp"

Persistent<FunctionTemplate> Datasource::constructor;

void Datasource::Initialize(Handle<Object> target) {

    HandleScope scope;
  
    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Datasource::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Datasource"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(constructor, "parameters", parameters);
    NODE_SET_PROTOTYPE_METHOD(constructor, "describe", describe);
    NODE_SET_PROTOTYPE_METHOD(constructor, "features", features);

    target->Set(String::NewSymbol("Datasource"),constructor->GetFunction());
}

Datasource::Datasource() :
  ObjectWrap(),
  datasource_() {}

Datasource::~Datasource()
{
}

Handle<Value> Datasource::New(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() == 1){
        return ThrowException(Exception::TypeError(
          String::New("accepts only one argument, an object of key:value datasource options")));
    }
    
    if (!args[0]->IsObject())
        return ThrowException(Exception::TypeError(
          String::New("must provide an object, eg {type: 'shape', file : 'world.shp'}")));

    Local<Object> options = args[0]->ToObject();
    
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
    Local<Array> names = options->GetPropertyNames();
    uint32_t i = 0;
    uint32_t a_length = names->Length();
    while (i < a_length) {
        Local<Value> name = names->Get(i)->ToString();
        Local<Value> value = options->Get(name);
        params[TOSTR(name)] = TOSTR(value);
        i++;
    }

    datasource_ptr ds;
    try
    {
        ds = mapnik::datasource_cache::create(params, bind);
    }
    catch (const mapnik::config_error & ex )
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }
    catch (const mapnik::datasource_exception & ex )
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }
    catch (const std::runtime_error & ex )
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }
    if (ds)
    {
        Datasource* d = new Datasource();
        d->Wrap(args.This());
        d->datasource_ = ds;
        return args.This();
    }

    return Undefined();
}

Handle<Value> Datasource::parameters(const Arguments& args)
{
    HandleScope scope;
    Datasource* d = ObjectWrap::Unwrap<Datasource>(args.This());
    Local<Object> ds = Object::New();
    mapnik::parameters::const_iterator it = d->datasource_->params().begin();
    mapnik::parameters::const_iterator end = d->datasource_->params().end();
    for (; it != end; ++it)
    {
        params_to_object serializer( ds , it->first);
        boost::apply_visitor( serializer, it->second );
    }
    return scope.Close(ds);
}

Handle<Value> Datasource::describe(const Arguments& args)
{
    HandleScope scope;
    Datasource* d = ObjectWrap::Unwrap<Datasource>(args.This());
    Local<Object> description = Object::New();
    describe_datasource(description,d->datasource_);
    return scope.Close(description);
}

Handle<Value> Datasource::features(const Arguments& args)
{

    HandleScope scope;
  
    Datasource* d = ObjectWrap::Unwrap<Datasource>(args.This());
  
    // TODO - we don't know features.length at this point
    Local<Array> a = Array::New(0);
    datasource_features(a,d->datasource_);

    return scope.Close(a);
}