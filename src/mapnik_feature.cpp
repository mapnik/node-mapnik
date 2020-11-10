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

Napi::Object Feature::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Feature", {
            InstanceMethod<&Feature::id>("id", prop_attr),
            InstanceMethod<&Feature::extent>("extent", prop_attr),
            InstanceMethod<&Feature::attributes>("attributes", prop_attr),
            InstanceMethod<&Feature::geometry>("geometry", prop_attr),
            InstanceMethod<&Feature::toJSON>("toJSON", prop_attr),
            StaticMethod<&Feature::fromJSON>("fromJSON", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Feature", func);
    return exports;
}

/**
 * **`mapnik.Feature`**
 *
 * A single geographic feature, with geometry and properties. This is
 * typically derived from data by a datasource, but can be manually
 * created.
 *
 * @class Feature
 */

Feature::Feature(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Feature>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 1)
    {
        if (info[0].IsExternal())
        {
            auto ext = info[0].As<Napi::External<mapnik::feature_ptr>>();
            if (ext) feature_ = *ext.Data();
            return;
        }
        else if (info[0].IsNumber())
        {
            ctx_ = std::make_shared<mapnik::context_type>();
            std::int64_t id = info[0].As<Napi::Number>().Int64Value();
            feature_ = mapnik::feature_factory::create(ctx_, id);
            return;
        }
    }
    Napi::Error::New(env, "requires one argument: an integer feature id")
        .ThrowAsJavaScriptException();
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
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "requires one argument: a string representing a GeoJSON feature").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string json = info[0].As<Napi::String>();
    try
    {
        mapnik::feature_ptr feature(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(), 1));
        if (!mapnik::json::from_geojson(json, *feature))
        {
            Napi::Error::New(env, "Failed to read GeoJSON").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Value arg = Napi::External<mapnik::feature_ptr>::New(env, &feature);
        Napi::Object obj = Feature::constructor.New({arg});
        return scope.Escape(obj);
    }
    catch (std::exception const& ex)
    {
        // no way currently for any of the above code to throw,
        // but we'll keep this try catch to protect against mapnik or v8 changing
        /* LCOV_EXCL_START */
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
        /* LCOV_EXCL_STOP */
    }
}

/**
 * @memberof Feature
 * @name id
 * @instance
 * @returns {number} id the feature's internal id
 */
Napi::Value Feature::id(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, feature_->id());
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
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    mapnik::box2d<double> const& envelope = feature_->envelope();
    Napi::Array arr = Napi::Array::New(env, 4);
    arr.Set(0u, Napi::Number::New(env, envelope.minx()));
    arr.Set(1u, Napi::Number::New(env, envelope.miny()));
    arr.Set(2u, Napi::Number::New(env, envelope.maxx()));
    arr.Set(3u, Napi::Number::New(env, envelope.maxy()));
    return scope.Escape(arr);
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
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Object attributes = Napi::Object::New(env);
    if (feature_)
    {
        for (auto const& attr : *feature_)
        {
            attributes.Set(std::get<0>(attr),
                           mapnik::util::apply_visitor(node_mapnik::value_converter(env), std::get<1>(attr)));
        }
    }
    return scope.Escape(attributes);
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
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Value arg = Napi::External<mapnik::feature_ptr>::New(env, &feature_);
    Napi::Object obj = Geometry::constructor.New({arg});
    return scope.Escape(obj);
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
    Napi::Env env = info.Env();
    std::string json;
    if (!mapnik::util::to_geojson(json, *feature_))
    {
        /* LCOV_EXCL_START */
        Napi::Error::New(env, "Failed to generate GeoJSON").ThrowAsJavaScriptException();
        return env.Undefined();
        /* LCOV_EXCL_STOP */
    }
    return Napi::String::New(env, json);
}
