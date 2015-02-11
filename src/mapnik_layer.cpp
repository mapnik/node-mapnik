#include "mapnik_layer.hpp"

#include "utils.hpp"                    // for TOSTR, ATTR, etc

#include "mapnik_datasource.hpp"
#include "mapnik_memory_datasource.hpp"

// mapnik
#include <mapnik/datasource.hpp>        // for datasource_ptr, datasource
#include <mapnik/layer.hpp>             // for layer
#include <mapnik/params.hpp>            // for parameters

// stl
#include <limits>

Persistent<FunctionTemplate> Layer::constructor;

void Layer::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Layer::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Layer"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(lcons, "describe", describe);

    // properties
    ATTR(lcons, "name", get_prop, set_prop);
    ATTR(lcons, "srs", get_prop, set_prop);
    ATTR(lcons, "styles", get_prop, set_prop);
    ATTR(lcons, "datasource", get_prop, set_prop);

    target->Set(NanNew("Layer"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Layer::Layer(std::string const& name):
    node::ObjectWrap(),
    layer_(std::make_shared<mapnik::layer>(name)) {}

Layer::Layer(std::string const& name, std::string const& srs):
    node::ObjectWrap(),
    layer_(std::make_shared<mapnik::layer>(name,srs)) {}

Layer::Layer():
    node::ObjectWrap(),
    layer_() {}


Layer::~Layer() {}

NAN_METHOD(Layer::New)
{
    NanScope();

    if (!args.IsConstructCall()) {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args[0]->IsExternal())
    {
        Local<External> ext = args[0].As<External>();
        void* ptr = ext->Value();
        Layer* l =  static_cast<Layer*>(ptr);
        l->Wrap(args.This());
        NanReturnValue(args.This());
    }

    if (args.Length() == 1)
    {
        if (!args[0]->IsString())
        {
            NanThrowTypeError("'name' must be a string");
            NanReturnUndefined();
        }
        Layer* l = new Layer(TOSTR(args[0]));
        l->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else if (args.Length() == 2)
    {
        if (!args[0]->IsString() || !args[1]->IsString()) {
            NanThrowTypeError("'name' and 'srs' must be a strings");
            NanReturnUndefined();
        }
        Layer* l = new Layer(TOSTR(args[0]),TOSTR(args[1]));
        l->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else
    {
        NanThrowTypeError("please provide Layer name and optional srs");
        NanReturnUndefined();
    }
    NanReturnValue(args.This());
}

Handle<Value> Layer::NewInstance(mapnik::layer const& lay_ref) {
    NanEscapableScope();
    Layer* l = new Layer();
    // copy new mapnik::layer into the shared_ptr
    l->layer_ = std::make_shared<mapnik::layer>(lay_ref);
    Handle<Value> ext = NanNew<External>(l);
    Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
    return NanEscapeScope(obj);
}

NAN_GETTER(Layer::get_prop)
{
    NanScope();
    Layer* l = node::ObjectWrap::Unwrap<Layer>(args.Holder());
    std::string a = TOSTR(property);
    if (a == "name")
        NanReturnValue(NanNew(l->layer_->name().c_str()));
    else if (a == "srs")
        NanReturnValue(NanNew(l->layer_->srs().c_str()));
    else if (a == "styles") {
        std::vector<std::string> const& style_names = l->layer_->styles();
        Local<Array> s = NanNew<Array>(style_names.size());
        for (unsigned i = 0; i < style_names.size(); ++i)
        {
            s->Set(i, NanNew(style_names[i].c_str()) );
        }
        NanReturnValue(s);
    }
    else if (a == "datasource") {
        mapnik::datasource_ptr ds = l->layer_->datasource();
        if (ds)
        {
            NanReturnValue(Datasource::NewInstance(ds));
        }
    }
    NanReturnUndefined();
}

NAN_SETTER(Layer::set_prop)
{
    NanScope();
    Layer* l = node::ObjectWrap::Unwrap<Layer>(args.Holder());
    std::string a = TOSTR(property);
    if (a == "name")
    {
        if (!value->IsString()) {
            NanThrowTypeError("'name' must be a string");
            return;
        } else {
            l->layer_->set_name(TOSTR(value));
        }
    }
    else if (a == "srs")
    {
        if (!value->IsString()) {
            NanThrowTypeError("'srs' must be a string");
            return;
        } else {
            l->layer_->set_srs(TOSTR(value));
        }
    }
    else if (a == "styles")
    {
        if (!value->IsArray()) {
            NanThrowTypeError("Must provide an array of style names");
            return;
        } else {
            Local<Array> arr = value.As<Array>();
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
        Local<Object> obj = value.As<Object>();
        if (value->IsNull() || value->IsUndefined()) {
            NanThrowTypeError("mapnik.Datasource, or mapnik.MemoryDatasource instance expected");
            return;
        } else {
            if (NanNew(Datasource::constructor)->HasInstance(obj)) {
                Datasource *d = node::ObjectWrap::Unwrap<Datasource>(obj);
                l->layer_->set_datasource(d->get());
            }
            /*else if (NanNew(JSDatasource::constructor)->HasInstance(obj))
            {
                JSDatasource *d = node::ObjectWrap::Unwrap<JSDatasource>(obj);
                l->layer_->set_datasource(d->get());
            }*/
            else if (NanNew(MemoryDatasource::constructor)->HasInstance(obj))
            {
                MemoryDatasource *d = node::ObjectWrap::Unwrap<MemoryDatasource>(obj);
                l->layer_->set_datasource(d->get());
            }
            else
            {
                NanThrowTypeError("mapnik.Datasource, mapnik.JSDatasource, or mapnik.MemoryDatasource instance expected");
                return;
            }
        }
    }
}

NAN_METHOD(Layer::describe)
{
    NanScope();

    Layer* l = node::ObjectWrap::Unwrap<Layer>(args.Holder());

    Local<Object> description = NanNew<Object>();
    mapnik::layer const& layer = *l->layer_;
    if ( layer.name() != "" )
    {
        description->Set(NanNew("name"), NanNew(layer.name().c_str()));
    }

    if ( layer.srs() != "" )
    {
        description->Set(NanNew("srs"), NanNew(layer.srs().c_str()));
    }

    if ( !layer.active())
    {
        description->Set(NanNew("status"), NanNew<Boolean>(layer.active()));
    }

    if ( layer.clear_label_cache())
    {
        description->Set(NanNew("clear_label_cache"), NanNew<Boolean>(layer.clear_label_cache()));
    }

    if ( layer.min_zoom() > 0)
    {
        description->Set(NanNew("minzoom"), NanNew<Number>(layer.min_zoom()));
    }

    if ( layer.max_zoom() != std::numeric_limits<double>::max() )
    {
        description->Set(NanNew("maxzoom"), NanNew<Number>(layer.max_zoom()));
    }

    if ( layer.queryable())
    {
        description->Set(NanNew("queryable"), NanNew<Boolean>(layer.queryable()));
    }

    std::vector<std::string> const& style_names = layer.styles();
    Local<Array> s = NanNew<Array>(style_names.size());
    for (unsigned i = 0; i < style_names.size(); ++i)
    {
        s->Set(i, NanNew(style_names[i].c_str()) );
    }

    description->Set(NanNew("styles"), s );

    mapnik::datasource_ptr datasource = layer.datasource();
    Local<v8::Object> ds = NanNew<Object>();
    description->Set(NanNew("datasource"), ds );
    if ( datasource )
    {
        mapnik::parameters::const_iterator it = datasource->params().begin();
        mapnik::parameters::const_iterator end = datasource->params().end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object serializer( ds , it->first);
            mapnik::util::apply_visitor( serializer, it->second );
        }
    }

    NanReturnValue(description);
}
