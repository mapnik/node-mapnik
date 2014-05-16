#include "mapnik_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "utils.hpp"
#include "ds_emitter.hpp"

// node
#include "node.h"                       // for NODE_SET_PROTOTYPE_METHOD
#include "node_object_wrap.h"           // for ObjectWrap
#include "v8.h"                         // for String, Handle, Object, etc

// mapnik
#include <mapnik/attribute_descriptor.hpp>  // for attribute_descriptor
#include <mapnik/box2d.hpp>             // for box2d
#include <mapnik/datasource.hpp>        // for datasource, datasource_ptr, etc
#include <mapnik/datasource_cache.hpp>  // for datasource_cache
#include <mapnik/feature_layer_desc.hpp>  // for layer_descriptor
#include <mapnik/params.hpp>            // for parameters
#include <mapnik/query.hpp>             // for query
#include <mapnik/version.hpp>           // for MAPNIK_VERSION

// stl
#include <exception>
#include <vector>

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
    NODE_SET_PROTOTYPE_METHOD(constructor, "featureset", featureset);
    NODE_SET_PROTOTYPE_METHOD(constructor, "extent", extent);

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

    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Datasource* d =  static_cast<Datasource*>(ptr);
        if (d->datasource_->type() == mapnik::datasource::Raster)
        {
            args.This()->Set(String::NewSymbol("type"),
                             String::NewSymbol("raster"),
                             static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete));
        }
        else
        {
            args.This()->Set(String::NewSymbol("type"),
                             String::NewSymbol("vector"),
                             static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete));
        }
        d->Wrap(args.This());
        return args.This();
    }
    if (args.Length() != 1){
        return ThrowException(Exception::TypeError(
                                  String::New("accepts only one argument, an object of key:value datasource options")));
    }

    if (!args[0]->IsObject())
        return ThrowException(Exception::TypeError(
                                  String::New("Must provide an object, eg {type: 'shape', file : 'world.shp'}")));

    Local<Object> options = args[0]->ToObject();

    mapnik::parameters params;
    Local<Array> names = options->GetPropertyNames();
    unsigned int i = 0;
    unsigned int a_length = names->Length();
    while (i < a_length) {
        Local<Value> name = names->Get(i)->ToString();
        Local<Value> value = options->Get(name);
        // TODO - don't treat everything as strings
        params[TOSTR(name)] = TOSTR(value);
        i++;
    }

    mapnik::datasource_ptr ds;
    try
    {
#if MAPNIK_VERSION >= 200200
        ds = mapnik::datasource_cache::instance().create(params);
#else
        ds = mapnik::datasource_cache::instance()->create(params);
#endif
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }

    if (ds)
    {
        if (ds->type() == mapnik::datasource::Raster)
        {
            args.This()->Set(String::NewSymbol("type"),
                             String::NewSymbol("raster"),
                             static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete));
        }
        else
        {
            args.This()->Set(String::NewSymbol("type"),
                             String::NewSymbol("vector"),
                             static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete));
        }
        Datasource* d = new Datasource();
        d->Wrap(args.This());
        d->datasource_ = ds;
        return args.This();
    }
    return Undefined();
}

Handle<Value> Datasource::New(mapnik::datasource_ptr ds_ptr) {
    HandleScope scope;
    Datasource* d = new Datasource();
    d->datasource_ = ds_ptr;
    Handle<Value> ext = External::New(d);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);
}

Handle<Value> Datasource::parameters(const Arguments& args)
{
    HandleScope scope;
    Datasource* d = node::ObjectWrap::Unwrap<Datasource>(args.This());
    Local<Object> ds = Object::New();
    mapnik::parameters::const_iterator it = d->datasource_->params().begin();
    mapnik::parameters::const_iterator end = d->datasource_->params().end();
    for (; it != end; ++it)
    {
        node_mapnik::params_to_object serializer( ds , it->first);
        boost::apply_visitor( serializer, it->second );
    }
    return scope.Close(ds);
}

Handle<Value> Datasource::extent(const Arguments& args)
{
    HandleScope scope;
    Datasource* d = node::ObjectWrap::Unwrap<Datasource>(args.This());
    mapnik::box2d<double> e;
    try
    {
        e = d->datasource_->envelope();
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }

    Local<Array> a = Array::New(4);
    a->Set(0, Number::New(e.minx()));
    a->Set(1, Number::New(e.miny()));
    a->Set(2, Number::New(e.maxx()));
    a->Set(3, Number::New(e.maxy()));
    return scope.Close(a);
}

Handle<Value> Datasource::describe(const Arguments& args)
{
    HandleScope scope;
    Datasource* d = node::ObjectWrap::Unwrap<Datasource>(args.This());
    Local<Object> description = Object::New();
    try
    {
        node_mapnik::describe_datasource(description,d->datasource_);
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }

    return scope.Close(description);
}

Handle<Value> Datasource::features(const Arguments& args)
{

    std::clog << "Datasource.features() is deprecated and will be removed at node-mapnik 1.3.x (please use Datasource.featureset instead)\n";
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

    Datasource* d = node::ObjectWrap::Unwrap<Datasource>(args.This());

    // TODO - we don't know features.length at this point
    Local<Array> a = Array::New(0);
    try
    {
        node_mapnik::datasource_features(a,d->datasource_,first,last);
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }

    return scope.Close(a);
}

Handle<Value> Datasource::featureset(const Arguments& args)
{

    HandleScope scope;

    Datasource* ds = node::ObjectWrap::Unwrap<Datasource>(args.This());

    mapnik::featureset_ptr fs;
    try
    {
        mapnik::query q(ds->datasource_->envelope());
        mapnik::layer_descriptor ld = ds->datasource_->get_descriptor();
        std::vector<mapnik::attribute_descriptor> const& desc = ld.get_descriptors();
        std::vector<mapnik::attribute_descriptor>::const_iterator itr = desc.begin();
        std::vector<mapnik::attribute_descriptor>::const_iterator end = desc.end();
        while (itr != end)
        {
            q.add_property_name(itr->get_name());
            ++itr;
        }

        fs = ds->datasource_->features(q);
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }

    if (fs)
    {
        return scope.Close(Featureset::New(fs));
    }

    return Undefined();
}
