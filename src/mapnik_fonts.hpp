#ifndef __NODE_MAPNIK_FONTS_H__
#define __NODE_MAPNIK_FONTS_H__


// v8
#include <v8.h>

// mapnik
#include <mapnik/version.hpp>
#include <mapnik/font_engine_freetype.hpp>

// stl
#include <vector>

#include "utils.hpp"

using namespace v8;

namespace node_mapnik {

static inline Handle<Value> register_fonts(const Arguments& args)
{
    HandleScope scope;

    try
    {
        if (args.Length() == 0 || !args[0]->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("first argument must be a path to a directory of fonts")));

        bool found = false;

        std::vector<std::string> const names_before = mapnik::freetype_engine::face_names();

        // option hash
        if (args.Length() == 2){
            if (!args[1]->IsObject())
                return ThrowException(Exception::TypeError(
                                          String::New("second argument is optional, but if provided must be an object, eg. { recurse:Boolean }")));

            Local<Object> options = args[1]->ToObject();
            if (options->Has(String::New("recurse")))
            {
                Local<Value> recurse_opt = options->Get(String::New("recurse"));
                if (!recurse_opt->IsBoolean())
                    return ThrowException(Exception::TypeError(
                                              String::New("'recurse' must be a Boolean")));

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

        std::vector<std::string> const& names_after = mapnik::freetype_engine::face_names();
        if (names_after.size() == names_before.size())
            found = false;

        return scope.Close(Boolean::New(found));
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

static inline Handle<Value> available_font_faces(const Arguments& args)
{
    HandleScope scope;
    std::vector<std::string> const& names = mapnik::freetype_engine::face_names();
    Local<Array> a = Array::New(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        a->Set(i, String::New(names[i].c_str()));
    }
    return scope.Close(a);
}

static inline Handle<Value> available_font_files(const Arguments& args)
{
    HandleScope scope;
#if MAPNIK_VERSION >= 200100
    std::map<std::string,std::pair<int,std::string> > const& mapping = mapnik::freetype_engine::get_mapping();
    Local<Object> obj = Object::New();
    std::map<std::string,std::pair<int,std::string> >::const_iterator itr;
    for (itr = mapping.begin();itr!=mapping.end();++itr)
    {
        obj->Set(String::NewSymbol(itr->first.c_str()),String::New(itr->second.second.c_str()));
    }
#else
    std::map<std::string,std::string> const& mapping = mapnik::freetype_engine::get_mapping();
    Local<Object> obj = Object::New();
    std::map<std::string,std::string>::const_iterator itr;
    for (itr = mapping.begin();itr!=mapping.end();++itr)
    {
        obj->Set(String::NewSymbol(itr->first.c_str()),String::New(itr->second.c_str()));
    }
#endif
    return scope.Close(obj);
}


}

#endif // __NODE_MAPNIK_FONTS_H__
