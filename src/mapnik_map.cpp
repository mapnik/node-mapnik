#include "mapnik_map.hpp"
#include "utils.hpp"
#include "mapnik_color.hpp"             // for Color, Color::constructor
#include "mapnik_featureset.hpp"        // for Featureset
#if defined(GRID_RENDERER)
#include "mapnik_grid.hpp"              // for Grid, Grid::constructor
#endif
#include "mapnik_image.hpp"             // for Image, Image::constructor
#include "mapnik_layer.hpp"             // for Layer, Layer::constructor
#include "mapnik_palette.hpp"           // for palette_ptr, Palette, etc
#include "mapnik_vector_tile.hpp"
#include "object_to_container.hpp"

// mapnik-vector-tile
#include "vector_tile_processor.hpp"

// mapnik
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
#include <mapnik/geometry/box2d.hpp>             // for box2d
#include <mapnik/color.hpp>             // for color
#include <mapnik/attribute.hpp>        // for attributes
#include <mapnik/featureset.hpp>        // for featureset_ptr
#if defined(GRID_RENDERER)
#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp>  // for grid_renderer
#endif
#include <mapnik/image.hpp>             // for image_rgba8
#include <mapnik/image_any.hpp>
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
#include <ostream>                      // for operator<<, basic_ostream, etc
#include <sstream>                      // for basic_ostringstream, etc

// boost
#include <boost/optional/optional.hpp>  // for optional

Nan::Persistent<v8::FunctionTemplate> Map::constructor;

/**
 * **`mapnik.Map`**
 *
 * A map in mapnik is an object that combines data sources and styles in
 * a way that lets you produce styled cartographic output.
 *
 * @class Map
 * @param {int} width in pixels
 * @param {int} height in pixels
 * @param {string} [projection='+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs'] projection as a proj4 code
 * typically used with '+init=epsg:3857'
 * @property {string} src
 * @property {number} width
 * @property {number} height
 * @property {number} bufferSize
 * @property {Array<number>} extent - extent of the map as an array `[ minx, miny, maxx, maxy ]`
 * @property {Array<number>} bufferedExtent - extent of the map's buffer `[ minx, miny, maxx, maxy ]`
 * @property {Array<number>} maximumExtent - combination of extent and bufferedExtent `[ minx, miny, maxx, maxy ]`
 * @property {mapnik.Color} background - background color as a {@link mapnik.Color} object
 * @property {} parameters
 * @property {} aspect_fix_mode
 * @example
 * var map = new mapnik.Map(600, 400, '+init=epsg:3857');
 * console.log(map);
 * // {
 * //   aspect_fix_mode: 0,
 * //   parameters: {},
 * //   background: undefined,
 * //   maximumExtent: undefined,
 * //   bufferedExtent: [ NaN, NaN, NaN, NaN ],
 * //   extent:
 * //   [ 1.7976931348623157e+308,
 * //    1.7976931348623157e+308,
 * //    -1.7976931348623157e+308,
 * //    -1.7976931348623157e+308 ],
 * //   bufferSize: 0,
 * //   height: 400,
 * //   width: 600,
 * //   srs: '+init=epsg:3857'
 * // }
 */
void Map::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Map::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Map").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "fonts", fonts);
    Nan::SetPrototypeMethod(lcons, "fontFiles", fontFiles);
    Nan::SetPrototypeMethod(lcons, "fontDirectory", fontDirectory);
    Nan::SetPrototypeMethod(lcons, "loadFonts", loadFonts);
    Nan::SetPrototypeMethod(lcons, "memoryFonts", memoryFonts);
    Nan::SetPrototypeMethod(lcons, "registerFonts", registerFonts);
    Nan::SetPrototypeMethod(lcons, "load", load);
    Nan::SetPrototypeMethod(lcons, "loadSync", loadSync);
    Nan::SetPrototypeMethod(lcons, "fromStringSync", fromStringSync);
    Nan::SetPrototypeMethod(lcons, "fromString", fromString);
    Nan::SetPrototypeMethod(lcons, "clone", clone);
    Nan::SetPrototypeMethod(lcons, "save", save);
    Nan::SetPrototypeMethod(lcons, "clear", clear);
    Nan::SetPrototypeMethod(lcons, "toXML", toXML);
    Nan::SetPrototypeMethod(lcons, "resize", resize);


    Nan::SetPrototypeMethod(lcons, "render", render);
    Nan::SetPrototypeMethod(lcons, "renderSync", renderSync);
    Nan::SetPrototypeMethod(lcons, "renderFile", renderFile);
    Nan::SetPrototypeMethod(lcons, "renderFileSync", renderFileSync);

    Nan::SetPrototypeMethod(lcons, "zoomAll", zoomAll);
    Nan::SetPrototypeMethod(lcons, "zoomToBox", zoomToBox); //setExtent
    Nan::SetPrototypeMethod(lcons, "scale", scale);
    Nan::SetPrototypeMethod(lcons, "scaleDenominator", scaleDenominator);
    Nan::SetPrototypeMethod(lcons, "queryPoint", queryPoint);
    Nan::SetPrototypeMethod(lcons, "queryMapPoint", queryMapPoint);

    // layer access
    Nan::SetPrototypeMethod(lcons, "add_layer", add_layer);
    Nan::SetPrototypeMethod(lcons, "remove_layer", remove_layer);
    Nan::SetPrototypeMethod(lcons, "get_layer", get_layer);
    Nan::SetPrototypeMethod(lcons, "layers", layers);

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

    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "ASPECT_GROW_BBOX",mapnik::Map::GROW_BBOX)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "ASPECT_GROW_CANVAS",mapnik::Map::GROW_CANVAS)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "ASPECT_SHRINK_BBOX",mapnik::Map::SHRINK_BBOX)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "ASPECT_SHRINK_CANVAS",mapnik::Map::SHRINK_CANVAS)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "ASPECT_ADJUST_BBOX_WIDTH",mapnik::Map::ADJUST_BBOX_WIDTH)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "ASPECT_ADJUST_BBOX_HEIGHT",mapnik::Map::ADJUST_BBOX_HEIGHT)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "ASPECT_ADJUST_CANVAS_WIDTH",mapnik::Map::ADJUST_CANVAS_WIDTH)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "ASPECT_ADJUST_CANVAS_HEIGHT",mapnik::Map::ADJUST_CANVAS_HEIGHT)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "ASPECT_RESPECT",mapnik::Map::RESPECT)
    Nan::Set(target, Nan::New("Map").ToLocalChecked(),Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

Map::Map(int width, int height) :
    Nan::ObjectWrap(),
    map_(std::make_shared<mapnik::Map>(width,height)),
    in_use_(false) {}

Map::Map(int width, int height, std::string const& srs) :
    Nan::ObjectWrap(),
    map_(std::make_shared<mapnik::Map>(width,height,srs)),
    in_use_(false) {}

Map::Map() :
    Nan::ObjectWrap(),
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
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    // accept a reference or v8:External?
    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = info[0].As<v8::External>();
        void* ptr = ext->Value();
        Map* m =  static_cast<Map*>(ptr);
        m->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }

    if (info.Length() == 2)
    {
        if (!info[0]->IsNumber() || !info[1]->IsNumber())
        {
            Nan::ThrowTypeError("'width' and 'height' must be integers");
            return;
        }
        Map* m = new Map(Nan::To<int>(info[0]).FromJust(),Nan::To<int>(info[1]).FromJust());
        m->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    else if (info.Length() == 3)
    {
        if (!info[0]->IsNumber() || !info[1]->IsNumber())
        {
            Nan::ThrowTypeError("'width' and 'height' must be integers");
            return;
        }
        if (!info[2]->IsString())
        {
            Nan::ThrowError("'srs' value must be a string");
            return;
        }
        Map* m = new Map(Nan::To<int>(info[0]).FromJust(), Nan::To<int>(info[1]).FromJust(), TOSTR(info[2]));
        m->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    else
    {
        Nan::ThrowError("please provide Map width and height and optional srs");
        return;
    }
    return;
}

NAN_GETTER(Map::get_prop)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    std::string a = TOSTR(property);
    if(a == "extent") {
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
        mapnik::box2d<double> const& e = m->map_->get_current_extent();
        Nan::Set(arr, 0, Nan::New<v8::Number>(e.minx()));
        Nan::Set(arr, 1, Nan::New<v8::Number>(e.miny()));
        Nan::Set(arr, 2, Nan::New<v8::Number>(e.maxx()));
        Nan::Set(arr, 3, Nan::New<v8::Number>(e.maxy()));
        info.GetReturnValue().Set(arr);
    }
    else if(a == "bufferedExtent") {
        boost::optional<mapnik::box2d<double> > const& e = m->map_->get_buffered_extent();
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
        Nan::Set(arr, 0, Nan::New<v8::Number>(e->minx()));
        Nan::Set(arr, 1, Nan::New<v8::Number>(e->miny()));
        Nan::Set(arr, 2, Nan::New<v8::Number>(e->maxx()));
        Nan::Set(arr, 3, Nan::New<v8::Number>(e->maxy()));
        info.GetReturnValue().Set(arr);
    }
    else if(a == "maximumExtent") {
        boost::optional<mapnik::box2d<double> > const& e = m->map_->maximum_extent();
        if (!e)
            return;
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
        Nan::Set(arr, 0, Nan::New<v8::Number>(e->minx()));
        Nan::Set(arr, 1, Nan::New<v8::Number>(e->miny()));
        Nan::Set(arr, 2, Nan::New<v8::Number>(e->maxx()));
        Nan::Set(arr, 3, Nan::New<v8::Number>(e->maxy()));
        info.GetReturnValue().Set(arr);
    }
    else if(a == "aspect_fix_mode")
        info.GetReturnValue().Set(Nan::New<v8::Integer>(m->map_->get_aspect_fix_mode()));
    else if(a == "width")
        info.GetReturnValue().Set(Nan::New<v8::Integer>(m->map_->width()));
    else if(a == "height")
        info.GetReturnValue().Set(Nan::New<v8::Integer>(m->map_->height()));
    else if (a == "srs")
        info.GetReturnValue().Set(Nan::New<v8::String>(m->map_->srs()).ToLocalChecked());
    else if(a == "bufferSize")
        info.GetReturnValue().Set(Nan::New<v8::Integer>(m->map_->buffer_size()));
    else if (a == "background") {
        boost::optional<mapnik::color> c = m->map_->background();
        if (c)
            info.GetReturnValue().Set(Color::NewInstance(*c));
        else
            return;
    }
    else //if (a == "parameters")
    {
        v8::Local<v8::Object> ds = Nan::New<v8::Object>();
        mapnik::parameters const& params = m->map_->get_extra_parameters();
        mapnik::parameters::const_iterator it = params.begin();
        mapnik::parameters::const_iterator end = params.end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object(ds, it->first, it->second);
        }
        info.GetReturnValue().Set(ds);
    }
}

