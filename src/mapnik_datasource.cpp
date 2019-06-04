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

Nan::Persistent<v8::FunctionTemplate> Datasource::constructor;

/**
 * **`mapnik.Datasource`**
 *
 * A Datasource object. This is the connector from Mapnik to any kind
 * of file, network, or database source of geographical data.
 *
 * @class Datasource
 */
void Datasource::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Datasource::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Datasource").ToLocalChecked());

    // methods
    Nan::SetPrototypeMethod(lcons, "parameters", parameters);
    Nan::SetPrototypeMethod(lcons, "describe", describe);
    Nan::SetPrototypeMethod(lcons, "featureset", featureset);
    Nan::SetPrototypeMethod(lcons, "extent", extent);
    Nan::SetPrototypeMethod(lcons, "fields", fields);

    Nan::Set(target, Nan::New("Datasource").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

Datasource::Datasource() :
    Nan::ObjectWrap(),
    datasource_() {}

Datasource::~Datasource()
{
}

NAN_METHOD(Datasource::New)
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
        Datasource* d =  static_cast<Datasource*>(ptr);
        if (d->datasource_->type() == mapnik::datasource::Raster)
        {
            Nan::Set(info.This(), Nan::New("type").ToLocalChecked(),
                             Nan::New("raster").ToLocalChecked());
        }
        else
        {
            Nan::Set(info.This(), Nan::New("type").ToLocalChecked(),
                             Nan::New("vector").ToLocalChecked());
        }
        d->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    if (info.Length() != 1)
    {
        Nan::ThrowTypeError("accepts only one argument, an object of key:value datasource options");
        return;
    }

    if (!info[0]->IsObject())
    {
        Nan::ThrowTypeError("Must provide an object, eg {type: 'shape', file : 'world.shp'}");
        return;
    }

    v8::Local<v8::Object> options = info[0].As<v8::Object>();

    mapnik::parameters params;
    v8::Local<v8::Array> names = Nan::GetPropertyNames(options).ToLocalChecked();
    unsigned int i = 0;
    unsigned int a_length = names->Length();
    while (i < a_length) {
        v8::Local<v8::Value> name = Nan::Get(names, i).ToLocalChecked()->ToString(Nan::GetCurrentContext()).ToLocalChecked();
        v8::Local<v8::Value> value = Nan::Get(options, name).ToLocalChecked();
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
        Nan::ThrowError(ex.what());
        return;
    }

    if (ds)
    {
        if (ds->type() == mapnik::datasource::Raster)
        {
            Nan::Set(info.This(), Nan::New("type").ToLocalChecked(),
                             Nan::New("raster").ToLocalChecked());
        }
        else
        {
            Nan::Set(info.This(), Nan::New("type").ToLocalChecked(),
                             Nan::New("vector").ToLocalChecked());
        }
        Datasource* d = new Datasource();
        d->Wrap(info.This());
        d->datasource_ = ds;
        info.GetReturnValue().Set(info.This());
        return;
    }
    // Not sure this point could ever be reached, because if a ds is created,
    // even if it is an empty or bad dataset the pointer will still exist
    /* LCOV_EXCL_START */
    return;
    /* LCOV_EXCL_STOP */
}

v8::Local<v8::Value> Datasource::NewInstance(mapnik::datasource_ptr ds_ptr) {
    Nan::EscapableHandleScope scope;
    Datasource* d = new Datasource();
    d->datasource_ = ds_ptr;
    v8::Local<v8::Value> ext = Nan::New<v8::External>(d);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Datasource instance");
    return scope.Escape(maybe_local.ToLocalChecked());
}

NAN_METHOD(Datasource::parameters)
{
    Datasource* d = Nan::ObjectWrap::Unwrap<Datasource>(info.This());
    v8::Local<v8::Object> ds = Nan::New<v8::Object>();
    mapnik::parameters::const_iterator it = d->datasource_->params().begin();
    mapnik::parameters::const_iterator end = d->datasource_->params().end();
    for (; it != end; ++it)
    {
        node_mapnik::params_to_object(ds, it->first, it->second);
    }
    info.GetReturnValue().Set(ds);
}

