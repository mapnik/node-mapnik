
#include <node_buffer.h>
#include <node_version.h>

// mapnik

// provides MAPNIK_SUPPORTS_GRID_RENDERER
#include <mapnik/config.hpp>

// mapnik renderers
#include <mapnik/agg_renderer.hpp>
#if defined(MAPNIK_SUPPORTS_GRID_RENDERER)
#include <mapnik/grid/grid_renderer.hpp>
#endif
#if defined(HAVE_CAIRO)
#include <mapnik/cairo_renderer.hpp>
#endif

#include <mapnik/version.hpp>
#include <mapnik/map.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/filter_factory.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/config_error.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/save_map.hpp>
#include <mapnik/query.hpp>
#include <mapnik/ctrans.hpp>

// stl
#include <exception>
#include <set>

// boost
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional/optional.hpp>


#include "utils.hpp"
#include "js_grid_utils.hpp"
#include "mapnik_map.hpp"
#include "ds_emitter.hpp"
#include "layer_emitter.hpp"
#include "mapnik_layer.hpp"
#include "mapnik_image.hpp"
#include "mapnik_grid.hpp"
#include "mapnik_palette.hpp"
#include "mapnik_color.hpp"


Persistent<FunctionTemplate> Map::constructor;

void Map::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Map::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Map"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "load", load);
    NODE_SET_PROTOTYPE_METHOD(constructor, "loadSync", loadSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "fromStringSync", fromStringSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "fromString", fromString);
    NODE_SET_PROTOTYPE_METHOD(constructor, "save", save);
    NODE_SET_PROTOTYPE_METHOD(constructor, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(constructor, "toXML", to_string);
    NODE_SET_PROTOTYPE_METHOD(constructor, "resize", resize);


    NODE_SET_PROTOTYPE_METHOD(constructor, "render", render);
    NODE_SET_PROTOTYPE_METHOD(constructor, "renderSync", renderSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "renderFile", renderFile);
    NODE_SET_PROTOTYPE_METHOD(constructor, "renderFileSync", renderFileSync);

    // deprecated
    NODE_SET_PROTOTYPE_METHOD(constructor, "render_grid", render_grid);
    NODE_SET_PROTOTYPE_METHOD(constructor, "renderLayerSync", renderLayerSync);

    NODE_SET_PROTOTYPE_METHOD(constructor, "zoomAll", zoomAll);
    NODE_SET_PROTOTYPE_METHOD(constructor, "zoomToBox", zoomToBox); //setExtent
    NODE_SET_PROTOTYPE_METHOD(constructor, "scaleDenominator", scaleDenominator);

    // layer access
    NODE_SET_PROTOTYPE_METHOD(constructor, "add_layer", add_layer);
    NODE_SET_PROTOTYPE_METHOD(constructor, "get_layer", get_layer);

    // temp hack to expose layer metadata
    NODE_SET_PROTOTYPE_METHOD(constructor, "layers", layers);
    NODE_SET_PROTOTYPE_METHOD(constructor, "features", features);
    NODE_SET_PROTOTYPE_METHOD(constructor, "describe_data", describe_data);

    // properties
    ATTR(constructor, "srs", get_prop, set_prop);
    ATTR(constructor, "width", get_prop, set_prop);
    ATTR(constructor, "height", get_prop, set_prop);
    ATTR(constructor, "bufferSize", get_prop, set_prop);
    ATTR(constructor, "extent", get_prop, set_prop);
    ATTR(constructor, "maximumExtent", get_prop, set_prop);
    ATTR(constructor, "background", get_prop, set_prop);
    ATTR(constructor, "parameters", get_prop, set_prop)

    target->Set(String::NewSymbol("Map"),constructor->GetFunction());
    //eio_set_max_poll_reqs(10);
    //eio_set_min_parallel(10);
}

Map::Map(int width, int height) :
  ObjectWrap(),
  map_(boost::make_shared<mapnik::Map>(width,height)),
  in_use_(0) {}

Map::Map(int width, int height, std::string const& srs) :
  ObjectWrap(),
  map_(boost::make_shared<mapnik::Map>(width,height,srs)),
  in_use_(0) {}

Map::~Map()
{
    // std::clog << "~Map(node)\n";
    // release is handled by boost::shared_ptr
}

void Map::acquire() {
    //std::cerr << "acquiring!!\n";
    ++in_use_;
}

void Map::release() {
    //std::cerr << "releasing!!\n";
    --in_use_;
}

int Map::active() const {
    return in_use_;
}

Handle<Value> Map::New(const Arguments& args)
{
    HandleScope scope;

    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    // accept a reference or v8:External?
    if (args[0]->IsExternal())
    {
        return ThrowException(String::New("No support yet for passing v8:External wrapper around C++ void*"));
    }

    if (args.Length() == 2)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
            return ThrowException(Exception::Error(
               String::New("'width' and 'height' must be a integers")));
        Map* m = new Map(args[0]->IntegerValue(),args[1]->IntegerValue());
        m->Wrap(args.This());
        return args.This();
    }
    else if (args.Length() == 3)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
            return ThrowException(Exception::Error(
               String::New("'width' and 'height' must be a integers")));
        if (!args[2]->IsString())
            return ThrowException(Exception::Error(
               String::New("'srs' value must be a string")));
        Map* m = new Map(args[0]->IntegerValue(),args[1]->IntegerValue(),TOSTR(args[2]));
        m->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
          String::New("please provide Map width and height and optional srs")));
    }
    //return args.This();
    return Undefined();
}

