#include "mapnik_layer.hpp"
#include "mapnik_datasource.hpp"
#include "utils.hpp"
// mapnik
#include <mapnik/datasource.hpp>        // for datasource_ptr, datasource
#include <mapnik/memory_datasource.hpp> // for memory_datasource
#include <mapnik/layer.hpp>             // for layer
#include <mapnik/params.hpp>            // for parameters
// stl
#include <limits>

Napi::FunctionReference Layer::constructor;

Napi::Object Layer::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Layer", {
            InstanceMethod<&Layer::describe>("describe", prop_attr),
            InstanceAccessor<&Layer::name, &Layer::name>("name", prop_attr),
            InstanceAccessor<&Layer::active, &Layer::active>("active", prop_attr),
            InstanceAccessor<&Layer::srs, &Layer::srs>("srs", prop_attr),
            InstanceAccessor<&Layer::styles, &Layer::styles>("styles", prop_attr),
            InstanceAccessor<&Layer::datasource, &Layer::datasource>("datasource", prop_attr),
            InstanceAccessor<&Layer::minimum_scale_denominator, &Layer::minimum_scale_denominator>("minimum_scale_denominator", prop_attr),
            InstanceAccessor<&Layer::maximum_scale_denominator, &Layer::maximum_scale_denominator>("maximum_scale_denominator", prop_attr),
            InstanceAccessor<&Layer::queryable, &Layer::queryable>("queryable", prop_attr),
            InstanceAccessor<&Layer::clear_label_cache, &Layer::clear_label_cache>("clear_label_cache", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Layer", func);
    return exports;
}
Layer::Layer(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Layer>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 1 && info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<layer_ptr>>();
        if (ext) layer_ = *ext.Data();
        return;
    }
    if (info.Length() == 1)
    {
        if (!info[0].IsString())
        {
            Napi::TypeError::New(env, "'name' must be a string").ThrowAsJavaScriptException();
            return;
        }
        layer_ = std::make_shared<mapnik::layer>(info[0].As<Napi::String>());
    }
    else if (info.Length() == 2)
    {
        if (!info[0].IsString() || !info[1].IsString())
        {
            Napi::TypeError::New(env, "'name' and 'srs' must be a strings").ThrowAsJavaScriptException();
            return;
        }
        layer_ = std::make_shared<mapnik::layer>(info[0].As<Napi::String>(), info[1].As<Napi::String>());
    }
    else
    {
        Napi::TypeError::New(env, "please provide Layer name and optional srs").ThrowAsJavaScriptException();
    }
}

// accessors

// name
Napi::Value Layer::name(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::String::New(env, layer_->name());
}

void Layer::name(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsString())
    {
        Napi::TypeError::New(env, "'name' must be a string").ThrowAsJavaScriptException();
        return;
    }
    layer_->set_name(value.As<Napi::String>());
}

// srs
Napi::Value Layer::srs(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::String::New(env, layer_->srs());
}

void Layer::srs(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsString())
    {
        Napi::TypeError::New(env, "'srs' must be a string").ThrowAsJavaScriptException();
        return;
    }
    layer_->set_srs(value.As<Napi::String>());
}

// styles
Napi::Value Layer::styles(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::vector<std::string> const& style_names = layer_->styles();
    std::size_t size = style_names.size();
    Napi::Array arr = Napi::Array::New(env, size);
    for (std::size_t index = 0; index < size; ++index)
    {
        arr.Set(index, Napi::String::New(env, style_names[index]));
    }
    return scope.Escape(arr);
}

void Layer::styles(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsArray())
    {
        Napi::TypeError::New(env, "Must provide an array of style names").ThrowAsJavaScriptException();
    }
    else
    {
        Napi::Array arr = value.As<Napi::Array>();
        std::size_t arr_size = arr.Length();
        for (std::size_t index = 0; index < arr_size; ++index)
        {
            layer_->add_style(arr.Get(index).As<Napi::String>());
        }
    }
}

