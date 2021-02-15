#pragma once

#include <napi.h>

// stl
#include <string>
#include <memory>

// core types
#include <mapnik/unicode.hpp>
#include <mapnik/value/types.hpp>
#include <mapnik/value.hpp>
#include <mapnik/version.hpp>
#include <mapnik/params.hpp>

namespace node_mapnik {

using value_integer = mapnik::value_integer;

// adapted to work for both mapnik features and mapnik parameters
struct value_converter
{
    explicit value_converter(Napi::Env env)
        : env_(env) {}

    Napi::Value operator()(value_integer val) const
    {
        return Napi::Number::New(env_, val);
    }

    Napi::Value operator()(mapnik::value_bool val) const
    {
        return Napi::Boolean::New(env_, val);
    }

    Napi::Value operator()(double val) const
    {
        return Napi::Number::New(env_, val);
    }

    Napi::Value operator()(std::string const& val) const
    {
        return Napi::String::New(env_, val.c_str());
    }

    Napi::Value operator()(mapnik::value_unicode_string const& val) const
    {
        std::string buffer;
        mapnik::to_utf8(val, buffer);
        return Napi::String::New(env_, buffer.c_str());
    }

    Napi::Value operator()(mapnik::value_null const&) const
    {
        return env_.Null();
    }

  private:
    Napi::Env env_;
};

inline void params_to_object(Napi::Env env, Napi::Object& params, std::string const& key, mapnik::value_holder const& val)
{
    params.Set(key, mapnik::util::apply_visitor(value_converter(env), val));
}
} // namespace node_mapnik
