#include "utils.hpp"
#include "mapnik_geometry.hpp"

// mapnik
#include <mapnik/wkt/wkt_factory.hpp>

// boost
#include <boost/make_shared.hpp>

Persistent<FunctionTemplate> Geometry::constructor;

void Geometry::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Geometry::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Geometry"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "extent", extent);
    NODE_SET_PROTOTYPE_METHOD(constructor, "type", type);
    NODE_MAPNIK_DEFINE_CONSTANT(constructor->GetFunction(),
                                "Point",mapnik::Point)
    NODE_MAPNIK_DEFINE_CONSTANT(constructor->GetFunction(),
                                "LineString",mapnik::LineString)
    NODE_MAPNIK_DEFINE_CONSTANT(constructor->GetFunction(),
                                "Polygon",mapnik::Polygon)
    target->Set(String::NewSymbol("Geometry"),constructor->GetFunction());
}

Geometry::Geometry() :
    ObjectWrap(),
    this_() {}

Geometry::~Geometry()
{
}

Handle<Value> Geometry::New(const Arguments& args)
{
    HandleScope scope;
    //if (!args.IsConstructCall())
    //    return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Geometry* g =  static_cast<Geometry*>(ptr);
        g->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
                                  String::New("a mapnik.Geometry cannot be created directly - rather you should create mapnik.Path objects which can contain one or more geometries")));
    }
    return args.This();
}

Handle<Value> Geometry::extent(const Arguments& args)
{
    HandleScope scope;

    Geometry* g = ObjectWrap::Unwrap<Geometry>(args.This());

    Local<Array> a = Array::New(4);
    mapnik::box2d<double> const& e = g->get()->envelope();
    a->Set(0, Number::New(e.minx()));
    a->Set(1, Number::New(e.miny()));
    a->Set(2, Number::New(e.maxx()));
    a->Set(3, Number::New(e.maxy()));

    return scope.Close(a);
}

Handle<Value> Geometry::type(const Arguments& args)
{
    HandleScope scope;

    Geometry* g = ObjectWrap::Unwrap<Geometry>(args.This());

    mapnik::eGeomType type = g->get()->type();
    // TODO - can we return the actual symbol?
    //return scope.Close(constructor->GetFunction()->Get(String::NewSymbol("Point")));
    return scope.Close(Integer::New(static_cast<int>(type)));
}
