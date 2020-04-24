#include "mapnik_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "utils.hpp"
#include "ds_emitter.hpp"

// mapnik
#include <mapnik/attribute_descriptor.hpp>  // for attribute_descriptor
#include <mapnik/geometry/box2d.hpp>             // for box2d
#include <mapnik/datasource.hpp>        // for datasource, datasource_ptr, etc
#include <mapnik/datasource_cache.hpp>  // for datasource_cache
#include <mapnik/feature_layer_desc.hpp>  // for layer_descriptor
#include <mapnik/params.hpp>            // for parameters
#include <mapnik/query.hpp>             // for query

// stl
#include <exception>
#include <vector>

Napi::FunctionReference Datasource::constructor;

/**
 * **`mapnik.Datasource`**
 *
 * A Datasource object. This is the connector from Mapnik to any kind
 * of file, network, or database source of geographical data.
 *
 * @class Datasource
 */
void Datasource::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, Datasource::New);

    lcons->SetClassName(Napi::String::New(env, "Datasource"));

    // methods
    InstanceMethod("parameters", &parameters),
    InstanceMethod("describe", &describe),
    InstanceMethod("featureset", &featureset),
    InstanceMethod("extent", &extent),
    InstanceMethod("fields", &fields),

    (target).Set(Napi::String::New(env, "Datasource"), Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

Datasource::Datasource() : Napi::ObjectWrap<Datasource>(),
    datasource_() {}

Datasource::~Datasource()
{
}

Napi::Value Datasource::New(const Napi::CallbackInfo& info)
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
        Datasource* d =  static_cast<Datasource*>(ptr);
        if (d->datasource_->type() == mapnik::datasource::Raster)
        {
            (info.This()).Set(Napi::String::New(env, "type"),
                             Napi::String::New(env, "raster"));
        }
        else
        {
            (info.This()).Set(Napi::String::New(env, "type"),
                             Napi::String::New(env, "vector"));
        }
        d->Wrap(info.This());
        return info.This();
        return;
    }
    if (info.Length() != 1)
    {
        Napi::TypeError::New(env, "accepts only one argument, an object of key:value datasource options").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsObject())
    {
        Napi::TypeError::New(env, "Must provide an object, eg {type: 'shape', file : 'world.shp'}").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object options = info[0].As<Napi::Object>();

    mapnik::parameters params;
    Napi::Array names = Napi::GetPropertyNames(options);
    unsigned int i = 0;
    unsigned int a_length = names->Length();
    while (i < a_length) {
        Napi::Value name = (names).Get(i)->ToString(Napi::GetCurrentContext());
        Napi::Value value = (options).Get(name);
        // TODO - don't treat everything as strings
        params[TOSTR(name)] = const_cast<char const*>(TOSTR(value));
        i++;
    }

    mapnik::datasource_ptr ds;
    try
    {
        ds = mapnik::datasource_cache::instance().create(params);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    if (ds)
    {
        if (ds->type() == mapnik::datasource::Raster)
        {
            (info.This()).Set(Napi::String::New(env, "type"),
                             Napi::String::New(env, "raster"));
        }
        else
        {
            (info.This()).Set(Napi::String::New(env, "type"),
                             Napi::String::New(env, "vector"));
        }
        Datasource* d = new Datasource();
        d->Wrap(info.This());
        d->datasource_ = ds;
        return info.This();
        return;
    }
    // Not sure this point could ever be reached, because if a ds is created,
    // even if it is an empty or bad dataset the pointer will still exist
    /* LCOV_EXCL_START */
    return;
    /* LCOV_EXCL_STOP */
}

Napi::Value Datasource::NewInstance(mapnik::datasource_ptr ds_ptr) {
    Napi::EscapableHandleScope scope(env);
    Datasource* d = new Datasource();
    d->datasource_ = ds_ptr;
    Napi::Value ext = Napi::External::New(env, d);
    Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
    if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Datasource instance").ThrowAsJavaScriptException();

    return scope.Escape(maybe_local);
}

Napi::Value Datasource::parameters(const Napi::CallbackInfo& info)
{
    Datasource* d = this;
    Napi::Object ds = Napi::Object::New(env);
    mapnik::parameters::const_iterator it = d->datasource_->params().begin();
    mapnik::parameters::const_iterator end = d->datasource_->params().end();
    for (; it != end; ++it)
    {
        node_mapnik::params_to_object(ds, it->first, it->second);
    }
    return ds;
}

/**
 * Get the Datasource's extent
 *
 * @name extent
 * @memberof Datasource
 * @instance
 * @returns {Array<number>} extent [minx, miny, maxx, maxy] order feature extent.
 */
Napi::Value Datasource::extent(const Napi::CallbackInfo& info)
{
    Datasource* d = info.Holder().Unwrap<Datasource>();
    mapnik::box2d<double> e;
    try
    {
        e = d->datasource_->envelope();
    }
    catch (std::exception const& ex)
    {
        // The only time this could possibly throw is situations
        // where a plugin dynamically calculated extent such as
        // postgis plugin. Therefore this makes this difficult
        // to add to testing. Therefore marking it with exclusion
        /* LCOV_EXCL_START */
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
        /* LCOV_EXCL_STOP */
    }

    Napi::Array a = Napi::Array::New(env, 4);
    (a).Set(0, Napi::Number::New(env, e.minx()));
    (a).Set(1, Napi::Number::New(env, e.miny()));
    (a).Set(2, Napi::Number::New(env, e.maxx()));
    (a).Set(3, Napi::Number::New(env, e.maxy()));
    return a;
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
Napi::Value Datasource::describe(const Napi::CallbackInfo& info)
{
    Datasource* d = info.Holder().Unwrap<Datasource>();
    Napi::Object description = Napi::Object::New(env);
    try
    {
        node_mapnik::describe_datasource(description,d->datasource_);
    }
    catch (std::exception const& ex)
    {
        // The only time this could possibly throw is situations
        // where a plugin dynamically calculated extent such as
        // postgis plugin. Therefore this makes this difficult
        // to add to testing. Therefore marking it with exclusion
        /* LCOV_EXCL_START */
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
        /* LCOV_EXCL_STOP */
    }

    return description;
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
Napi::Value Datasource::featureset(const Napi::CallbackInfo& info)
{
    Datasource* ds = info.Holder().Unwrap<Datasource>();
    mapnik::box2d<double> extent = ds->datasource_->envelope();
    if (info.Length() > 0)
    {
        // options object
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }
        Napi::Object options = info[0].ToObject(Napi::GetCurrentContext());
        if ((options).Has(Napi::String::New(env, "extent")).FromMaybe(false))
        {
            Napi::Value extent_opt = (options).Get(Napi::String::New(env, "extent"));
            if (!extent_opt->IsArray())
            {
                Napi::TypeError::New(env, "extent value must be an array of [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Array bbox = extent_opt.As<Napi::Array>();
            auto len = bbox->Length();
            if (!(len == 4))
            {
                Napi::TypeError::New(env, "extent value must be an array of [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Value minx = (bbox).Get(0);
            Napi::Value miny = (bbox).Get(1);
            Napi::Value maxx = (bbox).Get(2);
            Napi::Value maxy = (bbox).Get(3);
            if (!minx.IsNumber() || !miny.IsNumber() || !maxx.IsNumber() || !maxy.IsNumber())
            {
                Napi::Error::New(env, "max_extent [minx,miny,maxx,maxy] must be numbers").ThrowAsJavaScriptException();
                return env.Null();
            }
            extent = mapnik::box2d<double>(minx.As<Napi::Number>().DoubleValue(),miny.As<Napi::Number>().DoubleValue(),
                                           maxx.As<Napi::Number>().DoubleValue(),maxy.As<Napi::Number>().DoubleValue());
        }
    }

    mapnik::featureset_ptr fs;
    try
    {
        mapnik::query q(extent);
        mapnik::layer_descriptor ld = ds->datasource_->get_descriptor();
        auto const& desc = ld.get_descriptors();
        for (auto const& attr_info : desc)
        {
            q.add_property_name(attr_info.get_name());
        }

        fs = ds->datasource_->features(q);
    }
    catch (std::exception const& ex)
    {
        // The only time this could possibly throw is situations
        // where a plugin dynamically calculated extent such as
        // postgis plugin. Therefore this makes this difficult
        // to add to testing. Therefore marking it with exclusion
        /* LCOV_EXCL_START */
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
        /* LCOV_EXCL_STOP */
    }

    if (fs && mapnik::is_valid(fs))
    {
        return Featureset::NewInstance(fs);
    }
    // This should never be able to be reached
    /* LCOV_EXCL_START */
    return;
    /* LCOV_EXCL_STOP */
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
Napi::Value Datasource::fields(const Napi::CallbackInfo& info)
{
    Datasource* d = info.Holder().Unwrap<Datasource>();
    Napi::Object fields = Napi::Object::New(env);
    node_mapnik::get_fields(fields,d->datasource_);
    return fields;
}
