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
#include "object_to_container.hpp"

// mapnik
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
#include <mapnik/box2d.hpp>             // for box2d
#include <mapnik/color.hpp>             // for color
#include <mapnik/attribute.hpp>        // for attributes
#include <mapnik/featureset.hpp>        // for featureset_ptr
#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp>  // for grid_renderer
#include <mapnik/image.hpp>             // for image_rgba8
#include <mapnik/image_util.hpp>        // for save_to_file, guess_type, etc
#include <mapnik/layer.hpp>             // for layer
#include <mapnik/load_map.hpp>          // for load_map, load_map_string
#include <mapnik/map.hpp>               // for Map, etc
#include <mapnik/params.hpp>            // for parameters
#include <mapnik/save_map.hpp>          // for save_map, etc
#include <mapnik/image_scaling.hpp>
#include <mapnik/request.hpp>
#if defined(HAVE_CAIRO)
#include <mapnik/cairo_io.hpp>
#endif

// stl
#include <exception>                    // for exception
#include <iosfwd>                       // for ostringstream, ostream
#include <iostream>                     // for clog
#include <ostream>                      // for operator<<, basic_ostream, etc
#include <sstream>                      // for basic_ostringstream, etc

// boost
#include <boost/optional/optional.hpp>  // for optional

Persistent<FunctionTemplate> Map::constructor;