NAN_SETTER(Map::set_prop)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    std::string a = TOSTR(property);
    if(a == "extent" || a == "maximumExtent") {
        if (!value->IsArray()) {
            Nan::ThrowError("Must provide an array of: [minx,miny,maxx,maxy]");
            return;
        } else {
            v8::Local<v8::Array> arr = value.As<v8::Array>();
            if (arr->Length() != 4) {
                Nan::ThrowError("Must provide an array of: [minx,miny,maxx,maxy]");
                return;
            } else {
                double minx = Nan::To<double>(Nan::Get(arr, 0).ToLocalChecked()).FromJust();
                double miny = Nan::To<double>(Nan::Get(arr, 1).ToLocalChecked()).FromJust();
                double maxx = Nan::To<double>(Nan::Get(arr, 2).ToLocalChecked()).FromJust();
                double maxy = Nan::To<double>(Nan::Get(arr, 3).ToLocalChecked()).FromJust();
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
            Nan::ThrowError("'aspect_fix_mode' must be a constant (number)");
            return;
        } else {
            int val = Nan::To<int>(value).FromJust();
            if (val < mapnik::Map::aspect_fix_mode_MAX && val >= 0) {
                m->map_->set_aspect_fix_mode(static_cast<mapnik::Map::aspect_fix_mode>(val));
            } else {
                Nan::ThrowError("'aspect_fix_mode' value is invalid");
                return;
            }
        }
    }
    else if (a == "srs")
    {
        if (!value->IsString()) {
            Nan::ThrowError("'srs' must be a string");
            return;
        } else {
            m->map_->set_srs(TOSTR(value));
        }
    }
    else if (a == "bufferSize") {
        if (!value->IsNumber()) {
            Nan::ThrowTypeError("Must provide an integer bufferSize");
            return;
        } else {
            m->map_->set_buffer_size(Nan::To<int>(value).FromJust());
        }
    }
    else if (a == "width") {
        if (!value->IsNumber()) {
            Nan::ThrowTypeError("Must provide an integer width");
            return;
        } else {
            m->map_->set_width(Nan::To<int>(value).FromJust());
        }
    }
    else if (a == "height") {
        if (!value->IsNumber()) {
            Nan::ThrowTypeError("Must provide an integer height");
            return;
        } else {
            m->map_->set_height(Nan::To<int>(value).FromJust());
        }
    }
    else if (a == "background") {
        if (!value->IsObject()) {
            Nan::ThrowTypeError("mapnik.Color expected");
            return;
        }

        v8::Local<v8::Object> obj = value.As<v8::Object>();
        if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Color::constructor)->HasInstance(obj)) {
            Nan::ThrowTypeError("mapnik.Color expected");
            return;
        }
        Color *c = Nan::ObjectWrap::Unwrap<Color>(obj);
        m->map_->set_background(*c->get());
    }
    else if (a == "parameters") {
        if (!value->IsObject()) {
            Nan::ThrowTypeError("object expected for map.parameters");
            return;
        }

        v8::Local<v8::Object> obj = value->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        mapnik::parameters params;
        v8::Local<v8::Array> names = Nan::GetPropertyNames(obj).ToLocalChecked();
        unsigned int i = 0;
        unsigned int a_length = names->Length();
        while (i < a_length) {
            v8::Local<v8::Value> name = Nan::Get(names, i).ToLocalChecked()->ToString(Nan::GetCurrentContext()).ToLocalChecked();
            v8::Local<v8::Value> a_value = Nan::Get(obj, name).ToLocalChecked();
            if (a_value->IsString()) {
                params[TOSTR(name)] = const_cast<char const*>(TOSTR(a_value));
            } else if (a_value->IsNumber()) {
                double num = Nan::To<double>(a_value).FromJust();
                // todo - round
                if (num == Nan::To<int>(a_value).FromJust()) {
                    params[TOSTR(name)] = Nan::To<node_mapnik::value_integer>(a_value).FromJust();
                } else {
                    double dub_val = Nan::To<double>(a_value).FromJust();
                    params[TOSTR(name)] = dub_val;
                }
            } else if (a_value->IsBoolean()) {
                params[TOSTR(name)] = Nan::To<mapnik::value_bool>(a_value).FromJust();
            }
            i++;
        }
        m->map_->set_extra_parameters(params);
    }
}

/**
 * Load fonts from local or external source
 *
 * @name loadFonts
 * @memberof Map
 * @instance
 *
 */
NAN_METHOD(Map::loadFonts)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(m->map_->load_fonts()));
}

NAN_METHOD(Map::memoryFonts)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    auto const& font_cache = m->map_->get_font_memory_cache();
    v8::Local<v8::Array> a = Nan::New<v8::Array>(font_cache.size());
    unsigned i = 0;
    for (auto const& kv : font_cache)
    {
        Nan::Set(a, i++, Nan::New(kv.first).ToLocalChecked());
    }
    info.GetReturnValue().Set(a);
}

