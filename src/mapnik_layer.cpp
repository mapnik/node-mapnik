#include "mapnik_layer.hpp"

#include "utils.hpp"                    // for TOSTR, ATTR, etc

#include "mapnik_datasource.hpp"
#include "mapnik_memory_datasource.hpp"

// mapnik
#include <mapnik/datasource.hpp>        // for datasource_ptr, datasource
#include <mapnik/memory_datasource.hpp> // for memory_datasource
#include <mapnik/layer.hpp>             // for layer
#include <mapnik/params.hpp>            // for parameters

// stl
#include <limits>

Napi::FunctionReference Layer::constructor;

void Layer::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, Layer::New);

    lcons->SetClassName(Napi::String::New(env, "Layer"));

    // methods
    InstanceMethod("describe", &describe),

    // properties
    ATTR(lcons, "name", get_prop, set_prop);
    ATTR(lcons, "active", get_prop, set_prop);
    ATTR(lcons, "srs", get_prop, set_prop);
    ATTR(lcons, "styles", get_prop, set_prop);
    ATTR(lcons, "datasource", get_prop, set_prop);
    ATTR(lcons, "minimum_scale_denominator", get_prop, set_prop);
    ATTR(lcons, "maximum_scale_denominator", get_prop, set_prop);
    ATTR(lcons, "queryable", get_prop, set_prop);
    ATTR(lcons, "clear_label_cache", get_prop, set_prop);

    (target).Set(Napi::String::New(env, "Layer"),Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

Layer::Layer(std::string const& name) : Napi::ObjectWrap<Layer>(),
    layer_(std::make_shared<mapnik::layer>(name)) {}

Layer::Layer(std::string const& name, std::string const& srs) : Napi::ObjectWrap<Layer>(),
    layer_(std::make_shared<mapnik::layer>(name,srs)) {}

Layer::Layer() : Napi::ObjectWrap<Layer>(),
    layer_() {}


Layer::~Layer() {}

Napi::Value Layer::New(const Napi::CallbackInfo& info)
{
    if (!info.IsConstructCall()) {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info[0].IsExternal())
    {
        Napi::External ext = info[0].As<Napi::External>();
        void* ptr = ext->Value();
        Layer* l =  static_cast<Layer*>(ptr);
        l->Wrap(info.This());
        return info.This();
        return;
    }

    if (info.Length() == 1)
    {
        if (!info[0].IsString())
        {
            Napi::TypeError::New(env, "'name' must be a string").ThrowAsJavaScriptException();
            return env.Null();
        }
        Layer* l = new Layer(TOSTR(info[0]));
        l->Wrap(info.This());
        return info.This();
        return;
    }
    else if (info.Length() == 2)
    {
        if (!info[0].IsString() || !info[1].IsString()) {
            Napi::TypeError::New(env, "'name' and 'srs' must be a strings").ThrowAsJavaScriptException();
            return env.Null();
        }
        Layer* l = new Layer(TOSTR(info[0]),TOSTR(info[1]));
        l->Wrap(info.This());
        return info.This();
        return;
    }
    else
    {
        Napi::TypeError::New(env, "please provide Layer name and optional srs").ThrowAsJavaScriptException();
        return env.Null();
    }
    return info.This();
}

Napi::Value Layer::NewInstance(mapnik::layer const& lay_ref) {
    Napi::EscapableHandleScope scope(env);
    Layer* l = new Layer();
    // copy new mapnik::layer into the shared_ptr
    l->layer_ = std::make_shared<mapnik::layer>(lay_ref);
    Napi::Value ext = Napi::External::New(env, l);
    Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
    if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Layer instance").ThrowAsJavaScriptException();

    return scope.Escape(maybe_local);
}

Napi::Value Layer::get_prop(const Napi::CallbackInfo& info)
{
    Layer* l = info.Holder().Unwrap<Layer>();
    std::string a = TOSTR(property);
    if (a == "name")
        return Napi::String::New(env, l->layer_->name());
    else if (a == "srs")
        return Napi::String::New(env, l->layer_->srs());
    else if (a == "styles") {
        std::vector<std::string> const& style_names = l->layer_->styles();
        Napi::Array s = Napi::Array::New(env, style_names.size());
        for (unsigned i = 0; i < style_names.size(); ++i)
        {
            (s).Set(i, Napi::String::New(env, style_names[i]) );
        }
        return s;
    }
    else if (a == "datasource") {
        mapnik::datasource_ptr ds = l->layer_->datasource();
        if (ds)
        {
            mapnik::memory_datasource * mem_ptr = dynamic_cast<mapnik::memory_datasource*>(ds.get());
            if (mem_ptr)
            {
                return MemoryDatasource::NewInstance(ds);
            }
            else
            {
                return Datasource::NewInstance(ds);
            }
        }
        return;
    }
    else if (a == "minimum_scale_denominator")
    {
        return Napi::Number::New(env, l->layer_->minimum_scale_denominator());
    }
    else if (a == "maximum_scale_denominator")
    {
        return Napi::Number::New(env, l->layer_->maximum_scale_denominator());
    }
    else if (a == "queryable")
    {
        return Napi::Boolean::New(env, l->layer_->queryable());
    }
    else if (a == "clear_label_cache")
    {
        return Napi::Boolean::New(env, l->layer_->clear_label_cache());
    }
    else // if (a == "active")
    {
        return Napi::Boolean::New(env, l->layer_->active());
    }
}

void Layer::set_prop(const Napi::CallbackInfo& info, const Napi::Value& value)
{
    Layer* l = info.Holder().Unwrap<Layer>();
    std::string a = TOSTR(property);
    if (a == "name")
    {
        if (!value.IsString()) {
            Napi::TypeError::New(env, "'name' must be a string").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            l->layer_->set_name(TOSTR(value));
        }
    }
    else if (a == "srs")
    {
        if (!value.IsString()) {
            Napi::TypeError::New(env, "'srs' must be a string").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            l->layer_->set_srs(TOSTR(value));
        }
    }
    else if (a == "styles")
    {
        if (!value->IsArray()) {
            Napi::TypeError::New(env, "Must provide an array of style names").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            Napi::Array arr = value.As<Napi::Array>();
            // todo - how to check if cast worked?
            unsigned int i = 0;
            unsigned int a_length = arr->Length();
            while (i < a_length) {
                l->layer_->add_style(TOSTR((arr).Get(i)));
                i++;
            }
        }
    }
    else if (a == "datasource")
    {
        if (!value.IsObject())
        {
            Napi::TypeError::New(env, "mapnik.Datasource, or mapnik.MemoryDatasource instance expected").ThrowAsJavaScriptException();
            return env.Null();
        }
        if (value->IsNull() || value->IsUndefined()) {
            Napi::TypeError::New(env, "mapnik.Datasource, or mapnik.MemoryDatasource instance expected").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            Napi::Object obj = value.As<Napi::Object>();
            if (Napi::New(env, Datasource::constructor)->HasInstance(obj)) {
                Datasource *d = obj.Unwrap<Datasource>();
                l->layer_->set_datasource(d->get());
            }
            /*else if (Napi::New(env, JSDatasource::constructor)->HasInstance(obj))
            {
                JSDatasource *d = obj.Unwrap<JSDatasource>();
                l->layer_->set_datasource(d->get());
            }*/
            else if (Napi::New(env, MemoryDatasource::constructor)->HasInstance(obj))
            {
                MemoryDatasource *d = obj.Unwrap<MemoryDatasource>();
                l->layer_->set_datasource(d->get());
            }
            else
            {
                Napi::TypeError::New(env, "mapnik.Datasource or mapnik.MemoryDatasource instance expected").ThrowAsJavaScriptException();
                return env.Null();
            }
        }
    }
    else if (a == "minimum_scale_denominator")
    {
        if (!value.IsNumber()) {
            Napi::TypeError::New(env, "Must provide a number").ThrowAsJavaScriptException();
            return env.Null();
        }
        l->layer_->set_minimum_scale_denominator(value.As<Napi::Number>().DoubleValue());
    }
    else if (a == "maximum_scale_denominator")
    {
        if (!value.IsNumber()) {
            Napi::TypeError::New(env, "Must provide a number").ThrowAsJavaScriptException();
            return env.Null();
        }
        l->layer_->set_maximum_scale_denominator(value.As<Napi::Number>().DoubleValue());
    }
    else if (a == "queryable")
    {
        if (!value->IsBoolean()) {
            Napi::TypeError::New(env, "Must provide a boolean").ThrowAsJavaScriptException();
            return env.Null();
        }
        l->layer_->set_queryable(value.As<Napi::Boolean>().Value());
    }
    else if (a == "clear_label_cache")
    {
        if (!value->IsBoolean()) {
            Napi::TypeError::New(env, "Must provide a boolean").ThrowAsJavaScriptException();
            return env.Null();
        }
        l->layer_->set_clear_label_cache(value.As<Napi::Boolean>().Value());
    }
    else if (a == "active")
    {
        if (!value->IsBoolean()) {
            Napi::TypeError::New(env, "Must provide a boolean").ThrowAsJavaScriptException();
            return env.Null();
        }
        l->layer_->set_active(value.As<Napi::Boolean>().Value());
    }
}

Napi::Value Layer::describe(const Napi::CallbackInfo& info)
{
    Layer* l = info.Holder().Unwrap<Layer>();

    Napi::Object description = Napi::Object::New(env);
    mapnik::layer const& layer = *l->layer_;

    (description).Set(Napi::String::New(env, "name"), Napi::String::New(env, layer.name()));

    (description).Set(Napi::String::New(env, "srs"), Napi::String::New(env, layer.srs()));

    (description).Set(Napi::String::New(env, "active"), Napi::Boolean::New(env, layer.active()));

    (description).Set(Napi::String::New(env, "clear_label_cache"), Napi::Boolean::New(env, layer.clear_label_cache()));

    (description).Set(Napi::String::New(env, "minimum_scale_denominator"), Napi::Number::New(env, layer.minimum_scale_denominator()));

    (description).Set(Napi::String::New(env, "maximum_scale_denominator"), Napi::Number::New(env, layer.maximum_scale_denominator()));

    (description).Set(Napi::String::New(env, "queryable"), Napi::Boolean::New(env, layer.queryable()));

    std::vector<std::string> const& style_names = layer.styles();
    Napi::Array s = Napi::Array::New(env, style_names.size());
    for (unsigned i = 0; i < style_names.size(); ++i)
    {
        (s).Set(i, Napi::String::New(env, style_names[i]) );
    }

    (description).Set(Napi::String::New(env, "styles"), s );

    mapnik::datasource_ptr datasource = layer.datasource();
    Napi::Object ds = Napi::Object::New(env);
    (description).Set(Napi::String::New(env, "datasource"), ds );
    if ( datasource )
    {
        mapnik::parameters::const_iterator it = datasource->params().begin();
        mapnik::parameters::const_iterator end = datasource->params().end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object(ds, it->first, it->second);
        }
    }

    return description;
}
