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

Napi::FunctionReference Feature::constructor;

/**
 * **`mapnik.Feature`**
 *
 * A single geographic feature, with geometry and properties. This is
 * typically derived from data by a datasource, but can be manually
 * created.
 *
 * @class Feature
 */
void Feature::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, Feature::New);

    lcons->SetClassName(Napi::String::New(env, "Feature"));

    InstanceMethod("id", &id),
    InstanceMethod("extent", &extent),
    InstanceMethod("attributes", &attributes),
    InstanceMethod("geometry", &geometry),
    InstanceMethod("toJSON", &toJSON),

    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(),
                    "fromJSON",
                    Feature::fromJSON);

    (target).Set(Napi::String::New(env, "Feature"),Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

Feature::Feature(mapnik::feature_ptr f) : Napi::ObjectWrap<Feature>(),
    this_(f) {}

Feature::Feature(int id) : Napi::ObjectWrap<Feature>(),
    this_() {
    // TODO - accept/require context object to reused
    ctx_ = std::make_shared<mapnik::context_type>();
    this_ = mapnik::feature_factory::create(ctx_,id);
}

Feature::~Feature()
{
}

Napi::Value Feature::New(Napi::CallbackInfo const& info)
{
    if (!info.IsConstructCall())
    {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info[0].IsExternal())
    {
        Napi::External ext = info[0].As<Napi::External>();
        void* ptr = ext->Value();
        Feature* f =  static_cast<Feature*>(ptr);
        f->Wrap(info.This());
        return info.This();
        return;
    }

    // TODO - expose mapnik.Context

    if (info.Length() > 1 || info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "requires one argument: an integer feature id").ThrowAsJavaScriptException();
        return env.Null();
    }

    Feature* f = new Feature(info[0].As<Napi::Number>().Int32Value());
    f->Wrap(info.This());
    return info.This();
}

/**
 * @memberof Feature
 * @static
 * @name fromJSON
 * @param {string} geojson string
 *
 * Create a feature from a GeoJSON representation.
 */
Napi::Value Feature::fromJSON(Napi::CallbackInfo const& info)
{
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "requires one argument: a string representing a GeoJSON feature").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::string json = TOSTR(info[0]);
    try
    {
        mapnik::feature_ptr f(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
        if (!mapnik::json::from_geojson(json,*f))
        {
            Napi::Error::New(env, "Failed to read GeoJSON").ThrowAsJavaScriptException();
            return env.Null();
        }
        Feature* feat = new Feature(f);
        Napi::Value ext = Napi::External::New(env, feat);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Feature instance").ThrowAsJavaScriptException();

        else return maybe_local;
    }
    catch (std::exception const& ex)
    {
        // no way currently for any of the above code to throw,
        // but we'll keep this try catch to protect against mapnik or v8 changing
        /* LCOV_EXCL_START */
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
        /* LCOV_EXCL_STOP */
    }
}

Napi::Value Feature::NewInstance(mapnik::feature_ptr f_ptr)
{
    Napi::EscapableHandleScope scope(env);
    Feature* f = new Feature(f_ptr);
    Napi::Value ext = Napi::External::New(env, f);
    Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
    if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Feature instance").ThrowAsJavaScriptException();

    return scope.Escape(maybe_local);
}

/**
 * @memberof Feature
 * @name id
 * @instance
 * @returns {number} id the feature's internal id
 */
Napi::Value Feature::id(Napi::CallbackInfo const& info)
{
    Feature* fp = info.Holder().Unwrap<Feature>();
    return Napi::Number::New(env, fp->get()->id());
}

/**
 * Get the feature's extent
 *
 * @name extent
 * @memberof Feature
 * @instance
 * @returns {Array<number>} extent [minx, miny, maxx, maxy] order feature extent.
 */
Napi::Value Feature::extent(Napi::CallbackInfo const& info)
{
    Feature* fp = info.Holder().Unwrap<Feature>();
    Napi::Array a = Napi::Array::New(env, 4);
    mapnik::box2d<double> const& e = fp->get()->envelope();
    (a).Set(0, Napi::Number::New(env, e.minx()));
    (a).Set(1, Napi::Number::New(env, e.miny()));
    (a).Set(2, Napi::Number::New(env, e.maxx()));
    (a).Set(3, Napi::Number::New(env, e.maxy()));

    return a;
}

/**
 * Get the feature's attributes as an object.
 *
 * @name attributes
 * @memberof Feature
 * @instance
 * @returns {Object} attributes
 */
Napi::Value Feature::attributes(Napi::CallbackInfo const& info)
{
    Feature* fp = info.Holder().Unwrap<Feature>();
    Napi::Object feat = Napi::Object::New(env);
    mapnik::feature_ptr feature = fp->get();
    if (feature)
    {
        for (auto const& attr : *feature)
        {
            (feat).Set(Napi::String::New(env, std::get<0>(attr)),
                      mapnik::util::apply_visitor(node_mapnik::value_converter(), std::get<1>(attr))
                );
        }
    }
    return feat;
}


/**
 * Get the feature's attributes as a Mapnik geometry.
 *
 * @name geometry
 * @memberof Feature
 * @instance
 * @returns {mapnik.Geometry} geometry
 */
Napi::Value Feature::geometry(Napi::CallbackInfo const& info)
{
    Feature* fp = info.Holder().Unwrap<Feature>();
    return Geometry::NewInstance(fp->get());
}

/**
 * Generate and return a GeoJSON representation of this feature
 *
 * @instance
 * @name toJSON
 * @memberof Feature
 * @returns {string} geojson Feature object in stringified GeoJSON
 */
Napi::Value Feature::toJSON(Napi::CallbackInfo const& info)
{
    Feature* fp = info.Holder().Unwrap<Feature>();
    std::string json;
    if (!mapnik::util::to_geojson(json, *(fp->get())))
    {
        /* LCOV_EXCL_START */
        Napi::Error::New(env, "Failed to generate GeoJSON").ThrowAsJavaScriptException();
        return env.Null();
        /* LCOV_EXCL_STOP */
    }
    return Napi::String::New(env, json);
}