void Map::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Map::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Map"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "fonts", fonts);
    NODE_SET_PROTOTYPE_METHOD(lcons, "fontFiles", fontFiles);
    NODE_SET_PROTOTYPE_METHOD(lcons, "fontDirectory", fontDirectory);
    NODE_SET_PROTOTYPE_METHOD(lcons, "loadFonts", loadFonts);
    NODE_SET_PROTOTYPE_METHOD(lcons, "memoryFonts", memoryFonts);
    NODE_SET_PROTOTYPE_METHOD(lcons, "registerFonts", registerFonts);
    NODE_SET_PROTOTYPE_METHOD(lcons, "load", load);
    NODE_SET_PROTOTYPE_METHOD(lcons, "loadSync", loadSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "fromStringSync", fromStringSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "fromString", fromString);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clone", clone);
    NODE_SET_PROTOTYPE_METHOD(lcons, "save", save);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toXML", to_string);
    NODE_SET_PROTOTYPE_METHOD(lcons, "resize", resize);


    NODE_SET_PROTOTYPE_METHOD(lcons, "render", render);
    NODE_SET_PROTOTYPE_METHOD(lcons, "renderSync", renderSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "renderFile", renderFile);
    NODE_SET_PROTOTYPE_METHOD(lcons, "renderFileSync", renderFileSync);

    NODE_SET_PROTOTYPE_METHOD(lcons, "zoomAll", zoomAll);
    NODE_SET_PROTOTYPE_METHOD(lcons, "zoomToBox", zoomToBox); //setExtent
    NODE_SET_PROTOTYPE_METHOD(lcons, "scale", scale);
    NODE_SET_PROTOTYPE_METHOD(lcons, "scaleDenominator", scaleDenominator);
    NODE_SET_PROTOTYPE_METHOD(lcons, "queryPoint", queryPoint);
    NODE_SET_PROTOTYPE_METHOD(lcons, "queryMapPoint", queryMapPoint);

    // layer access
    NODE_SET_PROTOTYPE_METHOD(lcons, "add_layer", add_layer);
    NODE_SET_PROTOTYPE_METHOD(lcons, "get_layer", get_layer);
    NODE_SET_PROTOTYPE_METHOD(lcons, "layers", layers);

    // properties
    ATTR(lcons, "srs", get_prop, set_prop);
    ATTR(lcons, "width", get_prop, set_prop);
    ATTR(lcons, "height", get_prop, set_prop);
    ATTR(lcons, "bufferSize", get_prop, set_prop);
    ATTR(lcons, "extent", get_prop, set_prop);
    ATTR(lcons, "bufferedExtent", get_prop, set_prop);
    ATTR(lcons, "maximumExtent", get_prop, set_prop);
    ATTR(lcons, "background", get_prop, set_prop);
    ATTR(lcons, "parameters", get_prop, set_prop);
    ATTR(lcons, "aspect_fix_mode", get_prop, set_prop);

    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "ASPECT_GROW_BBOX",mapnik::Map::GROW_BBOX)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "ASPECT_GROW_CANVAS",mapnik::Map::GROW_CANVAS)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "ASPECT_SHRINK_BBOX",mapnik::Map::SHRINK_BBOX)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "ASPECT_SHRINK_CANVAS",mapnik::Map::SHRINK_CANVAS)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "ASPECT_ADJUST_BBOX_WIDTH",mapnik::Map::ADJUST_BBOX_WIDTH)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "ASPECT_ADJUST_BBOX_HEIGHT",mapnik::Map::ADJUST_BBOX_HEIGHT)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "ASPECT_ADJUST_CANVAS_WIDTH",mapnik::Map::ADJUST_CANVAS_WIDTH)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "ASPECT_ADJUST_CANVAS_HEIGHT",mapnik::Map::ADJUST_CANVAS_HEIGHT)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "ASPECT_RESPECT",mapnik::Map::RESPECT)
    target->Set(NanNew("Map"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Map::Map(int width, int height) :
    node::ObjectWrap(),
    map_(std::make_shared<mapnik::Map>(width,height)),
    in_use_(false) {}

Map::Map(int width, int height, std::string const& srs) :
    node::ObjectWrap(),
    map_(std::make_shared<mapnik::Map>(width,height,srs)),
    in_use_(false) {}

Map::Map() :
    node::ObjectWrap(),
    map_(),
    in_use_(false) {}

Map::~Map() { }

bool Map::acquire() {
    if (in_use_)
    {
        return false;
    }
    in_use_ = true;
    return true;
}

void Map::release() {
    in_use_ = false;
}

NAN_METHOD(Map::New)
{
    NanScope();

    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    // accept a reference or v8:External?
    if (args[0]->IsExternal())
    {
        Local<External> ext = args[0].As<External>();
        void* ptr = ext->Value();
        Map* m =  static_cast<Map*>(ptr);
        m->Wrap(args.This());
        NanReturnValue(args.This());
    }

    if (args.Length() == 2)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
        {
            NanThrowTypeError("'width' and 'height' must be integers");
            NanReturnUndefined();
        }
        Map* m = new Map(args[0]->IntegerValue(),args[1]->IntegerValue());
        m->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else if (args.Length() == 3)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
        {
            NanThrowTypeError("'width' and 'height' must be integers");
            NanReturnUndefined();
        }
        if (!args[2]->IsString())
        {
            NanThrowError("'srs' value must be a string");
            NanReturnUndefined();
        }
        Map* m = new Map(args[0]->IntegerValue(), args[1]->IntegerValue(), TOSTR(args[2]));
        m->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else
    {
        NanThrowError("please provide Map width and height and optional srs");
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

NAN_GETTER(Map::get_prop)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    std::string a = TOSTR(property);
    if(a == "extent") {
        Local<Array> arr = NanNew<Array>(4);
        mapnik::box2d<double> const& e = m->map_->get_current_extent();
        arr->Set(0, NanNew<Number>(e.minx()));
        arr->Set(1, NanNew<Number>(e.miny()));
        arr->Set(2, NanNew<Number>(e.maxx()));
        arr->Set(3, NanNew<Number>(e.maxy()));
        NanReturnValue(arr);
    }
    else if(a == "bufferedExtent") {
        boost::optional<mapnik::box2d<double> > const& e = m->map_->get_buffered_extent();
        Local<Array> arr = NanNew<Array>(4);
        arr->Set(0, NanNew<Number>(e->minx()));
        arr->Set(1, NanNew<Number>(e->miny()));
        arr->Set(2, NanNew<Number>(e->maxx()));
        arr->Set(3, NanNew<Number>(e->maxy()));
        NanReturnValue(arr);
    }
    else if(a == "maximumExtent") {
        boost::optional<mapnik::box2d<double> > const& e = m->map_->maximum_extent();
        if (!e)
            NanReturnUndefined();
        Local<Array> arr = NanNew<Array>(4);
        arr->Set(0, NanNew<Number>(e->minx()));
        arr->Set(1, NanNew<Number>(e->miny()));
        arr->Set(2, NanNew<Number>(e->maxx()));
        arr->Set(3, NanNew<Number>(e->maxy()));
        NanReturnValue(arr);
    }
    else if(a == "aspect_fix_mode")
        NanReturnValue(NanNew<Integer>(m->map_->get_aspect_fix_mode()));
    else if(a == "width")
        NanReturnValue(NanNew<Integer>(m->map_->width()));
    else if(a == "height")
        NanReturnValue(NanNew<Integer>(m->map_->height()));
    else if (a == "srs")
        NanReturnValue(NanNew(m->map_->srs().c_str()));
    else if(a == "bufferSize")
        NanReturnValue(NanNew<Integer>(m->map_->buffer_size()));
    else if (a == "background") {
        boost::optional<mapnik::color> c = m->map_->background();
        if (c)
            NanReturnValue(Color::NewInstance(*c));
        else
            NanReturnUndefined();
    }
    else //if (a == "parameters") 
    {
        Local<Object> ds = NanNew<Object>();
        mapnik::parameters const& params = m->map_->get_extra_parameters();
        mapnik::parameters::const_iterator it = params.begin();
        mapnik::parameters::const_iterator end = params.end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object(ds, it->first, it->second);
        }
        NanReturnValue(ds);
    }
}

NAN_SETTER(Map::set_prop)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    std::string a = TOSTR(property);
    if(a == "extent" || a == "maximumExtent") {
        if (!value->IsArray()) {
            NanThrowError("Must provide an array of: [minx,miny,maxx,maxy]");
            return;
        } else {
            Local<Array> arr = value.As<Array>();
            if (arr->Length() != 4) {
                NanThrowError("Must provide an array of: [minx,miny,maxx,maxy]");
                return;
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
    else if (a == "aspect_fix_mode")
    {
        if (!value->IsNumber()) {
            NanThrowError("'aspect_fix_mode' must be a constant (number)");
            return;
        } else {
            int val = value->IntegerValue();
            if (val < mapnik::Map::aspect_fix_mode_MAX && val >= 0) {
                m->map_->set_aspect_fix_mode(static_cast<mapnik::Map::aspect_fix_mode>(val));
            } else {
                NanThrowError("'aspect_fix_mode' value is invalid");
                return;
            }
        }
    }
    else if (a == "srs")
    {
        if (!value->IsString()) {
            NanThrowError("'srs' must be a string");
            return;
        } else {
            m->map_->set_srs(TOSTR(value));
        }
    }
    else if (a == "bufferSize") {
        if (!value->IsNumber()) {
            NanThrowTypeError("Must provide an integer bufferSize");
            return;
        } else {
            m->map_->set_buffer_size(value->IntegerValue());
        }
    }
    else if (a == "width") {
        if (!value->IsNumber()) {
            NanThrowTypeError("Must provide an integer width");
            return;
        } else {
            m->map_->set_width(value->IntegerValue());
        }
    }
    else if (a == "height") {
        if (!value->IsNumber()) {
            NanThrowTypeError("Must provide an integer height");
            return;
        } else {
            m->map_->set_height(value->IntegerValue());
        }
    }
    else if (a == "background") {
        if (!value->IsObject()) {
            NanThrowTypeError("mapnik.Color expected");
            return;
        }

        Local<Object> obj = value.As<Object>();
        if (obj->IsNull() || obj->IsUndefined() || !NanNew(Color::constructor)->HasInstance(obj)) {
            NanThrowTypeError("mapnik.Color expected");
            return;
        }
        Color *c = node::ObjectWrap::Unwrap<Color>(obj);
        m->map_->set_background(*c->get());
    }
    else if (a == "parameters") {
        if (!value->IsObject()) {
            NanThrowTypeError("object expected for map.parameters");
            return;
        }

        Local<Object> obj = value->ToObject();
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
            } else if (a_value->IsBoolean()) {
                params[TOSTR(name)] = static_cast<mapnik::value_bool>(a_value->BooleanValue());
            }
            i++;
        }
        m->map_->set_extra_parameters(params);
    }
}

NAN_METHOD(Map::loadFonts)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    NanReturnValue(NanNew<Boolean>(m->map_->load_fonts()));
}

NAN_METHOD(Map::memoryFonts)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    auto const& font_cache = m->map_->get_font_memory_cache();
    Local<Array> a = NanNew<Array>(font_cache.size());
    unsigned i = 0;
    for (auto const& kv : font_cache)
    {
        a->Set(i++, NanNew(kv.first.c_str()));
    }
    NanReturnValue(a);
}

NAN_METHOD(Map::registerFonts)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    if (args.Length() == 0 || !args[0]->IsString())
    {
        NanThrowTypeError("first argument must be a path to a directory of fonts");
        NanReturnUndefined();
    }

    bool recurse = false;

    if (args.Length() >= 2)
    {
        if (!args[1]->IsObject())
        {
            NanThrowTypeError("second argument is optional, but if provided must be an object, eg. { recurse: true }");
            NanReturnUndefined();
        }
        Local<Object> options = args[1].As<Object>();
        if (options->Has(NanNew("recurse")))
        {
            Local<Value> recurse_opt = options->Get(NanNew("recurse"));
            if (!recurse_opt->IsBoolean())
            {
                NanThrowTypeError("'recurse' must be a Boolean");
                NanReturnUndefined();
            }
            recurse = recurse_opt->BooleanValue();
        }
    }
    std::string path = TOSTR(args[0]);
    NanReturnValue(NanNew(m->map_->register_fonts(path,recurse)));
}

