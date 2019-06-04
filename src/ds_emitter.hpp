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



namespace node_mapnik {

static void get_fields(v8::Local<v8::Object> fields, mapnik::datasource_ptr ds)
{
    Nan::HandleScope scope;
    mapnik::layer_descriptor ld = ds->get_descriptor();
    // field names and types
    auto const& desc = ld.get_descriptors();
    for (auto const& attr_info : desc)
    {
        unsigned field_type = attr_info.get_type();
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
        std::string const& name = attr_info.get_name();
        Nan::Set(fields, Nan::New<v8::String>(name).ToLocalChecked(), Nan::New<v8::String>(type).ToLocalChecked());
        Nan::Set(fields, Nan::New<v8::String>(name).ToLocalChecked(), Nan::New<v8::String>(type).ToLocalChecked());
        /* LCOV_EXCL_STOP */
    }
}

static void describe_datasource(v8::Local<v8::Object> description, mapnik::datasource_ptr ds)
{
    Nan::HandleScope scope;
    
    // type
    if (ds->type() == mapnik::datasource::Raster)
    {
        Nan::Set(description, Nan::New("type").ToLocalChecked(), Nan::New<v8::String>("raster").ToLocalChecked());
    }
    else
    {
        Nan::Set(description, Nan::New("type").ToLocalChecked(), Nan::New<v8::String>("vector").ToLocalChecked());
    }

    mapnik::layer_descriptor ld = ds->get_descriptor();

    // encoding
    Nan::Set(description, Nan::New("encoding").ToLocalChecked(), Nan::New<v8::String>(ld.get_encoding().c_str()).ToLocalChecked());

    // field names and types
    v8::Local<v8::Object> fields = Nan::New<v8::Object>();
    node_mapnik::get_fields(fields, ds);
    Nan::Set(description, Nan::New("fields").ToLocalChecked(), fields);

    v8::Local<v8::String> js_type = Nan::New<v8::String>("unknown").ToLocalChecked();
    if (ds->type() == mapnik::datasource::Raster)
    {
        js_type = Nan::New<v8::String>("raster").ToLocalChecked();
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
                js_type = Nan::New<v8::String>("point").ToLocalChecked();
                break;
            }
            case mapnik::datasource_geometry_t::LineString:
            {
                js_type = Nan::New<v8::String>("linestring").ToLocalChecked();
                break;
            }
            case mapnik::datasource_geometry_t::Polygon:
            {
                js_type = Nan::New<v8::String>("polygon").ToLocalChecked();
                break;
            }
            case mapnik::datasource_geometry_t::Collection:
            {
                js_type = Nan::New<v8::String>("collection").ToLocalChecked();
                break;
            }
            default:
            {
                break;
            }
            }
        }
    }
    Nan::Set(description, Nan::New("geometry_type").ToLocalChecked(), js_type);
    for (auto const& param : ld.get_extra_parameters()) 
    {
        node_mapnik::params_to_object(description,param.first, param.second);
    }
}

}

#endif
