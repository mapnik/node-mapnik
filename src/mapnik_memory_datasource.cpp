
// mapnik
#include <mapnik/version.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/value_types.hpp>

#include "mapnik_memory_datasource.hpp"
#include "mapnik_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "utils.hpp"
#include "ds_emitter.hpp"

// stl
#include <exception>

// boost
#include MAPNIK_MAKE_SHARED_INCLUDE

Persistent<FunctionTemplate> MemoryDatasource::constructor;

void MemoryDatasource::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(MemoryDatasource::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("MemoryDatasource"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(constructor, "parameters", parameters);
    NODE_SET_PROTOTYPE_METHOD(constructor, "describe", describe);
    NODE_SET_PROTOTYPE_METHOD(constructor, "features", features);
    NODE_SET_PROTOTYPE_METHOD(constructor, "featureset", featureset);
    NODE_SET_PROTOTYPE_METHOD(constructor, "add", add);

    target->Set(String::NewSymbol("MemoryDatasource"),constructor->GetFunction());
}

MemoryDatasource::MemoryDatasource() :
    ObjectWrap(),
    datasource_(),
    feature_id_(1),
    tr_(new mapnik::transcoder("utf8")) {}

MemoryDatasource::~MemoryDatasource()
{
}

Handle<Value> MemoryDatasource::New(const Arguments& args)
{
    HandleScope scope;

    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        MemoryDatasource* d =  static_cast<MemoryDatasource*>(ptr);
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
        params[TOSTR(name)] = TOSTR(value);
        i++;
    }

    //memory_datasource cache;
    MemoryDatasource* d = new MemoryDatasource();
    d->Wrap(args.This());
    d->datasource_ = MAPNIK_MAKE_SHARED<mapnik::memory_datasource>();
    return args.This();
}

Handle<Value> MemoryDatasource::New(mapnik::datasource_ptr ds_ptr) {
    HandleScope scope;
    MemoryDatasource* d = new MemoryDatasource();
    d->datasource_ = ds_ptr;
    Handle<Value> ext = External::New(d);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);
}

Handle<Value> MemoryDatasource::parameters(const Arguments& args)
{
    HandleScope scope;
    MemoryDatasource* d = node::ObjectWrap::Unwrap<MemoryDatasource>(args.This());
    Local<Object> ds = Object::New();
    if (d->datasource_) {
        mapnik::parameters::const_iterator it = d->datasource_->params().begin();
        mapnik::parameters::const_iterator end = d->datasource_->params().end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object serializer( ds , it->first);
            boost::apply_visitor( serializer, it->second );
        }
    }
    return scope.Close(ds);
}

Handle<Value> MemoryDatasource::describe(const Arguments& args)
{
    HandleScope scope;
    MemoryDatasource* d = node::ObjectWrap::Unwrap<MemoryDatasource>(args.This());
    Local<Object> description = Object::New();
    if (d->datasource_) {
        try {
            node_mapnik::describe_datasource(description,d->datasource_);
        }
        catch (std::exception const& ex)
        {
            return ThrowException(Exception::Error(
                                      String::New(ex.what())));
        }
    }
    return scope.Close(description);
}

Handle<Value> MemoryDatasource::features(const Arguments& args)
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

    MemoryDatasource* d = node::ObjectWrap::Unwrap<MemoryDatasource>(args.This());

    // TODO - we don't know features.length at this point
    Local<Array> a = Array::New(0);
    if (d->datasource_)
    {
        try
        {
            node_mapnik::datasource_features(a,d->datasource_,first,last);
        }
        catch (std::exception const& ex)
        {
            return ThrowException(Exception::Error(
                                      String::New(ex.what())));
        }
    }

    return scope.Close(a);
}

