
#include "utils.hpp"
#include "mapnik_layer.hpp"
#include "mapnik_datasource.hpp"
#include "mapnik_js_datasource.hpp"
#include "mapnik_memory_datasource.hpp"
#include "ds_emitter.hpp"
#include "layer_emitter.hpp"

//#include <node_buffer.h>
#include <node_version.h>

// boost
#include <boost/make_shared.hpp>

// mapnik
#include <mapnik/layer.hpp>

Persistent<FunctionTemplate> Layer::constructor;

void Layer::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Layer::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Layer"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(constructor, "describe", describe);
    NODE_SET_PROTOTYPE_METHOD(constructor, "features", features);
    NODE_SET_PROTOTYPE_METHOD(constructor, "describe_data", describe_data);

    // properties
    ATTR(constructor, "name", get_prop, set_prop);
    ATTR(constructor, "srs", get_prop, set_prop);
    ATTR(constructor, "styles", get_prop, set_prop);
    ATTR(constructor, "datasource", get_prop, set_prop);

    target->Set(String::NewSymbol("Layer"),constructor->GetFunction());
}

Layer::Layer(std::string const& name) :
  ObjectWrap(),
  layer_(boost::make_shared<mapnik::layer>(name)) {}

Layer::Layer(std::string const& name, std::string const& srs) :
  ObjectWrap(),
  layer_(boost::make_shared<mapnik::layer>(name,srs)) {}

Layer::Layer() :
  ObjectWrap(),
  layer_() {}


Layer::~Layer()
{
    //std::clog << "~node.Layer\n";
    // release is handled by boost::shared_ptr
}

Handle<Value> Layer::New(const Arguments& args)
{
    HandleScope scope;

    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    /*
    if (args.Length() > 0) {
        Local<Object> obj = args[0]->ToObject();
        if (!Layer::constructor->HasInstance(obj)) {
          Layer* l = ObjectWrap::Unwrap<Layer>(obj);
          // segfault
          l->Wrap(obj);
          return obj;
        }
    }
    */

    if (args[0]->IsExternal())
    {
        //return ThrowException(String::New("No support yet for passing v8:External wrapper around C++ void*"));
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Layer* l =  static_cast<Layer*>(ptr);
        l->Wrap(args.This());
        return args.This();
    }

    if (args.Length() == 1)
    {
        if (!args[0]->IsString())
        {
            return ThrowException(Exception::TypeError(
                String::New("'name' must be a string")));
        }
        Layer* l = new Layer(TOSTR(args[0]));
        l->Wrap(args.This());
        return args.This();
    }
    else if (args.Length() == 2)
    {
        if (!args[0]->IsString() || !args[1]->IsString())
            return ThrowException(Exception::TypeError(
               String::New("'name' and 'srs' must be a strings")));
        Layer* l = new Layer(TOSTR(args[0]),TOSTR(args[1]));
        l->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::TypeError(
          String::New("please provide Layer name and optional srs")));
    }

    //return args.This();
    return Undefined();

}

/*
Handle<Value> Layer::New(mapnik::layer & lay_ref) {
    HandleScope scope;
    Handle<Value> ext = String::New("temporary");
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    Layer* l = ObjectWrap::Unwrap<Layer>(obj);
    *l->layer_ = lay_ref;
    return obj;
}
*/

Handle<Value> Layer::New(mapnik::layer & lay_ref) {
    HandleScope scope;
    Layer* l = new Layer();
    // copy new mapnik::layer into the shared_ptr
    l->layer_ = boost::make_shared<mapnik::layer>(lay_ref);
    Handle<Value> ext = External::New(l);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);
}

Handle<Value> Layer::get_prop(Local<String> property,
                         const AccessorInfo& info)
{
    HandleScope scope;
    Layer* l = ObjectWrap::Unwrap<Layer>(info.This());
    std::string a = TOSTR(property);
    if (a == "name")
        return scope.Close(String::New(l->layer_->name().c_str()));
    else if (a == "srs")
        return scope.Close(String::New(l->layer_->srs().c_str()));
    else if (a == "styles") {
        std::vector<std::string> const& style_names = l->layer_->styles();
        Local<Array> s = Array::New(style_names.size());
        for (unsigned i = 0; i < style_names.size(); ++i)
        {
            s->Set(i, String::New(style_names[i].c_str()) );
        }
        return scope.Close(s);
    }
    else if (a == "datasource") {
        mapnik::datasource_ptr ds = l->layer_->datasource();
        if (ds)
        {
            return scope.Close(Datasource::New(ds));
        }
    }
    return Undefined();
}