NAN_METHOD(Map::fonts)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    auto const& mapping = m->map_->get_font_file_mapping();
    Local<Array> a = NanNew<Array>(mapping.size());
    unsigned i = 0;
    for (auto const& kv : mapping)
    {
        a->Set(i++, NanNew(kv.first.c_str()));
    }
    NanReturnValue(a);
}

NAN_METHOD(Map::fontFiles)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    auto const& mapping = m->map_->get_font_file_mapping();
    Local<Object> obj = NanNew<Object>();
    for (auto const& kv : mapping)
    {
        obj->Set(NanNew(kv.first.c_str()), NanNew(kv.second.second.c_str()));
    }
    NanReturnValue(obj);
}

NAN_METHOD(Map::fontDirectory)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    boost::optional<std::string> const& fdir = m->map_->font_directory();
    if (fdir)
    {
        NanReturnValue(NanNew(fdir->c_str()));
    }
    NanReturnUndefined();
}

NAN_METHOD(Map::scale)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    NanReturnValue(NanNew<Number>(m->map_->scale()));
}

NAN_METHOD(Map::scaleDenominator)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    NanReturnValue(NanNew<Number>(m->map_->scale_denominator()));
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


NAN_METHOD(Map::queryMapPoint)
{
    NanScope();
    abstractQueryPoint(args,false);
    NanReturnUndefined();
}

NAN_METHOD(Map::queryPoint)
{
    NanScope();
    abstractQueryPoint(args,true);
    NanReturnUndefined();
}

Handle<Value> Map::abstractQueryPoint(_NAN_METHOD_ARGS, bool geo_coords)
{
    NanScope();
    if (args.Length() < 3)
    {
        NanThrowError("requires at least three arguments, a x,y query and a callback");
        return NanUndefined();
    }

    double x,y;
    if (!args[0]->IsNumber() || !args[1]->IsNumber())
    {
        NanThrowTypeError("x,y arguments must be numbers");
        return NanUndefined();
    }
    else
    {
        x = args[0]->NumberValue();
        y = args[1]->NumberValue();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());

    Local<Object> options = NanNew<Object>();
    int layer_idx = -1;

    if (args.Length() > 3)
    {
        // options object
        if (!args[2]->IsObject()) {
            NanThrowTypeError("optional third argument must be an options object");
            return NanUndefined();
        }

        options = args[2]->ToObject();

        if (options->Has(NanNew("layer")))
        {
            std::vector<mapnik::layer> const& layers = m->map_->layers();
            Local<Value> layer_id = options->Get(NanNew("layer"));
            if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
                NanThrowTypeError("'layer' option required for map query and must be either a layer name(string) or layer index (integer)");
                return NanUndefined();
            }

            if (layer_id->IsString()) {
                bool found = false;
                unsigned int idx(0);
                std::string layer_name = TOSTR(layer_id);
                for (mapnik::layer const& lyr : layers)
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
                    NanThrowTypeError(s.str().c_str());
                    return NanUndefined();
                }
            }
            else if (layer_id->IsNumber())
            {
                layer_idx = layer_id->IntegerValue();
                std::size_t layer_num = layers.size();

                if (layer_idx < 0) {
                    std::ostringstream s;
                    s << "Zero-based layer index '" << layer_idx << "' not valid"
                      << " must be a positive integer, ";
                    if (layer_num > 0)
                    {
                        s << "only '" << layer_num << "' layers exist in map";
                    }
                    else
                    {
                        s << "no layers found in map";
                    }
                    NanThrowTypeError(s.str().c_str());
                    return NanUndefined();
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
                    NanThrowTypeError(s.str().c_str());
                    return NanUndefined();
                }
            }
        }
    }

    // ensure function callback
    Local<Value> callback = args[args.Length() - 1];
    if (!callback->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        return NanUndefined();
    }

    query_map_baton_t *closure = new query_map_baton_t();
    closure->request.data = closure;
    closure->m = m;
    closure->x = x;
    closure->y = y;
    closure->layer_idx = static_cast<std::size_t>(layer_idx);
    closure->geo_coords = geo_coords;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_QueryMap, (uv_after_work_cb)EIO_AfterQueryMap);
    m->Ref();
    return NanUndefined();
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
            for (mapnik::layer const& lyr : layers)
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
    NanScope();

    query_map_baton_t *closure = static_cast<query_map_baton_t *>(req->data);

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    } else {
        std::size_t num_result = closure->featuresets.size();
        if (num_result >= 1)
        {
            Local<Array> a = NanNew<Array>(num_result);
            typedef std::map<std::string,mapnik::featureset_ptr> fs_itr;
            fs_itr::const_iterator it = closure->featuresets.begin();
            fs_itr::const_iterator end = closure->featuresets.end();
            unsigned idx = 0;
            for (; it != end; ++it)
            {
                Local<Object> obj = NanNew<Object>();
                obj->Set(NanNew("layer"), NanNew(it->first.c_str()));
                obj->Set(NanNew("featureset"), Featureset::NewInstance(it->second));
                a->Set(idx, obj);
                ++idx;
            }
            closure->featuresets.clear();
            Local<Value> argv[2] = { NanNull(), a };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
        else
        {
            Local<Value> argv[2] = { NanNull(), NanUndefined() };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
    }

    closure->m->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}

NAN_METHOD(Map::layers)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    std::vector<mapnik::layer> const& layers = m->map_->layers();
    Local<Array> a = NanNew<Array>(layers.size());
    for (unsigned i = 0; i < layers.size(); ++i )
    {
        a->Set(i, Layer::NewInstance(layers[i]));
    }
    NanReturnValue(a);
}

NAN_METHOD(Map::add_layer) {
    NanScope();

    if (!args[0]->IsObject()) {
        NanThrowTypeError("mapnik.Layer expected");
        NanReturnUndefined();
    }

    Local<Object> obj = args[0].As<Object>();
    if (obj->IsNull() || obj->IsUndefined() || !NanNew(Layer::constructor)->HasInstance(obj)) {
        NanThrowTypeError("mapnik.Layer expected");
        NanReturnUndefined();
    }
    Layer *l = node::ObjectWrap::Unwrap<Layer>(obj);
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    m->map_->add_layer(*l->get());
    NanReturnUndefined();
}

