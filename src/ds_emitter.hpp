#ifndef __NODE_MAPNIK_DS_EMITTER_H__
#define __NODE_MAPNIK_DS_EMITTER_H__

// v8
#include <v8.h>

#include "utils.hpp"

// mapnik
#include <mapnik/attribute_descriptor.hpp>  // for attribute_descriptor, etc
#include <mapnik/datasource.hpp>        // for datasource, etc
#include <mapnik/feature.hpp>           // for feature_impl::iterator, etc
#include <mapnik/feature_layer_desc.hpp>  // for layer_descriptor
#include <mapnik/query.hpp>             // for query
#include <mapnik/value.hpp>             // for value_base, value
#include <mapnik/version.hpp>           // for MAPNIK_VERSION

using namespace v8;

namespace node_mapnik {

static void describe_datasource(Local<Object> description, mapnik::datasource_ptr ds)
{
    try
    {
        // type
        if (ds->type() == mapnik::datasource::Raster)
        {
            description->Set(String::NewSymbol("type"), String::New("raster"));
        }
        else
        {
            description->Set(String::NewSymbol("type"), String::New("vector"));
        }

        mapnik::layer_descriptor ld = ds->get_descriptor();

        // encoding
        description->Set(String::NewSymbol("encoding"), String::New(ld.get_encoding().c_str()));

        // field names and types
        Local<Object> fields = Object::New();
        std::vector<mapnik::attribute_descriptor> const& desc = ld.get_descriptors();
        std::vector<mapnik::attribute_descriptor>::const_iterator itr = desc.begin();
        std::vector<mapnik::attribute_descriptor>::const_iterator end = desc.end();
        while (itr != end)
        {
            unsigned field_type = itr->get_type();
            std::string type("");
            if (field_type == mapnik::Integer) type = "Number";
            else if (field_type == mapnik::Float) type = "Number";
            else if (field_type == mapnik::Double) type = "Number";
            else if (field_type == mapnik::String) type = "String";
            else if (field_type == mapnik::Boolean) type = "Boolean";
            else if (field_type == mapnik::Geometry) type = "Geometry";
            else if (field_type == mapnik::Object) type = "Object";
            else type = "Unknown";
            fields->Set(String::NewSymbol(itr->get_name().c_str()),String::New(type.c_str()));
            ++itr;
        }
        description->Set(String::NewSymbol("fields"), fields);

        Local<String> js_type = String::New("unknown");
#if MAPNIK_VERSION >= 200100
        boost::optional<mapnik::datasource::geometry_t> geom_type = ds->get_geometry_type();
        if (geom_type)
        {
            mapnik::datasource::geometry_t g_type = *geom_type;
            switch (g_type)
            {
            case mapnik::datasource::Point:
            {
                js_type = String::New("point");
                break;
            }
            case mapnik::datasource::LineString:
            {
                js_type = String::New("linestring");
                break;
            }
            case mapnik::datasource::Polygon:
            {
                js_type = String::New("polygon");
                break;
            }
            case mapnik::datasource::Collection:
            {
                js_type = String::New("collection");
                break;
            }
            default:
            {
                break;
            }
            }
        }
#endif
        description->Set(String::NewSymbol("geometry_type"), js_type);
    }
    catch (std::exception const& ex)
    {
        ThrowException(Exception::Error(
                           String::New(ex.what())));
    }
    catch (...)
    {
        ThrowException(Exception::Error(
                           String::New("unknown exception happened when calling describe_datasource, please file bug")));
    }
}


static void datasource_features(Local<Array> a, mapnik::datasource_ptr ds, unsigned first, unsigned last)
{

    try
    {
        mapnik::query q(ds->envelope());
        mapnik::layer_descriptor ld = ds->get_descriptor();
        std::vector<mapnik::attribute_descriptor> const& desc = ld.get_descriptors();
        std::vector<mapnik::attribute_descriptor>::const_iterator itr = desc.begin();
        std::vector<mapnik::attribute_descriptor>::const_iterator end = desc.end();
        while (itr != end)
        {
            q.add_property_name(itr->get_name());
            ++itr;
        }

        mapnik::featureset_ptr fs = ds->features(q);
        if (fs)
        {
            mapnik::feature_ptr fp;
            unsigned idx = 0;
            while ((fp = fs->next()))
            {
                if ((idx >= first) && (idx <= last || last == 0)) {
                    Local<Object> feat = Object::New();
#if MAPNIK_VERSION >= 200100
                    mapnik::feature_impl::iterator f_itr = fp->begin();
                    mapnik::feature_impl::iterator f_end = fp->end();
                    for ( ;f_itr!=f_end; ++f_itr)
                    {
                        node_mapnik::params_to_object serializer( feat , boost::get<0>(*f_itr));
                        // need to call base() since this is a mapnik::value
                        // not a mapnik::value_holder
                        boost::apply_visitor( serializer, boost::get<1>(*f_itr).base() );
                    }
#else
                    std::map<std::string,mapnik::value> const& fprops = fp->props();
                    std::map<std::string,mapnik::value>::const_iterator it = fprops.begin();
                    std::map<std::string,mapnik::value>::const_iterator end = fprops.end();
                    for (; it != end; ++it)
                    {
                        node_mapnik::params_to_object serializer( feat , it->first);
                        // need to call base() since this is a mapnik::value
                        // not a mapnik::value_holder
                        boost::apply_visitor( serializer, it->second.base() );
                    }
#endif
                    // add feature id
                    feat->Set(String::NewSymbol("__id__"), Number::New(fp->id()));

                    a->Set(idx, feat);
                }
                ++idx;
            }
        }
    }
    catch (std::exception const& ex)
    {
        ThrowException(Exception::Error(
                           String::New(ex.what())));
    }
    catch (...)
    {
        ThrowException(Exception::Error(
                           String::New("unknown exception happened when calling datasource_features, please file bug")));
    }

}

}

#endif