NAN_METHOD(Map::registerFonts)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    if (info.Length() == 0 || !info[0]->IsString())
    {
        Nan::ThrowTypeError("first argument must be a path to a directory of fonts");
        return;
    }

    bool recurse = false;

    if (info.Length() >= 2)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("second argument is optional, but if provided must be an object, eg. { recurse: true }");
            return;
        }
        v8::Local<v8::Object> options = info[1].As<v8::Object>();
        if (Nan::Has(options, Nan::New("recurse").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> recurse_opt = Nan::Get(options, Nan::New("recurse").ToLocalChecked()).ToLocalChecked();
            if (!recurse_opt->IsBoolean())
            {
                Nan::ThrowTypeError("'recurse' must be a Boolean");
                return;
            }
            recurse = Nan::To<bool>(recurse_opt).FromJust();
        }
    }
    std::string path = TOSTR(info[0]);
    info.GetReturnValue().Set(Nan::New(m->map_->register_fonts(path,recurse)));
}

/**
 * Get all of the fonts currently registered as part of this map
 * @memberof Map
 * @instance
 * @name font
 * @returns {Array<string>} fonts
 */
NAN_METHOD(Map::fonts)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    auto const& mapping = m->map_->get_font_file_mapping();
    v8::Local<v8::Array> a = Nan::New<v8::Array>(mapping.size());
    unsigned i = 0;
    for (auto const& kv : mapping)
    {
        Nan::Set(a, i++, Nan::New<v8::String>(kv.first).ToLocalChecked());
    }
    info.GetReturnValue().Set(a);
}

/**
 * Get all of the fonts currently registered as part of this map, as a mapping
 * from font to font file
 * @memberof Map
 * @instance
 * @name fontFiles
 * @returns {Object} fonts
 */
NAN_METHOD(Map::fontFiles)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    auto const& mapping = m->map_->get_font_file_mapping();
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    for (auto const& kv : mapping)
    {
        Nan::Set(obj, Nan::New<v8::String>(kv.first).ToLocalChecked(), Nan::New<v8::String>(kv.second.second).ToLocalChecked());
    }
    info.GetReturnValue().Set(obj);
}

/**
 * Get the currently-registered font directory, if any
 * @memberof Map
 * @instance
 * @name fontDirectory
 * @returns {string|undefined} fonts
 */
NAN_METHOD(Map::fontDirectory)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    boost::optional<std::string> const& fdir = m->map_->font_directory();
    if (fdir)
    {
        info.GetReturnValue().Set(Nan::New<v8::String>(*fdir).ToLocalChecked());
    }
    return;
}

/**
 * Get the map's scale factor. This is the ratio between pixels and geographical
 * units like meters.
 * @memberof Map
 * @instance
 * @name scale
 * @returns {number} scale
 */
NAN_METHOD(Map::scale)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(m->map_->scale()));
}

/**
 * Get the map's scale denominator.
 *
 * @memberof Map
 * @instance
 * @name scaleDenominator
 * @returns {number} scale denominator
 */
NAN_METHOD(Map::scaleDenominator)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(m->map_->scale_denominator()));
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
    Nan::Persistent<v8::Function> cb;
} query_map_baton_t;

/**
 * Query a `Mapnik#Map` object to retrieve layer and feature data based on an
 * X and Y `Mapnik#Map` coordinates (use `Map#queryPoint` to query with geographic coordinates).
 *
 * @name queryMapPoint
 * @memberof Map
 * @instance
 * @param {number} x - x coordinate
 * @param {number} y - y coordinate
 * @param {Object} [options]
 * @param {String|number} [options.layer] - layer name (string) or index (positive integer, 0 index)
 * to query. If left blank, will query all layers.
 * @param {Function} callback
 * @returns {Array} array - An array of `Featureset` objects and layer names, which each contain their own
 * `Feature` objects.
 * @example
 * // iterate over the first layer returned and get all attribute information for each feature
 * map.queryMapPoint(10, 10, {layer: 0}, function(err, results) {
 *   if (err) throw err;
 *   console.log(results); // => [{"layer":"layer_name","featureset":{}}]
 *   var featureset = results[0].featureset;
 *   var attributes = [];
 *   var feature;
 *   while ((feature = featureset.next())) {
 *     attributes.push(feature.attributes());
 *   }
 *   console.log(attributes); // => [{"attr_key": "attr_value"}, {...}, {...}]
 * });
 *
 */
NAN_METHOD(Map::queryMapPoint)
{
    abstractQueryPoint(info,false);
    return;
}

/**
 * Query a `Mapnik#Map` object to retrieve layer and feature data based on geographic
 * coordinates of the source data (use `Map#queryMapPoint` to query with XY coordinates).
 *
 * @name queryPoint
 * @memberof Map
 * @instance
 * @param {number} x - x geographic coordinate (CRS based on source data)
 * @param {number} y - y geographic coordinate (CRS based on source data)
 * @param {Object} [options]
 * @param {String|number} [options.layer] - layer name (string) or index (positive integer, 0 index)
 * to query. If left blank, will query all layers.
 * @param {Function} callback
 * @returns {Array} array - An array of `Featureset` objects and layer names, which each contain their own
 * `Feature` objects.
 * @example
 * // query based on web mercator coordinates
 * map.queryMapPoint(-12957605.0331, 5518141.9452, {layer: 0}, function(err, results) {
 *   if (err) throw err;
 *   console.log(results); // => [{"layer":"layer_name","featureset":{}}]
 *   var featureset = results[0].featureset;
 *   var attributes = [];
 *   var feature;
 *   while ((feature = featureset.next())) {
 *     attributes.push(feature.attributes());
 *   }
 *   console.log(attributes); // => [{"attr_key": "attr_value"}, {...}, {...}]
 * });
 *
 */
NAN_METHOD(Map::queryPoint)
{
    abstractQueryPoint(info,true);
    return;
}

v8::Local<v8::Value> Map::abstractQueryPoint(Nan::NAN_METHOD_ARGS_TYPE info, bool geo_coords)
{
    Nan::HandleScope scope;
    if (info.Length() < 3)
    {
        Nan::ThrowError("requires at least three arguments, a x,y query and a callback");
        return Nan::Undefined();
    }

    double x,y;
    if (!info[0]->IsNumber() || !info[1]->IsNumber())
    {
        Nan::ThrowTypeError("x,y arguments must be numbers");
        return Nan::Undefined();
    }
    else
    {
        x = Nan::To<double>(info[0]).FromJust();
        y = Nan::To<double>(info[1]).FromJust();
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());

    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    int layer_idx = -1;

    if (info.Length() > 3)
    {
        // options object
        if (!info[2]->IsObject()) {
            Nan::ThrowTypeError("optional third argument must be an options object");
            return Nan::Undefined();
        }

        options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::Has(options, Nan::New("layer").ToLocalChecked()).FromMaybe(false))
        {
            std::vector<mapnik::layer> const& layers = m->map_->layers();
            v8::Local<v8::Value> layer_id = Nan::Get(options, Nan::New("layer").ToLocalChecked()).ToLocalChecked();
            if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
                Nan::ThrowTypeError("'layer' option required for map query and must be either a layer name(string) or layer index (integer)");
                return Nan::Undefined();
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
                    Nan::ThrowTypeError(s.str().c_str());
                    return Nan::Undefined();
                }
            }
            else if (layer_id->IsNumber())
            {
                layer_idx = Nan::To<int>(layer_id).FromJust();
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
                    Nan::ThrowTypeError(s.str().c_str());
                    return Nan::Undefined();
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
                    Nan::ThrowTypeError(s.str().c_str());
                    return Nan::Undefined();
                }
            }
        }
    }

    // ensure function callback
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return Nan::Undefined();
    }

    query_map_baton_t *closure = new query_map_baton_t();
    closure->request.data = closure;
    closure->m = m;
    closure->x = x;
    closure->y = y;
    closure->layer_idx = static_cast<std::size_t>(layer_idx);
    closure->geo_coords = geo_coords;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_QueryMap, (uv_after_work_cb)EIO_AfterQueryMap);
    m->Ref();
    return Nan::Undefined();
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
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    query_map_baton_t *closure = static_cast<query_map_baton_t *>(req->data);
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    } else {
        std::size_t num_result = closure->featuresets.size();
        if (num_result >= 1)
        {
            v8::Local<v8::Array> a = Nan::New<v8::Array>(num_result);
            typedef std::map<std::string,mapnik::featureset_ptr> fs_itr;
            fs_itr::const_iterator it = closure->featuresets.begin();
            fs_itr::const_iterator end = closure->featuresets.end();
            unsigned idx = 0;
            for (; it != end; ++it)
            {
                v8::Local<v8::Object> obj = Nan::New<v8::Object>();
                Nan::Set(obj, Nan::New("layer").ToLocalChecked(), Nan::New<v8::String>(it->first).ToLocalChecked());
                Nan::Set(obj, Nan::New("featureset").ToLocalChecked(), Featureset::NewInstance(it->second));
                Nan::Set(a, idx, obj);
                ++idx;
            }
            closure->featuresets.clear();
            v8::Local<v8::Value> argv[2] = { Nan::Null(), a };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::Undefined() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }

    closure->m->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Get all of the currently-added layers in this map
 *
 * @memberof Map
 * @instance
 * @name layers
 * @returns {Array<mapnik.Layer>} layers
 */
NAN_METHOD(Map::layers)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    std::vector<mapnik::layer> const& layers = m->map_->layers();
    v8::Local<v8::Array> a = Nan::New<v8::Array>(layers.size());
    for (unsigned i = 0; i < layers.size(); ++i )
    {
        Nan::Set(a, i, Layer::NewInstance(layers[i]));
    }
    info.GetReturnValue().Set(a);
}