Handle<Value> Map::get_prop(Local<String> property,
                         const AccessorInfo& info)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(info.This());
    std::string a = TOSTR(property);
    if(a == "extent") {
        Local<Array> a = Array::New(4);
        mapnik::box2d<double> const& e = m->map_->get_current_extent();
        a->Set(0, Number::New(e.minx()));
        a->Set(1, Number::New(e.miny()));
        a->Set(2, Number::New(e.maxx()));
        a->Set(3, Number::New(e.maxy()));
        return scope.Close(a);
    }
    else if(a == "maximumExtent") {
        Local<Array> a = Array::New(4);
        boost::optional<mapnik::box2d<double> > const& e = m->map_->maximum_extent();
        if (!e)
            return Undefined();
        a->Set(0, Number::New(e->minx()));
        a->Set(1, Number::New(e->miny()));
        a->Set(2, Number::New(e->maxx()));
        a->Set(3, Number::New(e->maxy()));
        return scope.Close(a);
    }
    else if(a == "width")
        return scope.Close(Integer::New(m->map_->width()));
    else if(a == "height")
        return scope.Close(Integer::New(m->map_->height()));
    else if (a == "srs")
        return scope.Close(String::New(m->map_->srs().c_str()));
    else if(a == "bufferSize")
        return scope.Close(Integer::New(m->map_->buffer_size()));
    else if (a == "background") {
        boost::optional<mapnik::color> c = m->map_->background();
        if (c)
            return scope.Close(Color::New(*c));
        else
            return Undefined();
    }
    else if (a == "parameters") {
        Local<Object> ds = Object::New();
        mapnik::parameters const& params = m->map_->get_extra_parameters();
        mapnik::parameters::const_iterator it = params.begin();
        mapnik::parameters::const_iterator end = params.end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object serializer( ds , it->first);
            boost::apply_visitor( serializer, it->second );
        }
        return scope.Close(ds);
    }
    return Undefined();
}

void Map::set_prop(Local<String> property,
                         Local<Value> value,
                         const AccessorInfo& info)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(info.Holder());
    std::string a = TOSTR(property);
    if(a == "extent" || a == "maximumExtent") {
        if (!value->IsArray()) {
            ThrowException(Exception::Error(
               String::New("Must provide an array of: [minx,miny,maxx,maxy]")));
        } else {
            Local<Array> arr = Local<Array>::Cast(value);
            if (!arr->Length() == 4) {
                ThrowException(Exception::Error(
                   String::New("Must provide an array of: [minx,miny,maxx,maxy]")));
            } else {
                double minx = arr->Get(0)->NumberValue();
                double miny = arr->Get(1)->NumberValue();
                double maxx = arr->Get(2)->NumberValue();
                double maxy = arr->Get(3)->NumberValue();
                mapnik::box2d<double> box(minx,miny,maxx,maxy);
                if(a == "extent")
                    m->map_->zoom_to_box(box);
                else
                    m->map_->set_maximum_extent(box);
            }
        }
    }
    else if (a == "srs")
    {
        if (!value->IsString()) {
            ThrowException(Exception::Error(
               String::New("'srs' must be a string")));
        } else {
            m->map_->set_srs(TOSTR(value));
        }
    }
    else if (a == "bufferSize") {
        if (!value->IsNumber()) {
            ThrowException(Exception::Error(
               String::New("Must provide an integer bufferSize")));
        } else {
            m->map_->set_buffer_size(value->IntegerValue());
        }
    }
    else if (a == "width") {
        if (!value->IsNumber()) {
            ThrowException(Exception::Error(
               String::New("Must provide an integer width")));
        } else {
            m->map_->set_width(value->IntegerValue());
        }
    }
    else if (a == "height") {
        if (!value->IsNumber()) {
            ThrowException(Exception::Error(
               String::New("Must provide an integer height")));
        } else {
            m->map_->set_height(value->IntegerValue());
        }
    }
    else if (a == "background") {
        if (!value->IsObject())
            ThrowException(Exception::TypeError(
              String::New("mapnik.Color expected")));

        Local<Object> obj = value->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !Color::constructor->HasInstance(obj))
            ThrowException(Exception::TypeError(String::New("mapnik.Color expected")));
        Color *c = ObjectWrap::Unwrap<Color>(obj);
        m->map_->set_background(*c->get());
    }
    else if (a == "parameters") {
        if (!value->IsObject())
            ThrowException(Exception::TypeError(
              String::New("object expected for map.parameters")));

        Local<Object> obj = value->ToObject();
        if (obj->IsNull() || obj->IsUndefined())
            ThrowException(Exception::TypeError(String::New("object expected for map.parameters, cannot be null/undefined")));

        mapnik::parameters params;
        Local<Array> names = obj->GetPropertyNames();
        uint32_t i = 0;
        uint32_t a_length = names->Length();
        while (i < a_length) {
            Local<Value> name = names->Get(i)->ToString();
            Local<Value> value = obj->Get(name);
            if (value->IsString()) {
                params[TOSTR(name)] = TOSTR(value);            
            } else if (value->IsNumber()) {
                double num = value->NumberValue();
                // todo - round
                if (num == value->IntegerValue()) {
                    int integer = value->IntegerValue();
                    params[TOSTR(name)] = integer;
                } else {
                    double dub_val = value->NumberValue();
                    params[TOSTR(name)] = dub_val;
                }
            } else {
                std::clog << "unhandled type for property: " << TOSTR(name) << "\n";
            }
            i++;
        }
        m->map_->set_extra_parameters(params);
    }
}

Handle<Value> Map::scaleDenominator(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    return scope.Close(Number::New(m->map_->scale_denominator()));
}

Handle<Value> Map::add_layer(const Arguments &args) {
    HandleScope scope;

    if (!args[0]->IsObject())
      return ThrowException(Exception::TypeError(
        String::New("mapnik.Layer expected")));

    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !Layer::constructor->HasInstance(obj))
      return ThrowException(Exception::TypeError(String::New("mapnik.Layer expected")));
    Layer *l = ObjectWrap::Unwrap<Layer>(obj);
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    // TODO - addLayer should be add_layer in mapnik
    m->map_->addLayer(*l->get());
    return Undefined();
}

Handle<Value> Map::get_layer(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Please provide layer name or index")));

    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::vector<mapnik::layer> & layers = m->map_->layers();

    Local<Value> layer = args[0];
    if (layer->IsNumber())
    {
        unsigned int index = args[0]->IntegerValue();

        if (index < layers.size())
        {
            return scope.Close(Layer::New(layers[index]));
        }
        else
        {
          return ThrowException(Exception::TypeError(
            String::New("invalid layer index")));
        }
    }
    else if (layer->IsString())
    {
        bool found = false;
        unsigned int idx(0);
        std::string const & layer_name = TOSTR(layer);
        BOOST_FOREACH ( mapnik::layer const& lyr, layers )
        {
            if (lyr.name() == layer_name)
            {
                found = true;
                return scope.Close(Layer::New(layers[idx]));
            }
            ++idx;
        }
        if (!found)
        {
            std::ostringstream s;
            s << "Layer name '" << layer_name << "' not found";
            return ThrowException(Exception::TypeError(
              String::New(s.str().c_str())));
        }

    }
    else
    {
        return ThrowException(Exception::TypeError(
           String::New("first argument must be either a layer name(string) or layer index (integer)")));
    }

    return Undefined();

}


