
#include "mapnik_map.hpp"
#include "utils.hpp"
#include "mapnik_color.hpp"             // for Color, Color::constructor
#include "mapnik_featureset.hpp"        // for Featureset
#include "mapnik_grid.hpp"              // for Grid, Grid::constructor
#include "mapnik_image.hpp"             // for Image, Image::constructor
#include "mapnik_layer.hpp"             // for Layer, Layer::constructor
#include "mapnik_palette.hpp"           // for palette_ptr, Palette, etc
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"
#include "mapnik_vector_tile.hpp"

// node
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

// mapnik
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
#include <mapnik/box2d.hpp>             // for box2d
#include <mapnik/color.hpp>             // for color
#include <mapnik/datasource.hpp>        // for featureset_ptr
#include <mapnik/feature_type_style.hpp>  // for rules, feature_type_style
#include <mapnik/graphics.hpp>          // for image_32
#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp>  // for grid_renderer
#include <mapnik/image_data.hpp>        // for image_data_32
#include <mapnik/image_util.hpp>        // for save_to_file, guess_type, etc
#include <mapnik/layer.hpp>             // for layer
#include <mapnik/load_map.hpp>          // for load_map, load_map_string
#include <mapnik/map.hpp>               // for Map, etc
#include <mapnik/params.hpp>            // for parameters
#include <mapnik/rule.hpp>              // for rule, rule::symbolizers, etc
#include <mapnik/save_map.hpp>          // for save_map, etc
#include <mapnik/version.hpp>           // for MAPNIK_VERSION
#include <mapnik/scale_denominator.hpp>

// stl
#include <exception>                    // for exception
#include <iosfwd>                       // for ostringstream, ostream
#include <iostream>                     // for clog
#include <ostream>                      // for operator<<, basic_ostream, etc
#include <sstream>                      // for basic_ostringstream, etc

// boost
#include <boost/foreach.hpp>            // for auto_any_base, etc
#include <boost/optional/optional.hpp>  // for optional
#include "mapnik3x_compatibility.hpp"

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

    NODE_SET_PROTOTYPE_METHOD(constructor, "zoomAll", zoomAll);
    NODE_SET_PROTOTYPE_METHOD(constructor, "zoomToBox", zoomToBox); //setExtent
    NODE_SET_PROTOTYPE_METHOD(constructor, "scale", scale);
    NODE_SET_PROTOTYPE_METHOD(constructor, "scaleDenominator", scaleDenominator);
    NODE_SET_PROTOTYPE_METHOD(constructor, "queryPoint", queryPoint);
    NODE_SET_PROTOTYPE_METHOD(constructor, "queryMapPoint", queryMapPoint);

    // layer access
    NODE_SET_PROTOTYPE_METHOD(constructor, "add_layer", add_layer);
    NODE_SET_PROTOTYPE_METHOD(constructor, "get_layer", get_layer);
    NODE_SET_PROTOTYPE_METHOD(constructor, "layers", layers);

    // properties
    ATTR(constructor, "srs", get_prop, set_prop);
    ATTR(constructor, "width", get_prop, set_prop);
    ATTR(constructor, "height", get_prop, set_prop);
    ATTR(constructor, "bufferSize", get_prop, set_prop);
    ATTR(constructor, "extent", get_prop, set_prop);
    ATTR(constructor, "bufferedExtent", get_prop, set_prop);
    ATTR(constructor, "maximumExtent", get_prop, set_prop);
    ATTR(constructor, "background", get_prop, set_prop);
    ATTR(constructor, "parameters", get_prop, set_prop);

    target->Set(String::NewSymbol("Map"),constructor->GetFunction());
}

Map::Map(int width, int height) :
    ObjectWrap(),
    map_(MAPNIK_MAKE_SHARED<mapnik::Map>(width,height)),
    in_use_(0) {}

Map::Map(int width, int height, std::string const& srs) :
    ObjectWrap(),
    map_(MAPNIK_MAKE_SHARED<mapnik::Map>(width,height,srs)),
    in_use_(0) {}

Map::~Map() { }

void Map::acquire() {
    ++in_use_;
}

void Map::release() {
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
    return Undefined();
}