/**
 * Add a new layer to this map
 *
 * @memberof Map
 * @instance
 * @name add_layer
 * @param {mapnik.Layer} new layer
 */
NAN_METHOD(Map::add_layer) {
    if (!info[0]->IsObject()) {
        Nan::ThrowTypeError("mapnik.Layer expected");
        return;
    }

    v8::Local<v8::Object> obj = info[0].As<v8::Object>();
    if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Layer::constructor)->HasInstance(obj)) {
        Nan::ThrowTypeError("mapnik.Layer expected");
        return;
    }
    Layer *l = Nan::ObjectWrap::Unwrap<Layer>(obj);
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    m->map_->add_layer(*l->get());
    return;
}

/**
 * Remove layer from this map
 *
 * @memberof Map
 * @instance
 * @name remove_layer
 * @param {number} layer index
 */
NAN_METHOD(Map::remove_layer) {
    if (info.Length() != 1) {
        Nan::ThrowError("Please provide layer index");
        return;
    }

    if (!info[0]->IsNumber()) {
        Nan::ThrowTypeError("index must be number");
        return;
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    std::vector<mapnik::layer> const& layers = m->map_->layers();

    unsigned int index = Nan::To<int>(info[0]).FromJust();

    if (index < layers.size()) {
        m->map_->remove_layer(index);
        return;
    }

    Nan::ThrowTypeError("invalid layer index");
}

/**
 * Get a layer out of this map, given a name or index
 *
 * @memberof Map
 * @instance
 * @name get_layer
 * @param {string|number} layer name or index
 * @returns {mapnik.Layer} the layer
 * @throws {Error} if index is incorrect or layer is not found
 */
NAN_METHOD(Map::get_layer)
{
    if (info.Length() != 1) {
        Nan::ThrowError("Please provide layer name or index");
        return;
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    std::vector<mapnik::layer> const& layers = m->map_->layers();

    v8::Local<v8::Value> layer = info[0];
    if (layer->IsNumber())
    {
        unsigned int index = Nan::To<int>(info[0]).FromJust();

        if (index < layers.size())
        {
            info.GetReturnValue().Set(Layer::NewInstance(layers[index]));
            return;
        }
        else
        {
            Nan::ThrowTypeError("invalid layer index");
            return;
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
                info.GetReturnValue().Set(Layer::NewInstance(layers[idx]));
                return;
            }
            ++idx;
        }
        if (!found)
        {
            std::ostringstream s;
            s << "Layer name '" << layer_name << "' not found";
            Nan::ThrowTypeError(s.str().c_str());
            return;
        }

    }
    Nan::ThrowTypeError("first argument must be either a layer name(string) or layer index (integer)");
    return;
}

/**
 * Remove all layers and styles from this map
 *
 * @memberof Map
 * @instance
 * @name clear
 */
NAN_METHOD(Map::clear)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    m->map_->remove_all();
    return;
}

/**
 * Give this map new dimensions
 *
 * @memberof Map
 * @instance
 * @name resize
 * @param {number} width
 * @param {number} height
 */
NAN_METHOD(Map::resize)
{
    if (info.Length() != 2) {
        Nan::ThrowError("Please provide width and height");
        return;
    }

    if (!info[0]->IsNumber() || !info[1]->IsNumber()) {
        Nan::ThrowTypeError("width and height must be integers");
        return;
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    m->map_->resize(Nan::To<int>(info[0]).FromJust(),Nan::To<int>(info[1]).FromJust());
    return;
}


typedef struct {
    uv_work_t request;
    Map *m;
    std::string stylesheet;
    std::string base_path;
    bool strict;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} load_xml_baton_t;


/**
 * Load styles, layers, and other information for this map from a Mapnik
 * XML stylesheet.
 *
 * @memberof Map
 * @instance
 * @name load
 * @param {string} stylesheet path
 * @param {Object} [options={}]
 * @param {Function} callback
 */
NAN_METHOD(Map::load)
{
    if (info.Length() < 2) {
        Nan::ThrowError("please provide a stylesheet path, options, and callback");
        return;
    }

    // ensure stylesheet path is a string
    v8::Local<v8::Value> stylesheet = info[0];
    if (!stylesheet->IsString()) {
        Nan::ThrowTypeError("first argument must be a path to a mapnik stylesheet");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!callback->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    // ensure options object
    if (!info[1]->IsObject()) {
        Nan::ThrowTypeError("options must be an object, eg {strict: true}");
        return;
    }

    v8::Local<v8::Object> options = info[1].As<v8::Object>();

    bool strict = false;
    v8::Local<v8::String> param = Nan::New("strict").ToLocalChecked();
    if (Nan::Has(options, param).ToChecked())
    {
        v8::Local<v8::Value> param_val = Nan::Get(options, param).ToLocalChecked();
        if (!param_val->IsBoolean()) {
            Nan::ThrowTypeError("'strict' must be a Boolean");
            return;
        }
        strict = Nan::To<bool>(param_val).FromJust();
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());

    load_xml_baton_t *closure = new load_xml_baton_t();
    closure->request.data = closure;

    param = Nan::New("base").ToLocalChecked();
    if (Nan::Has(options, param).ToChecked())
    {
        v8::Local<v8::Value> param_val = Nan::Get(options, param).ToLocalChecked();
        if (!param_val->IsString()) {
            Nan::ThrowTypeError("'base' must be a string representing a filesystem path");
            return;
        }
        closure->base_path = TOSTR(param_val);
    }

    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Load, (uv_after_work_cb)EIO_AfterLoad);
    m->Ref();
    return;
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
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    } else {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->m->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->cb.Reset();
    delete closure;
}


/**
 * Load styles, layers, and other information for this map from a Mapnik
 * XML stylesheet.
 *
 * @memberof Map
 * @instance
 * @name loadSync
 * @param {string} stylesheet path
 * @param {Object} [options={}]
 * @example
 * map.loadSync('./style.xml');
 */
NAN_METHOD(Map::loadSync)
{
    if (!info[0]->IsString()) {
        Nan::ThrowTypeError("first argument must be a path to a mapnik stylesheet");
        return;
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    std::string stylesheet = TOSTR(info[0]);
    bool strict = false;
    std::string base_path;

    if (info.Length() > 2)
    {

        Nan::ThrowError("only accepts two arguments: a path to a mapnik stylesheet and an optional options object");
        return;
    }
    else if (info.Length() == 2)
    {
        // ensure options object
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("options must be an object, eg {strict: true}");
            return;
        }

        v8::Local<v8::Object> options = info[1].As<v8::Object>();

        v8::Local<v8::String> param = Nan::New("strict").ToLocalChecked();
        if (Nan::Has(options, param).ToChecked())
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, param).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("'strict' must be a Boolean");
                return;
            }
            strict = Nan::To<bool>(param_val).FromJust();
        }

        param = Nan::New("base").ToLocalChecked();
        if (Nan::Has(options, param).ToChecked())
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, param).ToLocalChecked();
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("'base' must be a string representing a filesystem path");
                return;
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
        Nan::ThrowError(ex.what());
        return;
    }
    return;
}

