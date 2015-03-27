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
    NanScope();

    try
    {
        if (args.Length() == 0 || !args[0]->IsString())
        {
            NanThrowTypeError("first argument must be a path to a directory of fonts");
            NanReturnUndefined();
        }

        bool found = false;

        // option hash
        if (args.Length() >= 2)
        {
            if (!args[1]->IsObject())
            {
                NanThrowTypeError("second argument is optional, but if provided must be an object, eg. { recurse: true }");
                NanReturnUndefined();
            }

            Local<Object> options = args[1].As<Object>();
            if (options->Has(NanNew("recurse")))
            {
                Local<Value> recurse_opt = options->Get(NanNew("recurse"));
                if (!recurse_opt->IsBoolean())
                {
                    NanThrowTypeError("'recurse' must be a Boolean");
                    NanReturnUndefined();
                }

                bool recurse = recurse_opt->BooleanValue();
                std::string path = TOSTR(args[0]);
                found = mapnik::freetype_engine::register_fonts(path,recurse);
            }
        }
        else
        {
            std::string path = TOSTR(args[0]);
            found = mapnik::freetype_engine::register_fonts(path);
        }

        NanReturnValue(NanNew(found));
    }
    catch (std::exception const& ex)
    {
        // Does not appear that this line can ever be reached, not certain what would ever throw an exception
        /* LCOV_EXCL_START */
        NanThrowError(ex.what());
        NanReturnUndefined();
        /* LCOV_EXCL_END */
    }
}

static inline NAN_METHOD(available_font_faces)
{
    NanScope();
    std::vector<std::string> const& names = mapnik::freetype_engine::face_names();
    Local<Array> a = NanNew<Array>(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        a->Set(i, NanNew(names[i].c_str()));
    }
    NanReturnValue(a);
}

static inline NAN_METHOD(memory_fonts)
{
    NanScope();
    auto const& font_cache = mapnik::freetype_engine::get_cache();
    Local<Array> a = NanNew<Array>(font_cache.size());
    unsigned i = 0;
    for (auto const& kv : font_cache)
    {
        a->Set(i++, NanNew(kv.first.c_str()));
    }
    NanReturnValue(a);
}

static inline NAN_METHOD(available_font_files)
{
    NanScope();
    std::map<std::string,std::pair<int,std::string> > const& mapping = mapnik::freetype_engine::get_mapping();
    Local<Object> obj = NanNew<Object>();
    for (auto const& kv : mapping)
    {
        obj->Set(NanNew(kv.first.c_str()), NanNew(kv.second.second.c_str()));
    }
    NanReturnValue(obj);
}


}

#endif // __NODE_MAPNIK_FONTS_H__
