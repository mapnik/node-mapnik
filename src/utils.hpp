#ifndef __NODE_MAPNIK_UTILS_H__
#define __NODE_MAPNIK_UTILS_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
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

#define TOSTR(obj) (*Nan::Utf8String(obj))

#define FUNCTION_ARG(I, VAR)                                            \
    if (info.Length() <= (I) || !info[I]->IsFunction()) {               \
        Nan::ThrowTypeError("Argument " #I " must be a function");      \
        return;                                                         \
    }                                                                   \
    v8::Local<v8::Function> VAR = info[I].As<v8::Function>();

#define ATTR(t, name, get, set)                                         \
    Nan::SetAccessor(t->InstanceTemplate(), Nan::New<v8::String>(name).ToLocalChecked(), get, set);

#define NODE_MAPNIK_DEFINE_CONSTANT(target, name, constant)             \
    Nan::Set((target), Nan::New<v8::String>(name).ToLocalChecked(), Nan::New<v8::Integer>(constant));

#define NODE_MAPNIK_DEFINE_64_BIT_CONSTANT(target, name, constant)      \
    Nan::Set((target), Nan::New<v8::String>(name).ToLocalChecked(),  Nan::New<v8::Number>(constant));




namespace node_mapnik {

typedef mapnik::value_integer value_integer;

// adapted to work for both mapnik features and mapnik parameters
struct value_converter
{
    v8::Local<v8::Value> operator () ( value_integer val ) const
    {
        return Nan::New<v8::Number>(val);
    }

    v8::Local<v8::Value> operator () (mapnik::value_bool val ) const
    {
        return Nan::New<v8::Boolean>(val);
    }

    v8::Local<v8::Value> operator () ( double val ) const
    {
        return Nan::New<v8::Number>(val);
    }

    v8::Local<v8::Value> operator () ( std::string const& val ) const
    {
        return Nan::New<v8::String>(val.c_str()).ToLocalChecked();
    }

    v8::Local<v8::Value> operator () ( mapnik::value_unicode_string const& val) const
    {
        std::string buffer;
        mapnik::to_utf8(val,buffer);
        return Nan::New<v8::String>(buffer.c_str()).ToLocalChecked();
    }

    v8::Local<v8::Value> operator () ( mapnik::value_null const& ) const
    {
        return Nan::Null();
    }
};

inline void params_to_object(v8::Local<v8::Object>& ds, std::string const& key, mapnik::value_holder const& val)
{
    Nan::Set(ds, Nan::New<v8::String>(key.c_str()).ToLocalChecked(), mapnik::util::apply_visitor(value_converter(), val));
}

inline Nan::MaybeLocal<v8::Object> NewBufferFrom(std::unique_ptr<std::string> && ptr)
{
    Nan::MaybeLocal<v8::Object> res = Nan::NewBuffer(
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
#endif