Handle<Value> Map::layers(const Arguments& args)
{
    HandleScope scope;

    // todo - optimize by allowing indexing...
    /*if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Please provide layer index")));

    if (!args[0]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("layer index must be an integer")));
    */

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    std::vector<mapnik::layer> const & layers = m->map_->layers();
    Local<Array> a = Array::New(layers.size());

    for (unsigned i = 0; i < layers.size(); ++i )
    {
        const mapnik::layer & layer = layers[i];
        Local<Object> meta = Object::New();
        node_mapnik::layer_as_json(meta,layer);
        a->Set(i, meta);
    }

    return scope.Close(a);

}

Handle<Value> Map::describe_data(const Arguments& args)
{
    HandleScope scope;

    // todo - optimize by allowing indexing...
    /*if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Please provide layer index")));

    if (!args[0]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("layer index must be an integer")));
    */

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    std::vector<mapnik::layer> const & layers = m->map_->layers();

    Local<Object> meta = Object::New();

    for (unsigned i = 0; i < layers.size(); ++i )
    {
        const mapnik::layer & layer = layers[i];
        Local<Object> description = Object::New();
        mapnik::datasource_ptr ds = layer.datasource();
        if (ds)
        {
            node_mapnik::describe_datasource(description,ds);
        }
        meta->Set(String::NewSymbol(layer.name().c_str()), description);
    }

    return scope.Close(meta);

}


Handle<Value> Map::features(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() >= 1)
      return ThrowException(Exception::Error(
        String::New("Please provide layer index")));

    if (!args[0]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("layer index must be an integer")));

    unsigned first = 0;
    unsigned last = 0;

    // we are slicing
    if (args.Length() == 3)
    {
        if (!args[1]->IsNumber() || !args[2]->IsNumber())
            return ThrowException(Exception::Error(
               String::New("Index of 'first' and 'last' feature must be an integer")));
        first = args[1]->IntegerValue();
        last = args[2]->IntegerValue();
    }

    unsigned index = args[0]->IntegerValue();

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    std::vector<mapnik::layer> const & layers = m->map_->layers();

    // TODO - we don't know features.length at this point
    Local<Array> a = Array::New(0);
    if ( index < layers.size())
    {
        mapnik::layer const& layer = layers[index];
        mapnik::datasource_ptr ds = layer.datasource();
        if (ds)
        {
            node_mapnik::datasource_features(a,ds,first,last);
        }
    }

    return scope.Close(a);

}

Handle<Value> Map::clear(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    m->map_->remove_all();
    return Undefined();
}

Handle<Value> Map::resize(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() == 2)
      return ThrowException(Exception::Error(
        String::New("Please provide width and height")));

    if (!args[0]->IsNumber() || !args[1]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("width and height must be integers")));

    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    m->map_->resize(args[0]->IntegerValue(),args[1]->IntegerValue());
    return Undefined();
}


typedef struct {
    uv_work_t request;
    Map *m;
    std::string stylesheet;
    bool strict;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} load_image_baton_t;


Handle<Value> Map::load(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() >= 2)
      return ThrowException(Exception::Error(
        String::New("please provide a stylesheet path, options, and callback")));

    // ensure stylesheet path is a string
    Local<Value> stylesheet = args[0];
    if (!stylesheet->IsString())
        return ThrowException(Exception::TypeError(
           String::New("first argument must be a path to a mapnik stylesheet")));

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                  String::New("last argument must be a callback function")));

    // ensure options object
    if (!args[1]->IsObject())
        return ThrowException(Exception::TypeError(
          String::New("options must be an object, eg {strict: true}")));

    Local<Object> options = args[1]->ToObject();

    bool strict = false;
    Local<String> param = String::New("strict");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsBoolean())
          return ThrowException(Exception::TypeError(
            String::New("'strict' must be a Boolean")));
        strict = param_val->BooleanValue();
    }

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    load_image_baton_t *closure = new load_image_baton_t();
    closure->request.data = closure;
    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Load, EIO_AfterLoad);
    m->Ref();
    //uv_ref(uv_default_loop());
    return Undefined();
}

void Map::EIO_Load(uv_work_t* req)
{
    load_image_baton_t *closure = static_cast<load_image_baton_t *>(req->data);

    try
    {
        mapnik::load_map(*closure->m->map_,closure->stylesheet,closure->strict);
    }
    catch (const std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happened while rendering the map,\n this should not happen, please submit a bug report";
    }
}

void Map::EIO_AfterLoad(uv_work_t* req)
{
    HandleScope scope;

    load_image_baton_t *closure = static_cast<load_image_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->m->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    closure->m->Unref();
    //uv_unref(uv_default_loop());
    closure->cb.Dispose();
    delete closure;
}


