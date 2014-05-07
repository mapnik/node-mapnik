#include "mapnik_layer.hpp"

#include "node.h"                       // for NODE_SET_PROTOTYPE_METHOD
#include "node_object_wrap.h"           // for ObjectWrap
#include "utils.hpp"                    // for TOSTR, ATTR, etc
#include "v8.h"                         // for String, Handle, Object, etc

#include "mapnik_datasource.hpp"
#include "mapnik_memory_datasource.hpp"

// mapnik
#include <mapnik/datasource.hpp>        // for datasource_ptr, datasource
#include <mapnik/layer.hpp>             // for layer
#include <mapnik/params.hpp>            // for parameters
#include <mapnik/version.hpp>           // for MAPNIK_VERSION

#include MAPNIK_MAKE_SHARED_INCLUDE

// stl
#include <limits>

#if MAPNIK_VERSION <= 200000
#define active isActive
#define min_zoom getMinZoom
#define max_zoom getMaxZoom
#define queryable isQueryable
#define active isActive
#define visible isVisible
#endif


Persistent<FunctionTemplate> Layer::constructor;

void Layer::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Layer::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Layer"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(constructor, "describe", describe);

    // properties
    ATTR(constructor, "name", get_prop, set_prop);
    ATTR(constructor, "srs", get_prop, set_prop);
    ATTR(constructor, "styles", get_prop, set_prop);
    ATTR(constructor, "datasource", get_prop, set_prop);

    target->Set(String::NewSymbol("Layer"),constructor->GetFunction());
}

Layer::Layer(std::string const& name):
    ObjectWrap(),
    layer_(MAPNIK_MAKE_SHARED<mapnik::layer>(name)) {}

Layer::Layer(std::string const& name, std::string const& srs):
    ObjectWrap(),
    layer_(MAPNIK_MAKE_SHARED<mapnik::layer>(name,srs)) {}

Layer::Layer():
    ObjectWrap(),
    layer_() {}


Layer::~Layer() {}

Handle<Value> Layer::New(const Arguments& args)
{
    HandleScope scope;

    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Layer* l =  static_cast<Layer*>(ptr);
        l->Wrap(args.This());
        return args.This();
    }

    if (args.Length() == 1)
    {
        if (!args[0]->IsString())
        {
            return ThrowException(Exception::TypeError(
                                      String::New("'name' must be a string")));
        }
        Layer* l = new Layer(TOSTR(args[0]));
        l->Wrap(args.This());
        return args.This();
    }
    else if (args.Length() == 2)
    {
        if (!args[0]->IsString() || !args[1]->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("'name' and 'srs' must be a strings")));
        Layer* l = new Layer(TOSTR(args[0]),TOSTR(args[1]));
        l->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::TypeError(
                                  String::New("please provide Layer name and optional srs")));
    }
    return args.This();
}

Handle<Value> Layer::New(mapnik::layer const& lay_ref) {
    HandleScope scope;
    Layer* l = new Layer();
    // copy new mapnik::layer into the shared_ptr
    l->layer_ = MAPNIK_MAKE_SHARED<mapnik::layer>(lay_ref);
    Handle<Value> ext = External::New(l);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);
}

Handle<Value> Layer::get_prop(Local<String> property,
                              const AccessorInfo& info)
{
    HandleScope scope;
    Layer* l = node::ObjectWrap::Unwrap<Layer>(info.This());
    std::string a = TOSTR(property);
    if (a == "name")
        return scope.Close(String::New(l->layer_->name().c_str()));
    else if (a == "srs")
        return scope.Close(String::New(l->layer_->srs().c_str()));
    else if (a == "styles") {
        std::vector<std::string> const& style_names = l->layer_->styles();
        Local<Array> s = Array::New(style_names.size());
        for (unsigned i = 0; i < style_names.size(); ++i)
        {
            s->Set(i, String::New(style_names[i].c_str()) );
        }
        return scope.Close(s);
    }
    else if (a == "datasource") {
        mapnik::datasource_ptr ds = l->layer_->datasource();
        if (ds)
        {
            return scope.Close(Datasource::New(ds));
        }
    }
    return Undefined();
}

