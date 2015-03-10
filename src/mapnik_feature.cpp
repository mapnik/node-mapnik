
#include "utils.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_geometry.hpp"

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/json/feature_parser.hpp>
#include <mapnik/value_types.hpp>

#include <mapnik/json/feature_generator_grammar.hpp>

Persistent<FunctionTemplate> Feature::constructor;

void Feature::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Feature::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Feature"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "id", id);
    NODE_SET_PROTOTYPE_METHOD(lcons, "extent", extent);
    NODE_SET_PROTOTYPE_METHOD(lcons, "attributes", attributes);
    NODE_SET_PROTOTYPE_METHOD(lcons, "geometry", geometry);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toJSON", toJSON);

    NODE_SET_METHOD(lcons->GetFunction(),
                    "fromJSON",
                    Feature::fromJSON);

    target->Set(NanNew("Feature"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Feature::Feature(mapnik::feature_ptr f) :
    node::ObjectWrap(),
    this_(f) {}

Feature::Feature(int id) :
    node::ObjectWrap(),
    this_() {
    // TODO - accept/require context object to reused
    ctx_ = std::make_shared<mapnik::context_type>();
    this_ = mapnik::feature_factory::create(ctx_,id);
}

Feature::~Feature()
{
}

NAN_METHOD(Feature::New)
{
    NanScope();

    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args[0]->IsExternal())
    {
        Local<External> ext = args[0].As<External>();
        void* ptr = ext->Value();
        Feature* f =  static_cast<Feature*>(ptr);
        f->Wrap(args.This());
        NanReturnValue(args.This());
    }

    // TODO - expose mapnik.Context

    if (args.Length() > 1 || args.Length() < 1 || !args[0]->IsNumber()) {
        NanThrowTypeError("requires one argument: an integer feature id");
        NanReturnUndefined();
    }

    Feature* f = new Feature(args[0]->IntegerValue());
    f->Wrap(args.This());
    NanReturnValue(args.This());
}

NAN_METHOD(Feature::fromJSON)
{
    NanScope();
    if (args.Length() < 1 || !args[0]->IsString()) {
        NanThrowTypeError("requires one argument: a string representing a GeoJSON feature");
        NanReturnUndefined();
    }
    std::string json = TOSTR(args[0]);
    try
    {
        mapnik::feature_ptr f(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
        if (!mapnik::json::from_geojson(json,*f))
        {
            NanThrowError("Failed to read GeoJSON");
            NanReturnUndefined();
        }
        Feature* feat = new Feature(f);
        Handle<Value> ext = NanNew<External>(feat);
        NanReturnValue(NanNew(constructor)->GetFunction()->NewInstance(1, &ext));
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
}

Handle<Value> Feature::NewInstance(mapnik::feature_ptr f_ptr)
{
    NanEscapableScope();
    Feature* f = new Feature(f_ptr);
    Handle<Value> ext = NanNew<External>(f);
    Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
    return NanEscapeScope(obj);
}

NAN_METHOD(Feature::id)
{
    NanScope();
    Feature* fp = node::ObjectWrap::Unwrap<Feature>(args.Holder());
    NanReturnValue(NanNew<Number>(fp->get()->id()));
}

NAN_METHOD(Feature::extent)
{
    NanScope();

    Feature* fp = node::ObjectWrap::Unwrap<Feature>(args.Holder());

    Local<Array> a = NanNew<Array>(4);
    mapnik::box2d<double> const& e = fp->get()->envelope();
    a->Set(0, NanNew<Number>(e.minx()));
    a->Set(1, NanNew<Number>(e.miny()));
    a->Set(2, NanNew<Number>(e.maxx()));
    a->Set(3, NanNew<Number>(e.maxy()));

    NanReturnValue(a);
}

NAN_METHOD(Feature::attributes)
{
    NanScope();

    Feature* fp = node::ObjectWrap::Unwrap<Feature>(args.Holder());

    Local<Object> feat = NanNew<Object>();

    mapnik::feature_ptr feature = fp->get();
    mapnik::feature_impl::iterator itr = feature->begin();
    mapnik::feature_impl::iterator end = feature->end();
    for ( ;itr!=end; ++itr)
    {
        feat->Set(NanNew(std::get<0>(*itr).c_str()), 
                  mapnik::util::apply_visitor(node_mapnik::value_converter(), std::get<1>(*itr))
        );
    }
    NanReturnValue(feat);
}

NAN_METHOD(Feature::geometry)
{
    NanScope();
    Feature* fp = node::ObjectWrap::Unwrap<Feature>(args.Holder());
    NanReturnValue(Geometry::NewInstance(fp->get()));
}

NAN_METHOD(Feature::toJSON)
{
    NanScope();
    Feature* fp = node::ObjectWrap::Unwrap<Feature>(args.Holder());
    typedef std::back_insert_iterator<std::string> sink_type;
    static const mapnik::json::feature_generator_grammar<sink_type,mapnik::feature_impl> grammar;
    std::string json;
    sink_type sink(json);
    if (!boost::spirit::karma::generate(sink, grammar, *(fp->get())))
    {
        /* LCOV_EXCL_START */
        NanThrowError("Failed to generate GeoJSON");
        NanReturnUndefined();
        /* LCOV_EXCL_END */
    }
    NanReturnValue(NanNew(json.c_str()));
}

