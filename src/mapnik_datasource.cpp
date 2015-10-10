
#include "mapnik_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "utils.hpp"
#include "ds_emitter.hpp"

// mapnik
#include <mapnik/attribute_descriptor.hpp>  // for attribute_descriptor
#include <mapnik/box2d.hpp>             // for box2d
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
 * A Datasource object. This is the connector from Mapnik to any kind
 * of file, network, or database source of geographical data.
 *
 * @name mapnik.Datasource
 * @class
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

    target->Set(Nan::New("Datasource").ToLocalChecked(), lcons->GetFunction());
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
            info.This()->Set(Nan::New("type").ToLocalChecked(),
                             Nan::New("raster").ToLocalChecked());
        }
        else
        {
            info.This()->Set(Nan::New("type").ToLocalChecked(),
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
    v8::Local<v8::Array> names = options->GetPropertyNames();
    unsigned int i = 0;
    unsigned int a_length = names->Length();
    while (i < a_length) {
        v8::Local<v8::Value> name = names->Get(i)->ToString();
        v8::Local<v8::Value> value = options->Get(name);
        // TODO - don't treat everything as strings
        params[TOSTR(name)] = TOSTR(value);
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
            info.This()->Set(Nan::New("type").ToLocalChecked(),
                             Nan::New("raster").ToLocalChecked());
        }
        else
        {
            info.This()->Set(Nan::New("type").ToLocalChecked(),
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
    /* LCOV_EXCL_END */
}

v8::Local<v8::Value> Datasource::NewInstance(mapnik::datasource_ptr ds_ptr) {
    Nan::EscapableHandleScope scope;
    Datasource* d = new Datasource();
    d->datasource_ = ds_ptr;
    v8::Local<v8::Value> ext = Nan::New<v8::External>(d);
    return scope.Escape(Nan::New(constructor)->GetFunction()->NewInstance(1, &ext));
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
 * @memberof mapnik.Datasource
 * @instance
 * @returns {v8::Array<number>} extent [minx, miny, maxx, maxy] order feature extent.
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
        /* LCOV_EXCL_END */
    }

    v8::Local<v8::Array> a = Nan::New<v8::Array>(4);
    a->Set(0, Nan::New<v8::Number>(e.minx()));
    a->Set(1, Nan::New<v8::Number>(e.miny()));
    a->Set(2, Nan::New<v8::Number>(e.maxx()));
    a->Set(3, Nan::New<v8::Number>(e.maxy()));
    info.GetReturnValue().Set(a);
}

/**
 * Describe the datasource's contents and type.
 *
 * @name describe
 * @memberof mapnik.Datasource
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
        /* LCOV_EXCL_END */
    }

    info.GetReturnValue().Set(description);
}

NAN_METHOD(Datasource::featureset)
{
    Datasource* ds = Nan::ObjectWrap::Unwrap<Datasource>(info.Holder());
    mapnik::featureset_ptr fs;
    try
    {
        mapnik::query q(ds->datasource_->envelope());
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
        /* LCOV_EXCL_END */
    }

    if (fs)
    {
        info.GetReturnValue().Set(Featureset::NewInstance(fs));
    }
    // This should never be able to be reached
    /* LCOV_EXCL_START */
    return;
    /* LCOV_EXCL_END */
}

NAN_METHOD(Datasource::fields)
{
    Datasource* d = Nan::ObjectWrap::Unwrap<Datasource>(info.Holder());
    v8::Local<v8::Object> fields = Nan::New<v8::Object>();
    node_mapnik::get_fields(fields,d->datasource_);
    info.GetReturnValue().Set(fields);
}
