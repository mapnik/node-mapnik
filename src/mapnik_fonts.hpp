#ifndef __NODE_MAPNIK_FONTS_H__
#define __NODE_MAPNIK_FONTS_H__


// mapnik
#include <mapnik/font_engine_freetype.hpp>

// stl
#include <vector>

#include "utils.hpp"

using namespace v8;

namespace node_mapnik {

static inline NAN_METHOD(register_fonts)
{
    Nan::HandleScope scope;

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

            Local<Object> options = info[1].As<Object>();
            if (options->Has(Nan::New("recurse").ToLocalChecked()))
            {
                Local<Value> recurse_opt = options->Get(Nan::New("recurse").ToLocalChecked());
                if (!recurse_opt->IsBoolean())
                {
                    Nan::ThrowTypeError("'recurse' must be a Boolean");
                    return;
                }

                bool recurse = recurse_opt->BooleanValue();
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
        /* LCOV_EXCL_END */
    }
}

static inline NAN_METHOD(available_font_faces)
{
    Nan::HandleScope scope;
    auto const& names = mapnik::freetype_engine::face_names();
    Local<Array> a = Nan::New<Array>(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        a->Set(i, Nan::New<String>(names[i].c_str()).ToLocalChecked());
    }
    info.GetReturnValue().Set(a);
}

static inline NAN_METHOD(memory_fonts)
{
    Nan::HandleScope scope;
    auto const& font_cache = mapnik::freetype_engine::get_cache();
    Local<Array> a = Nan::New<Array>(font_cache.size());
    unsigned i = 0;
    for (auto const& kv : font_cache)
    {
        a->Set(i++, Nan::New<String>(kv.first.c_str()).ToLocalChecked());
    }
    info.GetReturnValue().Set(a);
}

static inline NAN_METHOD(available_font_files)
{
    Nan::HandleScope scope;
    auto const& mapping = mapnik::freetype_engine::get_mapping();
    Local<Object> obj = Nan::New<Object>();
    for (auto const& kv : mapping)
    {
        obj->Set(Nan::New<String>(kv.first.c_str()).ToLocalChecked(), Nan::New<String>(kv.second.second.c_str()).ToLocalChecked());
    }
    info.GetReturnValue().Set(obj);
}


}

#endif // __NODE_MAPNIK_FONTS_H__