void Layer::set_prop(Local<String> property,
                     Local<Value> value,
                     const AccessorInfo& info)
{
    HandleScope scope;
    Layer* l = node::ObjectWrap::Unwrap<Layer>(info.This());
    std::string a = TOSTR(property);
    if (a == "name")
    {
        if (!value->IsString()) {
            ThrowException(Exception::Error(
                               String::New("'name' must be a string")));
        } else {
            l->layer_->set_name(TOSTR(value));
        }
    }
    else if (a == "srs")
    {
        if (!value->IsString()) {
            ThrowException(Exception::Error(
                               String::New("'srs' must be a string")));
        } else {
            l->layer_->set_srs(TOSTR(value));
        }
    }
    else if (a == "styles")
    {
        if (!value->IsArray())
            ThrowException(Exception::Error(
                               String::New("Must provide an array of style names")));
        else {
            Local<Array> arr = Local<Array>::Cast(value->ToObject());
            // todo - how to check if cast worked?
            unsigned int i = 0;
            unsigned int a_length = arr->Length();
            while (i < a_length) {
                l->layer_->add_style(TOSTR(arr->Get(i)));
                i++;
            }
        }
    }
    else if (a == "datasource")
    {
        Local<Object> obj = value->ToObject();
        if (value->IsNull() || value->IsUndefined()) {
            ThrowException(Exception::TypeError(String::New("mapnik.Datasource, or mapnik.MemoryDatasource instance expected")));
        } else {
            if (Datasource::constructor->HasInstance(obj)) {
                Datasource *d = node::ObjectWrap::Unwrap<Datasource>(obj);
                l->layer_->set_datasource(d->get());
            }
            /*else if (JSDatasource::constructor->HasInstance(obj))
            {
                JSDatasource *d = node::ObjectWrap::Unwrap<JSDatasource>(obj);
                l->layer_->set_datasource(d->get());
            }*/
            else if (MemoryDatasource::constructor->HasInstance(obj))
            {
                MemoryDatasource *d = node::ObjectWrap::Unwrap<MemoryDatasource>(obj);
                l->layer_->set_datasource(d->get());
            }
            else
            {
                ThrowException(Exception::TypeError(String::New("mapnik.Datasource, mapnik.JSDatasource, or mapnik.MemoryDatasource instance expected")));
            }
        }
    }
}

Handle<Value> Layer::describe(const Arguments& args)
{
    HandleScope scope;

    Layer* l = node::ObjectWrap::Unwrap<Layer>(args.This());

    Local<Object> description = Object::New();
    mapnik::layer const& layer = *l->layer_;
    if ( layer.name() != "" )
    {
        description->Set(String::NewSymbol("name"), String::New(layer.name().c_str()));
    }

    if ( layer.srs() != "" )
    {
        description->Set(String::NewSymbol("srs"), String::New(layer.srs().c_str()));
    }

    if ( !layer.active())
    {
        description->Set(String::NewSymbol("status"), Boolean::New(layer.active()));
    }

    if ( layer.clear_label_cache())
    {
        description->Set(String::NewSymbol("clear_label_cache"), Boolean::New(layer.clear_label_cache()));
    }

    if ( layer.min_zoom() > 0)
    {
        description->Set(String::NewSymbol("minzoom"), Number::New(layer.min_zoom()));
    }

    if ( layer.max_zoom() != std::numeric_limits<double>::max() )
    {
        description->Set(String::NewSymbol("maxzoom"), Number::New(layer.max_zoom()));
    }

    if ( layer.queryable())
    {
        description->Set(String::NewSymbol("queryable"), Boolean::New(layer.queryable()));
    }

    std::vector<std::string> const& style_names = layer.styles();
    Local<Array> s = Array::New(style_names.size());
    for (unsigned i = 0; i < style_names.size(); ++i)
    {
        s->Set(i, String::New(style_names[i].c_str()) );
    }

    description->Set(String::NewSymbol("styles"), s );

    mapnik::datasource_ptr datasource = layer.datasource();
    Local<v8::Object> ds = Object::New();
    description->Set(String::NewSymbol("datasource"), ds );
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

    return scope.Close(description);
}

#if MAPNIK_VERSION <= 200000
#undef active
#undef min_zoom
#undef max_zoom
#undef queryable
#undef active
#undef visible
#endif
