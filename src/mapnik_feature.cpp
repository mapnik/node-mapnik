#include "utils.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_geometry.hpp"

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/json/feature_parser.hpp>
#include <mapnik/value/types.hpp>
#include <mapnik/util/feature_to_geojson.hpp>

Nan::Persistent<v8::FunctionTemplate> Feature::constructor;

/**
 * **`mapnik.Feature`**
 *
 * A single geographic feature, with geometry and properties. This is
 * typically derived from data by a datasource, but can be manually
 * created.
 *
 * @class Feature
 */
void Feature::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Feature::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Feature").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "id", id);
    Nan::SetPrototypeMethod(lcons, "extent", extent);
    Nan::SetPrototypeMethod(lcons, "attributes", attributes);
    Nan::SetPrototypeMethod(lcons, "geometry", geometry);
    Nan::SetPrototypeMethod(lcons, "toJSON", toJSON);

    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(),
                    "fromJSON",
                    Feature::fromJSON);

    Nan::Set(target, Nan::New("Feature").ToLocalChecked(),Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

Feature::Feature(mapnik::feature_ptr f) :
    Nan::ObjectWrap(),
    this_(f) {}

Feature::Feature(int id) :
    Nan::ObjectWrap(),
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
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = info[0].As<v8::External>();
        void* ptr = ext->Value();
        Feature* f =  static_cast<Feature*>(ptr);
        f->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }

    // TODO - expose mapnik.Context

    if (info.Length() > 1 || info.Length() < 1 || !info[0]->IsNumber()) {
        Nan::ThrowTypeError("requires one argument: an integer feature id");
        return;
    }

    Feature* f = new Feature(Nan::To<int>(info[0]).FromJust());
    f->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

/**
 * @memberof Feature
 * @static
 * @name fromJSON
 * @param {string} geojson string
 *
 * Create a feature from a GeoJSON representation.
 */
NAN_METHOD(Feature::fromJSON)
{
    if (info.Length() < 1 || !info[0]->IsString()) {
        Nan::ThrowTypeError("requires one argument: a string representing a GeoJSON feature");
        return;
    }
    std::string json = TOSTR(info[0]);
    try
    {
        mapnik::feature_ptr f(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
        if (!mapnik::json::from_geojson(json,*f))
        {
            Nan::ThrowError("Failed to read GeoJSON");
            return;
        }
        Feature* feat = new Feature(f);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(feat);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Feature instance");
        else info.GetReturnValue().Set(maybe_local.ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        // no way currently for any of the above code to throw,
        // but we'll keep this try catch to protect against mapnik or v8 changing
        /* LCOV_EXCL_START */
        Nan::ThrowError(ex.what());
        return;
        /* LCOV_EXCL_STOP */
    }
}

v8::Local<v8::Value> Feature::NewInstance(mapnik::feature_ptr f_ptr)
{
    Nan::EscapableHandleScope scope;
    Feature* f = new Feature(f_ptr);
    v8::Local<v8::Value> ext = Nan::New<v8::External>(f);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Feature instance");
    return scope.Escape(maybe_local.ToLocalChecked());
}

/**
 * @memberof Feature
 * @name id
 * @instance
 * @returns {number} id the feature's internal id
 */
NAN_METHOD(Feature::id)
{
    Feature* fp = Nan::ObjectWrap::Unwrap<Feature>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(fp->get()->id()));
}

/**
 * Get the feature's extent
 *
 * @name extent
 * @memberof Feature
 * @instance
 * @returns {Array<number>} extent [minx, miny, maxx, maxy] order feature extent.
 */
NAN_METHOD(Feature::extent)
{
    Feature* fp = Nan::ObjectWrap::Unwrap<Feature>(info.Holder());
    v8::Local<v8::Array> a = Nan::New<v8::Array>(4);
    mapnik::box2d<double> const& e = fp->get()->envelope();
    Nan::Set(a, 0, Nan::New<v8::Number>(e.minx()));
    Nan::Set(a, 1, Nan::New<v8::Number>(e.miny()));
    Nan::Set(a, 2, Nan::New<v8::Number>(e.maxx()));
    Nan::Set(a, 3, Nan::New<v8::Number>(e.maxy()));

    info.GetReturnValue().Set(a);
}

/**
 * Get the feature's attributes as an object.
 *
 * @name attributes
 * @memberof Feature
 * @instance
 * @returns {Object} attributes
 */
NAN_METHOD(Feature::attributes)
{
    Feature* fp = Nan::ObjectWrap::Unwrap<Feature>(info.Holder());
    v8::Local<v8::Object> feat = Nan::New<v8::Object>();
    mapnik::feature_ptr feature = fp->get();
    if (feature)
    {
        for (auto const& attr : *feature)
        {
            Nan::Set(feat, Nan::New<v8::String>(std::get<0>(attr)).ToLocalChecked(),
                      mapnik::util::apply_visitor(node_mapnik::value_converter(), std::get<1>(attr))
                );
        }
    }
    info.GetReturnValue().Set(feat);
}


/**
 * Get the feature's attributes as a Mapnik geometry.
 *
 * @name geometry
 * @memberof Feature
 * @instance
 * @returns {mapnik.Geometry} geometry
 */
NAN_METHOD(Feature::geometry)
{
    Feature* fp = Nan::ObjectWrap::Unwrap<Feature>(info.Holder());
    info.GetReturnValue().Set(Geometry::NewInstance(fp->get()));
}

/**
 * Generate and return a GeoJSON representation of this feature
 *
 * @instance
 * @name toJSON
 * @memberof Feature
 * @returns {string} geojson Feature object in stringified GeoJSON
 */
NAN_METHOD(Feature::toJSON)
{
    Feature* fp = Nan::ObjectWrap::Unwrap<Feature>(info.Holder());
    std::string json;
    if (!mapnik::util::to_geojson(json, *(fp->get())))
    {
        /* LCOV_EXCL_START */
        Nan::ThrowError("Failed to generate GeoJSON");
        return;
        /* LCOV_EXCL_STOP */
    }
    info.GetReturnValue().Set(Nan::New<v8::String>(json).ToLocalChecked());
}
