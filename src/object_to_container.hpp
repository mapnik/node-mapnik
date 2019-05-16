#ifndef __NODE_MAPNIK_OBJECT_TO_CONTAINER__
#define __NODE_MAPNIK_OBJECT_TO_CONTAINER__

#include <mapnik/version.hpp>
#include <mapnik/attribute.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/value/types.hpp>

static inline void object_to_container(mapnik::attributes & cont, v8::Local<v8::Object> const& vars)
{
    v8::Local<v8::Array> names = Nan::GetPropertyNames(vars).ToLocalChecked();
    std::size_t a_length = names->Length();
    mapnik::transcoder tr("utf8");
    cont.reserve(a_length);
    for(std::size_t i=0; i < a_length; ++i) {
        v8::Local<v8::Value> name = Nan::Get(names, i).ToLocalChecked()->ToString(Nan::GetCurrentContext()).ToLocalChecked();
        v8::Local<v8::Value> value = Nan::Get(vars, name).ToLocalChecked();
        if (value->IsBoolean()) {
            cont[TOSTR(name)] = Nan::To<bool>(value).FromJust();
        } else if (value->IsString()) {
            cont[TOSTR(name)] = tr.transcode(TOSTR(value));
        } else if (value->IsNumber()) {
            mapnik::value_double num = Nan::To<double>(value).FromJust();
            if (num == Nan::To<int>(value).FromJust()) {
                cont[TOSTR(name)] = Nan::To<node_mapnik::value_integer>(value).FromJust();
            } else {
                cont[TOSTR(name)] = num;
            }
        }
    }
}

#endif // __NODE_MAPNIK_OBJECT_TO_CONTAINER__
