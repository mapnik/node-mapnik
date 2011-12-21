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
        description->Set(String::NewSymbol("has_features"), Boolean::New(false));
    
        // TODO - need to remove this after this lands:
        // https://github.com/mapnik/mapnik/issues/701
        if (fs)
        {
            mapnik::feature_ptr fp = fs->next();
            if (fp) {
    
                description->Set(String::NewSymbol("has_features"), Boolean::New(true));
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
    #if MAPNIK_VERSION >= 800
        mapnik::query q(ds->envelope());
    #else
        mapnik::query q(ds->envelope(),1.0,1.0);
    #endif
    
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
                if  ((idx >= first) && (idx <= last || last == 0)) {
                    std::map<std::string,mapnik::value> const& fprops = fp->props();
                    Local<Object> feat = Object::New();
                    std::map<std::string,mapnik::value>::const_iterator it = fprops.begin();
                    std::map<std::string,mapnik::value>::const_iterator end = fprops.end();
                    for (; it != end; ++it)
                    {
                        node_mapnik::params_to_object serializer( feat , it->first);
                        // need to call base() since this is a mapnik::value
                        // not a mapnik::value_holder
                        boost::apply_visitor( serializer, it->second.base() );
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

/*
// start, limit
static void datasource_stats(Local<Object> stats, mapnik::datasource_ptr ds, unsigned first, unsigned last)
{
    // TODO 
    // for strings, collect first 15 unique
    // allow options to specific which fields

#if MAPNIK_VERSION >= 800
    mapnik::query q(ds->envelope());
#else
    mapnik::query q(ds->envelope(),1.0,1.0);
#endif

    mapnik::layer_descriptor ld = ds->get_descriptor();
    std::vector<mapnik::attribute_descriptor> const& desc = ld.get_descriptors();
    Local<Object> fields = Object::New();
    std::vector<mapnik::attribute_descriptor>::const_iterator itr = desc.begin();
    std::vector<mapnik::attribute_descriptor>::const_iterator end = desc.end();
    unsigned int size = 0;
    typedef std::vector<mapnik::value> vals;
    std::map<std::string, vals > values;
    //Local<Object> values = Object::New();
    while (itr != end)
    {
        q.add_property_name(itr->get_name());
        Local<Object> field_hash = Object::New();
        int field_type = itr->get_type();
        std::string type("");
        if (field_type == 1) type = "Number";
        else if (field_type == 2) type = "Number";
        else if (field_type == 3) type = "Number";
        else if (field_type == 4) type = "String";
        else if (field_type == 5) type = "Geometry";
        else if (field_type == 6) type = "Mapnik Object";
        field_hash->Set(String::NewSymbol("type"),String::New(type.c_str()));
        fields->Set(String::NewSymbol(itr->get_name().c_str()),field_hash);
        //values->Set(String::NewSymbol(itr->get_name().c_str()),Local<Array>::New());
        ++itr;
        ++size;
    }
    stats->Set(String::NewSymbol("fields"), fields);

    // todo - ability to get mapnik features without also parseing geometries
    mapnik::featureset_ptr fs = ds->features(q);
    unsigned idx = 0;
    if (fs)
    {
        mapnik::feature_ptr fp;
        typedef std::map<std::string,mapnik::value> properties;
        properties min_prop;
        properties max_prop;
        first = true;
        while ((fp = fs->next()))
        {
            ++idx;
            properties const& fprops = fp->props();
            properties::const_iterator it;
            properties::const_iterator end;
            if (first){
                first = false;
                it = fprops.begin();
                end = fprops.end();
                for (; it != end; ++it)
                {
                    min_prop[it->first] = it->second;
                    max_prop[it->first] = it->second;
                    vals& v = values[it->first];
                    if(std::find(v.begin(), v.end(), it->second) == v.end()) {
                        v.push_back(it->second);
                    }
                }
            }
            else
            {
                it = fprops.begin();
                end = fprops.end();
                for (; it != end; ++it)
                {
                    vals& v = values[it->first];
                    if(std::find(v.begin(), v.end(), it->second) == v.end()) {
                        v.push_back(it->second);
                        if (it->second > max_prop[it->first])
                            max_prop[it->first] = it->second;
                            
                        if (it->second < min_prop[it->first])
                            min_prop[it->first] = it->second;
                    }
                }
            }
        }
        
        Local<Array> names = fields->GetPropertyNames();
        uint32_t i = 0;
        uint32_t a_length = names->Length();
        while (i < a_length) {
            Local<Value> name = names->Get(i)->ToString();
            Local<Object> hash = fields->Get(name)->ToObject();
            std::string key = TOSTR(name);
            node_mapnik::params_to_object serializer_min(hash, "min");
            boost::apply_visitor( serializer_min, min_prop[key].base() );
            node_mapnik::params_to_object serializer_max(hash, "max");
            boost::apply_visitor( serializer_max, max_prop[key].base() );
            vals& v = values[key];
            unsigned int num_vals = v.size();
            Local<Array> a = Array::New(num_vals);
            for (unsigned j = 0; j < num_vals; ++j)
            {
                a->Set(j, boost::apply_visitor(value_converter(),v[j].base()) );
            }
            hash->Set(String::NewSymbol("values"),a);
            i++;
        }
        
    }
    
    stats->Set(String::NewSymbol("count"), Number::New(idx));
}
*/

}

#endif