NAN_METHOD(Map::get_layer)
{
    NanScope();

    if (args.Length() != 1) {
        NanThrowError("Please provide layer name or index");
        NanReturnUndefined();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    std::vector<mapnik::layer> const& layers = m->map_->layers();

    Local<Value> layer = args[0];
    if (layer->IsNumber())
    {
        unsigned int index = args[0]->IntegerValue();

        if (index < layers.size())
        {
            NanReturnValue(Layer::NewInstance(layers[index]));
        }
        else
        {
            NanThrowTypeError("invalid layer index");
            NanReturnUndefined();
        }
    }
    else if (layer->IsString())
    {
        bool found = false;
        unsigned int idx(0);
        std::string layer_name = TOSTR(layer);
        for ( mapnik::layer const& lyr : layers)
        {
            if (lyr.name() == layer_name)
            {
                found = true;
                NanReturnValue(Layer::NewInstance(layers[idx]));
            }
            ++idx;
        }
        if (!found)
        {
            std::ostringstream s;
            s << "Layer name '" << layer_name << "' not found";
            NanThrowTypeError(s.str().c_str());
            NanReturnUndefined();
        }

    }
    NanThrowTypeError("first argument must be either a layer name(string) or layer index (integer)");
    NanReturnUndefined();
}

NAN_METHOD(Map::clear)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    m->map_->remove_all();
    NanReturnUndefined();
}

NAN_METHOD(Map::resize)
{
    NanScope();

    if (args.Length() != 2) {
        NanThrowError("Please provide width and height");
        NanReturnUndefined();
    }

    if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
        NanThrowTypeError("width and height must be integers");
        NanReturnUndefined();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    m->map_->resize(args[0]->IntegerValue(),args[1]->IntegerValue());
    NanReturnUndefined();
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


NAN_METHOD(Map::load)
{
    NanScope();

    if (args.Length() < 2) {
        NanThrowError("please provide a stylesheet path, options, and callback");
        NanReturnUndefined();
    }

    // ensure stylesheet path is a string
    Local<Value> stylesheet = args[0];
    if (!stylesheet->IsString()) {
        NanThrowTypeError("first argument must be a path to a mapnik stylesheet");
        NanReturnUndefined();
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!callback->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    // ensure options object
    if (!args[1]->IsObject()) {
        NanThrowTypeError("options must be an object, eg {strict: true}");
        NanReturnUndefined();
    }

    Local<Object> options = args[1].As<Object>();

    bool strict = false;
    Local<String> param = NanNew("strict");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsBoolean()) {
            NanThrowTypeError("'strict' must be a Boolean");
            NanReturnUndefined();
        }
        strict = param_val->BooleanValue();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());

    load_xml_baton_t *closure = new load_xml_baton_t();
    closure->request.data = closure;

    param = NanNew("base");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsString()) {
            NanThrowTypeError("'base' must be a string representing a filesystem path");
            NanReturnUndefined();
        }
        closure->base_path = TOSTR(param_val);
    }

    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Load, (uv_after_work_cb)EIO_AfterLoad);
    m->Ref();
    NanReturnUndefined();
}

void Map::EIO_Load(uv_work_t* req)
{
    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);

    try
    {
        mapnik::load_map(*closure->m->map_,closure->stylesheet,closure->strict,closure->base_path);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterLoad(uv_work_t* req)
{
    NanScope();

    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    } else {
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->m) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->m->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}


