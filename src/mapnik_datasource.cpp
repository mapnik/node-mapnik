#include "mapnik_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "utils.hpp"
#include "ds_emitter.hpp"

// mapnik
#include <mapnik/attribute_descriptor.hpp> // for attribute_descriptor
#include <mapnik/geometry/box2d.hpp>       // for box2d
#include <mapnik/datasource.hpp>           // for datasource, datasource_ptr, etc
#include <mapnik/datasource_cache.hpp>     // for datasource_cache
#include <mapnik/feature_layer_desc.hpp>   // for layer_descriptor
#include <mapnik/params.hpp>               // for parameters
#include <mapnik/query.hpp>                // for query

// stl
#include <exception>
#include <vector>

Napi::FunctionReference Datasource::constructor;

Napi::Object Datasource::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Datasource", {
            InstanceMethod<&Datasource::parameters>("parameters", prop_attr),
            InstanceMethod<&Datasource::describe>("describe", prop_attr),
            InstanceMethod<&Datasource::featureset>("featureset", prop_attr),
            InstanceMethod<&Datasource::extent>("extent", prop_attr),
            InstanceMethod<&Datasource::fields>("fields", prop_attr)

        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Datasource", func);
    return exports;
}

/**
 * **`mapnik.Datasource`**
 *
 * A Datasource object. This is the connector from Mapnik to any kind
 * of file, network, or database source of geographical data.
 *
 * @class Datasource
 */

Datasource::Datasource(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Datasource>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 1)
    {
        Napi::TypeError::New(env, "accepts only one argument, an object of key:value datasource options").ThrowAsJavaScriptException();
        return;
    }

    if (info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<datasource_ptr>>();
        if (ext)
        {
            datasource_ = *ext.Data();
            if (datasource_->type() == mapnik::datasource::Raster)
                info.This().As<Napi::Object>().Set("type", "raster");
            else
                info.This().As<Napi::Object>().Set("type", "vector");
        }
        return;
    }

    if (!info[0].IsObject())
    {
        Napi::TypeError::New(env, "Must provide an object, eg {type: 'shape', file : 'world.shp'}").ThrowAsJavaScriptException();
        return;
    }

    Napi::Object options = info[0].As<Napi::Object>();

    mapnik::parameters params;
    Napi::Array names = options.GetPropertyNames();
    unsigned int length = names.Length();
    for (unsigned index = 0; index < length; ++index)
    {
        std::string name = names.Get(index).As<Napi::String>();
        Napi::Value value = options.Get(name);
        // TODO - don't treat everything as strings (FIXME ??)
        params[name] = value.ToString().Utf8Value();
    }

    try
    {
        datasource_ = mapnik::datasource_cache::instance().create(params);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return;
    }

    if (datasource_->type() == mapnik::datasource::Raster)
        info.This().As<Napi::Object>().Set("type", "raster");
    else
        info.This().As<Napi::Object>().Set("type", "vector");
}

Napi::Value Datasource::parameters(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Object params = Napi::Object::New(env);
    for (auto const& kv : datasource_->params())
    {
        node_mapnik::params_to_object(env, params, std::get<0>(kv), std::get<1>(kv));
    }
    return scope.Escape(params);
}

/**
 * Get the Datasource's extent
 *
 * @name extent
 * @memberof Datasource
 * @instance
 * @returns {Array<number>} extent [minx, miny, maxx, maxy] order feature extent.
 */
Napi::Value Datasource::extent(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    mapnik::box2d<double> bbox;
    try
    {
        bbox = datasource_->envelope();
    }
    catch (std::exception const& ex)
    {
        // The only time this could possibly throw is situations
        // where a plugin dynamically calculated extent such as
        // postgis plugin. Therefore this makes this difficult
        // to add to testing. Therefore marking it with exclusion
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
        // LCOV_EXCL_STOP
    }

    Napi::Array arr = Napi::Array::New(env, 4);
    arr.Set(0u, Napi::Number::New(env, bbox.minx()));
    arr.Set(1u, Napi::Number::New(env, bbox.miny()));
    arr.Set(2u, Napi::Number::New(env, bbox.maxx()));
    arr.Set(3u, Napi::Number::New(env, bbox.maxy()));
    return scope.Escape(arr);
}