// datasource
Napi::Value Layer::datasource(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    mapnik::datasource_ptr ds = layer_->datasource();
    if (ds)
    {
        Napi::Value arg = Napi::External<mapnik::datasource_ptr>::New(env, &ds);
        Napi::Object obj = Datasource::constructor.New({arg});
        return scope.Escape(obj);
    }
    return env.Null();
}

void Layer::datasource(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsObject())
    {
        Napi::TypeError::New(env, "mapnik.Datasource instance expected").ThrowAsJavaScriptException();
        return;
    }
    else
    {
        Napi::Object obj = value.As<Napi::Object>();
        if (!obj.InstanceOf(Datasource::constructor.Value()))
        {
            Napi::TypeError::New(env, "mapnik.Datasource instance expected").ThrowAsJavaScriptException();
        }
        else
        {
            Datasource* ds = Napi::ObjectWrap<Datasource>::Unwrap(obj);
            if (ds) layer_->set_datasource(ds->impl());
        }
    }
}

// minimum_scale_denominator
Napi::Value Layer::minimum_scale_denominator(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, layer_->minimum_scale_denominator());
}

void Layer::minimum_scale_denominator(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsNumber())
    {
        Napi::TypeError::New(env, "Must provide a number").ThrowAsJavaScriptException();
    }
    else
    {
        layer_->set_minimum_scale_denominator(value.As<Napi::Number>().DoubleValue());
    }
}

// maximum_scale_denominator
Napi::Value Layer::maximum_scale_denominator(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, layer_->maximum_scale_denominator());
}

void Layer::maximum_scale_denominator(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsNumber())
    {
        Napi::TypeError::New(env, "Must provide a number").ThrowAsJavaScriptException();
    }
    else
    {
        layer_->set_maximum_scale_denominator(value.As<Napi::Number>().DoubleValue());
    }
}

// queryable
Napi::Value Layer::queryable(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, layer_->queryable());
}

void Layer::queryable(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsBoolean())
    {
        Napi::TypeError::New(env, "'name' must be a boolean").ThrowAsJavaScriptException();
    }
    else
    {
        layer_->set_queryable(value.As<Napi::Boolean>());
    }
}

// active
// queryable
Napi::Value Layer::active(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, layer_->active());
}

void Layer::active(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsBoolean())
    {
        Napi::TypeError::New(env, "'name' must be a boolean").ThrowAsJavaScriptException();
    }
    else
    {
        layer_->set_active(value.As<Napi::Boolean>());
    }
}

// clear_label_cache
Napi::Value Layer::clear_label_cache(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, layer_->clear_label_cache());
}

void Layer::clear_label_cache(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsBoolean())
    {
        Napi::TypeError::New(env, "'name' must be a boolean").ThrowAsJavaScriptException();
    }
    else
    {
        layer_->set_clear_label_cache(value.As<Napi::Boolean>());
    }
}

Napi::Value Layer::describe(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Object description = Napi::Object::New(env);
    description.Set("name", layer_->name());
    description.Set("srs", layer_->srs());
    description.Set("active", Napi::Boolean::New(env, layer_->active()));
    description.Set("clear_label_cache", Napi::Boolean::New(env, layer_->clear_label_cache()));
    description.Set("minimum_scale_denominator", Napi::Number::New(env, layer_->minimum_scale_denominator()));
    description.Set("maximum_scale_denominator", Napi::Number::New(env, layer_->maximum_scale_denominator()));
    description.Set("queryable", Napi::Boolean::New(env, layer_->queryable()));

    std::vector<std::string> const& style_names = layer_->styles();
    std::size_t size = style_names.size();
    Napi::Array styles = Napi::Array::New(env, size);
    for (std::size_t index = 0; index < size; ++index)
    {
        styles.Set(index, style_names[index]);
    }
    description.Set("styles", styles);
    Napi::Object ds = Napi::Object::New(env);
    description.Set(Napi::String::New(env, "datasource"), ds);

    mapnik::datasource_ptr datasource = layer_->datasource();
    if (datasource)
    {
        for (auto const& p : datasource->params())
        {
            node_mapnik::params_to_object(env, ds, std::get<0>(p), std::get<1>(p));
        }
    }
    return scope.Escape(description);
}
