#ifndef __NODE_MAPNIK_OBJECT_TO_CONTAINER__
#define __NODE_MAPNIK_OBJECT_TO_CONTAINER__

#include <mapnik/version.hpp>
#include <mapnik/attribute.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/value/types.hpp>

static inline void object_to_container(mapnik::attributes & cont, v8::Local<v8::Object> const& vars)
{
    v8::Local<v8::Array> names = vars->GetPropertyNames();
    std::size_t a_length = names->Length();
    mapnik::transcoder tr("utf8");
    cont.reserve(a_length);
    for(std::size_t i=0; i < a_length; ++i) {
        v8::Local<v8::Value> name = names->Get(i)->ToString();
        v8::Local<v8::Value> value = vars->Get(name);
        if (value->IsBoolean()) {
            cont[TOSTR(name)] = value->ToBoolean()->Value();
        } else if (value->IsString()) {
            cont[TOSTR(name)] = tr.transcode(TOSTR(value));
        } else if (value->IsNumber()) {
            mapnik::value_double num = value->NumberValue();
            if (num == value->IntegerValue()) {
                cont[TOSTR(name)] = static_cast<node_mapnik::value_integer>(value->IntegerValue());
            } else {
                cont[TOSTR(name)] = num;
            }
        }
    }
}

#endif // __NODE_MAPNIK_OBJECT_TO_CONTAINER__