/**
 * Get the Datasource's extent
 *
 * @name extent
 * @memberof Datasource
 * @instance
 * @returns {Array<number>} extent [minx, miny, maxx, maxy] order feature extent.
 */
NAN_METHOD(Datasource::extent)
{
    Datasource* d = Nan::ObjectWrap::Unwrap<Datasource>(info.Holder());
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
        Nan::ThrowError(ex.what());
        return;
        /* LCOV_EXCL_STOP */
    }

    v8::Local<v8::Array> a = Nan::New<v8::Array>(4);
    Nan::Set(a, 0, Nan::New<v8::Number>(e.minx()));
    Nan::Set(a, 1, Nan::New<v8::Number>(e.miny()));
    Nan::Set(a, 2, Nan::New<v8::Number>(e.maxx()));
    Nan::Set(a, 3, Nan::New<v8::Number>(e.maxy()));
    info.GetReturnValue().Set(a);
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
NAN_METHOD(Datasource::describe)
{
    Datasource* d = Nan::ObjectWrap::Unwrap<Datasource>(info.Holder());
    v8::Local<v8::Object> description = Nan::New<v8::Object>();
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
        Nan::ThrowError(ex.what());
        return;
        /* LCOV_EXCL_STOP */
    }

    info.GetReturnValue().Set(description);
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
NAN_METHOD(Datasource::featureset)
{
    Datasource* ds = Nan::ObjectWrap::Unwrap<Datasource>(info.Holder());
    mapnik::box2d<double> extent = ds->datasource_->envelope();
    if (info.Length() > 0)
    {
        // options object
        if (!info[0]->IsObject())
        {
            Nan::ThrowTypeError("optional second argument must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("extent").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> extent_opt = Nan::Get(options, Nan::New("extent").ToLocalChecked()).ToLocalChecked();
            if (!extent_opt->IsArray())
            {
                Nan::ThrowTypeError("extent value must be an array of [minx,miny,maxx,maxy]");
                return;
            }
            v8::Local<v8::Array> bbox = extent_opt.As<v8::Array>();
            auto len = bbox->Length();
            if (!(len == 4))
            {
                Nan::ThrowTypeError("extent value must be an array of [minx,miny,maxx,maxy]");
                return;
            }
            v8::Local<v8::Value> minx = Nan::Get(bbox, 0).ToLocalChecked();
            v8::Local<v8::Value> miny = Nan::Get(bbox, 1).ToLocalChecked();
            v8::Local<v8::Value> maxx = Nan::Get(bbox, 2).ToLocalChecked();
            v8::Local<v8::Value> maxy = Nan::Get(bbox, 3).ToLocalChecked();
            if (!minx->IsNumber() || !miny->IsNumber() || !maxx->IsNumber() || !maxy->IsNumber())
            {
                Nan::ThrowError("max_extent [minx,miny,maxx,maxy] must be numbers");
                return;
            }
            extent = mapnik::box2d<double>(Nan::To<double>(minx).FromJust(),Nan::To<double>(miny).FromJust(),
                                           Nan::To<double>(maxx).FromJust(),Nan::To<double>(maxy).FromJust());
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
        Nan::ThrowError(ex.what());
        return;
        /* LCOV_EXCL_STOP */
    }

    if (fs && mapnik::is_valid(fs))
    {
        info.GetReturnValue().Set(Featureset::NewInstance(fs));
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
NAN_METHOD(Datasource::fields)
{
    Datasource* d = Nan::ObjectWrap::Unwrap<Datasource>(info.Holder());
    v8::Local<v8::Object> fields = Nan::New<v8::Object>();
    node_mapnik::get_fields(fields,d->datasource_);
    info.GetReturnValue().Set(fields);
}
