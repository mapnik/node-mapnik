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
//using namespace node;

namespace node_mapnik {

static void layer_as_json(Local<Object> meta, const mapnik::layer & layer)
{

    if ( layer.name() != "" )
    {
        meta->Set(String::NewSymbol("name"), String::New(layer.name().c_str()));
    }

    if ( layer.srs() != "" )
    {
        meta->Set(String::NewSymbol("srs"), String::New(layer.srs().c_str()));
    }

    if ( !layer.active())
    {
        meta->Set(String::NewSymbol("status"), Boolean::New(layer.active()));
    }

    if ( layer.clear_label_cache())
    {
        meta->Set(String::NewSymbol("clear_label_cache"), Boolean::New(layer.clear_label_cache()));
    }

    if ( layer.min_zoom() > 0)
    {
        meta->Set(String::NewSymbol("minzoom"), Number::New(layer.min_zoom()));
    }

    if ( layer.max_zoom() != std::numeric_limits<double>::max() )
    {
        meta->Set(String::NewSymbol("maxzoom"), Number::New(layer.max_zoom()));
    }

    if ( layer.queryable())
    {
        meta->Set(String::NewSymbol("queryable"), Boolean::New(layer.queryable()));
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
            node_mapnik::params_to_object serializer( ds , it->first);
            boost::apply_visitor( serializer, it->second );
        }
    }
}

}
#endif
