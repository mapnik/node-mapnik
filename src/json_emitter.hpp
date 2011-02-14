#ifndef __NODE_MAPNIK_JSON_EMITTER_H__
#define __NODE_MAPNIK_JSON_EMITTER_H__

// v8
#include <v8.h>

// node
#include <node.h>

// mapnik
#include <mapnik/layer.hpp>
#include <mapnik/params.hpp>
#include <mapnik/feature_layer_desc.hpp>

using namespace v8;
using namespace node;


static void layer_as_json(Local<Object> meta, const mapnik::layer & layer)
{

    if ( layer.name() != "" )
    {
        meta->Set(String::NewSymbol("name"), String::New(layer.name().c_str()));
    }

    if ( layer.abstract() != "" )
    {
        meta->Set(String::NewSymbol("abstract"), String::New(layer.abstract().c_str()));
    }

    if ( layer.title() != "" )
    {
        meta->Set(String::NewSymbol("title"), String::New(layer.title().c_str()));
    }

    if ( layer.srs() != "" )
    {
        meta->Set(String::NewSymbol("srs"), String::New(layer.srs().c_str()));
    }

    if ( !layer.isActive())
    {
        meta->Set(String::NewSymbol("status"), Boolean::New(layer.isActive()));
    }

    if ( layer.clear_label_cache())
    {
        meta->Set(String::NewSymbol("clear_label_cache"), Boolean::New(layer.clear_label_cache()));
    }

    if ( layer.getMinZoom() )
    {
        meta->Set(String::NewSymbol("minzoom"), Number::New(layer.getMinZoom()));
    }

    if ( layer.getMaxZoom() != std::numeric_limits<double>::max() )
    {
        meta->Set(String::NewSymbol("maxzoom"), Number::New(layer.getMaxZoom()));
    }

    if ( layer.isQueryable())
    {
        meta->Set(String::NewSymbol("queryable"), Boolean::New(layer.isQueryable()));
    }

    std::vector<std::string> const& style_names = layer.styles();
    Local<Array> s = Array::New(style_names.size());
    for (unsigned i = 0; i < style_names.size(); ++i)
    {
        s->Set(i, String::New(style_names[i].c_str()) );
    }

    meta->Set(String::NewSymbol("styles"), s );

    mapnik::datasource_ptr datasource = layer.datasource();
    Local<v8::Object> ds = Object::New();
    meta->Set(String::NewSymbol("datasource"), ds );
    if ( datasource )
    {
        mapnik::parameters::const_iterator it = datasource->params().begin();
        mapnik::parameters::const_iterator end = datasource->params().end();
        for (; it != end; ++it)
        {
            params_to_object serializer( ds , it->first);
            boost::apply_visitor( serializer, it->second );
        }
    }
}

static void describe_datasource(Local<Object> description, mapnik::datasource_ptr ds)
{

    // todo collect active attributes in styles
    /*
    std::vector<std::string> const& style_names = layer.styles();
    Local<Array> s = Array::New(style_names.size());
    for (unsigned i = 0; i < style_names.size(); ++i)
    {
        s->Set(i, String::New(style_names[i].c_str()) );
    }
    meta->Set(String::NewSymbol("styles"), s );
    */

    // type
    if (ds->type() == mapnik::datasource::Raster)
    {
        description->Set(String::NewSymbol("type"), String::New("raster"));
    }
    else
    {
        description->Set(String::NewSymbol("type"), String::New("vector"));
    }

    // extent
    Local<Array> a = Array::New(4);
    mapnik::box2d<double> e = ds->envelope();
    a->Set(0, Number::New(e.minx()));
    a->Set(1, Number::New(e.miny()));
    a->Set(2, Number::New(e.maxx()));
    a->Set(3, Number::New(e.maxy()));
    description->Set(String::NewSymbol("extent"), a);

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
        int field_type = itr->get_type();
        std::string type("");
        if (field_type == 1) type = "Number";
        else if (field_type == 2) type = "Number";
        else if (field_type == 3) type = "Number";
        else if (field_type == 4) type = "String";
        else if (field_type == 5) type = "Geometry";
        else if (field_type == 6) type = "Mapnik Object";
        fields->Set(String::NewSymbol(itr->get_name().c_str()),String::New(type.c_str()));
        ++itr;
    }
    description->Set(String::NewSymbol("fields"), fields);

    // get first geometry type using naive first hit approach
    // TODO proper approach --> https://trac.mapnik.org/ticket/701
#if MAPNIK_VERSION >= 800
    mapnik::query q(ds->envelope());
#else
    mapnik::query q(ds->envelope(),1.0,1.0);
#endif

    mapnik::featureset_ptr fs = ds->features(q);
    description->Set(String::NewSymbol("geometry_type"), Undefined());

    if (fs)
    {
        mapnik::feature_ptr fp = fs->next();
        if (fp) {

            if (fp->num_geometries() > 0)
            {
                mapnik::geometry_type const& geom = fp->get_geometry(0);
                mapnik::eGeomType g_type = geom.type();
                if (g_type == mapnik::Point)
                {
                    description->Set(String::NewSymbol("geometry_type"), String::New("point"));
                }
                else if (g_type == mapnik::Polygon)
                {
                    description->Set(String::NewSymbol("geometry_type"), String::New("polygon"));
                }
                else if (g_type == mapnik::LineString)
                {
                    description->Set(String::NewSymbol("geometry_type"), String::New("linestring"));
                }
            }
        }
    }
}


static void datasource_features(Local<Array> a, mapnik::datasource_ptr ds, unsigned first, unsigned last)
{

#if MAPNIK_VERSION >= 800
    mapnik::query q(ds->envelope());
#else
    mapnik::query q(ds->envelope(),1.0,1.0);
#endif

    mapnik::layer_descriptor ld = ds->get_descriptor();
    std::vector<mapnik::attribute_descriptor> const& desc = ld.get_descriptors();
    std::vector<mapnik::attribute_descriptor>::const_iterator itr = desc.begin();
    std::vector<mapnik::attribute_descriptor>::const_iterator end = desc.end();
    unsigned size=0;
    while (itr != end)
    {
        q.add_property_name(itr->get_name());
        ++itr;
        ++size;
    }

    mapnik::featureset_ptr fs = ds->features(q);
    if (fs)
    {
        mapnik::feature_ptr fp;
        unsigned idx = 0;
        while ((fp = fs->next()))
        {
            if  ((idx >= first) && (idx <= last || last == 0)) {
		            std::map<std::string,mapnik::value> const& fprops = fp->props();
		            Local<Object> feat = Object::New();
		            std::map<std::string,mapnik::value>::const_iterator it = fprops.begin();
		            std::map<std::string,mapnik::value>::const_iterator end = fprops.end();
		            for (; it != end; ++it)
		            {
		                params_to_object serializer( feat , it->first);
		                // need to call base() since this is a mapnik::value
		                // not a mapnik::value_holder
		                boost::apply_visitor( serializer, it->second.base() );
		            }
		            a->Set(idx, feat);
            }
            ++idx;
        }
    }
}


#endif