NAN_METHOD(Map::loadSync)
{
    NanScope();
    if (!args[0]->IsString()) {
        NanThrowTypeError("first argument must be a path to a mapnik stylesheet");
        NanReturnUndefined();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    std::string stylesheet = TOSTR(args[0]);
    bool strict = false;
    std::string base_path;

    if (args.Length() > 2)
    {

        NanThrowError("only accepts two arguments: a path to a mapnik stylesheet and an optional options object");
        NanReturnUndefined();
    }
    else if (args.Length() == 2)
    {
        // ensure options object
        if (!args[1]->IsObject())
        {
            NanThrowTypeError("options must be an object, eg {strict: true}");
            NanReturnUndefined();
        }

        Local<Object> options = args[1].As<Object>();

        Local<String> param = NanNew("strict");
        if (options->Has(param))
        {
            Local<Value> param_val = options->Get(param);
            if (!param_val->IsBoolean())
            {
                NanThrowTypeError("'strict' must be a Boolean");
                NanReturnUndefined();
            }
            strict = param_val->BooleanValue();
        }

        param = NanNew("base");
        if (options->Has(param))
        {
            Local<Value> param_val = options->Get(param);
            if (!param_val->IsString())
            {
                NanThrowTypeError("'base' must be a string representing a filesystem path");
                NanReturnUndefined();
            }
            base_path = TOSTR(param_val);
        }
    }

    try
    {
        mapnik::load_map(*m->map_,stylesheet,strict,base_path);
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

NAN_METHOD(Map::fromStringSync)
{
    NanScope();
    if (args.Length() < 1) {
        NanThrowError("Accepts 2 arguments: stylesheet string and an optional options");
        NanReturnUndefined();
    }

    if (!args[0]->IsString()) {
        NanThrowTypeError("first argument must be a mapnik stylesheet string");
        NanReturnUndefined();
    }


    // defaults
    bool strict = false;
    std::string base_path("");

    if (args.Length() >= 2) {
        // ensure options object
        if (!args[1]->IsObject()) {
            NanThrowTypeError("options must be an object, eg {strict: true, base: \".\"'}");
            NanReturnUndefined();
        }

        Local<Object> options = args[1].As<Object>();

        Local<String> param = NanNew("strict");
        if (options->Has(param))
        {
            Local<Value> param_val = options->Get(param);
            if (!param_val->IsBoolean()) {
                NanThrowTypeError("'strict' must be a Boolean");
                NanReturnUndefined();
            }
            strict = param_val->BooleanValue();
        }

        param = NanNew("base");
        if (options->Has(param))
        {
            Local<Value> param_val = options->Get(param);
            if (!param_val->IsString()) {
                NanThrowTypeError("'base' must be a string representing a filesystem path");
                NanReturnUndefined();
            }
            base_path = TOSTR(param_val);
        }
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());

    std::string stylesheet = TOSTR(args[0]);

    try
    {
        mapnik::load_map_string(*m->map_,stylesheet,strict,base_path);
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

NAN_METHOD(Map::fromString)
{
    NanScope();

    if (args.Length() < 2)
    {
        NanThrowError("please provide a stylesheet string, options, and callback");
        NanReturnUndefined();
    }

    // ensure stylesheet path is a string
    Local<Value> stylesheet = args[0];
    if (!stylesheet->IsString())
    {
        NanThrowTypeError("first argument must be a path to a mapnik stylesheet string");
        NanReturnUndefined();
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
    {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    // ensure options object
    if (!args[1]->IsObject())
    {
        NanThrowTypeError("options must be an object, eg {strict: true, base: \".\"'}");
        NanReturnUndefined();
    }

    Local<Object> options = args[1]->ToObject();

    bool strict = false;
    Local<String> param = NanNew("strict");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsBoolean())
        {
            NanThrowTypeError("'strict' must be a Boolean");
            NanReturnUndefined();
        }
        strict = param_val->BooleanValue();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());

    load_xml_baton_t *closure = new load_xml_baton_t();
    closure->request.data = closure;

    param = NanNew("base");
    if (options->Has(param))
    {
        Local<Value> param_val = options->Get(param);
        if (!param_val->IsString())
        {
            NanThrowTypeError("'base' must be a string representing a filesystem path");
            NanReturnUndefined();
        }
        closure->base_path = TOSTR(param_val);
    }

    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromString, (uv_after_work_cb)EIO_AfterFromString);
    m->Ref();
    NanReturnUndefined();
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
    NanScope();

    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    } else {
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->m) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->m->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}

NAN_METHOD(Map::clone)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    Map* m2 = new Map();
    m2->map_ = std::make_shared<mapnik::Map>(*m->map_);
    Handle<Value> ext = NanNew<External>(m2);
    NanReturnValue(NanNew(constructor)->GetFunction()->NewInstance(1, &ext));
}

NAN_METHOD(Map::save)
{
    NanScope();
    if (args.Length() != 1 || !args[0]->IsString())
    {
        NanThrowTypeError("first argument must be a path to map.xml to save");
        NanReturnUndefined();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    std::string filename = TOSTR(args[0]);
    bool explicit_defaults = false;
    mapnik::save_map(*m->map_,filename,explicit_defaults);
    NanReturnUndefined();
}

NAN_METHOD(Map::to_string)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    bool explicit_defaults = false;
    std::string map_string = mapnik::save_map_to_string(*m->map_,explicit_defaults);
    NanReturnValue(NanNew(map_string.c_str()));
}

NAN_METHOD(Map::zoomAll)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    try
    {
        m->map_->zoom_all();
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

NAN_METHOD(Map::zoomToBox)
{
    NanScope();
    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());

    double minx;
    double miny;
    double maxx;
    double maxy;

    if (args.Length() == 1)
    {
        if (!args[0]->IsArray())
        {
            NanThrowError("Must provide an array of: [minx,miny,maxx,maxy]");
            NanReturnUndefined();
        }
        Local<Array> a = args[0].As<Array>();
        minx = a->Get(0)->NumberValue();
        miny = a->Get(1)->NumberValue();
        maxx = a->Get(2)->NumberValue();
        maxy = a->Get(3)->NumberValue();

    }
    else if (args.Length() != 4)
    {
        NanThrowError("Must provide 4 arguments: minx,miny,maxx,maxy");
        NanReturnUndefined();
    } 
    else if (args[0]->IsNumber() && 
               args[1]->IsNumber() &&
               args[2]->IsNumber() &&
               args[3]->IsNumber())
    {
        minx = args[0]->NumberValue();
        miny = args[1]->NumberValue();
        maxx = args[2]->NumberValue();
        maxy = args[3]->NumberValue();
    }
    else
    {
        NanThrowError("If you are providing 4 arguments: minx,miny,maxx,maxy - they must be all numbers");
        NanReturnUndefined();
    }

    mapnik::box2d<double> box(minx,miny,maxx,maxy);
    m->map_->zoom_to_box(box);
    NanReturnUndefined();
}

struct image_baton_t {
    uv_work_t request;
    Map *m;
    Image *im;
    int buffer_size; // TODO - no effect until mapnik::request is used
    double scale_factor;
    double scale_denominator;
    mapnik::attributes variables;
    unsigned offset_x;
    unsigned offset_y;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    image_baton_t() :
      buffer_size(0),
      scale_factor(1.0),
      scale_denominator(0.0),
      variables(),
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
    mapnik::attributes variables;
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
      variables(),
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
    mapnik::attributes variables;
    unsigned offset_x;
    unsigned offset_y;
    std::string image_format;
    mapnik::scaling_method_e scaling_method;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    vector_tile_baton_t() :
        tolerance(1),
        path_multiplier(16),
        scale_factor(1.0),
        scale_denominator(0.0),
        variables(),
        offset_x(0),
        offset_y(0),
        image_format("jpeg"),
        scaling_method(mapnik::SCALING_NEAR),
        error(false) {}
};

NAN_METHOD(Map::render)
{
    NanScope();

    // ensure at least 2 args
    if (args.Length() < 2) {
        NanThrowTypeError("requires at least two arguments, a renderable mapnik object, and a callback");
        NanReturnUndefined();
    }

    // ensure renderable object
    if (!args[0]->IsObject()) {
        NanThrowTypeError("requires a renderable mapnik object to be passed as first argument");
        NanReturnUndefined();
    }

    // ensure function callback
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());

    try
    {
        // parse options

        // defaults
        int buffer_size = 0;
        double scale_factor = 1.0;
        double scale_denominator = 0.0;
        unsigned offset_x = 0;
        unsigned offset_y = 0;

        Local<Object> options = NanNew<Object>();

        if (args.Length() > 2) {

            // options object
            if (!args[1]->IsObject()) {
                NanThrowTypeError("optional second argument must be an options object");
                NanReturnUndefined();
            }

            options = args[1]->ToObject();

            if (options->Has(NanNew("buffer_size"))) {
                Local<Value> bind_opt = options->Get(NanNew("buffer_size"));
                if (!bind_opt->IsNumber()) {
                    NanThrowTypeError("optional arg 'buffer_size' must be a number");
                    NanReturnUndefined();
                }

                buffer_size = bind_opt->IntegerValue();
            }

            if (options->Has(NanNew("scale"))) {
                Local<Value> bind_opt = options->Get(NanNew("scale"));
                if (!bind_opt->IsNumber()) {
                    NanThrowTypeError("optional arg 'scale' must be a number");
                    NanReturnUndefined();
                }

                scale_factor = bind_opt->NumberValue();
            }

            if (options->Has(NanNew("scale_denominator"))) {
                Local<Value> bind_opt = options->Get(NanNew("scale_denominator"));
                if (!bind_opt->IsNumber()) {
                    NanThrowTypeError("optional arg 'scale_denominator' must be a number");
                    NanReturnUndefined();
                }

                scale_denominator = bind_opt->NumberValue();
            }

            if (options->Has(NanNew("offset_x"))) {
                Local<Value> bind_opt = options->Get(NanNew("offset_x"));
                if (!bind_opt->IsNumber()) {
                    NanThrowTypeError("optional arg 'offset_x' must be a number");
                    NanReturnUndefined();
                }

                offset_x = bind_opt->IntegerValue();
            }

            if (options->Has(NanNew("offset_y"))) {
                Local<Value> bind_opt = options->Get(NanNew("offset_y"));
                if (!bind_opt->IsNumber()) {
                    NanThrowTypeError("optional arg 'offset_y' must be a number");
                    NanReturnUndefined();
                }

                offset_y = bind_opt->IntegerValue();
            }
        }

        Local<Object> obj = args[0]->ToObject();

        if (NanNew(Image::constructor)->HasInstance(obj)) {

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

            if (options->Has(NanNew("variables")))
            {
                Local<Value> bind_opt = options->Get(NanNew("variables"));
                if (!bind_opt->IsObject())
                {
                    delete closure;
                    NanThrowTypeError("optional arg 'variables' must be an object");
                    NanReturnUndefined();
                }
                object_to_container(closure->variables,bind_opt->ToObject());
            }
            if (!m->acquire())
            {
                delete closure;
                NanThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
                NanReturnUndefined();
            }
            NanAssignPersistent(closure->cb, args[args.Length() - 1].As<Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderImage, (uv_after_work_cb)EIO_AfterRenderImage);

        } else if (NanNew(Grid::constructor)->HasInstance(obj)) {

            Grid * g = node::ObjectWrap::Unwrap<Grid>(obj);

            std::size_t layer_idx = 0;

            // grid requires special options for now
            if (!options->Has(NanNew("layer"))) {
                NanThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
                NanReturnUndefined();
            } else {

                std::vector<mapnik::layer> const& layers = m->map_->layers();

                Local<Value> layer_id = options->Get(NanNew("layer"));
                if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
                    NanThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
                    NanReturnUndefined();
                }

                if (layer_id->IsString()) {
                    bool found = false;
                    unsigned int idx(0);
                    std::string const & layer_name = TOSTR(layer_id);
                    for (mapnik::layer const& lyr : layers)
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
                        NanThrowTypeError(s.str().c_str());
                        NanReturnUndefined();
                    }
                } else { // IS NUMBER
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
                        NanThrowTypeError(s.str().c_str());
                        NanReturnUndefined();
                    }
                }
            }

            if (options->Has(NanNew("fields"))) {

                Local<Value> param_val = options->Get(NanNew("fields"));
                if (!param_val->IsArray()) {
                    NanThrowTypeError("option 'fields' must be an array of strings");
                    NanReturnUndefined();
                }
                Local<Array> a = Local<Array>::Cast(param_val);
                unsigned int i = 0;
                unsigned int num_fields = a->Length();
                while (i < num_fields) {
                    Local<Value> name = a->Get(i);
                    if (name->IsString()){
                        g->get()->add_field(TOSTR(name));
                    }
                    i++;
                }
            }

            grid_baton_t *closure = new grid_baton_t();

            if (options->Has(NanNew("variables")))
            {
                Local<Value> bind_opt = options->Get(NanNew("variables"));
                if (!bind_opt->IsObject())
                {
                    delete closure;
                    NanThrowTypeError("optional arg 'variables' must be an object");
                    NanReturnUndefined();
                }
                object_to_container(closure->variables,bind_opt->ToObject());
            }

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
            if (!m->acquire())
            {
                delete closure;
                NanThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
                NanReturnUndefined();
            }
            NanAssignPersistent(closure->cb, args[args.Length() - 1].As<Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderGrid, (uv_after_work_cb)EIO_AfterRenderGrid);
        } else if (NanNew(VectorTile::constructor)->HasInstance(obj)) {

            vector_tile_baton_t *closure = new vector_tile_baton_t();
            VectorTile * vector_tile_obj = node::ObjectWrap::Unwrap<VectorTile>(obj);

            if (options->Has(NanNew("image_scaling"))) {
                Local<Value> param_val = options->Get(NanNew("image_scaling"));
                if (!param_val->IsString()) {
                    delete closure;
                    NanThrowTypeError("option 'image_scaling' must be a string");
                    NanReturnUndefined();
                }
                std::string image_scaling = TOSTR(param_val);
                boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
                if (!method) {
                    delete closure;
                    NanThrowTypeError("option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')");
                    NanReturnUndefined();
                }
                closure->scaling_method = *method;
            }

            if (options->Has(NanNew("image_format"))) {
                Local<Value> param_val = options->Get(NanNew("image_format"));
                if (!param_val->IsString()) {
                    delete closure;
                    NanThrowTypeError("option 'image_format' must be a string");
                    NanReturnUndefined();
                }
                closure->image_format = TOSTR(param_val);
            }

            if (options->Has(NanNew("tolerance"))) {
                Local<Value> param_val = options->Get(NanNew("tolerance"));
                if (!param_val->IsNumber()) {
                    delete closure;
                    NanThrowTypeError("option 'tolerance' must be an unsigned integer");
                    NanReturnUndefined();
                }
                closure->tolerance = param_val->IntegerValue();
            }

            if (options->Has(NanNew("path_multiplier"))) {
                Local<Value> param_val = options->Get(NanNew("path_multiplier"));
                if (!param_val->IsNumber()) {
                    delete closure;
                    NanThrowTypeError("option 'path_multiplier' must be an unsigned integer");
                    NanReturnUndefined();
                }
                closure->path_multiplier = param_val->NumberValue();
            }

            if (options->Has(NanNew("variables")))
            {
                Local<Value> bind_opt = options->Get(NanNew("variables"));
                if (!bind_opt->IsObject())
                {
                    delete closure;
                    NanThrowTypeError("optional arg 'variables' must be an object");
                    NanReturnUndefined();
                }
                object_to_container(closure->variables,bind_opt->ToObject());
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
            if (!m->acquire())
            {
                    delete closure;
                NanThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
                NanReturnUndefined();
            }
            NanAssignPersistent(closure->cb, args[args.Length() - 1].As<Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderVectorTile, (uv_after_work_cb)EIO_AfterRenderVectorTile);
        } else {
            NanThrowTypeError("renderable mapnik object expected");
            NanReturnUndefined();
        }

        m->Ref();
        NanReturnUndefined();
    }
    catch (std::exception const& ex)
    {
        // I am not quite sure it is possible to put a test in to cover an exception here
        /* LCOV_EXCL_START */
        NanThrowTypeError(ex.what());
        NanReturnUndefined();
        /* LCOV_EXCL_END */
    }
}