Handle<Value> Map::loadSync(const Arguments& args)
{
    HandleScope scope;
    if (!args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a path to a mapnik stylesheet")));

    if (args.Length() != 1)
      return ThrowException(Exception::TypeError(
        String::New("only accepts one argument: a path to a mapnik stylesheet")));

    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::string const& stylesheet = TOSTR(args[0]);
    bool strict = false;
    try
    {
        mapnik::load_map(*m->map_,stylesheet,strict);
    }
    catch (const std::exception & ex)
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (...)
    {
      return ThrowException(Exception::TypeError(
        String::New("something went wrong loading the map")));
    }
    return Undefined();
}

Handle<Value> Map::fromStringSync(const Arguments& args)
{
    HandleScope scope;
    if (!args.Length() >= 1) {
        return ThrowException(Exception::TypeError(
        String::New("Accepts 2 arguments: stylesheet string and an optional options")));
    }

    if (!args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a mapnik stylesheet string")));


    // defaults
    bool strict = false;
    std::string base_path("");

    if (args.Length() >= 2) {
        // ensure options object
        if (!args[1]->IsObject())
            return ThrowException(Exception::TypeError(
              String::New("options must be an object, eg {strict: true, base: \".\"'}")));

        Local<Object> options = args[1]->ToObject();

        Local<String> param = String::New("strict");
        if (options->Has(param))
        {
            Local<Value> param_val = options->Get(param);
            if (!param_val->IsBoolean())
              return ThrowException(Exception::TypeError(
                String::New("'strict' must be a Boolean")));
            strict = param_val->BooleanValue();
        }

        param = String::New("base");
        if (options->Has(param))
        {
            Local<Value> param_val = options->Get(param);
            if (!param_val->IsString())
              return ThrowException(Exception::TypeError(
                String::New("'base' must be a string representing a filesystem path")));
            base_path = TOSTR(param_val);
        }
    }

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    std::string const& stylesheet = TOSTR(args[0]);

    try
    {
        mapnik::load_map_string(*m->map_,stylesheet,strict,base_path);
    }
    catch (const std::exception & ex)
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (...)
    {
      return ThrowException(Exception::TypeError(
        String::New("something went wrong loading the map")));
    }
    return Undefined();
}

typedef struct {
    uv_work_t request;
    Map *m;
    std::string stylesheet;
    std::string base_url;
    bool strict;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} load_string_image_baton_t;


Handle<Value> Map::fromString(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() >= 2)
      return ThrowException(Exception::Error(
        String::New("please provide a stylesheet string, options, and callback")));

    // ensure stylesheet path is a string
    Local<Value> stylesheet = args[0];
    if (!stylesheet->IsString())
        return ThrowException(Exception::TypeError(
           String::New("first argument must be a path to a mapnik stylesheet string")));

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                  String::New("last argument must be a callback function")));

    // ensure options object
    if (!args[1]->IsObject())
        return ThrowException(Exception::TypeError(
          String::New("options must be an object, eg {strict: true, base: \".\"'}")));

    Local<Object> options = args[1]->ToObject();

    bool strict = false;
    Local<String> param = String::New("strict");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsBoolean())
          return ThrowException(Exception::TypeError(
            String::New("'strict' must be a Boolean")));
        strict = param_val->BooleanValue();
    }

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    load_string_image_baton_t *closure = new load_string_image_baton_t();
    closure->request.data = closure;

    param = String::New("base");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsString())
          return ThrowException(Exception::TypeError(
            String::New("'base' must be a string representing a filesystem path")));
        closure->base_url = TOSTR(param_val);
    }

    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromString, EIO_AfterFromString);
    m->Ref();
    //uv_ref(uv_default_loop());
    return Undefined();
}

void Map::EIO_FromString(uv_work_t* req)
{
    load_string_image_baton_t *closure = static_cast<load_string_image_baton_t *>(req->data);

    try
    {
        mapnik::load_map_string(*closure->m->map_,closure->stylesheet,closure->strict,closure->base_url);
    }
    catch (const std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happened while rendering the map,\n this should not happen, please submit a bug report";
    }
}

void Map::EIO_AfterFromString(uv_work_t* req)
{
    HandleScope scope;

    load_string_image_baton_t *closure = static_cast<load_string_image_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->m->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    closure->m->Unref();
    //uv_unref(uv_default_loop());
    closure->cb.Dispose();
    delete closure;
}


Handle<Value> Map::save(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() != 1 || !args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a path to map.xml to save")));

    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::string const& filename = TOSTR(args[0]);
    bool explicit_defaults = false;
    mapnik::save_map(*m->map_,filename,explicit_defaults);
    return Undefined();
}

Handle<Value> Map::to_string(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    bool explicit_defaults = false;
    std::string map_string = mapnik::save_map_to_string(*m->map_,explicit_defaults);
    return scope.Close(String::New(map_string.c_str()));
}

Handle<Value> Map::zoomAll(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    try {
      m->map_->zoom_all();
    }
    catch (const std::exception & ex)
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::TypeError(
          String::New("Unknown exception happened while zooming, please submit a bug report")));
    }
    return Undefined();
}

Handle<Value> Map::zoomToBox(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    double minx;
    double miny;
    double maxx;
    double maxy;

    if (args.Length() == 1)
    {
        if (!args[0]->IsArray())
            return ThrowException(Exception::Error(
               String::New("Must provide an array of: [minx,miny,maxx,maxy]")));
        Local<Array> a = Local<Array>::Cast(args[0]);
        minx = a->Get(0)->NumberValue();
        miny = a->Get(1)->NumberValue();
        maxx = a->Get(2)->NumberValue();
        maxy = a->Get(3)->NumberValue();

    }
    else if (args.Length() != 4)
      return ThrowException(Exception::Error(
        String::New("Must provide 4 arguments: minx,miny,maxx,maxy")));
    else {
        minx = args[0]->NumberValue();
        miny = args[1]->NumberValue();
        maxx = args[2]->NumberValue();
        maxy = args[3]->NumberValue();
    }
    mapnik::box2d<double> box(minx,miny,maxx,maxy);
    m->map_->zoom_to_box(box);
    return Undefined();
}

typedef struct {
    uv_work_t request;
    Map *m;
    Image *im;
    double scale_factor;
    unsigned offset_x;
    unsigned offset_y;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} image_baton_t;

typedef struct {
    uv_work_t request;
    Map *m;
    Grid *g;
    std::size_t layer_idx;
    double scale_factor;
    unsigned offset_x;
    unsigned offset_y;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} grid_baton_t;


