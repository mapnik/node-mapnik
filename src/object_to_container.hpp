#ifndef __NODE_MAPNIK_OBJECT_TO_CONTAINER__
#define __NODE_MAPNIK_OBJECT_TO_CONTAINER__

#include <mapnik/version.hpp>
#include <mapnik/attribute.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/value/types.hpp>

static inline void object_to_container(mapnik::attributes & cont, Napi::Object const& vars)
{
    Napi::Array names = Napi::GetPropertyNames(vars);
    std::size_t a_length = names->Length();
    mapnik::transcoder tr("utf8");
    cont.reserve(a_length);
    for(std::size_t i=0; i < a_length; ++i) {
        Napi::Value name = (names).Get(i)->ToString(Napi::GetCurrentContext());
        Napi::Value value = (vars).Get(name);
        if (value->IsBoolean()) {
            cont[TOSTR(name)] = value.As<Napi::Boolean>().Value();
        } else if (value.IsString()) {
            cont[TOSTR(name)] = tr.transcode(TOSTR(value));
        } else if (value.IsNumber()) {
            mapnik::value_double num = value.As<Napi::Number>().DoubleValue();
            if (num == value.As<Napi::Number>().Int32Value()) {
                cont[TOSTR(name)] = Napi::To<node_mapnik::value_integer>(value);
            } else {
                cont[TOSTR(name)] = num;
            }
        }
    }
}

#endif // __NODE_MAPNIK_OBJECT_TO_CONTAINER__
