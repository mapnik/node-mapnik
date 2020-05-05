#ifndef __NODE_MAPNIK_FONTS_H__
#define __NODE_MAPNIK_FONTS_H__


// mapnik
#include <mapnik/font_engine_freetype.hpp>

// stl
#include <vector>

#include "utils.hpp"



namespace node_mapnik {

static inline Napi::Value register_fonts(Napi::CallbackInfo const& info)
{
    try
    {
        if (info.Length() == 0 || !info[0].IsString())
        {
            Napi::TypeError::New(env, "first argument must be a path to a directory of fonts").ThrowAsJavaScriptException();
            return env.Null();
        }

        bool found = false;

        // option hash
        if (info.Length() >= 2)
        {
            if (!info[1].IsObject())
            {
                Napi::TypeError::New(env, "second argument is optional, but if provided must be an object, eg. { recurse: true }").ThrowAsJavaScriptException();
                return env.Null();
            }

            Napi::Object options = info[1].As<Napi::Object>();
            if ((options).Has(Napi::String::New(env, "recurse")).FromMaybe(false))
            {
                Napi::Value recurse_opt = (options).Get(Napi::String::New(env, "recurse"));
                if (!recurse_opt->IsBoolean())
                {
                    Napi::TypeError::New(env, "'recurse' must be a Boolean").ThrowAsJavaScriptException();
                    return env.Null();
                }

                bool recurse = recurse_opt.As<Napi::Boolean>().Value();
                std::string path = TOSTR(info[0]);
                found = mapnik::freetype_engine::register_fonts(path,recurse);
            }
        }
        else
        {
            std::string path = TOSTR(info[0]);
            found = mapnik::freetype_engine::register_fonts(path);
        }

        return Napi::New(env, found);
    }
    catch (std::exception const& ex)
    {
        // Does not appear that this line can ever be reached, not certain what would ever throw an exception
        /* LCOV_EXCL_START */
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
        /* LCOV_EXCL_STOP */
    }
}

static inline Napi::Value available_font_faces(Napi::CallbackInfo const& info)
{
    auto const& names = mapnik::freetype_engine::face_names();
    Napi::Array a = Napi::Array::New(env, names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        (a).Set(i, Napi::String::New(env, names[i].c_str()));
    }
    return a;
}

static inline Napi::Value memory_fonts(Napi::CallbackInfo const& info)
{
    auto const& font_cache = mapnik::freetype_engine::get_cache();
    Napi::Array a = Napi::Array::New(env, font_cache.size());
    unsigned i = 0;
    for (auto const& kv : font_cache)
    {
        (a).Set(i++, Napi::String::New(env, kv.first.c_str()));
    }
    return a;
}

static inline Napi::Value available_font_files(Napi::CallbackInfo const& info)
{
    auto const& mapping = mapnik::freetype_engine::get_mapping();
    Napi::Object obj = Napi::Object::New(env);
    for (auto const& kv : mapping)
    {
        (obj).Set(Napi::String::New(env, kv.first.c_str()), Napi::String::New(env, kv.second.second.c_str()));
    }
    return obj;
}


}

#endif // __NODE_MAPNIK_FONTS_H__
