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

Napi::FunctionReference MemoryDatasource::constructor;

void MemoryDatasource::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, MemoryDatasource::New);

    lcons->SetClassName(Napi::String::New(env, "MemoryDatasource"));

    // methods
    InstanceMethod("parameters", &parameters),
    InstanceMethod("describe", &describe),
    InstanceMethod("featureset", &featureset),
    InstanceMethod("add", &add),
    InstanceMethod("fields", &fields),

    (target).Set(Napi::String::New(env, "MemoryDatasource"), Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

MemoryDatasource::MemoryDatasource() : Napi::ObjectWrap<MemoryDatasource>(),
    datasource_(),
    feature_id_(1),
    tr_("utf8") {}

MemoryDatasource::~MemoryDatasource()
{
}

Napi::Value MemoryDatasource::New(Napi::CallbackInfo const& info)
{
    std::clog << "WARNING: MemoryDatasource is deprecated and will be removed in node-mapnik >= 3.7.x\n";
    if (!info.IsConstructCall())
    {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info[0].IsExternal())
    {
        Napi::External ext = info[0].As<Napi::External>();
        void* ptr = ext->Value();
        MemoryDatasource* d =  static_cast<MemoryDatasource*>(ptr);
        d->Wrap(info.This());
        return info.This();
        return;
    }
    if (info.Length() != 1){
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
        if (value->IsUint32() || value.IsNumber())
        {
            params[TOSTR(name)] = Napi::To<mapnik::value_integer>(value);
        }
        else if (value.IsNumber())
        {
            params[TOSTR(name)] = value.As<Napi::Number>().DoubleValue();
        }
        else if (value->IsBoolean())
        {
            params[TOSTR(name)] = value.As<Napi::Boolean>().Value();
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
    return info.This();
}

Napi::Value MemoryDatasource::NewInstance(mapnik::datasource_ptr ds_ptr) {
    Napi::EscapableHandleScope scope(env);
    MemoryDatasource* d = new MemoryDatasource();
    d->datasource_ = ds_ptr;
    Napi::Value ext = Napi::External::New(env, d);
    Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
    if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new MemoryDatasource instance").ThrowAsJavaScriptException();

    return scope.Escape(maybe_local);
}

Napi::Value MemoryDatasource::parameters(Napi::CallbackInfo const& info)
{
    MemoryDatasource* d = info.Holder().Unwrap<MemoryDatasource>();
    Napi::Object ds = Napi::Object::New(env);
    if (d->datasource_) {
        mapnik::parameters::const_iterator it = d->datasource_->params().begin();
        mapnik::parameters::const_iterator end = d->datasource_->params().end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object(ds, it->first, it->second);
        }
    }
    return ds;
}

Napi::Value MemoryDatasource::describe(Napi::CallbackInfo const& info)
{
    MemoryDatasource* d = info.Holder().Unwrap<MemoryDatasource>();
    Napi::Object description = Napi::Object::New(env);
    if (d->datasource_)
    {
        node_mapnik::describe_datasource(description,d->datasource_);
    }
    return description;
}

Napi::Value MemoryDatasource::featureset(Napi::CallbackInfo const& info)
{
    MemoryDatasource* d = info.Holder().Unwrap<MemoryDatasource>();

    if (d->datasource_) {
        mapnik::box2d<double> extent = d->datasource_->envelope();
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
            return Featureset::NewInstance(fs);
        }
    }

    // Even if there is an empty query, a featureset is still created
    // therefore it should be impossible to reach this point in the code.
    /* LCOV_EXCL_START */
    return;
    /* LCOV_EXCL_STOP */
}

Napi::Value MemoryDatasource::add(Napi::CallbackInfo const& info)
{
    if ((info.Length() != 1) || !info[0].IsObject())
    {
        Napi::Error::New(env, "accepts one argument: an object including x and y (or wkt) and properties").ThrowAsJavaScriptException();
        return env.Null();
    }

    MemoryDatasource* d = info.Holder().Unwrap<MemoryDatasource>();

    Napi::Object obj = info[0].As<Napi::Object>();

    if (((obj).Has(Napi::String::New(env, "wkt")).FromMaybe(false)) ||
        ((obj).Has(Napi::String::New(env, "x")).FromMaybe(false) && (obj).Has(Napi::String::New(env, "y")).FromMaybe(false)))
    {
        if ((obj).Has(Napi::String::New(env, "wkt")).FromMaybe(false))
        {
            Napi::Error::New(env, "wkt not yet supported").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Value x = (obj).Get(Napi::String::New(env, "x"));
        Napi::Value y = (obj).Get(Napi::String::New(env, "y"));
        if (x.IsNumber() && y.IsNumber())
        {
            mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
            mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,d->feature_id_));
            ++(d->feature_id_);
            feature->set_geometry(mapnik::geometry::point<double>(x.As<Napi::Number>().DoubleValue(),y.As<Napi::Number>().DoubleValue()));
            if ((obj).Has(Napi::String::New(env, "properties")).FromMaybe(false))
            {
                Napi::Value props = (obj).Get(Napi::String::New(env, "properties"));
                if (props.IsObject())
                {
                    Napi::Object p_obj = props->ToObject(Napi::GetCurrentContext());
                    Napi::Array names = Napi::GetPropertyNames(p_obj);
                    unsigned int i = 0;
                    unsigned int a_length = names->Length();
                    while (i < a_length)
                    {
                        Napi::Value name = (names).Get(i)->ToString(Napi::GetCurrentContext());
                        // if name in q.property_names() ?
                        Napi::Value value = (p_obj).Get(name);
                        if (value.IsString()) {
                            mapnik::value_unicode_string ustr = d->tr_.transcode(TOSTR(value));
                            feature->put_new(TOSTR(name),ustr);
                        } else if (value.IsNumber()) {
                            double num = value.As<Napi::Number>().DoubleValue();
                            // todo - round
                            if (num == value.As<Napi::Number>().Int32Value()) {
                                feature->put_new(TOSTR(name), Napi::To<node_mapnik::value_integer>(value));
                            } else {
                                double dub_val = value.As<Napi::Number>().DoubleValue();
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
            return env.True();
            return;
        }
    }
    return env.False();
}

Napi::Value MemoryDatasource::fields(Napi::CallbackInfo const& info)
{
    MemoryDatasource* d = info.Holder().Unwrap<MemoryDatasource>();
    Napi::Object fields = Napi::Object::New(env);
    if (d->datasource_) {
        node_mapnik::get_fields(fields,d->datasource_);
    }
    return fields;
}