Handle<Value> Map::get_prop(Local<String> property,
                            const AccessorInfo& info)
{
    HandleScope scope;
    Map* m = node::ObjectWrap::Unwrap<Map>(info.This());
    std::string a = TOSTR(property);
    if(a == "extent") {
        Local<Array> arr = Array::New(4);
        mapnik::box2d<double> const& e = m->map_->get_current_extent();
        arr->Set(0, Number::New(e.minx()));
        arr->Set(1, Number::New(e.miny()));
        arr->Set(2, Number::New(e.maxx()));
        arr->Set(3, Number::New(e.maxy()));
        return scope.Close(arr);
    }
    else if(a == "bufferedExtent") {
        boost::optional<mapnik::box2d<double> > const& e = m->map_->get_buffered_extent();
        if (!e)
            return Undefined();
        Local<Array> arr = Array::New(4);
        arr->Set(0, Number::New(e->minx()));
        arr->Set(1, Number::New(e->miny()));
        arr->Set(2, Number::New(e->maxx()));
        arr->Set(3, Number::New(e->maxy()));
        return scope.Close(arr);
    }
    else if(a == "maximumExtent") {
        boost::optional<mapnik::box2d<double> > const& e = m->map_->maximum_extent();
        if (!e)
            return Undefined();
        Local<Array> arr = Array::New(4);
        arr->Set(0, Number::New(e->minx()));
        arr->Set(1, Number::New(e->miny()));
        arr->Set(2, Number::New(e->maxx()));
        arr->Set(3, Number::New(e->maxy()));
        return scope.Close(arr);
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
#if MAPNIK_VERSION >= 200100
        mapnik::parameters const& params = m->map_->get_extra_parameters();
        mapnik::parameters::const_iterator it = params.begin();
        mapnik::parameters::const_iterator end = params.end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object serializer( ds , it->first);
            boost::apply_visitor( serializer, it->second );
        }
#endif
        return scope.Close(ds);
    }
    return Undefined();
}

