
#include "utils.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_geometry.hpp"

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/json/feature_parser.hpp>
#include <mapnik/value_types.hpp>
#include <mapnik/util/feature_to_geojson.hpp>

Persistent<FunctionTemplate> Feature::constructor;

/**
 * A single geographic feature, with geometry and properties. This is
 * typically derived from data by a datasource, but can be manually
 * created.
 *
 * @name mapnik.Feature
 * @class
 */
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

/**
 * @memberof mapnik.Feature
 * @static
 * @name fromJSON
 * @param {string} geojson string
 *
 * Create a feature from a GeoJSON representation.
 */
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

/**
 * @memberof mapnik.Feature
 * @name id
 * @instance
 * @returns {number} id the feature's internal id
 */
NAN_METHOD(Feature::id)
{
    NanScope();
    Feature* fp = node::ObjectWrap::Unwrap<Feature>(args.Holder());
    NanReturnValue(NanNew<Number>(fp->get()->id()));
}

/**
 * Get the feature's extent
 *
 * @name extent
 * @memberof mapnik.Feature
 * @instance
 * @returns {Array<number>} extent [minx, miny, maxx, maxy] order feature extent.
 */
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

/**
 * Get the feature's attributes as an object.
 *
 * @name attributes
 * @memberof mapnik.Feature
 * @instance
 * @returns {Object} attributes
 */
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


/**
 * Get the feature's attributes as a Mapnik geometry.
 *
 * @name geometry
 * @memberof mapnik.Feature
 * @instance
 * @returns {mapnik.Geometry} geometry
 */
NAN_METHOD(Feature::geometry)
{
    NanScope();
    Feature* fp = node::ObjectWrap::Unwrap<Feature>(args.Holder());
    NanReturnValue(Geometry::NewInstance(fp->get()));
}

/**
 * Generate and return a GeoJSON representation of this feature
 *
 * @instance
 * @name toJSON
 * @memberof mapnik.Feature
 * @returns {string} geojson Feature object in stringified GeoJSON
 */
NAN_METHOD(Feature::toJSON)
{
    NanScope();
    Feature* fp = node::ObjectWrap::Unwrap<Feature>(args.Holder());
    std::string json;
    if (!mapnik::util::to_geojson(json, *(fp->get())))
    {
        /* LCOV_EXCL_START */
        NanThrowError("Failed to generate GeoJSON");
        NanReturnUndefined();
        /* LCOV_EXCL_END */
    }
    NanReturnValue(NanNew(json.c_str()));
}

