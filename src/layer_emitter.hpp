#ifndef __NODE_MAPNIK_LAYER_EMITTER_H__
#define __NODE_MAPNIK_LAYER_EMITTER_H__

// v8
#include <v8.h>

// node
//#include <node.h>

// mapnik
#include <mapnik/layer.hpp>
#include <mapnik/params.hpp>
//#include <mapnik/feature_layer_desc.hpp>

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

#endif