Handle<Value> MemoryDatasource::featureset(const Arguments& args)
{

    HandleScope scope;

    MemoryDatasource* d = node::ObjectWrap::Unwrap<MemoryDatasource>(args.This());

    try
    {
        if (d->datasource_) {
            mapnik::query q(d->datasource_->envelope());
            mapnik::layer_descriptor ld = d->datasource_->get_descriptor();
            std::vector<mapnik::attribute_descriptor> const& desc = ld.get_descriptors();
            std::vector<mapnik::attribute_descriptor>::const_iterator itr = desc.begin();
            std::vector<mapnik::attribute_descriptor>::const_iterator end = desc.end();
            while (itr != end)
            {
                q.add_property_name(itr->get_name());
                ++itr;
            }
            mapnik::featureset_ptr fs = d->datasource_->features(q);
            if (fs)
            {
                return scope.Close(Featureset::New(fs));
            }
        }
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }

    return Undefined();
}

Handle<Value> MemoryDatasource::add(const Arguments& args)
{

    HandleScope scope;

    if ((args.Length() != 1) || !args[0]->IsObject())
    {
        return ThrowException(Exception::Error(
                                  String::New("accepts one argument: an object including x and y (or wkt) and properties")));
    }

    MemoryDatasource* d = node::ObjectWrap::Unwrap<MemoryDatasource>(args.This());

    Local<Object> obj = args[0]->ToObject();

    if (obj->Has(String::New("wkt")) || (obj->Has(String::New("x")) && obj->Has(String::New("y"))))
    {
        if (obj->Has(String::New("wkt")))
            return ThrowException(Exception::Error(
                                      String::New("wkt not yet supported")));

        Local<Value> x = obj->Get(String::New("x"));
        Local<Value> y = obj->Get(String::New("y"));
        if (!x->IsUndefined() && x->IsNumber() && !y->IsUndefined() && y->IsNumber())
        {
            mapnik::geometry_type * pt = new mapnik::geometry_type(MAPNIK_POINT);
            pt->move_to(x->NumberValue(),y->NumberValue());
#if MAPNIK_VERSION >= 200100
            mapnik::context_ptr ctx = MAPNIK_MAKE_SHARED<mapnik::context_type>();
            mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,d->feature_id_));
#else
            mapnik::feature_ptr feature(mapnik::feature_factory::create(d->feature_id_));
#endif
            ++(d->feature_id_);
            feature->add_geometry(pt);
            if (obj->Has(String::New("properties")))
            {
                Local<Value> props = obj->Get(String::New("properties"));
                if (props->IsObject())
                {
                    Local<Object> p_obj = props->ToObject();
                    Local<Array> names = p_obj->GetPropertyNames();
                    unsigned int i = 0;
                    unsigned int a_length = names->Length();
                    while (i < a_length)
                    {
                        Local<Value> name = names->Get(i)->ToString();
                        // if name in q.property_names() ?
                        Local<Value> value = p_obj->Get(name);
                        if (value->IsString()) {
                            mapnik::value_unicode_string ustr = d->tr_->transcode(TOSTR(value));
#if MAPNIK_VERSION >= 200100
                            feature->put_new(TOSTR(name),ustr);
#else
                            boost::put(*feature,TOSTR(name),ustr);
#endif
                        } else if (value->IsNumber()) {
                            double num = value->NumberValue();
                            // todo - round
                            if (num == value->IntegerValue()) {
#if MAPNIK_VERSION >= 200100
                                feature->put_new(TOSTR(name),static_cast<node_mapnik::value_integer>(value->IntegerValue()));
#else
                                boost::put(*feature,TOSTR(name),static_cast<int>(value->IntegerValue()));
#endif
                            } else {
                                double dub_val = value->NumberValue();
#if MAPNIK_VERSION >= 200100
                                feature->put_new(TOSTR(name),dub_val);
#else
                                boost::put(*feature,TOSTR(name),dub_val);
#endif
                            }
                        } else if (value->IsNull()) {
#if MAPNIK_VERSION >= 200100
                            feature->put_new(TOSTR(name),mapnik::value_null());
#else
                            boost::put(*feature,TOSTR(name),mapnik::value_null());
#endif
                        } else {
                            std::clog << "unhandled type for property: " << TOSTR(name) << "\n";
                        }
                        i++;
                    }
                }
            }
            mapnik::memory_datasource *cache = dynamic_cast<mapnik::memory_datasource *>(d->datasource_.get());
            cache->push(feature);
        }
    }
    return scope.Close(False());
}
