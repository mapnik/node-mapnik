#ifndef __NODE_MAPNIK_FONTS_H__
#define __NODE_MAPNIK_FONTS_H__


// mapnik
#include <mapnik/font_engine_freetype.hpp>

// stl
#include <vector>

#include "utils.hpp"



namespace node_mapnik {

static inline NAN_METHOD(register_fonts)
{
    try
    {
        if (info.Length() == 0 || !info[0]->IsString())
        {
            Nan::ThrowTypeError("first argument must be a path to a directory of fonts");
            return;
        }

        bool found = false;

        // option hash
        if (info.Length() >= 2)
        {
            if (!info[1]->IsObject())
            {
                Nan::ThrowTypeError("second argument is optional, but if provided must be an object, eg. { recurse: true }");
                return;
            }

            v8::Local<v8::Object> options = info[1].As<v8::Object>();
            if (Nan::Has(options, Nan::New("recurse").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> recurse_opt = Nan::Get(options, Nan::New("recurse").ToLocalChecked()).ToLocalChecked();
                if (!recurse_opt->IsBoolean())
                {
                    Nan::ThrowTypeError("'recurse' must be a Boolean");
                    return;
                }

                bool recurse = Nan::To<bool>(recurse_opt).FromJust();
                std::string path = TOSTR(info[0]);
                found = mapnik::freetype_engine::register_fonts(path,recurse);
            }
        }
        else
        {
            std::string path = TOSTR(info[0]);
            found = mapnik::freetype_engine::register_fonts(path);
        }

        info.GetReturnValue().Set(Nan::New(found));
    }
    catch (std::exception const& ex)
    {
        // Does not appear that this line can ever be reached, not certain what would ever throw an exception
        /* LCOV_EXCL_START */
        Nan::ThrowError(ex.what());
        return;
        /* LCOV_EXCL_STOP */
    }
}

static inline NAN_METHOD(available_font_faces)
{
    auto const& names = mapnik::freetype_engine::face_names();
    v8::Local<v8::Array> a = Nan::New<v8::Array>(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        Nan::Set(a, i, Nan::New<v8::String>(names[i].c_str()).ToLocalChecked());
    }
    info.GetReturnValue().Set(a);
}

static inline NAN_METHOD(memory_fonts)
{
    auto const& font_cache = mapnik::freetype_engine::get_cache();
    v8::Local<v8::Array> a = Nan::New<v8::Array>(font_cache.size());
    unsigned i = 0;
    for (auto const& kv : font_cache)
    {
        Nan::Set(a, i++, Nan::New<v8::String>(kv.first.c_str()).ToLocalChecked());
    }
    info.GetReturnValue().Set(a);
}

static inline NAN_METHOD(available_font_files)
{
    auto const& mapping = mapnik::freetype_engine::get_mapping();
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    for (auto const& kv : mapping)
    {
        Nan::Set(obj, Nan::New<v8::String>(kv.first.c_str()).ToLocalChecked(), Nan::New<v8::String>(kv.second.second.c_str()).ToLocalChecked());
    }
    info.GetReturnValue().Set(obj);
}


}

#endif // __NODE_MAPNIK_FONTS_H__