/**
 * Load styles, layers, and other information for this map from a Mapnik
 * XML stylesheet given as a string.
 *
 * @memberof Map
 * @instance
 * @name fromStringSync
 * @param {string} stylesheet contents
 * @param {Object} [options={}]
 * @example
 * var fs = require('fs');
 * map.fromStringSync(fs.readFileSync('./style.xml', 'utf8'));
 */
NAN_METHOD(Map::fromStringSync)
{
    if (info.Length() < 1) {
        Nan::ThrowError("Accepts 2 arguments: stylesheet string and an optional options");
        return;
    }

    if (!info[0]->IsString()) {
        Nan::ThrowTypeError("first argument must be a mapnik stylesheet string");
        return;
    }


    // defaults
    bool strict = false;
    std::string base_path("");

    if (info.Length() >= 2) {
        // ensure options object
        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("options must be an object, eg {strict: true, base: \".\"'}");
            return;
        }

        v8::Local<v8::Object> options = info[1].As<v8::Object>();

        v8::Local<v8::String> param = Nan::New("strict").ToLocalChecked();
        if (Nan::Has(options, param).ToChecked())
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, param).ToLocalChecked();
            if (!param_val->IsBoolean()) {
                Nan::ThrowTypeError("'strict' must be a Boolean");
                return;
            }
            strict = Nan::To<bool>(param_val).FromJust();
        }

        param = Nan::New("base").ToLocalChecked();
        if (Nan::Has(options, param).ToChecked())
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, param).ToLocalChecked();
            if (!param_val->IsString()) {
                Nan::ThrowTypeError("'base' must be a string representing a filesystem path");
                return;
            }
            base_path = TOSTR(param_val);
        }
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());

    std::string stylesheet = TOSTR(info[0]);

    try
    {
        mapnik::load_map_string(*m->map_,stylesheet,strict,base_path);
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
    return;
}

/**
 * Load styles, layers, and other information for this map from a Mapnik
 * XML stylesheet given as a string.
 *
 * @memberof Map
 * @instance
 * @name fromString
 * @param {string} stylesheet contents
 * @param {Object} [options={}]
 * @param {Function} callback
 * @example
 * var fs = require('fs');
 * map.fromString(fs.readFileSync('./style.xml', 'utf8'), function(err, res) {
 *   // details loaded
 * });
 */
NAN_METHOD(Map::fromString)
{
    if (info.Length() < 2)
    {
        Nan::ThrowError("please provide a stylesheet string, options, and callback");
        return;
    }

    // ensure stylesheet path is a string
    v8::Local<v8::Value> stylesheet = info[0];
    if (!stylesheet->IsString())
    {
        Nan::ThrowTypeError("first argument must be a path to a mapnik stylesheet string");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    // ensure options object
    if (!info[1]->IsObject())
    {
        Nan::ThrowTypeError("options must be an object, eg {strict: true, base: \".\"'}");
        return;
    }

    v8::Local<v8::Object> options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

    bool strict = false;
    v8::Local<v8::String> param = Nan::New("strict").ToLocalChecked();
    if (Nan::Has(options, param).ToChecked())
    {
        v8::Local<v8::Value> param_val = Nan::Get(options, param).ToLocalChecked();
        if (!param_val->IsBoolean())
        {
            Nan::ThrowTypeError("'strict' must be a Boolean");
            return;
        }
        strict = Nan::To<bool>(param_val).FromJust();
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());

    load_xml_baton_t *closure = new load_xml_baton_t();
    closure->request.data = closure;

    param = Nan::New("base").ToLocalChecked();
    if (Nan::Has(options, param).ToChecked())
    {
        v8::Local<v8::Value> param_val = Nan::Get(options, param).ToLocalChecked();
        if (!param_val->IsString())
        {
            Nan::ThrowTypeError("'base' must be a string representing a filesystem path");
            return;
        }
        closure->base_path = TOSTR(param_val);
    }

    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_FromString, (uv_after_work_cb)EIO_AfterFromString);
    m->Ref();
    return;
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
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    } else {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->m->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Clone this map object, returning a value which can be changed
 * without mutating the original
 *
 * @instance
 * @name clone
 * @memberof Map
 * @returns {mapnik.Map} clone
 */
NAN_METHOD(Map::clone)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    Map* m2 = new Map();
    m2->map_ = std::make_shared<mapnik::Map>(*m->map_);
    v8::Local<v8::Value> ext = Nan::New<v8::External>(m2);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Map instance");
    else info.GetReturnValue().Set(maybe_local.ToLocalChecked());
}

/**
 * Writes the map to an xml file
 *
 * @memberof Map
 * @instance
 * @name save
 * @param {string} file path
 * @example
 * map.save("path/to/map.xml");
 */

NAN_METHOD(Map::save)
{
    if (info.Length() != 1 || !info[0]->IsString())
    {
        Nan::ThrowTypeError("first argument must be a path to map.xml to save");
        return;
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    std::string filename = TOSTR(info[0]);
    bool explicit_defaults = false;
    mapnik::save_map(*m->map_,filename,explicit_defaults);
    return;
}

/**
 * Converts the map to an XML string
 *
 * @memberof Map
 * @instance
 * @name toXML
 * @example
 * var xml = map.toXML();
 */
NAN_METHOD(Map::toXML)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    bool explicit_defaults = false;
    std::string map_string = mapnik::save_map_to_string(*m->map_,explicit_defaults);
    info.GetReturnValue().Set(Nan::New<v8::String>(map_string).ToLocalChecked());
}

NAN_METHOD(Map::zoomAll)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    try
    {
        m->map_->zoom_all();
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
    return;
}

NAN_METHOD(Map::zoomToBox)
{
    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());

    double minx;
    double miny;
    double maxx;
    double maxy;

    if (info.Length() == 1)
    {
        if (!info[0]->IsArray())
        {
            Nan::ThrowError("Must provide an array of: [minx,miny,maxx,maxy]");
            return;
        }
        v8::Local<v8::Array> a = info[0].As<v8::Array>();
        minx = Nan::To<double>(Nan::Get(a, 0).ToLocalChecked()).FromJust();
        miny = Nan::To<double>(Nan::Get(a, 1).ToLocalChecked()).FromJust();
        maxx = Nan::To<double>(Nan::Get(a, 2).ToLocalChecked()).FromJust();
        maxy = Nan::To<double>(Nan::Get(a, 3).ToLocalChecked()).FromJust();

    }
    else if (info.Length() != 4)
    {
        Nan::ThrowError("Must provide 4 arguments: minx,miny,maxx,maxy");
        return;
    }
    else if (info[0]->IsNumber() &&
               info[1]->IsNumber() &&
               info[2]->IsNumber() &&
               info[3]->IsNumber())
    {
        minx = Nan::To<double>(info[0]).FromJust();
        miny = Nan::To<double>(info[1]).FromJust();
        maxx = Nan::To<double>(info[2]).FromJust();
        maxy = Nan::To<double>(info[3]).FromJust();
    }
    else
    {
        Nan::ThrowError("If you are providing 4 arguments: minx,miny,maxx,maxy - they must be all numbers");
        return;
    }

    mapnik::box2d<double> box(minx,miny,maxx,maxy);
    m->map_->zoom_to_box(box);
    return;
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
    Nan::Persistent<v8::Function> cb;
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