void Map::set_prop(Local<String> property,
                   Local<Value> value,
                   const AccessorInfo& info)
{
    HandleScope scope;
    Map* m = node::ObjectWrap::Unwrap<Map>(info.Holder());
    std::string a = TOSTR(property);
    if(a == "extent" || a == "maximumExtent") {
        if (!value->IsArray()) {
            ThrowException(Exception::Error(
                               String::New("Must provide an array of: [minx,miny,maxx,maxy]")));
        } else {
            Local<Array> arr = Local<Array>::Cast(value);
            if (arr->Length() != 4) {
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
        Color *c = node::ObjectWrap::Unwrap<Color>(obj);
        m->map_->set_background(*c->get());
    }
    else if (a == "parameters") {
#if MAPNIK_VERSION >= 200100
        if (!value->IsObject())
            ThrowException(Exception::TypeError(
                               String::New("object expected for map.parameters")));

        Local<Object> obj = value->ToObject();
        if (obj->IsNull() || obj->IsUndefined())
            ThrowException(Exception::TypeError(String::New("object expected for map.parameters, cannot be null/undefined")));

        mapnik::parameters params;
        Local<Array> names = obj->GetPropertyNames();
        unsigned int i = 0;
        unsigned int a_length = names->Length();
        while (i < a_length) {
            Local<Value> name = names->Get(i)->ToString();
            Local<Value> a_value = obj->Get(name);
            if (a_value->IsString()) {
                params[TOSTR(name)] = TOSTR(a_value);
            } else if (a_value->IsNumber()) {
                double num = a_value->NumberValue();
                // todo - round
                if (num == a_value->IntegerValue()) {
                    params[TOSTR(name)] = static_cast<node_mapnik::value_integer>(a_value->IntegerValue());
                } else {
                    double dub_val = a_value->NumberValue();
                    params[TOSTR(name)] = dub_val;
                }
            } else {
                std::clog << "unhandled type for property: " << TOSTR(name) << "\n";
            }
            i++;
        }
        m->map_->set_extra_parameters(params);
#endif
    }
}

Handle<Value> Map::scale(const Arguments& args)
{
    HandleScope scope;
    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    return scope.Close(Number::New(m->map_->scale()));
}

Handle<Value> Map::scaleDenominator(const Arguments& args)
{
    HandleScope scope;
    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    return scope.Close(Number::New(m->map_->scale_denominator()));
}

typedef struct {
    uv_work_t request;
    Map *m;
    std::map<std::string,mapnik::featureset_ptr> featuresets;
    int layer_idx;
    bool geo_coords;
    double x;
    double y;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} query_map_baton_t;


Handle<Value> Map::queryMapPoint(const Arguments& args)
{
    HandleScope scope;
    abstractQueryPoint(args,false);
    return Undefined();
}

Handle<Value> Map::queryPoint(const Arguments& args)
{
    HandleScope scope;
    abstractQueryPoint(args,true);
    return Undefined();
}

Handle<Value> Map::abstractQueryPoint(const Arguments& args, bool geo_coords)
{
    HandleScope scope;
    if (args.Length() < 3)
    {
        return ThrowException(Exception::TypeError(
                                  String::New("requires at least three arguments, a x,y query and a callback")));
    }

    double x,y;
    if (!args[0]->IsNumber() || !args[1]->IsNumber())
    {
        return ThrowException(Exception::TypeError(
                                  String::New("x,y arguments must be numbers")));
    }
    else
    {
        x = args[0]->NumberValue();
        y = args[1]->NumberValue();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());

    Local<Object> options = Object::New();
    int layer_idx = -1;

    if (args.Length() > 3)
    {
        // options object
        if (!args[2]->IsObject())
            return ThrowException(Exception::TypeError(
                                      String::New("optional third argument must be an options object")));

        options = args[2]->ToObject();

        if (options->Has(String::New("layer")))
        {
            std::vector<mapnik::layer> const& layers = m->map_->layers();
            Local<Value> layer_id = options->Get(String::New("layer"));
            if (! (layer_id->IsString() || layer_id->IsNumber()) )
                return ThrowException(Exception::TypeError(
                                          String::New("'layer' option required for map query and must be either a layer name(string) or layer index (integer)")));

            if (layer_id->IsString()) {
                bool found = false;
                unsigned int idx(0);
                std::string layer_name = TOSTR(layer_id);
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
            }
            else if (layer_id->IsNumber())
            {
                layer_idx = layer_id->IntegerValue();
                std::size_t layer_num = layers.size();

                if (layer_idx < 0) {
                    std::ostringstream s;
                    s << "Zero-based layer index '" << layer_idx << "' not valid"
                      << " must be a positive integer";
                    if (layer_num > 0)
                    {
                        s << "only '" << layer_num << "' layers exist in map";
                    }
                    else
                    {
                        s << "no layers found in map";
                    }
                    return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
                } else if (layer_idx >= static_cast<int>(layer_num)) {
                    std::ostringstream s;
                    s << "Zero-based layer index '" << layer_idx << "' not valid, ";
                    if (layer_num > 0)
                    {
                        s << "only '" << layer_num << "' layers exist in map";
                    }
                    else
                    {
                        s << "no layers found in map";
                    }
                    return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
                }
            } else {
                return ThrowException(Exception::TypeError(String::New("layer id must be a string or index number")));
            }
        }
    }

    // ensure function callback
    Local<Value> callback = args[args.Length()-1];
    if (!callback->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    query_map_baton_t *closure = new query_map_baton_t();
    closure->request.data = closure;
    closure->m = m;
    closure->x = x;
    closure->y = y;
    closure->layer_idx = static_cast<std::size_t>(layer_idx);
    closure->geo_coords = geo_coords;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_QueryMap, (uv_after_work_cb)EIO_AfterQueryMap);
    m->Ref();
    return Undefined();
}

void Map::EIO_QueryMap(uv_work_t* req)
{
    query_map_baton_t *closure = static_cast<query_map_baton_t *>(req->data);

    try
    {
        std::vector<mapnik::layer> const& layers = closure->m->map_->layers();
        if (closure->layer_idx >= 0)
        {
            mapnik::featureset_ptr fs;
            if (closure->geo_coords)
            {
                fs = closure->m->map_->query_point(closure->layer_idx,
                                                   closure->x,
                                                   closure->y);
            }
            else
            {
                fs = closure->m->map_->query_map_point(closure->layer_idx,
                                                       closure->x,
                                                       closure->y);
            }
            mapnik::layer const& lyr = layers[closure->layer_idx];
            closure->featuresets.insert(std::make_pair(lyr.name(),fs));
        }
        else
        {
            // query all layers
            unsigned idx = 0;
            BOOST_FOREACH ( mapnik::layer const& lyr, layers )
            {
                mapnik::featureset_ptr fs;
                if (closure->geo_coords)
                {
                    fs = closure->m->map_->query_point(idx,
                                                       closure->x,
                                                       closure->y);
                }
                else
                {
                    fs = closure->m->map_->query_map_point(idx,
                                                           closure->x,
                                                           closure->y);
                }
                closure->featuresets.insert(std::make_pair(lyr.name(),fs));
                ++idx;
            }
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterQueryMap(uv_work_t* req)
{
    HandleScope scope;

    query_map_baton_t *closure = static_cast<query_map_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        std::size_t num_result = closure->featuresets.size();
        if (num_result >= 1)
        {
            Local<Array> a = Array::New(num_result);
            typedef std::map<std::string,mapnik::featureset_ptr> fs_itr;
            fs_itr::const_iterator it = closure->featuresets.begin();
            fs_itr::const_iterator end = closure->featuresets.end();
            unsigned idx = 0;
            for (; it != end; ++it)
            {
                Local<Object> obj = Object::New();
                obj->Set(String::NewSymbol("layer"), String::New(it->first.c_str()));
                obj->Set(String::NewSymbol("featureset"), Featureset::New(it->second));
                a->Set(idx, obj);
                ++idx;
            }
            closure->featuresets.clear();
            Local<Value> argv[2] = { Local<Value>::New(Null()), a };
            closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        }
        else
        {
            Local<Value> argv[2] = { Local<Value>::New(Null()),
                                     Local<Value>::New(Undefined()) };
            closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        }
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->m->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> Map::layers(const Arguments& args)
{
    HandleScope scope;
    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    std::vector<mapnik::layer> const& layers = m->map_->layers();
    Local<Array> a = Array::New(layers.size());
    for (unsigned i = 0; i < layers.size(); ++i )
    {
        a->Set(i, Layer::New(layers[i]));
    }
    return scope.Close(a);
}

Handle<Value> Map::add_layer(const Arguments &args) {
    HandleScope scope;

    if (!args[0]->IsObject())
        return ThrowException(Exception::TypeError(
                                  String::New("mapnik.Layer expected")));

    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !Layer::constructor->HasInstance(obj))
        return ThrowException(Exception::TypeError(String::New("mapnik.Layer expected")));
    Layer *l = node::ObjectWrap::Unwrap<Layer>(obj);
    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    m->map_->MAPNIK_ADD_LAYER(*l->get());
    return Undefined();
}

Handle<Value> Map::get_layer(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() == 1)
        return ThrowException(Exception::Error(
                                  String::New("Please provide layer name or index")));

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    std::vector<mapnik::layer> const& layers = m->map_->layers();

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
        std::string layer_name = TOSTR(layer);
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

Handle<Value> Map::clear(const Arguments& args)
{
    HandleScope scope;
    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    m->map_->remove_all();
    return Undefined();
}

Handle<Value> Map::resize(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() != 2)
        return ThrowException(Exception::Error(
                                  String::New("Please provide width and height")));

    if (!args[0]->IsNumber() || !args[1]->IsNumber())
        return ThrowException(Exception::TypeError(
                                  String::New("width and height must be integers")));

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    m->map_->resize(args[0]->IntegerValue(),args[1]->IntegerValue());
    return Undefined();
}


typedef struct {
    uv_work_t request;
    Map *m;
    std::string stylesheet;
    std::string base_path;
    bool strict;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} load_xml_baton_t;


Handle<Value> Map::load(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 2)
        return ThrowException(Exception::Error(
                                  String::New("please provide a stylesheet path, options, and callback")));

    // ensure stylesheet path is a string
    Local<Value> stylesheet = args[0];
    if (!stylesheet->IsString())
        return ThrowException(Exception::TypeError(
                                  String::New("first argument must be a path to a mapnik stylesheet")));

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!callback->IsFunction())
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

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());

    load_xml_baton_t *closure = new load_xml_baton_t();
    closure->request.data = closure;

    param = String::New("base");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("'base' must be a string representing a filesystem path")));
        closure->base_path = TOSTR(param_val);
    }

    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Load, (uv_after_work_cb)EIO_AfterLoad);
    m->Ref();
    return Undefined();
}

