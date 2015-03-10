
// mapnik
#include <mapnik/version.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/value_types.hpp>

#include "mapnik_memory_datasource.hpp"
#include "mapnik_featureset.hpp"
#include "utils.hpp"
#include "ds_emitter.hpp"

// stl
#include <exception>

Persistent<FunctionTemplate> MemoryDatasource::constructor;

void MemoryDatasource::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(MemoryDatasource::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("MemoryDatasource"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(lcons, "parameters", parameters);
    NODE_SET_PROTOTYPE_METHOD(lcons, "describe", describe);
    NODE_SET_PROTOTYPE_METHOD(lcons, "featureset", featureset);
    NODE_SET_PROTOTYPE_METHOD(lcons, "add", add);

    target->Set(NanNew("MemoryDatasource"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

MemoryDatasource::MemoryDatasource() :
    node::ObjectWrap(),
    datasource_(),
    feature_id_(1),
    tr_("utf8") {}

MemoryDatasource::~MemoryDatasource()
{
}

NAN_METHOD(MemoryDatasource::New)
{
    NanScope();

    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args[0]->IsExternal())
    {
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        MemoryDatasource* d =  static_cast<MemoryDatasource*>(ptr);
        d->Wrap(args.This());
        NanReturnValue(args.This());
    }
    if (args.Length() != 1){
        NanThrowTypeError("accepts only one argument, an object of key:value datasource options");
        NanReturnUndefined();
    }

    if (!args[0]->IsObject())
    {
        NanThrowTypeError("Must provide an object, eg {type: 'shape', file : 'world.shp'}");
        NanReturnUndefined();
    }

    Local<Object> options = args[0].As<Object>();

    mapnik::parameters params;
    Local<Array> names = options->GetPropertyNames();
    unsigned int i = 0;
    unsigned int a_length = names->Length();
    while (i < a_length) {
        Local<Value> name = names->Get(i)->ToString();
        Local<Value> value = options->Get(name);
        if (value->IsUint32() || value->IsInt32())
        {
            params[TOSTR(name)] = value->IntegerValue();
        }
        else if (value->IsNumber())
        {   
            params[TOSTR(name)] = value->NumberValue();
        }
        else if (value->IsBoolean())
        {
            params[TOSTR(name)] = value->BooleanValue();
        }
        else
        {
            params[TOSTR(name)] = TOSTR(value);
        }
        i++;
    }
    params["type"] = "memory";
    //memory_datasource cache;
    MemoryDatasource* d = new MemoryDatasource();
    d->Wrap(args.This());
    d->datasource_ = std::make_shared<mapnik::memory_datasource>(params);
    NanReturnValue(args.This());
}

Handle<Value> MemoryDatasource::NewInstance(mapnik::datasource_ptr ds_ptr) {
    NanEscapableScope();
    MemoryDatasource* d = new MemoryDatasource();
    d->datasource_ = ds_ptr;
    Handle<Value> ext = NanNew<External>(d);
    Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
    return NanEscapeScope(obj);
}

NAN_METHOD(MemoryDatasource::parameters)
{
    NanScope();
    MemoryDatasource* d = node::ObjectWrap::Unwrap<MemoryDatasource>(args.Holder());
    Local<Object> ds = NanNew<Object>();
    if (d->datasource_) {
        mapnik::parameters::const_iterator it = d->datasource_->params().begin();
        mapnik::parameters::const_iterator end = d->datasource_->params().end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object(ds, it->first, it->second);
        }
    }
    NanReturnValue(ds);
}

NAN_METHOD(MemoryDatasource::describe)
{
    NanScope();
    MemoryDatasource* d = node::ObjectWrap::Unwrap<MemoryDatasource>(args.Holder());
    Local<Object> description = NanNew<Object>();
    if (d->datasource_) {
        try {
            node_mapnik::describe_datasource(description,d->datasource_);
        }
        catch (std::exception const& ex)
        {
            NanThrowError(ex.what());
            NanReturnUndefined();
        }
    }
    NanReturnValue(description);
}

NAN_METHOD(MemoryDatasource::featureset)
{

    NanScope();

    MemoryDatasource* d = node::ObjectWrap::Unwrap<MemoryDatasource>(args.Holder());

    try
    {
        if (d->datasource_) {
            mapnik::query q(d->datasource_->envelope());
            mapnik::layer_descriptor ld = d->datasource_->get_descriptor();
            std::vector<mapnik::attribute_descriptor> const& desc = ld.get_descriptors();
            std::vector<mapnik::attribute_descriptor>::const_iterator itr = desc.begin();
            std::vector<mapnik::attribute_descriptor>::const_iterator end = desc.end();
            while (itr != end)
            {
                q.add_property_name(itr->get_name());
                ++itr;
            }
            mapnik::featureset_ptr fs = d->datasource_->features(q);
            if (fs)
            {
                NanReturnValue(Featureset::NewInstance(fs));
            }
        }
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }

    NanReturnUndefined();
}

NAN_METHOD(MemoryDatasource::add)
{

    NanScope();

    if ((args.Length() != 1) || !args[0]->IsObject())
    {
        NanThrowError("accepts one argument: an object including x and y (or wkt) and properties");
        NanReturnUndefined();
    }

    MemoryDatasource* d = node::ObjectWrap::Unwrap<MemoryDatasource>(args.Holder());

    Local<Object> obj = args[0].As<Object>();

    if (obj->Has(NanNew("wkt")) || (obj->Has(NanNew("x")) && obj->Has(NanNew("y"))))
    {
        if (obj->Has(NanNew("wkt")))
        {
            NanThrowError("wkt not yet supported");
            NanReturnUndefined();
        }

        Local<Value> x = obj->Get(NanNew("x"));
        Local<Value> y = obj->Get(NanNew("y"));
        if (!x->IsUndefined() && x->IsNumber() && !y->IsUndefined() && y->IsNumber())
        {
            mapnik::geometry_type * pt = new mapnik::geometry_type(mapnik::geometry_type::types::Point);
            pt->move_to(x->NumberValue(),y->NumberValue());
            mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
            mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,d->feature_id_));
            ++(d->feature_id_);
            feature->add_geometry(pt);
            if (obj->Has(NanNew("properties")))
            {
                Local<Value> props = obj->Get(NanNew("properties"));
                if (props->IsObject())
                {
                    Local<Object> p_obj = props->ToObject();
                    Local<Array> names = p_obj->GetPropertyNames();
                    unsigned int i = 0;
                    unsigned int a_length = names->Length();
                    while (i < a_length)
                    {
                        Local<Value> name = names->Get(i)->ToString();
                        // if name in q.property_names() ?
                        Local<Value> value = p_obj->Get(name);
                        if (value->IsString()) {
                            mapnik::value_unicode_string ustr = d->tr_.transcode(TOSTR(value));
                            feature->put_new(TOSTR(name),ustr);
                        } else if (value->IsNumber()) {
                            double num = value->NumberValue();
                            // todo - round
                            if (num == value->IntegerValue()) {
                                feature->put_new(TOSTR(name),static_cast<node_mapnik::value_integer>(value->IntegerValue()));
                            } else {
                                double dub_val = value->NumberValue();
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
            NanReturnValue(NanTrue());
        }
    }
    NanReturnValue(NanFalse());
}
