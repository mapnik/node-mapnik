#include "utils.hpp"
#include "mapnik_feature.hpp"

// mapnik
#include <mapnik/feature_factory.hpp>

#include <boost/make_shared.hpp>

Persistent<FunctionTemplate> Feature::constructor;

void Feature::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Feature::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Feature"));
    
    NODE_SET_PROTOTYPE_METHOD(constructor, "id", id);
    NODE_SET_PROTOTYPE_METHOD(constructor, "extent", extent);
    NODE_SET_PROTOTYPE_METHOD(constructor, "attributes", attributes);
    target->Set(String::NewSymbol("Feature"),constructor->GetFunction());
}

Feature::Feature(mapnik::feature_ptr f) :
  ObjectWrap(),
  this_(f) {}

Feature::Feature(int id) :
  ObjectWrap(),
  this_(mapnik::feature_factory::create(id)) {}
  
Feature::~Feature()
{
}

Handle<Value> Feature::New(const Arguments& args)
{
    HandleScope scope;

    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Feature* f =  static_cast<Feature*>(ptr);
        f->Wrap(args.This());
        return args.This();
    }
    
    if (!args.Length() == 1 || !args[0]->IsNumber()) {
        return ThrowException(Exception::TypeError(
          String::New("requires one argument: an integer feature id")));
    }
    
    Feature* f = new Feature(args[0]->IntegerValue());
    f->Wrap(args.This());
    return args.This();
}

Handle<Value> Feature::New(mapnik::feature_ptr f_ptr)
{
    HandleScope scope;
    Feature* f = new Feature(f_ptr);
    Handle<Value> ext = External::New(f);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);
}

Handle<Value> Feature::id(const Arguments& args)
{
    HandleScope scope;

    Feature* fp = ObjectWrap::Unwrap<Feature>(args.This());
    // TODO - provide custom 64 bit integer type?
    return scope.Close(Integer::New(fp->get()->id()));
}

Handle<Value> Feature::extent(const Arguments& args)
{
    HandleScope scope;

    Feature* fp = ObjectWrap::Unwrap<Feature>(args.This());

    Local<Array> a = Array::New(4);
    mapnik::box2d<double> const& e = fp->get()->envelope();
    a->Set(0, Number::New(e.minx()));
    a->Set(1, Number::New(e.miny()));
    a->Set(2, Number::New(e.maxx()));
    a->Set(3, Number::New(e.maxy()));
 
    return scope.Close(a);
}

Handle<Value> Feature::attributes(const Arguments& args)
{
    HandleScope scope;

    Feature* fp = ObjectWrap::Unwrap<Feature>(args.This());
    
    Local<Object> feat = Object::New();

    std::map<std::string,mapnik::value> const& fprops = fp->get()->props();
    std::map<std::string,mapnik::value>::const_iterator it = fprops.begin();
    std::map<std::string,mapnik::value>::const_iterator end = fprops.end();
    for (; it != end; ++it)
    {
        node_mapnik::params_to_object serializer( feat , it->first);
        boost::apply_visitor( serializer, it->second.base() );
    }
    
    return scope.Close(feat);
}