#if defined(GRID_RENDERER)
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
    Nan::Persistent<v8::Function> cb;
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
#endif

struct vector_tile_baton_t {
    uv_work_t request;
    Map *m;
    VectorTile *d;
    double area_threshold;
    double scale_factor;
    double scale_denominator;
    mapnik::attributes variables;
    unsigned offset_x;
    unsigned offset_y;
    std::string image_format;
    mapnik::scaling_method_e scaling_method;
    double simplify_distance;
    bool error;
    bool strictly_simple;
    bool multi_polygon_union;
    mapnik::vector_tile_impl::polygon_fill_type fill_type;
    bool process_all_rings;
    std::launch threading_mode;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    vector_tile_baton_t() :
        area_threshold(0.1),
        scale_factor(1.0),
        scale_denominator(0.0),
        variables(),
        offset_x(0),
        offset_y(0),
        image_format("webp"),
        scaling_method(mapnik::SCALING_BILINEAR),
        simplify_distance(0.0),
        error(false),
        strictly_simple(true),
        multi_polygon_union(false),
        fill_type(mapnik::vector_tile_impl::positive_fill),
        process_all_rings(false),
        threading_mode(std::launch::deferred) {}
};

/**
 * Renders a mapnik object (image tile, grid, vector tile) by passing in a renderable mapnik object.
 *
 * @instance
 * @name render
 * @memberof Map
 * @param {mapnik.Image} renderable mapnik object
 * @param {Object} [options={}]
 * @param {Number} [options.buffer_size=0] size of the buffer on the image
 * @param {Number} [options.scale=1.0] scale the image
 * @param {Number} [options.scale_denominator=0.0]
 * @param {Number} [options.offset_x=0] pixel offset along the x-axis
 * @param {Number} [options.offset_y=0] pixel offset along the y-axis
 * @param {String} [options.image_scaling] must be a valid scaling method (used when rendering a vector tile)
 * @param {String} [options.image_format] must be a string and valid image format (used when rendering a vector tile)
 * @param {Number} [options.area_threshold] used to discard small polygons by setting a minimum size (used when rendering a vector tile)
 * @param {Boolean} [options.strictly_simple=] ensure all geometry is valid according to
 * OGC Simple definition (used when rendering a vector tile)
 * @param {Boolean} [options.multi_polygon_union] union all multipolygons (used when rendering a vector tile)
 * @param {String} [options.fill_type] the fill type used in determining what are holes and what are outer rings. See the
 * [Clipper documentation](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/PolyFillType.htm)
 * to learn more about fill types. (used when rendering a vector tile)
 * @param {String} [options.threading_mode] (used when rendering a vector tile)
 * @param {Number} [options.simplify_distance] Simplification works to generalize
 * geometries before encoding into vector tiles.simplification distance The
 * `simplify_distance` value works in integer space over a 4096 pixel grid and uses
 * the [Douglas-Peucker algorithm](https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm).
 * (used when rendering a vector tile)
 * @param {Object} [options.variables] Mapnik 3.x ONLY: A javascript object
 * containing key value pairs that should be passed into Mapnik as variables
 * for rendering and for datasource queries. For example if you passed
 * `vtile.render(map,image,{ variables : {zoom:1} },cb)` then the `@zoom`
 * variable would be usable in Mapnik symbolizers like `line-width:"@zoom"`
 * and as a token in Mapnik postgis sql sub-selects like
 * `(select * from table where some_field > @zoom)` as tmp (used when rendering a vector tile)
 * @param {Boolean} [options.process_all_rings] if `true`, don't assume winding order and ring order of
 * polygons are correct according to the [`2.0` Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec)
 * (used when rendering a vector tile)
 * @returns {mapnik.Map} rendered image tile
 *
 * @example
 * // render data to an image object
 * var map = new mapnik.Map(256, 256);
 * map.loadSync('./path/to/stylesheet.xml');
 * var image = new mapnik.Image(map.width, map.height);
 * map.render(image, {}, function(err, image) {
 *     if (err) throw err;
 *     console.log(image) // => mapnik image object with data from xml
 * });
 *
 * @example
 * // render data to a vector tile object
 * var map = new mapnik.Map(256, 256);
 * map.loadSync('./path/to/stylesheet.xml');
 * var vtile = new mapnik.VectorTile(9,112,195);
 * map.render(vtile, {}, function(err, vtile) {
 *     if (err) throw err;
 *     console.log(vtile); // => vector tile object with data from xml
 * });
 */
NAN_METHOD(Map::render)
{
    // ensure at least 2 args
    if (info.Length() < 2) {
        Nan::ThrowTypeError("requires at least two arguments, a renderable mapnik object, and a callback");
        return;
    }

    // ensure renderable object
    if (!info[0]->IsObject()) {
        Nan::ThrowTypeError("requires a renderable mapnik object to be passed as first argument");
        return;
    }

    // ensure function callback
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());

    try
    {
        // parse options

        // defaults
        int buffer_size = 0;
        double scale_factor = 1.0;
        double scale_denominator = 0.0;
        unsigned offset_x = 0;
        unsigned offset_y = 0;

        v8::Local<v8::Object> options = Nan::New<v8::Object>();

        if (info.Length() > 2) {

            // options object
            if (!info[1]->IsObject()) {
                Nan::ThrowTypeError("optional second argument must be an options object");
                return;
            }

            options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

            if (Nan::Has(options, Nan::New("buffer_size").ToLocalChecked()).FromMaybe(false)) {
                v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("buffer_size").ToLocalChecked()).ToLocalChecked();
                if (!bind_opt->IsNumber()) {
                    Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                    return;
                }
                buffer_size = Nan::To<int>(bind_opt).FromJust();
            }

            if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false)) {
                v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
                if (!bind_opt->IsNumber()) {
                    Nan::ThrowTypeError("optional arg 'scale' must be a number");
                    return;
                }

                scale_factor = Nan::To<double>(bind_opt).FromJust();
            }

            if (Nan::Has(options, Nan::New("scale_denominator").ToLocalChecked()).FromMaybe(false)) {
                v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale_denominator").ToLocalChecked()).ToLocalChecked();
                if (!bind_opt->IsNumber()) {
                    Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                    return;
                }

                scale_denominator = Nan::To<double>(bind_opt).FromJust();
            }

            if (Nan::Has(options, Nan::New("offset_x").ToLocalChecked()).FromMaybe(false)) {
                v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_x").ToLocalChecked()).ToLocalChecked();
                if (!bind_opt->IsNumber()) {
                    Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
                    return;
                }

                offset_x = Nan::To<int>(bind_opt).FromJust();
            }

            if (Nan::Has(options, Nan::New("offset_y").ToLocalChecked()).FromMaybe(false)) {
                v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_y").ToLocalChecked()).ToLocalChecked();
                if (!bind_opt->IsNumber()) {
                    Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
                    return;
                }

                offset_y = Nan::To<int>(bind_opt).FromJust();
            }
        }

        v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::New(Image::constructor)->HasInstance(obj)) {

            image_baton_t *closure = new image_baton_t();
            closure->request.data = closure;
            closure->m = m;
            closure->im = Nan::ObjectWrap::Unwrap<Image>(obj);
            closure->buffer_size = buffer_size;
            closure->scale_factor = scale_factor;
            closure->scale_denominator = scale_denominator;
            closure->offset_x = offset_x;
            closure->offset_y = offset_y;
            closure->error = false;

            if (Nan::Has(options, Nan::New("variables").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("variables").ToLocalChecked()).ToLocalChecked();
                if (!bind_opt->IsObject())
                {
                    delete closure;
                    Nan::ThrowTypeError("optional arg 'variables' must be an object");
                    return;
                }
                object_to_container(closure->variables,bind_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
            }
            if (!m->acquire())
            {
                delete closure;
                Nan::ThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
                return;
            }
            closure->cb.Reset(info[info.Length() - 1].As<v8::Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderImage, (uv_after_work_cb)EIO_AfterRenderImage);
            closure->im->Ref();
        }
#if defined(GRID_RENDERER)
        else if (Nan::New(Grid::constructor)->HasInstance(obj)) {

            Grid * g = Nan::ObjectWrap::Unwrap<Grid>(obj);

            std::size_t layer_idx = 0;

            // grid requires special options for now
            if (!Nan::Has(options, Nan::New("layer").ToLocalChecked()).FromMaybe(false)) {
                Nan::ThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
                return;
            } else {

                std::vector<mapnik::layer> const& layers = m->map_->layers();

                v8::Local<v8::Value> layer_id = Nan::Get(options, Nan::New("layer").ToLocalChecked()).ToLocalChecked();
                if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
                    Nan::ThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
                    return;
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
                        Nan::ThrowTypeError(s.str().c_str());
                        return;
                    }
                } else { // IS NUMBER
                    layer_idx = Nan::To<int>(layer_id).FromJust();
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
                        Nan::ThrowTypeError(s.str().c_str());
                        return;
                    }
                }
            }

            if (Nan::Has(options, Nan::New("fields").ToLocalChecked()).FromMaybe(false)) {

                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("fields").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsArray()) {
                    Nan::ThrowTypeError("option 'fields' must be an array of strings");
                    return;
                }
                v8::Local<v8::Array> a = v8::Local<v8::Array>::Cast(param_val);
                unsigned int i = 0;
                unsigned int num_fields = a->Length();
                while (i < num_fields) {
                    v8::Local<v8::Value> name = Nan::Get(a, i).ToLocalChecked();
                    if (name->IsString()){
                        g->get()->add_field(TOSTR(name));
                    }
                    i++;
                }
            }

            grid_baton_t *closure = new grid_baton_t();

            if (Nan::Has(options, Nan::New("variables").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("variables").ToLocalChecked()).ToLocalChecked();
                if (!bind_opt->IsObject())
                {
                    delete closure;
                    Nan::ThrowTypeError("optional arg 'variables' must be an object");
                    return;
                }
                object_to_container(closure->variables,bind_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
            }

            closure->request.data = closure;
            closure->m = m;
            closure->g = g;
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
                Nan::ThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
                return;
            }
            closure->cb.Reset(info[info.Length() - 1].As<v8::Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderGrid, (uv_after_work_cb)EIO_AfterRenderGrid);
            closure->g->Ref();
        }
