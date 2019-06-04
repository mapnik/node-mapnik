// mapnik
#include <mapnik/version.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/value/types.hpp>

#include "mapnik_memory_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "utils.hpp"
#include "ds_emitter.hpp"

// stl
#include <exception>

Nan::Persistent<v8::FunctionTemplate> MemoryDatasource::constructor;

void MemoryDatasource::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(MemoryDatasource::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("MemoryDatasource").ToLocalChecked());

    // methods
    Nan::SetPrototypeMethod(lcons, "parameters", parameters);
    Nan::SetPrototypeMethod(lcons, "describe", describe);
    Nan::SetPrototypeMethod(lcons, "featureset", featureset);
    Nan::SetPrototypeMethod(lcons, "add", add);
    Nan::SetPrototypeMethod(lcons, "fields", fields);

    Nan::Set(target, Nan::New("MemoryDatasource").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

MemoryDatasource::MemoryDatasource() :
    Nan::ObjectWrap(),
    datasource_(),
    feature_id_(1),
    tr_("utf8") {}

MemoryDatasource::~MemoryDatasource()
{
}

NAN_METHOD(MemoryDatasource::New)
{
    std::clog << "WARNING: MemoryDatasource is deprecated and will be removed in node-mapnik >= 3.7.x\n";
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = v8::Local<v8::External>::Cast(info[0]);
        void* ptr = ext->Value();
        MemoryDatasource* d =  static_cast<MemoryDatasource*>(ptr);
        d->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    if (info.Length() != 1){
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
        if (value->IsUint32() || value->IsInt32())
        {
            params[TOSTR(name)] = Nan::To<mapnik::value_integer>(value).FromJust();
        }
        else if (value->IsNumber())
        {
            params[TOSTR(name)] = Nan::To<double>(value).FromJust();
        }
        else if (value->IsBoolean())
        {
            params[TOSTR(name)] = Nan::To<bool>(value).FromJust();
        }
        else
        {
            params[TOSTR(name)] = const_cast<char const*>(TOSTR(value));
        }
        i++;
    }
    params["type"] = "memory";
    //memory_datasource cache;
    MemoryDatasource* d = new MemoryDatasource();
    d->Wrap(info.This());
    d->datasource_ = std::make_shared<mapnik::memory_datasource>(params);
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Value> MemoryDatasource::NewInstance(mapnik::datasource_ptr ds_ptr) {
    Nan::EscapableHandleScope scope;
    MemoryDatasource* d = new MemoryDatasource();
    d->datasource_ = ds_ptr;
    v8::Local<v8::Value> ext = Nan::New<v8::External>(d);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new MemoryDatasource instance");
    return scope.Escape(maybe_local.ToLocalChecked());
}

NAN_METHOD(MemoryDatasource::parameters)
{
    MemoryDatasource* d = Nan::ObjectWrap::Unwrap<MemoryDatasource>(info.Holder());
    v8::Local<v8::Object> ds = Nan::New<v8::Object>();
    if (d->datasource_) {
        mapnik::parameters::const_iterator it = d->datasource_->params().begin();
        mapnik::parameters::const_iterator end = d->datasource_->params().end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object(ds, it->first, it->second);
        }
    }
    info.GetReturnValue().Set(ds);
}

NAN_METHOD(MemoryDatasource::describe)
{
    MemoryDatasource* d = Nan::ObjectWrap::Unwrap<MemoryDatasource>(info.Holder());
    v8::Local<v8::Object> description = Nan::New<v8::Object>();
    if (d->datasource_)
    {
        node_mapnik::describe_datasource(description,d->datasource_);
    }
    info.GetReturnValue().Set(description);
}

NAN_METHOD(MemoryDatasource::featureset)
{
    MemoryDatasource* d = Nan::ObjectWrap::Unwrap<MemoryDatasource>(info.Holder());

    if (d->datasource_) {
        mapnik::box2d<double> extent = d->datasource_->envelope();
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

        mapnik::query q(extent);
        mapnik::layer_descriptor ld = d->datasource_->get_descriptor();
        auto const& desc = ld.get_descriptors();
        for (auto const& attr_info : desc)
        {
            // There is currently no way in the memory_datasource within mapnik to even
            // add a descriptor. Therefore it is impossible that this will ever be reached
            // currently.
            /* LCOV_EXCL_START */
            q.add_property_name(attr_info.get_name());
            /* LCOV_EXCL_STOP */
        }
        mapnik::featureset_ptr fs = d->datasource_->features(q);
        if (fs && mapnik::is_valid(fs))
        {
            info.GetReturnValue().Set(Featureset::NewInstance(fs));
        }
    }

    // Even if there is an empty query, a featureset is still created
    // therefore it should be impossible to reach this point in the code.
    /* LCOV_EXCL_START */
    return;
    /* LCOV_EXCL_STOP */
}

NAN_METHOD(MemoryDatasource::add)
{
    if ((info.Length() != 1) || !info[0]->IsObject())
    {
        Nan::ThrowError("accepts one argument: an object including x and y (or wkt) and properties");
        return;
    }

    MemoryDatasource* d = Nan::ObjectWrap::Unwrap<MemoryDatasource>(info.Holder());

    v8::Local<v8::Object> obj = info[0].As<v8::Object>();

    if ((Nan::Has(obj, Nan::New("wkt").ToLocalChecked()).FromMaybe(false)) ||
        (Nan::Has(obj, Nan::New("x").ToLocalChecked()).FromMaybe(false) && Nan::Has(obj, Nan::New("y").ToLocalChecked()).FromMaybe(false)))
    {
        if (Nan::Has(obj, Nan::New("wkt").ToLocalChecked()).FromMaybe(false))
        {
            Nan::ThrowError("wkt not yet supported");
            return;
        }

        v8::Local<v8::Value> x = Nan::Get(obj, Nan::New("x").ToLocalChecked()).ToLocalChecked();
        v8::Local<v8::Value> y = Nan::Get(obj, Nan::New("y").ToLocalChecked()).ToLocalChecked();
        if (!x->IsUndefined() && x->IsNumber() && !y->IsUndefined() && y->IsNumber())
        {
            mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
            mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,d->feature_id_));
            ++(d->feature_id_);
            feature->set_geometry(mapnik::geometry::point<double>(Nan::To<double>(x).FromJust(),Nan::To<double>(y).FromJust()));
            if (Nan::Has(obj, Nan::New("properties").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> props = Nan::Get(obj, Nan::New("properties").ToLocalChecked()).ToLocalChecked();
                if (props->IsObject())
                {
                    v8::Local<v8::Object> p_obj = props->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
                    v8::Local<v8::Array> names = Nan::GetPropertyNames(p_obj).ToLocalChecked();
                    unsigned int i = 0;
                    unsigned int a_length = names->Length();
                    while (i < a_length)
                    {
                        v8::Local<v8::Value> name = Nan::Get(names, i).ToLocalChecked()->ToString(Nan::GetCurrentContext()).ToLocalChecked();
                        // if name in q.property_names() ?
                        v8::Local<v8::Value> value = Nan::Get(p_obj, name).ToLocalChecked();
                        if (value->IsString()) {
                            mapnik::value_unicode_string ustr = d->tr_.transcode(TOSTR(value));
                            feature->put_new(TOSTR(name),ustr);
                        } else if (value->IsNumber()) {
                            double num = Nan::To<double>(value).FromJust();
                            // todo - round
                            if (num == Nan::To<int>(value).FromJust()) {
                                feature->put_new(TOSTR(name), Nan::To<node_mapnik::value_integer>(value).FromJust());
                            } else {
                                double dub_val = Nan::To<double>(value).FromJust();
                                feature->put_new(TOSTR(name),dub_val);
                            }
                        } else if (value->IsNull()) {
                            feature->put_new(TOSTR(name),mapnik::value_null());
                        }
                        i++;
                    }
                }
            }
            mapnik::memory_datasource *cache = dynamic_cast<mapnik::memory_datasource *>(d->datasource_.get());
            cache->push(feature);
            info.GetReturnValue().Set(Nan::True());
            return;
        }
    }
    info.GetReturnValue().Set(Nan::False());
}

NAN_METHOD(MemoryDatasource::fields)
{
    MemoryDatasource* d = Nan::ObjectWrap::Unwrap<MemoryDatasource>(info.Holder());
    v8::Local<v8::Object> fields = Nan::New<v8::Object>();
    if (d->datasource_) {
        node_mapnik::get_fields(fields,d->datasource_);
    }
    info.GetReturnValue().Set(fields);
}