Handle<Value> Map::render(const Arguments& args)
{
    HandleScope scope;

    // ensure at least 2 args
    if (!args.Length() >= 2) {
        return ThrowException(Exception::TypeError(
          String::New("requires at least two arguments, a renderable mapnik object, and a callback")));
    }

    // ensure renderable object
    if (!args[0]->IsObject()) {
        return ThrowException(Exception::TypeError(
          String::New("requires a renderable mapnik object to be passed as first argument")));
    }

    // ensure function callback
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
          String::New("last argument must be a callback function")));

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    if (m->active() != 0) {
        std::ostringstream s;
        s << "render: this map appears to be in use by "
          << m->active()
          << " other thread(s) which is not allowed."
          << " You need to use a map pool to avoid sharing map objects between concurrent rendering";
        std::cerr << s.str() << "\n";
    }

    // parse options

    // defaults
    double scale_factor = 1.0;
    unsigned offset_x = 0;
    unsigned offset_y = 0;

    Local<Object> options;

    if (args.Length() > 2) {

        // options object
        if (!args[1]->IsObject())
            return ThrowException(Exception::TypeError(
               String::New("optional second argument must be an options object")));

        options = args[1]->ToObject();

        if (options->Has(String::New("scale"))) {
            Local<Value> bind_opt = options->Get(String::New("scale"));
            if (!bind_opt->IsNumber())
              return ThrowException(Exception::TypeError(
                String::New("optional arg 'scale' must be a number")));

            scale_factor = bind_opt->NumberValue();
        }

        if (options->Has(String::New("offset_x"))) {
            Local<Value> bind_opt = options->Get(String::New("offset_x"));
            if (!bind_opt->IsNumber())
              return ThrowException(Exception::TypeError(
                String::New("optional arg 'offset_x' must be a number")));

            offset_x = bind_opt->IntegerValue();
        }

        if (options->Has(String::New("offset_y"))) {
            Local<Value> bind_opt = options->Get(String::New("offset_y"));
            if (!bind_opt->IsNumber())
              return ThrowException(Exception::TypeError(
                String::New("optional arg 'offset_y' must be a number")));

            offset_y = bind_opt->IntegerValue();
        }
    }

    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined())
        return ThrowException(Exception::TypeError(String::New("first argument is invalid, must be a renderable mapnik object, not null/undefined")));

    if (Image::constructor->HasInstance(obj)) {

        image_baton_t *closure = new image_baton_t();
        closure->request.data = closure;
        closure->m = m;
        closure->im = ObjectWrap::Unwrap<Image>(obj);
        closure->im->_ref();
        closure->scale_factor = scale_factor;
        closure->offset_x = offset_x;
        closure->offset_y = offset_y;
        closure->error = false;
        closure->cb = Persistent<Function>::New(Handle<Function>::Cast(args[args.Length()-1]));
        uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderImage, EIO_AfterRenderImage);

    } else if (Grid::constructor->HasInstance(obj)) {

        Grid * g = ObjectWrap::Unwrap<Grid>(obj);

        std::size_t layer_idx = 0;

        // grid requires special options for now
        if (!options->Has(String::New("layer"))) {
            return ThrowException(Exception::TypeError(
              String::New("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)")));
        } else {

            std::vector<mapnik::layer> const& layers = m->map_->layers();

            Local<Value> layer_id = options->Get(String::New("layer"));
            if (! (layer_id->IsString() || layer_id->IsNumber()) )
                return ThrowException(Exception::TypeError(
                  String::New("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)")));

            if (layer_id->IsString()) {
                bool found = false;
                unsigned int idx(0);
                std::string const & layer_name = TOSTR(layer_id);
                BOOST_FOREACH ( mapnik::layer const& lyr, layers )
                {
                    if (lyr.name() == layer_name)
                    {
                        found = true;
                        layer_idx = idx;
                        break;
                    }
                    ++idx;
                }
                if (!found)
                {
                    std::ostringstream s;
                    s << "Layer name '" << layer_name << "' not found";
                    return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
                }
            } else if (layer_id->IsNumber()) {
                layer_idx = layer_id->IntegerValue();
                std::size_t layer_num = layers.size();

                if (layer_idx >= layer_num) {
                    std::ostringstream s;
                    s << "Zero-based layer index '" << layer_idx << "' not valid, only '"
                      << layers.size() << "' layers are in map";
                    return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
                }
            } else {
                return ThrowException(Exception::TypeError(String::New("layer id must be a string or index number")));
            }
        }

        if (options->Has(String::New("fields"))) {

            Local<Value> param_val = options->Get(String::New("fields"));
            if (!param_val->IsArray())
              return ThrowException(Exception::TypeError(
                String::New("option 'fields' must be an array of strings")));
            Local<Array> a = Local<Array>::Cast(param_val);
            uint32_t i = 0;
            uint32_t num_fields = a->Length();
            while (i < num_fields) {
                Local<Value> name = a->Get(i);
                if (name->IsString()){
                    g->get()->add_property_name(TOSTR(name));
                }
                i++;
            }
        }

        grid_baton_t *closure = new grid_baton_t();
        closure->request.data = closure;
        closure->m = m;
        closure->g = g;
        closure->g->_ref();
        closure->layer_idx = layer_idx;
        closure->scale_factor = scale_factor;
        closure->offset_x = offset_x;
        closure->offset_y = offset_y;
        closure->error = false;
        closure->cb = Persistent<Function>::New(Handle<Function>::Cast(args[args.Length()-1]));
        uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderGrid, EIO_AfterRenderGrid);
    } else {
        return ThrowException(Exception::TypeError(String::New("renderable mapnik object expected")));
    }

    m->acquire();
    m->Ref();
    //uv_unref(uv_default_loop());
    return Undefined();
}

void Map::EIO_RenderGrid(uv_work_t* req)
{

    grid_baton_t *closure = static_cast<grid_baton_t *>(req->data);

    std::vector<mapnik::layer> const& layers = closure->m->map_->layers();

    try
    {
        // copy property names
        std::set<std::string> attributes = closure->g->get()->property_names();

        std::string join_field = closure->g->get()->get_key();
        if (join_field == closure->g->get()->id_name_)
        {
            // TODO - should feature.id() be a first class attribute?
            if (attributes.find(join_field) != attributes.end())
            {
                attributes.erase(join_field);
            }
        }
        else if (attributes.find(join_field) == attributes.end())
        {
            attributes.insert(join_field);
        }

        mapnik::grid_renderer<mapnik::grid> ren(*closure->m->map_,
                *closure->g->get(),
                closure->scale_factor,
                closure->offset_x,
                closure->offset_y);
        mapnik::layer const& layer = layers[closure->layer_idx];
        ren.apply(layer,attributes);

    }
    catch (const std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "Unknown error occured, please file bug";
    }
}


void Map::EIO_AfterRenderGrid(uv_work_t* req)
{
    HandleScope scope;

    grid_baton_t *closure = static_cast<grid_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        // TODO - add more attributes
        // https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/Error
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->g->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    closure->m->release();
    closure->m->Unref();
    //uv_unref(uv_default_loop());
    closure->g->_unref();
    closure->cb.Dispose();
    delete closure;
}

void Map::EIO_RenderImage(uv_work_t* req)
{
    image_baton_t *closure = static_cast<image_baton_t *>(req->data);

    try
    {
        mapnik::agg_renderer<mapnik::image_32> ren(*closure->m->map_,
                *closure->im->get(),
                closure->scale_factor,
                closure->offset_x,
                closure->offset_y);
        ren.apply();
    }
    catch (const std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happened while rendering the map,\n this should not happen, please submit a bug report";
    }
}

void Map::EIO_AfterRenderImage(uv_work_t* req)
{
    HandleScope scope;

    image_baton_t *closure = static_cast<image_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->im->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    closure->m->release();
    closure->m->Unref();
    //uv_unref(uv_default_loop());
    closure->im->_unref();
    closure->cb.Dispose();
    delete closure;
}