#endif
        else if (Nan::New(VectorTile::constructor)->HasInstance(obj))
        {

            vector_tile_baton_t *closure = new vector_tile_baton_t();

            if (Nan::Has(options, Nan::New("image_scaling").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_scaling").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsString())
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'image_scaling' must be a string");
                    return;
                }
                std::string image_scaling = TOSTR(param_val);
                boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
                if (!method)
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')");
                    return;
                }
                closure->scaling_method = *method;
            }

            if (Nan::Has(options, Nan::New("image_format").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_format").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsString())
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'image_format' must be a string");
                    return;
                }
                closure->image_format = TOSTR(param_val);
            }

            if (Nan::Has(options, Nan::New("area_threshold").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("area_threshold").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsNumber())
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'area_threshold' must be a number");
                    return;
                }
                closure->area_threshold = Nan::To<double>(param_val).FromJust();
                if (closure->area_threshold < 0.0)
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'area_threshold' must not be a negative number");
                    return;
                }
            }

            if (Nan::Has(options, Nan::New("strictly_simple").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("strictly_simple").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsBoolean())
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'strictly_simple' must be a boolean");
                    return;
                }
                closure->strictly_simple = Nan::To<bool>(param_val).FromJust();
            }

            if (Nan::Has(options, Nan::New("multi_polygon_union").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("multi_polygon_union").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsBoolean())
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'multi_polygon_union' must be a boolean");
                    return;
                }
                closure->multi_polygon_union = Nan::To<bool>(param_val).FromJust();
            }

            if (Nan::Has(options, Nan::New("fill_type").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("fill_type").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsNumber())
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'fill_type' must be an unsigned integer");
                    return;
                }
                closure->fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(Nan::To<int>(param_val).FromJust());
                if (closure->fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
                {
                    delete closure;
                    Nan::ThrowTypeError("optional arg 'fill_type' out of possible range");
                    return;
                }
            }

            if (Nan::Has(options, Nan::New("threading_mode").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("threading_mode").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsNumber())
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'threading_mode' must be an unsigned integer");
                    return;
                }
                closure->threading_mode = static_cast<std::launch>(Nan::To<int>(param_val).FromJust());
                if (closure->threading_mode != std::launch::async &&
                    closure->threading_mode != std::launch::deferred &&
                    closure->threading_mode != (std::launch::async | std::launch::deferred))
                {
                    delete closure;
                    Nan::ThrowTypeError("optional arg 'threading_mode' value passed is invalid");
                    return;
                }
            }

            if (Nan::Has(options, Nan::New("simplify_distance").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("simplify_distance").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsNumber())
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'simplify_distance' must be an floating point number");
                    return;
                }
                closure->simplify_distance = Nan::To<double>(param_val).FromJust();
                if (closure->simplify_distance < 0)
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'simplify_distance' can not be negative");
                    return;
                }
            }

            if (Nan::Has(options, Nan::New("variables").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("variables").ToLocalChecked()).ToLocalChecked();
                if (!bind_opt->IsObject())
                {
                    delete closure;
                    Nan::ThrowTypeError("optional arg 'variables' must be an object");
                    return;
                }
                object_to_container(closure->variables,bind_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
            }

            if (Nan::Has(options, Nan::New("process_all_rings").ToLocalChecked()).FromMaybe(false))
            {
                v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("process_all_rings").ToLocalChecked()).ToLocalChecked();
                if (!param_val->IsBoolean())
                {
                    delete closure;
                    Nan::ThrowTypeError("option 'process_all_rings' must be a boolean");
                    return;
                }
                closure->process_all_rings = Nan::To<bool>(param_val).FromJust();
            }

            closure->request.data = closure;
            closure->m = m;
            closure->d = Nan::ObjectWrap::Unwrap<VectorTile>(obj);
            closure->scale_factor = scale_factor;
            closure->scale_denominator = scale_denominator;
            closure->offset_x = offset_x;
            closure->offset_y = offset_y;
            closure->error = false;
            if (!m->acquire())
            {
                delete closure;
                Nan::ThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
                return;
            }
            closure->cb.Reset(info[info.Length() - 1].As<v8::Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderVectorTile, (uv_after_work_cb)EIO_AfterRenderVectorTile);
            closure->d->Ref();
        }
        else
        {
            Nan::ThrowTypeError("renderable mapnik object expected");
            return;
        }

        m->Ref();
        return;
    }
    catch (std::exception const& ex)
    {
        // I am not quite sure it is possible to put a test in to cover an exception here
        /* LCOV_EXCL_START */
        Nan::ThrowTypeError(ex.what());
        return;
        /* LCOV_EXCL_STOP */
    }
}

