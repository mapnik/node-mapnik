#pragma once

#include <mapnik/version.hpp>
#include <mapnik/attribute.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/value/types.hpp>

static inline void object_to_container(mapnik::attributes& cont, Napi::Object const& vars)
{
    Napi::Array names = vars.GetPropertyNames();
    std::size_t length = names.Length();
    mapnik::transcoder tr("utf8");
    cont.reserve(length);
    for (std::size_t i = 0; i < length; ++i)
    {
        std::string name = names.Get(i).ToString();
        Napi::Value value = vars.Get(name);

        if (value.IsBoolean())
        {
            cont[name] = value.As<Napi::Boolean>().Value();
        }
        else if (value.IsString())
        {
            cont[name] = tr.transcode(value.As<Napi::String>().Utf8Value().c_str());
        }
        else if (value.IsNumber())
        {
            mapnik::value_double num = value.As<Napi::Number>().DoubleValue();
            if (num == value.As<Napi::Number>().Int32Value())
            {
                cont[name] = static_cast<mapnik::value_integer>(num);
            }
            else
            {
                cont[name] = num;
            }
        }
    }
}