/**
 * Describe the datasource's contents and type.
 *
 * @name describe
 * @memberof Datasource
 * @instance
 * @returns {Object} description: an object with type, fields, encoding,
 * geometry_type, and proj4 code
 */
Napi::Value Datasource::describe(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Object description = Napi::Object::New(env);
    try
    {
        node_mapnik::describe_datasource(env, description, datasource_);
    }
    catch (std::exception const& ex)
    {
        // The only time this could possibly throw is situations
        // where a plugin dynamically calculated extent such as
        // postgis plugin. Therefore this makes this difficult
        // to add to testing. Therefore marking it with exclusion
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
        // LCOV_EXCL_STOP
    }
    return scope.Escape(description);
}

/**
 * Get features from this dataset using an iterator.
 *
 * @name featureset
 * @memberof Datasource
 * @instance
 * @param {Object} [options]
 * @param {Array<number>} [options.extent=[minx,miny,maxx,maxy]]
 * @returns {Object} an iterator with a `.next()` method that returns
 * features from a dataset.
 * @example
 * var features = [];
 * var featureset = ds.featureset();
 * var feature;
 * while ((feature = featureset.next())) {
 *     features.push(feature);
 * }
 */

Napi::Value Datasource::featureset(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    mapnik::box2d<double> extent = datasource_->envelope();

    if (info.Length() > 0)
    {
        // options object
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[0].As<Napi::Object>();
        if (options.Has("extent"))
        {
            Napi::Value extent_opt = options.Get("extent");
            if (!extent_opt.IsArray())
            {
                Napi::TypeError::New(env, "extent value must be an array of [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Array bbox = extent_opt.As<Napi::Array>();
            auto len = bbox.Length();
            if (!(len == 4))
            {
                Napi::TypeError::New(env, "extent value must be an array of [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Value minx = bbox.Get(0u);
            Napi::Value miny = bbox.Get(1u);
            Napi::Value maxx = bbox.Get(2u);
            Napi::Value maxy = bbox.Get(3u);
            if (!minx.IsNumber() || !miny.IsNumber() || !maxx.IsNumber() || !maxy.IsNumber())
            {
                Napi::Error::New(env, "max_extent [minx,miny,maxx,maxy] must be numbers").ThrowAsJavaScriptException();
                return env.Null();
            }
            extent = mapnik::box2d<double>(minx.As<Napi::Number>().DoubleValue(), miny.As<Napi::Number>().DoubleValue(),
                                           maxx.As<Napi::Number>().DoubleValue(), maxy.As<Napi::Number>().DoubleValue());
        }
    }

    mapnik::featureset_ptr fs;
    try
    {
        mapnik::query q(extent);
        mapnik::layer_descriptor ld = datasource_->get_descriptor();
        auto const& desc = ld.get_descriptors();
        for (auto const& attr_info : desc)
        {
            q.add_property_name(attr_info.get_name());
        }

        fs = datasource_->features(q);
    }
    catch (std::exception const& ex)
    {
        // The only time this could possibly throw is situations
        // where a plugin dynamically calculated extent such as
        // postgis plugin. Therefore this makes this difficult
        // to add to testing. Therefore marking it with exclusion
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (fs && mapnik::is_valid(fs))
    {
        Napi::Value arg = Napi::External<mapnik::featureset_ptr>::New(env, &fs);
        Napi::Object obj = Featureset::constructor.New({arg});
        return scope.Escape(obj);
    }
    return env.Null(); // an empty Featureset
}

/**
 * Get only the fields metadata from a dataset.
 *
 * @name fields
 * @memberof Datasource
 * @instance
 * @returns {Object} an object that maps from a field name to it type
 * @example
 * var fields = ds.fields();
 * // {
 * //     FIPS: 'String',
 * //     ISO2: 'String',
 * //     ISO3: 'String',
 * //     UN: 'Number',
 * //     NAME: 'String',
 * //     AREA: 'Number',
 * //     POP2005: 'Number',
 * //     REGION: 'Number',
 * //     SUBREGION: 'Number',
 * //     LON: 'Number',
 * //     LAT: 'Number'
 * // }
 */

Napi::Value Datasource::fields(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Object fields = Napi::Object::New(env);
    node_mapnik::get_fields(env, fields, datasource_);
    return scope.Escape(fields);
}