void Map::EIO_Load(uv_work_t* req)
{
    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);

    try
    {
#if MAPNIK_VERSION >= 200200
        mapnik::load_map(*closure->m->map_,closure->stylesheet,closure->strict,closure->base_path);
#else
        mapnik::load_map(*closure->m->map_,closure->stylesheet,closure->strict);
#endif

    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterLoad(uv_work_t* req)
{
    HandleScope scope;

    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->m->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->m->Unref();
    closure->cb.Dispose();
    delete closure;
}


Handle<Value> Map::loadSync(const Arguments& args)
{
    HandleScope scope;
    if (!args[0]->IsString())
        return ThrowException(Exception::TypeError(
                                  String::New("first argument must be a path to a mapnik stylesheet")));

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    std::string stylesheet = TOSTR(args[0]);
    bool strict = false;
    std::string base_path;

    if (args.Length() > 2)
    {

        return ThrowException(Exception::TypeError(
                                  String::New("only accepts two arguments: a path to a mapnik stylesheet and an optional options object")));
    }
    else if (args.Length() == 2)
    {
        // ensure options object
        if (!args[1]->IsObject())
            return ThrowException(Exception::TypeError(
                                      String::New("options must be an object, eg {strict: true}")));

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

    try
    {
#if MAPNIK_VERSION >= 200200
        mapnik::load_map(*m->map_,stylesheet,strict,base_path);
#else
        mapnik::load_map(*m->map_,stylesheet,strict);
#endif
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    return Undefined();
}

Handle<Value> Map::fromStringSync(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() < 1) {
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

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());

    std::string stylesheet = TOSTR(args[0]);

    try
    {
        mapnik::load_map_string(*m->map_,stylesheet,strict,base_path);
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    return Undefined();
}

Handle<Value> Map::fromString(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 2)
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

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());

    load_xml_baton_t *closure = new load_xml_baton_t();
    closure->request.data = closure;

    param = String::New("base");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("'base' must be a string representing a filesystem path")));
        closure->base_path = TOSTR(param_val);
    }

    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromString, (uv_after_work_cb)EIO_AfterFromString);
    m->Ref();
    return Undefined();
}