typedef struct {
    uv_work_t request;
    Map *m;
    std::string format;
    std::string output;
    palette_ptr palette;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} render_file_baton_t;

Handle<Value> Map::renderFile(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() >= 1 || !args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a path to a file to save")));

    std::string format = "png";
    palette_ptr palette;

    Local<Value> callback = args[args.Length()-1];

    if (!callback->IsFunction())
        return ThrowException(Exception::TypeError(
                    String::New("last argument must be a callback function")));

    if (!args[1]->IsFunction() && args[1]->IsObject()) {
        Local<Object> options = args[1]->ToObject();
        if (options->Has(String::New("format")))
        {
            Local<Value> format_opt = options->Get(String::New("format"));
            if (!format_opt->IsString())
              return ThrowException(Exception::TypeError(
                String::New("'format' must be a String")));

            format = TOSTR(format_opt);
        }

        if (options->Has(String::New("palette")))
        {
            Local<Value> format_opt = options->Get(String::New("palette"));
            if (!format_opt->IsObject())
              return ThrowException(Exception::TypeError(
                String::New("'palette' must be an object")));

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Palette::constructor->HasInstance(obj))
              return ThrowException(Exception::TypeError(String::New("mapnik.Palette expected as second arg")));

            palette = ObjectWrap::Unwrap<Palette>(obj)->palette();
        }

    } else if (!args[1]->IsFunction()) {
        return ThrowException(Exception::TypeError(
                    String::New("optional argument must be an object")));
    }

    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::string const& output = TOSTR(args[0]);

    //maybe do this in the async part?
    if (format.empty()) {
        format = mapnik::guess_type(output);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << output << "\n";
            return ThrowException(Exception::Error(
                String::New(s.str().c_str())));
        }
    }

    if (format == "pdf" || format == "svg" || format == "ps" || format == "ARGB32" || format == "RGB24") {
#if defined(HAVE_CAIRO)
#else
        std::ostringstream s("");
        s << "Cairo backend is not available, cannot write to " << format << "\n";
        return ThrowException(Exception::Error(
          String::New(s.str().c_str())));
#endif
    }

    render_file_baton_t *closure = new render_file_baton_t();
    closure->request.data = closure;

    closure->m = m;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));

    closure->format = format;
    closure->palette = palette;
    closure->output = output;

    uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderFile, EIO_AfterRenderFile);
    //uv_ref(uv_default_loop())
    m->Ref();

    return Undefined();

}

void Map::EIO_RenderFile(uv_work_t* req)
{
    render_file_baton_t *closure = static_cast<render_file_baton_t *>(req->data);

    try
    {
        if(closure->format == "pdf" || closure->format == "svg" || closure->format == "ps" || closure->format == "ARGB32" || closure->format == "RGB24") {
#if defined(HAVE_CAIRO)
            mapnik::save_to_cairo_file(*closure->m->map_,closure->output,closure->format);
#else

#endif
        }
        else
        {
            mapnik::image_32 im(closure->m->map_->width(),closure->m->map_->height());
            // causes hang with node v0.6.0
            //V8::AdjustAmountOfExternalAllocatedMemory(4 * im.width() * im.height());
            mapnik::agg_renderer<mapnik::image_32> ren(*closure->m->map_,im);
            ren.apply();

            if (closure->palette.get()) {
                mapnik::save_to_file<mapnik::image_data_32>(im.data(),closure->output,*closure->palette);
            } else {
                mapnik::save_to_file<mapnik::image_data_32>(im.data(),closure->output);
            }
        }
    }
    catch (const std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happend while rendering image to file,\n this should not happen, please submit a bug report";
    }
}

void Map::EIO_AfterRenderFile(uv_work_t* req)
{
    HandleScope scope;

    render_file_baton_t *closure = static_cast<render_file_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(),1, argv);
    } else {
        Local<Value> argv[1] = { Local<Value>::New(Null()) };
        closure->cb->Call(Context::GetCurrent()->Global(),1, argv);
    }

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    //uv_unref(uv_default_loop());
    closure->m->release();
    closure->m->Unref();
    closure->cb.Dispose();
    delete closure;

}

Handle<Value> Map::renderLayerSync(const Arguments& args)
{
    HandleScope scope;

    if ((!args.Length() >= 1) || (!args[0]->IsObject())) {
      return ThrowException(Exception::TypeError(
        String::New("requires a mapnik.Grid to be passed as first argument")));
    }

    Local<Object> obj = args[0]->ToObject();
    if (args[0]->IsNull() || args[0]->IsUndefined())
      return ThrowException(Exception::TypeError(String::New("mapnik.Grid expected")));

    if (!Grid::constructor->HasInstance(obj))
      return ThrowException(Exception::TypeError(String::New("mapnik.Grid expected")));

    std::size_t layer_idx = 0;

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    std::vector<mapnik::layer> const& layers = m->map_->layers();

    if (args.Length() >= 2) {

        Local<Value> layer_id = args[1];

        if (layer_id->IsString()) {
            bool found = false;
            unsigned int idx(0);
            std::string const & layer_name = TOSTR(layer_id);
            BOOST_FOREACH ( mapnik::layer const& lyr, layers )
            {
                if (lyr.name() == layer_name)
                {
                    found = true;
                    layer_idx = idx;
                    break;
                }
                ++idx;
            }
            if (!found)
            {
                std::ostringstream s;
                s << "Layer name '" << layer_name << "' not found";
                return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
            }
        } else if (layer_id->IsNumber()) {
            layer_idx = layer_id->IntegerValue();
            std::size_t layer_num = layers.size();

            if (layer_idx >= layer_num) {
                std::ostringstream s;
                s << "Zero-based layer index '" << layer_idx << "' not valid, only '"
                  << layers.size() << "' layers are in map";
                return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
            }
        } else {
            return ThrowException(Exception::TypeError(String::New("layer id must be a string or index number")));
        }
    }

    Grid *g = ObjectWrap::Unwrap<Grid>(obj);

    // defaults
    double scale_factor = 1;

    if (args.Length() >= 3) {

        if (!args[2]->IsObject())
            return ThrowException(Exception::TypeError(
              String::New("optional second argument must be an options object")));

        Local<Object> options = args[2]->ToObject();

        if (options->Has(String::New("fields"))) {

            Local<Value> param_val = options->Get(String::New("fields"));
            if (!param_val->IsArray())
              return ThrowException(Exception::TypeError(
                String::New("option 'fields' must be an array of strings")));
            Local<Array> a = Local<Array>::Cast(param_val);
            uint32_t i = 0;
            uint32_t num_fields = a->Length();
            while (i < num_fields) {
                Local<Value> name = a->Get(i);
                if (name->IsString()){
                    g->get()->add_property_name(TOSTR(name));
                }
                i++;
            }

        }

        if (options->Has(String::New("scale"))) {
            Local<Value> bind_opt = options->Get(String::New("scale"));
            if (!bind_opt->IsNumber())
              return ThrowException(Exception::TypeError(
                String::New("optional arg 'scale' must be a number")));

            scale_factor = bind_opt->NumberValue();
        }
    }

    try
    {
        // copy property names
        std::set<std::string> attributes = g->get()->property_names();

        std::string join_field = g->get()->get_key();
        if (join_field == g->get()->id_name_)
        {
            // TODO - should feature.id() be a first class attribute?
            if (attributes.find(join_field) != attributes.end())
            {
                attributes.erase(join_field);
            }
        }
        else if (attributes.find(join_field) == attributes.end())
        {
            attributes.insert(join_field);
        }

        mapnik::grid_renderer<mapnik::grid> ren(*m->map_,*g->get());
        mapnik::layer const& layer = layers[layer_idx];
        ren.apply(layer,attributes);

    }
    catch (const std::exception & ex)
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::TypeError(
          String::New("unknown exception happened while rendering the map, please submit a bug report")));
    }

    return Undefined();
}