void Map::EIO_RenderVectorTile(uv_work_t* req)
{
    vector_tile_baton_t *closure = static_cast<vector_tile_baton_t *>(req->data);
    try
    {
        typedef mapnik::vector_tile_impl::backend_pbf backend_type;
        typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
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
                          closure->tolerance,
                          closure->image_format,
                          closure->scaling_method);
        ren.apply(closure->scale_denominator);
        closure->d->painted(ren.painted());
        closure->d->cache_bytesize();

    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterRenderVectorTile(uv_work_t* req)
{
    NanScope();

    vector_tile_baton_t *closure = static_cast<vector_tile_baton_t *>(req->data);

    closure->m->release();

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    } else {
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->d) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->d->_unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}

void Map::EIO_RenderGrid(uv_work_t* req)
{

    grid_baton_t *closure = static_cast<grid_baton_t *>(req->data);

    std::vector<mapnik::layer> const& layers = closure->m->map_->layers();

    try
    {
        // copy property names
        std::set<std::string> attributes = closure->g->get()->get_fields();

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
    NanScope();

    grid_baton_t *closure = static_cast<grid_baton_t *>(req->data);

    closure->m->release();

    if (closure->error) {
        // TODO - add more attributes
        // https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/Error
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    } else {
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->g) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->g->_unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}

struct agg_renderer_visitor
{
    agg_renderer_visitor(mapnik::Map const& m, 
                         mapnik::request const& req, 
                         mapnik::attributes const& vars,
                         double scale_factor, 
                         unsigned offset_x, 
                         unsigned offset_y, 
                         double scale_denominator)
        : m_(m),
          req_(req),
          vars_(vars),
          scale_factor_(scale_factor), 
          offset_x_(offset_x), 
          offset_y_(offset_y),
          scale_denominator_(scale_denominator) {}

    void operator() (mapnik::image_rgba8 & pixmap)
    {
        mapnik::agg_renderer<mapnik::image_rgba8> ren(m_,req_,vars_,pixmap,scale_factor_,offset_x_,offset_y_);
        ren.apply(scale_denominator_);
    }
    
    template <typename T>
    void operator() (T &)
    {
        throw std::runtime_error("This image type is not currently supported for rendering.");
    }

  private:
    mapnik::Map const& m_;
    mapnik::request const& req_;
    mapnik::attributes const& vars_;
    double scale_factor_;
    unsigned offset_x_;
    unsigned offset_y_;
    double scale_denominator_;
};

void Map::EIO_RenderImage(uv_work_t* req)
{
    image_baton_t *closure = static_cast<image_baton_t *>(req->data);

    try
    {
        mapnik::Map const& map = *closure->m->map_;
        mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
        m_req.set_buffer_size(closure->buffer_size);
        agg_renderer_visitor visit(map, 
                                   m_req, 
                                   closure->variables, 
                                   closure->scale_factor, 
                                   closure->offset_x,
                                   closure->offset_y,
                                   closure->scale_denominator);
        mapnik::util::apply_visitor(visit, *closure->im->get());
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterRenderImage(uv_work_t* req)
{
    NanScope();

    image_baton_t *closure = static_cast<image_baton_t *>(req->data);

    closure->m->release();

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    } else {
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->im) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->im->_unref();
    NanDisposePersistent(closure->cb);
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
    mapnik::attributes variables;
    bool use_cairo;
    int buffer_size; // TODO - no effect until mapnik::request is used
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} render_file_baton_t;

NAN_METHOD(Map::renderFile)
{
    NanScope();

    if (args.Length() < 1 || !args[0]->IsString()) {
        NanThrowTypeError("first argument must be a path to a file to save");
        NanReturnUndefined();
    }

    // defaults
    std::string format = "png";
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    palette_ptr palette;
    int buffer_size = 0;

    Local<Value> callback = args[args.Length()-1];

    if (!callback->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    Local<Object> options = NanNew<Object>();

    if (!args[1]->IsFunction() && args[1]->IsObject()) {
        options = args[1]->ToObject();
        if (options->Has(NanNew("format")))
        {
            Local<Value> format_opt = options->Get(NanNew("format"));
            if (!format_opt->IsString()) {
                NanThrowTypeError("'format' must be a String");
                NanReturnUndefined();
            }

            format = TOSTR(format_opt);
        }

        if (options->Has(NanNew("palette")))
        {
            Local<Value> format_opt = options->Get(NanNew("palette"));
            if (!format_opt->IsObject()) {
                NanThrowTypeError("'palette' must be an object");
                NanReturnUndefined();
            }

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(Palette::constructor)->HasInstance(obj)) {
                NanThrowTypeError("mapnik.Palette expected as second arg");
                NanReturnUndefined();
            }

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
        if (options->Has(NanNew("scale"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'scale' must be a number");
                NanReturnUndefined();
            }

            scale_factor = bind_opt->NumberValue();
        }

        if (options->Has(NanNew("scale_denominator"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale_denominator"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'scale_denominator' must be a number");
                NanReturnUndefined();
            }

            scale_denominator = bind_opt->NumberValue();
        }

        if (options->Has(NanNew("buffer_size"))) {
            Local<Value> bind_opt = options->Get(NanNew("buffer_size"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'buffer_size' must be a number");
                NanReturnUndefined();
            }

            buffer_size = bind_opt->IntegerValue();
        }

    } else if (!args[1]->IsFunction()) {
        NanThrowTypeError("optional argument must be an object");
        NanReturnUndefined();
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    std::string output = TOSTR(args[0]);

    //maybe do this in the async part?
    if (format.empty()) {
        format = mapnik::guess_type(output);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << output << "\n";
            NanThrowError(s.str().c_str());
            NanReturnUndefined();
        }
    }

    render_file_baton_t *closure = new render_file_baton_t();

    if (options->Has(NanNew("variables")))
    {
        Local<Value> bind_opt = options->Get(NanNew("variables"));
        if (!bind_opt->IsObject())
        {
            delete closure;
            NanThrowTypeError("optional arg 'variables' must be an object");
            NanReturnUndefined();
        }
        object_to_container(closure->variables,bind_opt->ToObject());
    }

    if (format == "pdf" || format == "svg" || format == "ps" || format == "ARGB32" || format == "RGB24") {
#if defined(HAVE_CAIRO)
        closure->use_cairo = true;
#else
        delete closure;
        std::ostringstream s("");
        s << "Cairo backend is not available, cannot write to " << format << "\n";
        NanThrowError(s.str().c_str());
        NanReturnUndefined();
#endif
    } else {
        closure->use_cairo = false;
    }

    if (!m->acquire())
    {
        delete closure;
        NanThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
        NanReturnUndefined();
    }
    closure->request.data = closure;

    closure->m = m;
    closure->scale_factor = scale_factor;
    closure->scale_denominator = scale_denominator;
    closure->buffer_size = buffer_size;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());

    closure->format = format;
    closure->palette = palette;
    closure->output = output;

    uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderFile, (uv_after_work_cb)EIO_AfterRenderFile);
    m->Ref();

    NanReturnUndefined();

}

void Map::EIO_RenderFile(uv_work_t* req)
{
    render_file_baton_t *closure = static_cast<render_file_baton_t *>(req->data);

    try
    {
        if(closure->use_cairo)
        {
#if defined(HAVE_CAIRO)
            // https://github.com/mapnik/mapnik/issues/1930
            mapnik::save_to_cairo_file(*closure->m->map_,closure->output,closure->format,closure->scale_factor,closure->scale_denominator);
#else
#endif
        }
        else
        {
            mapnik::image_rgba8 im(closure->m->map_->width(),closure->m->map_->height());
            mapnik::Map const& map = *closure->m->map_;
            mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
            m_req.set_buffer_size(closure->buffer_size);
            mapnik::agg_renderer<mapnik::image_rgba8> ren(map,
                                                   m_req,
                                                   closure->variables,
                                                   im,
                                                   closure->scale_factor);
            ren.apply(closure->scale_denominator);

            if (closure->palette.get()) {
                mapnik::save_to_file(im,closure->output,*closure->palette);
            } else {
                mapnik::save_to_file(im,closure->output);
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
    NanScope();

    render_file_baton_t *closure = static_cast<render_file_baton_t *>(req->data);

    closure->m->release();

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    } else {
        Local<Value> argv[1] = { NanNull() };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }

    closure->m->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;

}

// TODO - add support for grids
NAN_METHOD(Map::renderSync)
{
    NanScope();

    std::string format = "png";
    palette_ptr palette;
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    int buffer_size = 0;

    if (args.Length() >= 1) 
    {
        if (!args[0]->IsObject()) 
        {
            NanThrowTypeError("first argument is optional, but if provided must be an object, eg. {format: 'pdf'}");
            NanReturnUndefined();
        }

        Local<Object> options = args[0]->ToObject();
        if (options->Has(NanNew("format")))
        {
            Local<Value> format_opt = options->Get(NanNew("format"));
            if (!format_opt->IsString()) {
                NanThrowTypeError("'format' must be a String");
                NanReturnUndefined();
            }

            format = TOSTR(format_opt);
        }

        if (options->Has(NanNew("palette")))
        {
            Local<Value> format_opt = options->Get(NanNew("palette"));
            if (!format_opt->IsObject()) {
                NanThrowTypeError("'palette' must be an object");
                NanReturnUndefined();
            }

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(Palette::constructor)->HasInstance(obj)) {
                NanThrowTypeError("mapnik.Palette expected as second arg");
                NanReturnUndefined();
            }

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
        if (options->Has(NanNew("scale"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'scale' must be a number");
                NanReturnUndefined();
            }

            scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("scale_denominator"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale_denominator"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'scale_denominator' must be a number");
                NanReturnUndefined();
            }

            scale_denominator = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("buffer_size"))) {
            Local<Value> bind_opt = options->Get(NanNew("buffer_size"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'buffer_size' must be a number");
                NanReturnUndefined();
            }

            buffer_size = bind_opt->IntegerValue();
        }
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    if (!m->acquire())
    {
        NanThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
        NanReturnUndefined();
    }
    std::string s;
    try
    {
        mapnik::image_rgba8 im(m->map_->width(),m->map_->height());
        mapnik::Map const& map = *m->map_;
        mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
        m_req.set_buffer_size(buffer_size);
        mapnik::agg_renderer<mapnik::image_rgba8> ren(map,
                                                   m_req,
                                                   mapnik::attributes(),
                                                   im,
                                                   scale_factor);
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
        m->release();
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    m->release();
    NanReturnValue(NanNewBufferHandle((char*)s.data(), s.size()));
}

NAN_METHOD(Map::renderFileSync)
{
    NanScope();
    if (args.Length() < 1 || !args[0]->IsString()) {
        NanThrowTypeError("first argument must be a path to a file to save");
        NanReturnUndefined();
    }

    if (args.Length() > 2) {
        NanThrowError("accepts two arguments, a required path to a file, an optional options object, eg. {format: 'pdf'}");
        NanReturnUndefined();
    }

    // defaults
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    int buffer_size = 0;
    std::string format = "png";
    palette_ptr palette;

    if (args.Length() >= 2){
        if (!args[1]->IsObject()) {
            NanThrowTypeError("second argument is optional, but if provided must be an object, eg. {format: 'pdf'}");
            NanReturnUndefined();
        }

        Local<Object> options = args[1].As<Object>();
        if (options->Has(NanNew("format")))
        {
            Local<Value> format_opt = options->Get(NanNew("format"));
            if (!format_opt->IsString()) {
                NanThrowTypeError("'format' must be a String");
                NanReturnUndefined();
            }

            format = TOSTR(format_opt);
        }

        if (options->Has(NanNew("palette")))
        {
            Local<Value> format_opt = options->Get(NanNew("palette"));
            if (!format_opt->IsObject()) {
                NanThrowTypeError("'palette' must be an object");
                NanReturnUndefined();
            }

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(Palette::constructor)->HasInstance(obj)) {
                NanThrowTypeError("mapnik.Palette expected as second arg");
                NanReturnUndefined();
            }

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
        if (options->Has(NanNew("scale"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'scale' must be a number");
                NanReturnUndefined();
            }

            scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("scale_denominator"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale_denominator"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'scale_denominator' must be a number");
                NanReturnUndefined();
            }

            scale_denominator = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("buffer_size"))) {
            Local<Value> bind_opt = options->Get(NanNew("buffer_size"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'buffer_size' must be a number");
                NanReturnUndefined();
            }

            buffer_size = bind_opt->IntegerValue();
        }
    }

    Map* m = node::ObjectWrap::Unwrap<Map>(args.Holder());
    std::string output = TOSTR(args[0]);

    if (format.empty()) {
        format = mapnik::guess_type(output);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << output << "\n";
            NanThrowError(s.str().c_str());
            NanReturnUndefined();
        }
    }
    if (!m->acquire())
    {
        NanThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
        NanReturnUndefined();
    }

    try
    {

        if (format == "pdf" || format == "svg" || format =="ps" || format == "ARGB32" || format == "RGB24")
        {
#if defined(HAVE_CAIRO)
            mapnik::save_to_cairo_file(*m->map_,output,format,scale_factor,scale_denominator);
#else
            std::ostringstream s("");
            s << "Cairo backend is not available, cannot write to " << format << "\n";
            m->release();
            NanThrowError(s.str().c_str());
            NanReturnUndefined();
#endif
        }
        else
        {
            mapnik::image_rgba8 im(m->map_->width(),m->map_->height());
            mapnik::Map const& map = *m->map_;
            mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
            m_req.set_buffer_size(buffer_size);
            mapnik::agg_renderer<mapnik::image_rgba8> ren(map,
                                                   m_req,
                                                   mapnik::attributes(),
                                                   im,
                                                   scale_factor);

            ren.apply(scale_denominator);

            if (palette.get())
            {
                mapnik::save_to_file(im,output,*palette);
            }
            else {
                mapnik::save_to_file(im,output);
            }
        }
    }
    catch (std::exception const& ex)
    {
        m->release();
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    m->release();
    NanReturnUndefined();
}