void Map::EIO_FromString(uv_work_t* req)
{
    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);

    try
    {
        mapnik::load_map_string(*closure->m->map_,closure->stylesheet,closure->strict,closure->base_path);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterFromString(uv_work_t* req)
{
    HandleScope scope;

    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->m->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->m->Unref();
    closure->cb.Dispose();
    delete closure;
}


Handle<Value> Map::save(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() != 1 || !args[0]->IsString())
        return ThrowException(Exception::TypeError(
                                  String::New("first argument must be a path to map.xml to save")));

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    std::string filename = TOSTR(args[0]);
    bool explicit_defaults = false;
    mapnik::save_map(*m->map_,filename,explicit_defaults);
    return Undefined();
}

Handle<Value> Map::to_string(const Arguments& args)
{
    HandleScope scope;
    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    bool explicit_defaults = false;
    std::string map_string = mapnik::save_map_to_string(*m->map_,explicit_defaults);
    return scope.Close(String::New(map_string.c_str()));
}

Handle<Value> Map::zoomAll(const Arguments& args)
{
    HandleScope scope;
    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    try
    {
        m->map_->zoom_all();
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    return Undefined();
}

Handle<Value> Map::zoomToBox(const Arguments& args)
{
    HandleScope scope;
    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());

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

struct image_baton_t {
    uv_work_t request;
    Map *m;
    Image *im;
    int buffer_size; // TODO - no effect until mapnik::request is used
    double scale_factor;
    double scale_denominator;
    unsigned offset_x;
    unsigned offset_y;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    image_baton_t() :
      buffer_size(0),
      scale_factor(1.0),
      scale_denominator(0.0),
      offset_x(0),
      offset_y(0),
      error(false),
      error_name() {}
};

struct grid_baton_t {
    uv_work_t request;
    Map *m;
    Grid *g;
    std::size_t layer_idx;
    int buffer_size; // TODO - no effect until mapnik::request is used
    double scale_factor;
    double scale_denominator;
    unsigned offset_x;
    unsigned offset_y;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    grid_baton_t() :
      layer_idx(-1),
      buffer_size(0),
      scale_factor(1.0),
      scale_denominator(0.0),
      offset_x(0),
      offset_y(0),
      error(false),
      error_name() {}
};

struct vector_tile_baton_t {
    uv_work_t request;
    Map *m;
    VectorTile *d;
    unsigned tolerance;
    unsigned path_multiplier;
    int buffer_size;
    double scale_factor;
    double scale_denominator;
    unsigned offset_x;
    unsigned offset_y;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    vector_tile_baton_t() :
        tolerance(1),
        path_multiplier(16),
        scale_factor(1.0),
        scale_denominator(0.0),
        offset_x(0),
        offset_y(0),
        error(false) {}
};

Handle<Value> Map::render(const Arguments& args)
{
    HandleScope scope;

    // ensure at least 2 args
    if (args.Length() < 2) {
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

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());

    if (m->active() != 0) {
        std::ostringstream s;
        s << "render: this map appears to be in use by "
          << m->active()
          << " other thread(s) which is not allowed."
          << " You need to use a map pool to avoid sharing map objects between concurrent rendering";
        std::clog << s.str() << "\n";
    }

    // parse options

    // defaults
    int buffer_size = 0;
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    unsigned offset_x = 0;
    unsigned offset_y = 0;

    Local<Object> options = Object::New();

    if (args.Length() > 2) {

        // options object
        if (!args[1]->IsObject())
            return ThrowException(Exception::TypeError(
                                      String::New("optional second argument must be an options object")));

        options = args[1]->ToObject();

        if (options->Has(String::New("buffer_size"))) {
            Local<Value> bind_opt = options->Get(String::New("buffer_size"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'buffer_size' must be a number")));

            buffer_size = bind_opt->IntegerValue();
        }

        if (options->Has(String::New("scale"))) {
            Local<Value> bind_opt = options->Get(String::New("scale"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale' must be a number")));

            scale_factor = bind_opt->NumberValue();
        }

        if (options->Has(String::New("scale_denominator"))) {
            Local<Value> bind_opt = options->Get(String::New("scale_denominator"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale_denominator' must be a number")));

            scale_denominator = bind_opt->NumberValue();
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
        closure->im = node::ObjectWrap::Unwrap<Image>(obj);
        closure->im->_ref();
        closure->buffer_size = buffer_size;
        closure->scale_factor = scale_factor;
        closure->scale_denominator = scale_denominator;
        closure->offset_x = offset_x;
        closure->offset_y = offset_y;
        closure->error = false;
        closure->cb = Persistent<Function>::New(Handle<Function>::Cast(args[args.Length()-1]));
        uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderImage, (uv_after_work_cb)EIO_AfterRenderImage);

    } else if (Grid::constructor->HasInstance(obj)) {

        Grid * g = node::ObjectWrap::Unwrap<Grid>(obj);

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
                    s << "Zero-based layer index '" << layer_idx << "' not valid, ";
                    if (layer_num > 0)
                    {
                        s << "only '" << layer_num << "' layers exist in map";
                    }
                    else
                    {
                        s << "no layers found in map";
                    }
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
            unsigned int i = 0;
            unsigned int num_fields = a->Length();
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
        closure->buffer_size = buffer_size;
        closure->scale_factor = scale_factor;
        closure->scale_denominator = scale_denominator;
        closure->offset_x = offset_x;
        closure->offset_y = offset_y;
        closure->error = false;
        closure->cb = Persistent<Function>::New(Handle<Function>::Cast(args[args.Length()-1]));
        uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderGrid, (uv_after_work_cb)EIO_AfterRenderGrid);
    } else if (VectorTile::constructor->HasInstance(obj)) {

        vector_tile_baton_t *closure = new vector_tile_baton_t();
        VectorTile * vector_tile_obj = node::ObjectWrap::Unwrap<VectorTile>(obj);

        if (options->Has(String::New("tolerance"))) {

            Local<Value> param_val = options->Get(String::New("tolerance"));
            if (!param_val->IsNumber()) {
                delete closure;
                return ThrowException(Exception::TypeError(
                                          String::New("option 'tolerance' must be an unsigned integer")));
            }
            closure->tolerance = param_val->IntegerValue();
        }

        if (options->Has(String::New("path_multiplier"))) {
            Local<Value> param_val = options->Get(String::New("path_multiplier"));
            if (!param_val->IsNumber()) {
                delete closure;
                return ThrowException(Exception::TypeError(
                                          String::New("option 'path_multiplier' must be an unsigned integer")));
            }
            closure->path_multiplier = param_val->NumberValue();
        }

        closure->request.data = closure;
        closure->m = m;
        closure->d = vector_tile_obj;
        closure->d->_ref();
        closure->buffer_size = buffer_size;
        closure->scale_factor = scale_factor;
        closure->scale_denominator = scale_denominator;
        closure->offset_x = offset_x;
        closure->offset_y = offset_y;
        closure->error = false;
        closure->cb = Persistent<Function>::New(Handle<Function>::Cast(args[args.Length()-1]));
        uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderVectorTile, (uv_after_work_cb)EIO_AfterRenderVectorTile);
    } else {
        return ThrowException(Exception::TypeError(String::New("renderable mapnik object expected")));
    }

    m->acquire();
    m->Ref();
    return Undefined();
}

void Map::EIO_RenderVectorTile(uv_work_t* req)
{
    vector_tile_baton_t *closure = static_cast<vector_tile_baton_t *>(req->data);
    try
    {
        typedef mapnik::vector::backend_pbf backend_type;
        typedef mapnik::vector::processor<backend_type> renderer_type;
        backend_type backend(closure->d->get_tile_nonconst(),
                             closure->path_multiplier);
        mapnik::Map const& map = *closure->m->get();
        mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
        m_req.set_buffer_size(closure->buffer_size);
        renderer_type ren(backend,
                          map,
                          m_req,
                          closure->scale_factor,
                          closure->offset_x,
                          closure->offset_y,
                          closure->tolerance);
        ren.apply(closure->scale_denominator);
        closure->d->painted(ren.painted());

    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterRenderVectorTile(uv_work_t* req)
{
    HandleScope scope;

    vector_tile_baton_t *closure = static_cast<vector_tile_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->d->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->m->release();
    closure->m->Unref();
    closure->d->_unref();
    closure->cb.Dispose();
    delete closure;
}

void Map::EIO_RenderGrid(uv_work_t* req)
{

    grid_baton_t *closure = static_cast<grid_baton_t *>(req->data);

    std::vector<mapnik::layer> const& layers = closure->m->map_->layers();

    try
    {
        // copy property names
        std::set<std::string> attributes = closure->g->get()->property_names();

        // todo - make this a static constant
        std::string known_id_key = "__id__";
        if (attributes.find(known_id_key) != attributes.end())
        {
            attributes.erase(known_id_key);
        }

        std::string join_field = closure->g->get()->get_key();
        if (known_id_key != join_field &&
            attributes.find(join_field) == attributes.end())
        {
            attributes.insert(join_field);
        }

        mapnik::grid_renderer<mapnik::grid> ren(*closure->m->map_,
                                                *closure->g->get(),
                                                closure->scale_factor,
                                                closure->offset_x,
                                                closure->offset_y);
        mapnik::layer const& layer = layers[closure->layer_idx];
        ren.apply(layer,attributes,closure->scale_denominator);

    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
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
        node::FatalException(try_catch);
    }

    closure->m->release();
    closure->m->Unref();
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
        ren.apply(closure->scale_denominator);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
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
        node::FatalException(try_catch);
    }

    closure->m->release();
    closure->m->Unref();
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
    double scale_factor;
    double scale_denominator;
    bool use_cairo;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} render_file_baton_t;

Handle<Value> Map::renderFile(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 1 || !args[0]->IsString())
        return ThrowException(Exception::TypeError(
                                  String::New("first argument must be a path to a file to save")));

    // defaults
    std::string format = "png";
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
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

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
        if (options->Has(String::New("scale"))) {
            Local<Value> bind_opt = options->Get(String::New("scale"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale' must be a number")));

            scale_factor = bind_opt->NumberValue();
        }

        if (options->Has(String::New("scale_denominator"))) {
            Local<Value> bind_opt = options->Get(String::New("scale_denominator"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale_denominator' must be a number")));

            scale_denominator = bind_opt->NumberValue();
        }

    } else if (!args[1]->IsFunction()) {
        return ThrowException(Exception::TypeError(
                                  String::New("optional argument must be an object")));
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    std::string output = TOSTR(args[0]);

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

    render_file_baton_t *closure = new render_file_baton_t();

    if (format == "pdf" || format == "svg" || format == "ps" || format == "ARGB32" || format == "RGB24") {
#if defined(HAVE_CAIRO)
        closure->use_cairo = true;
#else
        delete closure;
        std::ostringstream s("");
        s << "Cairo backend is not available, cannot write to " << format << "\n";
        return ThrowException(Exception::Error(
                                  String::New(s.str().c_str())));
#endif
    } else {
        closure->use_cairo = false;
    }

    closure->request.data = closure;

    closure->m = m;
    closure->scale_factor = scale_factor;
    closure->scale_denominator = scale_denominator;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));

    closure->format = format;
    closure->palette = palette;
    closure->output = output;

    uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderFile, (uv_after_work_cb)EIO_AfterRenderFile);
    m->Ref();

    return Undefined();

}

void Map::EIO_RenderFile(uv_work_t* req)
{
    render_file_baton_t *closure = static_cast<render_file_baton_t *>(req->data);

    try
    {
        if(closure->use_cairo)
        {
#if defined(HAVE_CAIRO)
#if MAPNIK_VERSION > 200200
            // https://github.com/mapnik/mapnik/issues/1930
            mapnik::save_to_cairo_file(*closure->m->map_,closure->output,closure->format,closure->scale_factor,closure->scale_denominator);
#else
#if MAPNIK_VERSION >= 200100
            mapnik::save_to_cairo_file(*closure->m->map_,closure->output,closure->format,closure->scale_factor);
#else
            mapnik::save_to_cairo_file(*closure->m->map_,closure->output,closure->format);
#endif
#endif
#else
#endif
        }
        else
        {
            mapnik::image_32 im(closure->m->map_->width(),closure->m->map_->height());
            mapnik::agg_renderer<mapnik::image_32> ren(*closure->m->map_,im,closure->scale_factor);
            ren.apply(closure->scale_denominator);

            if (closure->palette.get()) {
                mapnik::save_to_file<mapnik::image_data_32>(im.data(),closure->output,*closure->palette);
            } else {
                mapnik::save_to_file<mapnik::image_data_32>(im.data(),closure->output);
            }
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
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
        node::FatalException(try_catch);
    }

    closure->m->release();
    closure->m->Unref();
    closure->cb.Dispose();
    delete closure;

}

// TODO - add support for grids
Handle<Value> Map::renderSync(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() < 1 || !args[0]->IsString())
        return ThrowException(Exception::TypeError(
                                  String::New("argument must be a format string")));

    std::string format = TOSTR(args[0]);
    palette_ptr palette;
    double scale_factor = 1.0;
    double scale_denominator = 0.0;

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

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
        if (options->Has(String::New("scale"))) {
            Local<Value> bind_opt = options->Get(String::New("scale"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale' must be a number")));

            scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(String::New("scale_denominator"))) {
            Local<Value> bind_opt = options->Get(String::New("scale_denominator"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale_denominator' must be a number")));

            scale_denominator = bind_opt->NumberValue();
        }
    }

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

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    std::string s;
    try
    {
        mapnik::image_32 im(m->map_->width(),m->map_->height());
        mapnik::agg_renderer<mapnik::image_32> ren(*m->map_,im,scale_factor);
        ren.apply(scale_denominator);

        if (palette.get())
        {
            s = save_to_string(im, format, *palette);
        }
        else {
            s = save_to_string(im, format);
        }
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    #if NODE_VERSION_AT_LEAST(0, 11, 0)
    return scope.Close(node::Buffer::New((char*)s.data(),s.size()));
    #else
    return scope.Close(node::Buffer::New((char*)s.data(),s.size())->handle_);
    #endif
}

Handle<Value> Map::renderFileSync(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() < 1 || !args[0]->IsString())
        return ThrowException(Exception::TypeError(
                                  String::New("first argument must be a path to a file to save")));

    if (args.Length() > 2)
        return ThrowException(Exception::TypeError(
                                  String::New("accepts two arguments, a required path to a file, an optional options object, eg. {format: 'pdf'}")));

    // defaults
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
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

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
        if (options->Has(String::New("scale"))) {
            Local<Value> bind_opt = options->Get(String::New("scale"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale' must be a number")));

            scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(String::New("scale_denominator"))) {
            Local<Value> bind_opt = options->Get(String::New("scale_denominator"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale_denominator' must be a number")));

            scale_denominator = bind_opt->NumberValue();
        }
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.This());
    std::string output = TOSTR(args[0]);

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
#if MAPNIK_VERSION > 200200
            mapnik::save_to_cairo_file(*m->map_,output,format,scale_factor,scale_denominator);
#else
#if MAPNIK_VERSION >= 200100
            mapnik::save_to_cairo_file(*m->map_,output,format,scale_factor);
#else
            mapnik::save_to_cairo_file(*m->map_,output,format);
#endif
#endif
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
            mapnik::agg_renderer<mapnik::image_32> ren(*m->map_,im,scale_factor);
            ren.apply(scale_denominator);

            if (palette.get())
            {
                mapnik::save_to_file<mapnik::image_data_32>(im.data(),output,*palette);
            }
            else {
                mapnik::save_to_file<mapnik::image_data_32>(im.data(),output);
            }
        }
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    return Undefined();
}