Handle<Value> Map::renderSync(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() >= 1 || !args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("argument must be a format string")));

    std::string format = TOSTR(args[0]);
    palette_ptr palette;

    // options hash
    if (args.Length() >= 2) {
        if (!args[1]->IsObject())
          return ThrowException(Exception::TypeError(
            String::New("optional second arg must be an options object")));

        Local<Object> options = args[1]->ToObject();

        if (options->Has(String::New("palette")))
        {
            Local<Value> bind_opt = options->Get(String::New("palette"));
            if (!bind_opt->IsObject())
              return ThrowException(Exception::TypeError(
                  String::New("mapnik.Palette expected as second arg")));

            Local<Object> obj = bind_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Palette::constructor->HasInstance(obj))
              return ThrowException(Exception::TypeError(String::New("mapnik.Palette expected as second arg")));

            palette = ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::string s;
    try
    {
        mapnik::image_32 im(m->map_->width(),m->map_->height());
        V8::AdjustAmountOfExternalAllocatedMemory(4 * im.width() * im.height());
        mapnik::agg_renderer<mapnik::image_32> ren(*m->map_,im);
        ren.apply();

        if (palette.get())
        {
            s = save_to_string(im, format, *palette);
        }
        else {
            s = save_to_string(im, format);
        }
    }
    catch (const std::exception & ex)
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::TypeError(
          String::New("unknown exception happened while rendering the map, please submit a bug report")));
    }

    #if NODE_VERSION_AT_LEAST(0,3,0)
      node::Buffer *retbuf = Buffer::New((char*)s.data(),s.size());
    #else
      node::Buffer *retbuf = Buffer::New(s.size());
      memcpy(retbuf->data(), s.data(), s.size());
    #endif

    return scope.Close(retbuf->handle_);
}

Handle<Value> Map::renderFileSync(const Arguments& args)
{
    HandleScope scope;
    if (!args.Length() >= 1 || !args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a path to a file to save")));

    if (args.Length() > 2)
      return ThrowException(Exception::TypeError(
        String::New("accepts two arguments, a required path to a file, an optional options object, eg. {format: 'pdf'}")));

    std::string format = "png";
    palette_ptr palette;

    if (args.Length() >= 2){
      if (!args[1]->IsObject())
        return ThrowException(Exception::TypeError(
          String::New("second argument is optional, but if provided must be an object, eg. {format: 'pdf'}")));

        Local<Object> options = args[1]->ToObject();
        if (options->Has(String::New("format")))
        {
            Local<Value> format_opt = options->Get(String::New("format"));
            if (!format_opt->IsString())
              return ThrowException(Exception::TypeError(
                String::New("'format' must be a String")));

            format = TOSTR(format_opt);
        }

        if (options->Has(String::New("palette")))
        {
            Local<Value> format_opt = options->Get(String::New("palette"));
            if (!format_opt->IsObject())
              return ThrowException(Exception::TypeError(
                String::New("'palette' must be an object")));

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Palette::constructor->HasInstance(obj))
              return ThrowException(Exception::TypeError(String::New("mapnik.Palette expected as second arg")));

            palette = ObjectWrap::Unwrap<Palette>(obj)->palette();
        }

    }

    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::string const& output = TOSTR(args[0]);

    if (format.empty()) {
        format = mapnik::guess_type(output);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << output << "\n";
            return ThrowException(Exception::Error(
                String::New(s.str().c_str())));
        }
    }

    try
    {

        if (format == "pdf" || format == "svg" || format =="ps" || format == "ARGB32" || format == "RGB24")
        {
    #if defined(HAVE_CAIRO)
            mapnik::save_to_cairo_file(*m->map_,output,format);
    #else
            std::ostringstream s("");
            s << "Cairo backend is not available, cannot write to " << format << "\n";
            return ThrowException(Exception::Error(
              String::New(s.str().c_str())));
    #endif
        }
        else
        {
            mapnik::image_32 im(m->map_->width(),m->map_->height());
            V8::AdjustAmountOfExternalAllocatedMemory(4 * im.width() * im.height());
            mapnik::agg_renderer<mapnik::image_32> ren(*m->map_,im);
            ren.apply();

            if (palette.get())
            {
                mapnik::save_to_file<mapnik::image_data_32>(im.data(),output,*palette);
            }
            else {
                mapnik::save_to_file<mapnik::image_data_32>(im.data(),output);
            }
        }
    }
    catch (const std::exception & ex)
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::TypeError(
          String::New("unknown exception happened while rendering the map, please submit a bug report")));
    }
    return Undefined();
}

struct grid_t {
    uv_work_t request;
    Map *m;
    boost::shared_ptr<mapnik::grid> grid_ptr;
    std::size_t layer_idx;
    std::string layer_name;
    std::string join_field;
    uint32_t num_fields;
    int size;
    bool error;
    std::string error_name;
    bool include_features;
    Persistent<Function> cb;
    bool grid_initialized;
};

