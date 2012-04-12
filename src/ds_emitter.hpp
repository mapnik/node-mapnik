#ifndef __NODE_MAPNIK_DS_EMITTER_H__
#define __NODE_MAPNIK_DS_EMITTER_H__

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

namespace node_mapnik {

static void describe_datasource(Local<Object> description, mapnik::datasource_ptr ds)
{
    try
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

        mapnik::query q(ds->envelope());

        mapnik::featureset_ptr fs = ds->features(q);
        description->Set(String::NewSymbol("geometry_type"), Undefined());
        description->Set(String::NewSymbol("has_features"), False());

        // TODO - need to remove this after this lands:
        // https://github.com/mapnik/mapnik/issues/701
        if (fs)
        {
            mapnik::feature_ptr fp = fs->next();
            if (fp) {

                description->Set(String::NewSymbol("has_features"), True());
                if (fp->num_geometries() > 0)
                {
                    mapnik::geometry_type const& geom = fp->get_geometry(0);
                    mapnik::eGeomType g_type = geom.type();
                    Local<String> js_type = String::New("unknown");
                    switch (g_type)
                    {
                    case mapnik::Point:
                    {
                        if (fp->num_geometries() > 1) {
                            js_type = String::New("multipoint");
                        } else {
                            js_type = String::New("point");
                        }
                        break;
                    }
                    case mapnik::Polygon:
                    {
                        if (fp->num_geometries() > 1) {
                            js_type = String::New("multipolygon");
                        } else {
                            js_type = String::New("polygon");
                        }
                        break;
                    }
                    case mapnik::LineString:
                    {
                        if (fp->num_geometries() > 1) {
                            js_type = String::New("multilinestring");
                        } else {
                            js_type = String::New("linestring");
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                    }
                    description->Set(String::NewSymbol("geometry_type"), js_type);
                }
            }
        }
    }
    catch (const std::exception & ex)
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
                    mapnik::feature_impl::iterator itr = fp->begin();
                    mapnik::feature_impl::iterator end = fp->end();
                    for ( ;itr!=end; ++itr)
                    {
                        node_mapnik::params_to_object serializer( feat , boost::get<0>(*itr));
                        // need to call base() since this is a mapnik::value
                        // not a mapnik::value_holder
                        boost::apply_visitor( serializer, boost::get<1>(*itr).base() );
                    }

                    // add feature id
                    feat->Set(String::NewSymbol("__id__"), Integer::New(fp->id()));

                    a->Set(idx, feat);
                }
                ++idx;
            }
        }
    }
    catch (const std::exception & ex)
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
