#ifndef __NODE_MAPNIK_DS_EMITTER_H__
#define __NODE_MAPNIK_DS_EMITTER_H__

#include "mapnik3x_compatibility.hpp"
#include MAPNIK_VARIANT_INCLUDE

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
    NanScope();
    try
    {
        // type
        if (ds->type() == mapnik::datasource::Raster)
        {
            description->Set(NanNew("type"), NanNew<String>("raster"));
        }
        else
        {
            description->Set(NanNew("type"), NanNew<String>("vector"));
        }

        mapnik::layer_descriptor ld = ds->get_descriptor();

        // encoding
        description->Set(NanNew("encoding"), NanNew<String>(ld.get_encoding().c_str()));

        // field names and types
        Local<Object> fields = NanNew<Object>();
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
            fields->Set(NanNew(itr->get_name().c_str()), NanNew(type.c_str()));
            fields->Set(NanNew(itr->get_name().c_str()), NanNew(type.c_str()));
            ++itr;
        }
        description->Set(NanNew("fields"), fields);

        Local<String> js_type = NanNew<String>("unknown");
        if (ds->type() == mapnik::datasource::Raster)
        {
            js_type = NanNew<String>("raster");
        }
        else
        {
#if MAPNIK_VERSION >= 200100
            boost::optional<mapnik::datasource::geometry_t> geom_type = ds->get_geometry_type();
            if (geom_type)
            {
                mapnik::datasource::geometry_t g_type = *geom_type;
                switch (g_type)
                {
                case mapnik::datasource::Point:
                {
                    js_type = NanNew<String>("point");
                    break;
                }
                case mapnik::datasource::LineString:
                {
                    js_type = NanNew<String>("linestring");
                    break;
                }
                case mapnik::datasource::Polygon:
                {
                    js_type = NanNew<String>("polygon");
                    break;
                }
                case mapnik::datasource::Collection:
                {
                    js_type = NanNew<String>("collection");
                    break;
                }
                default:
                {
                    break;
                }
                }
            }
#endif
        }
        description->Set(NanNew("geometry_type"), js_type);
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
    }
    catch (...)
    {
        NanThrowError("unknown exception happened when calling describe_datasource, please file bug");
    }
}


static void datasource_features(Local<Array> a, mapnik::datasource_ptr ds, unsigned first, unsigned last)
{
    NanScope();
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
                    Local<Object> feat = NanNew<Object>();
                    mapnik::feature_impl::iterator f_itr = fp->begin();
                    mapnik::feature_impl::iterator f_end = fp->end();
                    for ( ;f_itr!=f_end; ++f_itr)
                    {
                        node_mapnik::params_to_object serializer( feat , MAPNIK_GET<0>(*f_itr));
                        // need to call base() since this is a mapnik::value
                        // not a mapnik::value_holder
                        MAPNIK_APPLY_VISITOR( serializer, MAPNIK_GET<1>(*f_itr).base() );
                    }
                    // add feature id
                    feat->Set(NanNew("__id__"), NanNew<Number>(fp->id()));
                    a->Set(idx, feat);
                }
                ++idx;
            }
        }
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
    }
    catch (...)
    {
        NanThrowError("unknown exception happened when calling datasource_features, please file bug");
    }

}

}

#endif
