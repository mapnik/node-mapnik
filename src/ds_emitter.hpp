#ifndef __NODE_MAPNIK_DS_EMITTER_H__
#define __NODE_MAPNIK_DS_EMITTER_H__

// nan
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

// mapnik
#include <mapnik/attribute_descriptor.hpp>  // for attribute_descriptor, etc
#include <mapnik/datasource.hpp>        // for datasource, etc
#include <mapnik/feature_layer_desc.hpp>  // for layer_descriptor

// node mapnik
#include "utils.hpp"

using namespace v8;

namespace node_mapnik {

static void get_fields(Local<Object> fields, mapnik::datasource_ptr ds)
{
    NanScope();
    mapnik::layer_descriptor ld = ds->get_descriptor();
    // field names and types
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
        /* LCOV_EXCL_START */
        // Not currently possible to author these values in mapnik core
        // Should likely be considered for removing in mapnik
        else if (field_type == mapnik::Boolean) type = "Boolean";
        else if (field_type == mapnik::Geometry) type = "Geometry";
        else if (field_type == mapnik::Object) type = "Object";
        else type = "Unknown";
        /* LCOV_EXCL_END */
        fields->Set(NanNew(itr->get_name().c_str()), NanNew(type.c_str()));
        fields->Set(NanNew(itr->get_name().c_str()), NanNew(type.c_str()));
        ++itr;
    }
}

static void describe_datasource(Local<Object> description, mapnik::datasource_ptr ds)
{
    NanScope();
    
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
    node_mapnik::get_fields(fields, ds);
    description->Set(NanNew("fields"), fields);

    Local<String> js_type = NanNew<String>("unknown");
    if (ds->type() == mapnik::datasource::Raster)
    {
        js_type = NanNew<String>("raster");
    }
    else
    {
        boost::optional<mapnik::datasource_geometry_t> geom_type = ds->get_geometry_type();
        if (geom_type)
        {
            mapnik::datasource_geometry_t g_type = *geom_type;
            switch (g_type)
            {
            case mapnik::datasource_geometry_t::Point:
            {
                js_type = NanNew<String>("point");
                break;
            }
            case mapnik::datasource_geometry_t::LineString:
            {
                js_type = NanNew<String>("linestring");
                break;
            }
            case mapnik::datasource_geometry_t::Polygon:
            {
                js_type = NanNew<String>("polygon");
                break;
            }
            case mapnik::datasource_geometry_t::Collection:
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
    }
    description->Set(NanNew("geometry_type"), js_type);
    for (auto const& param : ld.get_extra_parameters()) 
    {
        node_mapnik::params_to_object(description,param.first, param.second);
    }
}

}

#endif