void Map::EIO_RenderVectorTile(uv_work_t* req)
{
    vector_tile_baton_t *closure = static_cast<vector_tile_baton_t *>(req->data);
    try
    {
        mapnik::Map const& map = *closure->m->get();

        mapnik::vector_tile_impl::processor ren(map, closure->variables);
        ren.set_simplify_distance(closure->simplify_distance);
        ren.set_multi_polygon_union(closure->multi_polygon_union);
        ren.set_fill_type(closure->fill_type);
        ren.set_process_all_rings(closure->process_all_rings);
        ren.set_scale_factor(closure->scale_factor);
        ren.set_strictly_simple(closure->strictly_simple);
        ren.set_image_format(closure->image_format);
        ren.set_scaling_method(closure->scaling_method);
        ren.set_area_threshold(closure->area_threshold);
        ren.set_threading_mode(closure->threading_mode);

        ren.update_tile(*closure->d->get_tile(),
                        closure->scale_denominator,
                        closure->offset_x,
                        closure->offset_y);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterRenderVectorTile(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    vector_tile_baton_t *closure = static_cast<vector_tile_baton_t *>(req->data);
    closure->m->release();

    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->d->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

#if defined(GRID_RENDERER)
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
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);

    grid_baton_t *closure = static_cast<grid_baton_t *>(req->data);
    closure->m->release();

    if (closure->error) {
        // TODO - add more attributes
        // https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/Error
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    } else {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->g->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->g->Unref();
    closure->cb.Reset();
    delete closure;
}
#endif

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
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    image_baton_t *closure = static_cast<image_baton_t *>(req->data);
    closure->m->release();

    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    } else {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->im->Unref();
    closure->cb.Reset();
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
    Nan::Persistent<v8::Function> cb;
} render_file_baton_t;

NAN_METHOD(Map::renderFile)
{
    if (info.Length() < 1 || !info[0]->IsString()) {
        Nan::ThrowTypeError("first argument must be a path to a file to save");
        return;
    }

    // defaults
    std::string format = "png";
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    palette_ptr palette;
    int buffer_size = 0;

    v8::Local<v8::Value> callback = info[info.Length()-1];

    if (!callback->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    v8::Local<v8::Object> options = Nan::New<v8::Object>();

    if (!info[1]->IsFunction() && info[1]->IsObject()) {
        options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("format").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("format").ToLocalChecked()).ToLocalChecked();
            if (!format_opt->IsString()) {
                Nan::ThrowTypeError("'format' must be a String");
                return;
            }

            format = TOSTR(format_opt);
        }

        if (Nan::Has(options, Nan::New("palette").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("palette").ToLocalChecked()).ToLocalChecked();
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }

            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
        if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return;
            }

            scale_factor = Nan::To<double>(bind_opt).FromJust();
        }

        if (Nan::Has(options, Nan::New("scale_denominator").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale_denominator").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                return;
            }

            scale_denominator = Nan::To<double>(bind_opt).FromJust();
        }

        if (Nan::Has(options, Nan::New("buffer_size").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("buffer_size").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }

            buffer_size = Nan::To<int>(bind_opt).FromJust();
        }

    } else if (!info[1]->IsFunction()) {
        Nan::ThrowTypeError("optional argument must be an object");
        return;
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    std::string output = TOSTR(info[0]);

    //maybe do this in the async part?
    if (format.empty()) {
        format = mapnik::guess_type(output);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << output << "\n";
            Nan::ThrowError(s.str().c_str());
            return;
        }
    }

    render_file_baton_t *closure = new render_file_baton_t();

    if (Nan::Has(options, Nan::New("variables").ToLocalChecked()).FromMaybe(false))
    {
        v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("variables").ToLocalChecked()).ToLocalChecked();
        if (!bind_opt->IsObject())
        {
            delete closure;
            Nan::ThrowTypeError("optional arg 'variables' must be an object");
            return;
        }
        object_to_container(closure->variables,bind_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
    }

    if (format == "pdf" || format == "svg" || format == "ps" || format == "ARGB32" || format == "RGB24") {
#if defined(HAVE_CAIRO)
        closure->use_cairo = true;
#else
        delete closure;
        std::ostringstream s("");
        s << "Cairo backend is not available, cannot write to " << format << "\n";
        Nan::ThrowError(s.str().c_str());
        return;
#endif
    } else {
        closure->use_cairo = false;
    }

    if (!m->acquire())
    {
        delete closure;
        Nan::ThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
        return;
    }
    closure->request.data = closure;

    closure->m = m;
    closure->scale_factor = scale_factor;
    closure->scale_denominator = scale_denominator;
    closure->buffer_size = buffer_size;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());

    closure->format = format;
    closure->palette = palette;
    closure->output = output;

    uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderFile, (uv_after_work_cb)EIO_AfterRenderFile);
    m->Ref();

    return;

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
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    render_file_baton_t *closure = static_cast<render_file_baton_t *>(req->data);
    closure->m->release();

    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    } else {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }

    closure->m->Unref();
    closure->cb.Reset();
    delete closure;

}

// TODO - add support for grids
NAN_METHOD(Map::renderSync)
{
    std::string format = "png";
    palette_ptr palette;
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    int buffer_size = 0;

    if (info.Length() >= 1)
    {
        if (!info[0]->IsObject())
        {
            Nan::ThrowTypeError("first argument is optional, but if provided must be an object, eg. {format: 'pdf'}");
            return;
        }

        v8::Local<v8::Object> options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("format").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("format").ToLocalChecked()).ToLocalChecked();
            if (!format_opt->IsString()) {
                Nan::ThrowTypeError("'format' must be a String");
                return;
            }

            format = TOSTR(format_opt);
        }

        if (Nan::Has(options, Nan::New("palette").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("palette").ToLocalChecked()).ToLocalChecked();
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }

            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
        if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return;
            }

            scale_factor = Nan::To<double>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("scale_denominator").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale_denominator").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                return;
            }

            scale_denominator = Nan::To<double>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("buffer_size").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("buffer_size").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }

            buffer_size = Nan::To<int>(bind_opt).FromJust();
        }
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    if (!m->acquire())
    {
        Nan::ThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
        return;
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
        Nan::ThrowError(ex.what());
        return;
    }
    m->release();
    info.GetReturnValue().Set(Nan::CopyBuffer((char*)s.data(), s.size()).ToLocalChecked());
}

NAN_METHOD(Map::renderFileSync)
{
    if (info.Length() < 1 || !info[0]->IsString()) {
        Nan::ThrowTypeError("first argument must be a path to a file to save");
        return;
    }

    if (info.Length() > 2) {
        Nan::ThrowError("accepts two arguments, a required path to a file, an optional options object, eg. {format: 'pdf'}");
        return;
    }

    // defaults
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    int buffer_size = 0;
    std::string format = "png";
    palette_ptr palette;

    if (info.Length() >= 2){
        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("second argument is optional, but if provided must be an object, eg. {format: 'pdf'}");
            return;
        }

        v8::Local<v8::Object> options = info[1].As<v8::Object>();
        if (Nan::Has(options, Nan::New("format").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("format").ToLocalChecked()).ToLocalChecked();
            if (!format_opt->IsString()) {
                Nan::ThrowTypeError("'format' must be a String");
                return;
            }

            format = TOSTR(format_opt);
        }

        if (Nan::Has(options, Nan::New("palette").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("palette").ToLocalChecked()).ToLocalChecked();
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }

            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
        if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return;
            }

            scale_factor = Nan::To<double>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("scale_denominator").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale_denominator").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                return;
            }

            scale_denominator = Nan::To<double>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("buffer_size").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("buffer_size").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }

            buffer_size = Nan::To<int>(bind_opt).FromJust();
        }
    }

    Map* m = Nan::ObjectWrap::Unwrap<Map>(info.Holder());
    std::string output = TOSTR(info[0]);

    if (format.empty()) {
        format = mapnik::guess_type(output);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << output << "\n";
            Nan::ThrowError(s.str().c_str());
            return;
        }
    }
    if (!m->acquire())
    {
        Nan::ThrowTypeError("render: Map currently in use by another thread. Consider using a map pool.");
        return;
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
            Nan::ThrowError(s.str().c_str());
            return;
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
        Nan::ThrowError(ex.what());
        return;
    }
    m->release();
    return;
}
