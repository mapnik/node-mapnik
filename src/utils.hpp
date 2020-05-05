#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

// stl
#include <string>
#include <memory>

// core types
#include <mapnik/unicode.hpp>
#include <mapnik/value/types.hpp>
#include <mapnik/value.hpp>
#include <mapnik/version.hpp>
#include <mapnik/params.hpp>

#define TOSTR(obj) (obj->As<Napi::String>().Utf8Value().c_str())

#define FUNCTION_ARG(I, VAR)                                            \
    if (info.Length() <= (I) || !info[I].IsFunction()) {               \
        Napi::TypeError::New(env, "Argument " #I " must be a function").ThrowAsJavaScriptException(); \
        return;                                                         \
    }                                                                   \
    Napi::Function VAR = info[I].As<Napi::Function>();

#define ATTR(t, name, get, set)                                         \
    Napi::SetAccessor(t->InstanceTemplate(), Napi::String::New(env, name), get, set);

#define NODE_MAPNIK_DEFINE_CONSTANT(target, name, constant)             \
    ((target)).Set(Napi::String::New(env, name), Napi::Number::New(env, constant));

#define NODE_MAPNIK_DEFINE_64_BIT_CONSTANT(target, name, constant)      \
    ((target)).Set(Napi::String::New(env, name),  Napi::Number::New(env, constant));




inline Napi::Value CallbackError(Napi::Env env, std::string const& message, Napi::Function const& func) {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("message", message);
    return func.Call({obj});
}


namespace node_mapnik {

typedef mapnik::value_integer value_integer;

// adapted to work for both mapnik features and mapnik parameters
struct value_converter
{
    Napi::Value operator () ( value_integer val ) const
    {
        return Napi::Number::New(env, val);
    }

    Napi::Value operator () (mapnik::value_bool val ) const
    {
        return Napi::Boolean::New(env, val);
    }

    Napi::Value operator () ( double val ) const
    {
        return Napi::Number::New(env, val);
    }

    Napi::Value operator () ( std::string const& val ) const
    {
        return Napi::String::New(env, val.c_str());
    }

    Napi::Value operator () ( mapnik::value_unicode_string const& val) const
    {
        std::string buffer;
        mapnik::to_utf8(val,buffer);
        return Napi::String::New(env, buffer.c_str());
    }

    Napi::Value operator () ( mapnik::value_null const& ) const
    {
        return env.Null();
    }
};

inline void params_to_object(Napi::Object& ds, std::string const& key, mapnik::value_holder const& val)
{
    (ds).Set(Napi::String::New(env, key.c_str()), mapnik::util::apply_visitor(value_converter(), val));
}

inline Napi::MaybeLocal<v8::Object> NewBufferFrom(std::unique_ptr<std::string> && ptr)
{
    Napi::MaybeLocal<v8::Object> res = Napi::Buffer<char>::New(env,
            &(*ptr)[0],
            ptr->size(),
            [](char*, void* hint) {
                delete static_cast<std::string*>(hint);
            },
            ptr.get());
    if (!res.IsEmpty())
    {
        ptr.release();
    }
    return res;
}

} // end ns