void Layer::set_prop(Local<String> property,
                         Local<Value> value,
                         const AccessorInfo& info)
{
    HandleScope scope;
    Layer* l = ObjectWrap::Unwrap<Layer>(info.Holder());
    std::string a = TOSTR(property);
    if (a == "name")
    {
        if (!value->IsString()) {
            ThrowException(Exception::Error(
               String::New("'name' must be a string")));
        } else {
            l->layer_->set_name(TOSTR(value));
        }
    }
    else if (a == "srs")
    {
        if (!value->IsString()) {
            ThrowException(Exception::Error(
               String::New("'srs' must be a string")));
        } else {
            l->layer_->set_srs(TOSTR(value));
        }
    }
    else if (a == "styles")
    {
        if (!value->IsArray())
            ThrowException(Exception::Error(
               String::New("Must provide an array of style names")));
        else {
            Local<Array> a = Local<Array>::Cast(value->ToObject());
            // todo - how to check if cast worked?
            uint32_t i = 0;
            uint32_t a_length = a->Length();
            while (i < a_length) {
                l->layer_->add_style(TOSTR(a->Get(i)));
                i++;
            }
        }
    }
    else if (a == "datasource")
    {
        Local<Object> obj = value->ToObject();
        if (value->IsNull() || value->IsUndefined()) {
            ThrowException(Exception::TypeError(String::New("mapnik.Datasource, mapnik.JSDatasource, or mapnik.MemoryDatasource instance expected")));
        } else {
            if (Datasource::constructor->HasInstance(obj)) {
                JSDatasource *d = ObjectWrap::Unwrap<JSDatasource>(obj);
                // TODO - addLayer should be add_layer in mapnik
                l->layer_->set_datasource(d->get());
            }
            else if (JSDatasource::constructor->HasInstance(obj))
            {
                JSDatasource *d = ObjectWrap::Unwrap<JSDatasource>(obj);
                // TODO - addLayer should be add_layer in mapnik
                l->layer_->set_datasource(d->get());
            }
            else if (MemoryDatasource::constructor->HasInstance(obj))
            {
                MemoryDatasource *d = ObjectWrap::Unwrap<MemoryDatasource>(obj);
                // TODO - addLayer should be add_layer in mapnik
                l->layer_->set_datasource(d->get());
            }
            else
            {
                ThrowException(Exception::TypeError(String::New("mapnik.Datasource, mapnik.JSDatasource, or mapnik.MemoryDatasource instance expected")));
            }
        }
    }
}


Handle<Value> Layer::describe(const Arguments& args)
{
    HandleScope scope;

    Layer* l = ObjectWrap::Unwrap<Layer>(args.This());

    Local<Object> description = Object::New();
    layer_as_json(description,*l->layer_);

    return scope.Close(description);
}

Handle<Value> Layer::describe_data(const Arguments& args)
{
    HandleScope scope;
    Layer* l = ObjectWrap::Unwrap<Layer>(args.This());
    Local<Object> description = Object::New();
    mapnik::datasource_ptr ds = l->layer_->datasource();
    if (ds)
    {
        describe_datasource(description,ds);
    }
    return scope.Close(description);
}

Handle<Value> Layer::features(const Arguments& args)
{

    HandleScope scope;

    unsigned first = 0;
    unsigned last = 0;
    // we are slicing
    if (args.Length() == 2)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
            return ThrowException(Exception::Error(
               String::New("Index of 'first' and 'last' feature must be an integer")));
        first = args[0]->IntegerValue();
        last = args[1]->IntegerValue();
    }


    Layer* l = ObjectWrap::Unwrap<Layer>(args.This());

    // TODO - we don't know features.length at this point
    Local<Array> a = Array::New(0);
    mapnik::datasource_ptr ds = l->layer_->datasource();
    if (ds)
    {
        datasource_features(a,ds,first,last);
    }

    return scope.Close(a);
}

