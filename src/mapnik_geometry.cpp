#include "utils.hpp"
#include "mapnik_geometry.hpp"

// mapnik
#include <mapnik/wkt/wkt_factory.hpp>

// boost
#include MAPNIK_MAKE_SHARED_INCLUDE

Persistent<FunctionTemplate> Geometry::constructor;

void Geometry::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Geometry::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Geometry"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "extent", extent);
    NODE_SET_PROTOTYPE_METHOD(lcons, "type", type);
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "Point",MAPNIK_POINT)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "LineString",MAPNIK_LINESTRING)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "Polygon",MAPNIK_POLYGON)
    target->Set(NanNew("Geometry"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Geometry::Geometry() :
    ObjectWrap(),
    this_() {}

Geometry::~Geometry()
{
}

NAN_METHOD(Geometry::New)
{
    NanScope();
    //if (!args.IsConstructCall()) {
    //    NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    //    NanReturnUndefined();
    //}

    if (args[0]->IsExternal())
    {
        Local<External> ext = args[0].As<External>();
        void* ptr = ext->Value();
        Geometry* g =  static_cast<Geometry*>(ptr);
        g->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else
    {
        NanThrowError("a mapnik.Geometry cannot be created directly - rather you should create mapnik.Path objects which can contain one or more geometries");
        NanReturnUndefined();
    }
    NanReturnValue(args.This());
}

NAN_METHOD(Geometry::extent)
{
    NanScope();

    Geometry* g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());

    Local<Array> a = NanNew<Array>(4);
    mapnik::box2d<double> const& e = g->this_->envelope();
    a->Set(0, NanNew<Number>(e.minx()));
    a->Set(1, NanNew<Number>(e.miny()));
    a->Set(2, NanNew<Number>(e.maxx()));
    a->Set(3, NanNew<Number>(e.maxy()));

    NanReturnValue(a);
}

NAN_METHOD(Geometry::type)
{
    NanScope();

    Geometry* g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());

    MAPNIK_GEOM_TYPE type = g->this_->type();
    // TODO - can we return the actual symbol?
    //NanReturnValue(constructor->GetFunction()->Get(NanNew("Point")));
    NanReturnValue(NanNew(static_cast<int>(type)));
}