Handle<Value> Map::render_grid(const Arguments& args)
{
    HandleScope scope;

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    if (m->active() != 0) {
        std::ostringstream s;
        s << "render_grid: this map appears to be in use by "
          << m->active()
          << " other thread(s) which is not allowed."
          << " You need to use a map pool to avoid sharing map objects between concurrent rendering";
        std::cerr << s.str() << "\n";
        //return ThrowException(Exception::Error(
        //  String::New(s.str().c_str())));
    }

    if (!args.Length() >= 2)
      return ThrowException(Exception::Error(
        String::New("please provide layer name or index, options, and callback")));

    // make sure layer name is a string
    Local<Value> layer = args[0];
    if (! (layer->IsString() || layer->IsNumber()) )
        return ThrowException(Exception::TypeError(
           String::New("first argument must be either a layer name(string) or layer index (integer)")));

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                  String::New("last argument must be a callback function")));

    // ensure options object
    if (!args[1]->IsObject())
        return ThrowException(Exception::TypeError(
          String::New("options must be an object, eg {key: '__id__', resolution : 4, fields: ['name']}")));

    Local<Object> options = args[1]->ToObject();

    std::string join_field("__id__");
    Local<String> param = String::New("key");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsString())
          return ThrowException(Exception::TypeError(
            String::New("'key' must be a string")));
        join_field = TOSTR(param_val);
    }

    unsigned int step(4);
    param = String::New("resolution");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsNumber())
          return ThrowException(Exception::TypeError(
            String::New("'resolution' must be an integer")));
        step = param_val->IntegerValue();
    }

    grid_t *closure = new grid_t();
    closure->request.data = closure;

    if (!closure) {
        V8::LowMemoryNotification();
        return ThrowException(Exception::Error(
            String::New("Could not allocate enough memory")));
    }

    if (layer->IsString()) {
        closure->layer_name = TOSTR(layer);
    } else if (layer->IsNumber()) {
        closure->layer_idx = static_cast<std::size_t>(layer->NumberValue());
    }

    closure->m = m;
    closure->join_field = join_field;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    closure->num_fields = 0;

    unsigned int grid_width = m->map_->width()/step;
    unsigned int grid_height = m->map_->height()/step;

    closure->grid_ptr = boost::make_shared<mapnik::grid>(
                                            grid_width,
                                            grid_height,
                                            closure->join_field,
                                            step);

    param = String::New("fields");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsArray())
          return ThrowException(Exception::TypeError(
            String::New("'fields' must be an array of strings")));
        Local<Array> a = Local<Array>::Cast(param_val);
        uint32_t i = 0;
        closure->num_fields = a->Length();
        while (i < closure->num_fields) {
            Local<Value> name = a->Get(i);
            if (name->IsString()){
                closure->grid_ptr->add_property_name(TOSTR(name));
            }
            i++;
        }
    }

    uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderGrid2, EIO_AfterRenderGrid2);
    m->acquire();
    m->Ref();
    //uv_ref(uv_default_loop());
    return Undefined();

}


void Map::EIO_RenderGrid2(uv_work_t* req)
{

    grid_t *closure = static_cast<grid_t *>(req->data);

    std::vector<mapnik::layer> const& layers = closure->m->map_->layers();

    if (!closure->layer_name.empty()) {
        bool found = false;
        unsigned int idx(0);
        std::string const & layer_name = closure->layer_name;
        BOOST_FOREACH ( mapnik::layer const& lyr, layers )
        {
            if (lyr.name() == layer_name)
            {
                found = true;
                closure->layer_idx = idx;
                break;
            }
            ++idx;
        }
        if (!found)
        {
            std::ostringstream s;
            s << "Layer name '" << layer_name << "' not found";
            closure->error = true;
            closure->error_name = s.str();
            return;
        }
    }
    else
    {
        std::size_t layer_num = layers.size();
        std::size_t layer_idx = closure->layer_idx;

        if (layer_idx >= layer_num) {
            std::ostringstream s;
            s << "Zero-based layer index '" << layer_idx << "' not valid, only '"
              << layers.size() << "' layers are in map";
            closure->error = true;
            closure->error_name = s.str();
            return;
        }    
    }

    // copy property names
    std::set<std::string> attributes = closure->grid_ptr->property_names();

    std::string const& join_field = closure->join_field;

    if (join_field == closure->grid_ptr->id_name_)
    {
        // TODO - should feature.id() be a first class attribute?
        if (attributes.find(join_field) != attributes.end())
        {
            attributes.erase(join_field);
        }
    }
    else if (attributes.find(join_field) == attributes.end())
    {
        attributes.insert(join_field);
    }

    try
    {
        mapnik::grid_renderer<mapnik::grid> ren(*closure->m->map_,*closure->grid_ptr,1.0,0,0);
        mapnik::layer const& layer = layers[closure->layer_idx];
        ren.apply(layer,attributes);
    }
    catch (const std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "Unknown error occured, please file bug";
    }
}


void Map::EIO_AfterRenderGrid2(uv_work_t* req)
{
    HandleScope scope;

    grid_t *closure = static_cast<grid_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        // TODO - add more attributes
        // https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/Error
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        // convert buffer to utf and gather key order
        Local<Array> grid_array = Array::New();
        std::vector<mapnik::grid::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid>(*closure->grid_ptr,grid_array,key_order);

        // convert key order to proper javascript array
        Local<Array> keys_a = Array::New(key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it < key_order.end(); ++it, ++i)
        {
            keys_a->Set(i, String::New((*it).c_str()));
        }

        // gather feature data
        Local<Object> feature_data = Object::New();
        if (closure->num_fields > 0) {
            node_mapnik::write_features<mapnik::grid>(*closure->grid_ptr,
                           feature_data,
                           key_order
                           /*closure->join_field,
                           closure->grid_ptr->property_names()*/);
        }

        // Create the return hash.
        Local<Object> json = Object::New();
        json->Set(String::NewSymbol("grid"), grid_array);
        json->Set(String::NewSymbol("keys"), keys_a);
        json->Set(String::NewSymbol("data"), feature_data);
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(json) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    closure->m->release();
    closure->m->Unref();
    //uv_unref(uv_default_loop());
    closure->cb.Dispose();
    delete closure;
}

