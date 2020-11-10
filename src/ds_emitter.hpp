#pragma once
// mapnik
#include <mapnik/attribute_descriptor.hpp> // for attribute_descriptor, etc
#include <mapnik/datasource.hpp>           // for datasource, etc
#include <mapnik/feature_layer_desc.hpp>   // for layer_descriptor

// node mapnik
#include "utils.hpp"
//
#include <napi.h>

namespace node_mapnik {

static void get_fields(Napi::Env env, Napi::Object& fields, mapnik::datasource_ptr ds)
{
    mapnik::layer_descriptor ld = ds->get_descriptor();
    // field names and types
    auto const& desc = ld.get_descriptors();
    for (auto const& attr_info : desc)
    {
        unsigned field_type = attr_info.get_type();
        std::string type("");
        if (field_type == mapnik::Integer)
            type = "Number";
        else if (field_type == mapnik::Float)
            type = "Number";
        else if (field_type == mapnik::Double)
            type = "Number";
        else if (field_type == mapnik::String)
            type = "String";
        // LCOV_EXCL_START
        // Not currently possible to author these values in mapnik core
        // Should likely be considered for removing in mapnik
        else if (field_type == mapnik::Boolean)
            type = "Boolean";
        else if (field_type == mapnik::Geometry)
            type = "Geometry";
        else if (field_type == mapnik::Object)
            type = "Object";
        else
            type = "Unknown";
        std::string const& name = attr_info.get_name();
        fields.Set(name, type);
        // LCOV_EXCL_STOP
    }
}

static void describe_datasource(Napi::Env env, Napi::Object description, mapnik::datasource_ptr ds)
{
    // type
    if (ds->type() == mapnik::datasource::Raster)
    {
        description.Set(Napi::String::New(env, "type"), Napi::String::New(env, "raster"));
    }
    else
    {
        description.Set(Napi::String::New(env, "type"), Napi::String::New(env, "vector"));
    }

    mapnik::layer_descriptor ld = ds->get_descriptor();

    // encoding
    description.Set(Napi::String::New(env, "encoding"), Napi::String::New(env, ld.get_encoding().c_str()));

    // field names and types
    Napi::Object fields = Napi::Object::New(env);
    node_mapnik::get_fields(env, fields, ds);
    description.Set(Napi::String::New(env, "fields"), fields);

    Napi::String js_type = Napi::String::New(env, "unknown");
    if (ds->type() == mapnik::datasource::Raster)
    {
        js_type = Napi::String::New(env, "raster");
    }
    else
    {
        boost::optional<mapnik::datasource_geometry_t> geom_type = ds->get_geometry_type();
        if (geom_type)
        {
            mapnik::datasource_geometry_t g_type = *geom_type;
            switch (g_type)
            {
            case mapnik::datasource_geometry_t::Point: {
                js_type = Napi::String::New(env, "point");
                break;
            }
            case mapnik::datasource_geometry_t::LineString: {
                js_type = Napi::String::New(env, "linestring");
                break;
            }
            case mapnik::datasource_geometry_t::Polygon: {
                js_type = Napi::String::New(env, "polygon");
                break;
            }
            case mapnik::datasource_geometry_t::Collection: {
                js_type = Napi::String::New(env, "collection");
                break;
            }
            default:
                break;
            }
        }
    }
    description.Set("geometry_type", js_type);
    for (auto const& param : ld.get_extra_parameters())
    {
        node_mapnik::params_to_object(env, description, param.first, param.second);
    }
}

} // namespace node_mapnik
