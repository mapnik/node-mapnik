#include "mapnik_map.hpp"
#include "utils.hpp"
#include "mapnik_color.hpp"             // for Color, Color::constructor
#include "mapnik_featureset.hpp"        // for Featureset
#if defined(GRID_RENDERER)
//#include "mapnik_grid.hpp"              // for Grid, Grid::constructor
#endif
#include "mapnik_image.hpp"             // for Image, Image::constructor
#include "mapnik_layer.hpp"             // for Layer, Layer::constructor
#include "mapnik_palette.hpp"           // for palette_ptr, Palette, etc
//#include "mapnik_vector_tile.hpp"
//#include "object_to_container.hpp"

// mapnik-vector-tile
//#include "vector_tile_processor.hpp"

// mapnik
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
#include <mapnik/geometry/box2d.hpp>    // for box2d
#include <mapnik/color.hpp>             // for color
#include <mapnik/attribute.hpp>         // for attributes
#include <mapnik/featureset.hpp>        // for featureset_ptr
#if defined(GRID_RENDERER)
#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp>// for grid_renderer
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

Napi::FunctionReference Map::constructor;

Napi::Object Map::Initialize(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "Map", {
            InstanceMethod<&Map::fonts>("fonts"),
            InstanceMethod<&Map::fontFiles>("fontFiles"),
            InstanceMethod<&Map::fontDirectory>("fontDirectory"),
            InstanceMethod<&Map::memoryFonts>("memoryFonts"),
            InstanceMethod<&Map::registerFonts>("registerFonts"),
            //InstanceMethod<&Map::load>("load"),
            //InstanceMethod<&Map::loadSync>("loadSync"),
            //InstanceMethod<&Map::fromStringSync>("fromStringSync"),
            //InstanceMethod<&Map::fromString>("fromString"),
            InstanceMethod<&Map::clone>("clone"),
            InstanceMethod<&Map::save>("save"),
            InstanceMethod<&Map::clear>("clear"),
            InstanceMethod<&Map::toXML>("toXML"),
            InstanceMethod<&Map::resize>("resize"),
            //InstanceMethod<&Map::render>("render"),
            //InstanceMethod<&Map::renderSync>("renderSync"),
            //InstanceMethod<&Map::renderFile>("renderFile"),
            //InstanceMethod<&Map::renderFileSync>("renderFileSync"),
            InstanceMethod<&Map::zoomAll>("zoomAll"),
            InstanceMethod<&Map::zoomToBox>("zoomToBox"),
            InstanceMethod<&Map::scale>("scale"),
            InstanceMethod<&Map::scaleDenominator>("scaleDenominator"),
            //InstanceMethod<&Map::queryPoint>("queryPoint"),
            //InstanceMethod<&Map::queryMapPoint>("queryMapPoint"),
            InstanceMethod<&Map::add_layer>("add_layer"),
            InstanceMethod<&Map::remove_layer>("remove_layer"),
            InstanceMethod<&Map::get_layer>("get_layer"),
            InstanceMethod<&Map::layers>("layers"),
            // accessors
            InstanceAccessor<&Map::srs, &Map::srs>("srs"),
            InstanceAccessor<&Map::width, &Map::width>("width"),
            InstanceAccessor<&Map::height, &Map::height>("height"),
            InstanceAccessor<&Map::bufferSize, &Map::bufferSize>("bufferSize"),
            InstanceAccessor<&Map::extent, &Map::extent>("extent"),
            InstanceAccessor<&Map::bufferedExtent>("bufferedExtent"),
            InstanceAccessor<&Map::maximumExtent, &Map::maximumExtent>("maximumExtent"),
            InstanceAccessor<&Map::background, &Map::background>("background"),
            //InstanceAccessor<&Map::parameters, &Map::parameters>("parameters"),
            InstanceAccessor<&Map::aspect_fix_mode, &Map::aspect_fix_mode>("aspect_fix_mode")

        });

    func.Set("ASPECT_GROW_BBOX", Napi::Number::New(env, mapnik::Map::GROW_BBOX));
    func.Set("ASPECT_GROW_CANVAS", Napi::Number::New(env, mapnik::Map::GROW_CANVAS));
    func.Set("ASPECT_SHRINK_BBOX", Napi::Number::New(env, mapnik::Map::SHRINK_BBOX));
    func.Set("ASPECT_SHRINK_CANVAS", Napi::Number::New(env, mapnik::Map::SHRINK_CANVAS));
    func.Set("ASPECT_ADJUST_BBOX_WIDTH", Napi::Number::New(env, mapnik::Map::ADJUST_BBOX_WIDTH));
    func.Set("ASPECT_ADJUST_BBOX_HEIGHT", Napi::Number::New(env, mapnik::Map::ADJUST_BBOX_HEIGHT));
    func.Set("ASPECT_ADJUST_CANVAS_WIDTH", Napi::Number::New(env, mapnik::Map::ADJUST_CANVAS_WIDTH));
    func.Set("ASPECT_ADJUST_CANVAS_HEIGHT", Napi::Number::New(env, mapnik::Map::ADJUST_CANVAS_HEIGHT));
    func.Set("ASPECT_RESPECT", Napi::Number::New(env, mapnik::Map::RESPECT));

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Map", func);
    return exports;
}

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

/*
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
*/

Map::Map(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Map>(info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() == 1 && info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<map_ptr>>();
        if (ext) map_ = *ext.Data();
        return;
    }

    if (info.Length() == 2)
    {
        if (!info[0].IsNumber() || !info[1].IsNumber())
        {
            Napi::TypeError::New(env, "'width' and 'height' must be integers").ThrowAsJavaScriptException();
            return;
        }
        map_ = std::make_shared<mapnik::Map>(info[0].As<Napi::Number>().Int32Value(),info[1].As<Napi::Number>().Int32Value());
        return;
    }
    else if (info.Length() == 3)
    {
        if (!info[0].IsNumber() || !info[1].IsNumber())
        {
            Napi::TypeError::New(env, "'width' and 'height' must be integers").ThrowAsJavaScriptException();
            return;
        }
        if (!info[2].IsString())
        {
            Napi::Error::New(env, "'srs' value must be a string").ThrowAsJavaScriptException();
            return;
        }
        map_ = std::make_shared<mapnik::Map>(info[0].As<Napi::Number>().Int32Value(),
                                             info[1].As<Napi::Number>().Int32Value(),
                                             info[2].As<Napi::String>());
    }
    else
    {
        Napi::Error::New(env, "please provide Map width and height and optional srs").ThrowAsJavaScriptException();
    }
}

// accessors

// srs
Napi::Value Map::srs(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::String::New(env, map_->srs());
}

void Map::srs(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsString())
    {
        Napi::TypeError::New(env, "'srs' must be a string").ThrowAsJavaScriptException();
        return;
    }
    map_->set_srs(value.As<Napi::String>());
}

// extent
Napi::Value Map::extent(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    mapnik::box2d<double> const& e = map_->get_current_extent();
    Napi::Array arr = Napi::Array::New(env, 4u);
    arr.Set(0u, Napi::Number::New(env, e.minx()));
    arr.Set(1u, Napi::Number::New(env, e.miny()));
    arr.Set(2u, Napi::Number::New(env, e.maxx()));
    arr.Set(3u, Napi::Number::New(env, e.maxy()));
    return scope.Escape(arr);
}

void Map::extent(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    if (!value.IsArray())
    {
        Napi::Error::New(env, "Must provide an array of: [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
        return;
    }
    Napi::Array arr = value.As<Napi::Array>();
    double minx = arr.Get(0u).As<Napi::Number>().DoubleValue();
    double miny = arr.Get(1u).As<Napi::Number>().DoubleValue();
    double maxx = arr.Get(2u).As<Napi::Number>().DoubleValue();
    double maxy = arr.Get(3u).As<Napi::Number>().DoubleValue();
    mapnik::box2d<double> box{minx, miny, maxx, maxy};
    map_->zoom_to_box(box);
}

// maximumExtent
Napi::Value Map::maximumExtent(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    boost::optional<mapnik::box2d<double> > const& e = map_->maximum_extent();
    if (!e) return env.Undefined();

    Napi::Array arr = Napi::Array::New(env, 4u);
    arr.Set(0u, Napi::Number::New(env, e->minx()));
    arr.Set(1u, Napi::Number::New(env, e->miny()));
    arr.Set(2u, Napi::Number::New(env, e->maxx()));
    arr.Set(3u, Napi::Number::New(env, e->maxy()));
    return scope.Escape(arr);
}

void Map::maximumExtent(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    if (!value.IsArray())
    {
        Napi::Error::New(env, "Must provide an array of: [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
        return;
    }
    Napi::Array arr = value.As<Napi::Array>();
    double minx = arr.Get(0u).As<Napi::Number>().DoubleValue();
    double miny = arr.Get(1u).As<Napi::Number>().DoubleValue();
    double maxx = arr.Get(2u).As<Napi::Number>().DoubleValue();
    double maxy = arr.Get(3u).As<Napi::Number>().DoubleValue();
    mapnik::box2d<double> box{minx, miny, maxx, maxy};
    map_->set_maximum_extent(box);
}


// bufferedExtent
Napi::Value Map::bufferedExtent(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    boost::optional<mapnik::box2d<double> > const& e = map_->get_buffered_extent();
    if (!e) return env.Undefined();

    Napi::Array arr = Napi::Array::New(env, 4u);
    arr.Set(0u, Napi::Number::New(env, e->minx()));
    arr.Set(1u, Napi::Number::New(env, e->miny()));
    arr.Set(2u, Napi::Number::New(env, e->maxx()));
    arr.Set(3u, Napi::Number::New(env, e->maxy()));
    return scope.Escape(arr);
}

// width

Napi::Value Map::width(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, map_->width());
}

void Map::width(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();

     if (!value.IsNumber())
     {
         Napi::TypeError::New(env, "Must provide an integer width").ThrowAsJavaScriptException();
         return;
     }
     map_->set_width(value.As<Napi::Number>().Int32Value());
}

// height
Napi::Value Map::height(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, map_->height());
}

void Map::height(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();

     if (!value.IsNumber())
     {
         Napi::TypeError::New(env, "Must provide an integer height").ThrowAsJavaScriptException();
         return;
     }
     map_->set_height(value.As<Napi::Number>().Int32Value());
}

// aspect_fix_mode
Napi::Value Map::aspect_fix_mode(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, map_->get_aspect_fix_mode());
}

void Map::aspect_fix_mode(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();

     if (!value.IsNumber())
     {
         Napi::TypeError::New(env, "'aspect_fix_mode' must be a constant (number)").ThrowAsJavaScriptException();
         return;
     }
     int val = value.As<Napi::Number>().Int32Value();
     if (val < mapnik::Map::aspect_fix_mode_MAX && val >= 0)
     {
         map_->set_aspect_fix_mode(static_cast<mapnik::Map::aspect_fix_mode>(val));
     }
     else
     {
         Napi::Error::New(env, "'aspect_fix_mode' value is invalid").ThrowAsJavaScriptException();
     }
}


// bufferSize
Napi::Value Map::bufferSize(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, map_->buffer_size());
}

void Map::bufferSize(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsNumber())
    {
        Napi::TypeError::New(env, "Must provide an integer bufferSize").ThrowAsJavaScriptException();
        return;
    }
    map_->set_buffer_size(value.As<Napi::Number>().Int32Value());
}


// background
Napi::Value Map::background(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    boost::optional<mapnik::color> col = map_->background();
    if (col)
    {
        Napi::Value arg = Napi::External<mapnik::color>::New(env, &(*col));
        Napi::Object obj = Color::constructor.New({arg});
        return scope.Escape(napi_value(obj)).ToObject();
    }
    return env.Undefined();
}

void Map::background(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();

    if (!value.IsObject())
    {
        Napi::TypeError::New(env, "mapnik.Color expected").ThrowAsJavaScriptException();
        return;
    }
    Napi::Object obj = value.As<Napi::Object>();

    if (!obj.InstanceOf(Color::constructor.Value()))
    {
        Napi::TypeError::New(env, "Must provide an integer height").ThrowAsJavaScriptException();
        return;
    }
    Color* c = Napi::ObjectWrap<Color>::Unwrap(obj);
    map_->set_background(c->color_);
}


/*
Napi::Value Map::get_prop(Napi::CallbackInfo const& info)
{
    Map* m = info.Holder().Unwrap<Map>();
    std::string a = TOSTR(property);
    if(a == "extent") {
        Napi::Array arr = Napi::Array::New(env, 4);
        mapnik::box2d<double> const& e = m->map_->get_current_extent();
        arr.Set(0, Napi::Number::New(env, e.minx()));
        arr.Set(1, Napi::Number::New(env, e.miny()));
        arr.Set(2, Napi::Number::New(env, e.maxx()));
        arr.Set(3, Napi::Number::New(env, e.maxy()));
        return arr;
    }
    else if(a == "bufferedExtent") {
        boost::optional<mapnik::box2d<double> > const& e = m->map_->get_buffered_extent();
        Napi::Array arr = Napi::Array::New(env, 4);
        arr.Set(0, Napi::Number::New(env, e->minx()));
        arr.Set(1, Napi::Number::New(env, e->miny()));
        arr.Set(2, Napi::Number::New(env, e->maxx()));
        arr.Set(3, Napi::Number::New(env, e->maxy()));
        return arr;
    }
    else if(a == "maximumExtent") {
    boost::optional<mapnik::box2d<double> > const& e = m->map_->maximum_extent();
        if (!e)
            return;
        Napi::Array arr = Napi::Array::New(env, 4);
        arr.Set(0, Napi::Number::New(env, e->minx()));
        arr.Set(1, Napi::Number::New(env, e->miny()));
        arr.Set(2, Napi::Number::New(env, e->maxx()));
        arr.Set(3, Napi::Number::New(env, e->maxy()));
        return arr;
    }
    else if(a == "aspect_fix_mode")
        return Napi::Number::New(env, m->map_->get_aspect_fix_mode());
    else if(a == "width")
        return Napi::Number::New(env, m->map_->width());
    else if(a == "height")
        return Napi::Number::New(env, m->map_->height());
    else if (a == "srs")
        return Napi::String::New(env, m->map_->srs());
    else if(a == "bufferSize")
        return Napi::Number::New(env, m->map_->buffer_size());
    else if (a == "background") {
        boost::optional<mapnik::color> c = m->map_->background();
        if (c)
            return Color::NewInstance(*c);
        else
            return;
    }
    else //if (a == "parameters")
    {
        Napi::Object ds = Napi::Object::New(env);
        mapnik::parameters const& params = m->map_->get_extra_parameters();
        mapnik::parameters::const_iterator it = params.begin();
        mapnik::parameters::const_iterator end = params.end();
        for (; it != end; ++it)
        {
            node_mapnik::params_to_object(ds, it->first, it->second);
        }
        return ds;
    }
}

void Map::set_prop(Napi::CallbackInfo const& info, const Napi::Value& value)
{
    Map* m = info.Holder().Unwrap<Map>();
    std::string a = TOSTR(property);
    if(a == "extent" || a == "maximumExtent") {
        if (!value->IsArray()) {
            Napi::Error::New(env, "Must provide an array of: [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            Napi::Array arr = value.As<Napi::Array>();
            if (arr->Length() != 4) {
                Napi::Error::New(env, "Must provide an array of: [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Null();
            } else {
                double minx = arr.Get(0.As<Napi::Number>().DoubleValue());
                double miny = arr.Get(1.As<Napi::Number>().DoubleValue());
                double maxx = arr.Get(2.As<Napi::Number>().DoubleValue());
                double maxy = arr.Get(3.As<Napi::Number>().DoubleValue());
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
        if (!value.IsNumber()) {
            Napi::Error::New(env, "'aspect_fix_mode' must be a constant (number)").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            int val = value.As<Napi::Number>().Int32Value();
            if (val < mapnik::Map::aspect_fix_mode_MAX && val >= 0) {
                m->map_->set_aspect_fix_mode(static_cast<mapnik::Map::aspect_fix_mode>(val));
            } else {
                Napi::Error::New(env, "'aspect_fix_mode' value is invalid").ThrowAsJavaScriptException();
                return env.Null();
            }
        }
    }
    else if (a == "srs")
    {
        if (!value.IsString()) {
            Napi::Error::New(env, "'srs' must be a string").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            m->map_->set_srs(TOSTR(value));
        }
    }
    else if (a == "bufferSize") {
        if (!value.IsNumber()) {
            Napi::TypeError::New(env, "Must provide an integer bufferSize").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            m->map_->set_buffer_size(value.As<Napi::Number>().Int32Value());
        }
    }
    else if (a == "width") {
        if (!value.IsNumber()) {
            Napi::TypeError::New(env, "Must provide an integer width").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            m->map_->set_width(value.As<Napi::Number>().Int32Value());
        }
    }
    else if (a == "height") {
        if (!value.IsNumber()) {
            Napi::TypeError::New(env, "Must provide an integer height").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            m->map_->set_height(value.As<Napi::Number>().Int32Value());
        }
    }
    else if (a == "background") {
        if (!value.IsObject()) {
            Napi::TypeError::New(env, "mapnik.Color expected").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object obj = value.As<Napi::Object>();
        if (!Napi::New(env, Color::constructor)->HasInstance(obj)) {
            Napi::TypeError::New(env, "mapnik.Color expected").ThrowAsJavaScriptException();
            return env.Null();
        }
        Color *c = obj.Unwrap<Color>();
        m->map_->set_background(*c->get());
    }
    else if (a == "parameters") {
        if (!value.IsObject()) {
            Napi::TypeError::New(env, "object expected for map.parameters").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object obj = value->ToObject(Napi::GetCurrentContext());
        mapnik::parameters params;
        Napi::Array names = Napi::GetPropertyNames(obj);
        unsigned int i = 0;
        unsigned int a_length = names->Length();
        while (i < a_length) {
            Napi::Value name = (names).Get(i)->ToString(Napi::GetCurrentContext());
            Napi::Value a_value = (obj).Get(name);
            if (a_value.IsString()) {
                params[TOSTR(name)] = const_cast<char const*>(TOSTR(a_value));
            } else if (a_value.IsNumber()) {
                double num = a_value.As<Napi::Number>().DoubleValue();
                // todo - round
                if (num == a_value.As<Napi::Number>().Int32Value()) {
                    params[TOSTR(name)] = Napi::To<node_mapnik::value_integer>(a_value);
                } else {
                    double dub_val = a_value.As<Napi::Number>().DoubleValue();
                    params[TOSTR(name)] = dub_val;
                }
            } else if (a_value->IsBoolean()) {
                params[TOSTR(name)] = Napi::To<mapnik::value_bool>(a_value);
            }
            i++;
        }
        m->map_->set_extra_parameters(params);
    }
}
*/
/**
 * Load fonts from local or external source
 *
 * @name loadFonts
 * @memberof Map
 * @instance
 *
 */

Napi::Value Map::loadFonts(Napi::CallbackInfo const& info)
{
    return Napi::Boolean::New(info.Env(), map_->load_fonts());
}

Napi::Value Map::memoryFonts(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    auto const& font_cache = map_->get_font_memory_cache();
    Napi::Array arr = Napi::Array::New(env, font_cache.size());
    std::size_t index = 0u;
    for (auto const& kv : font_cache)
    {
        arr.Set(index++, kv.first);
    }
    return scope.Escape(arr);
}

Napi::Value Map::registerFonts(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 0 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to a directory of fonts")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    bool recurse = false;
    if (info.Length() >= 2)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "second argument is optional, but if provided must be an object, eg. { recurse: true }")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("recurse"))
        {
            Napi::Value recurse_opt = options.Get("recurse");
            if (!recurse_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "'recurse' must be a Boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            recurse = recurse_opt.As<Napi::Boolean>();
        }
    }
    std::string path = info[0].As<Napi::String>();
    return Napi::Boolean::New(env, map_->register_fonts(path, recurse));
}

/**
 * Get all of the fonts currently registered as part of this map
 * @memberof Map
 * @instance
 * @name font
 * @returns {Array<string>} fonts
 */
Napi::Value Map::fonts(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    auto const& mapping = map_->get_font_file_mapping();
    Napi::Array arr = Napi::Array::New(env, mapping.size());
    std::size_t index = 0u;
    for (auto const& kv : mapping)
    {
        arr.Set(index++, kv.first);
    }
    return scope.Escape(arr);
}

/**
 * Get all of the fonts currently registered as part of this map, as a mapping
 * from font to font file
 * @memberof Map
 * @instance
 * @name fontFiles
 * @returns {Object} fonts
 */
Napi::Value Map::fontFiles(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    auto const& mapping = map_->get_font_file_mapping();
    Napi::Object obj = Napi::Object::New(env);
    for (auto const& kv : mapping)
    {
        obj.Set(kv.first, kv.second.second);
    }
    return scope.Escape(obj);
}

/**
 * Get the currently-registered font directory, if any
 * @memberof Map
 * @instance
 * @name fontDirectory
 * @returns {string|undefined} fonts
 */
Napi::Value Map::fontDirectory(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    boost::optional<std::string> const& font_dir = map_->font_directory();
    if (font_dir)
    {
        return Napi::String::New(env, *font_dir);
    }
    return env.Null();
}

/**
 * Get the map's scale factor. This is the ratio between pixels and geographical
 * units like meters.
 * @memberof Map
 * @instance
 * @name scale
 * @returns {number} scale
 */
Napi::Value Map::scale(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), map_->scale());
}

/**
 * Get the map's scale denominator.
 *
 * @memberof Map
 * @instance
 * @name scaleDenominator
 * @returns {number} scale denominator
 */
Napi::Value Map::scaleDenominator(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), map_->scale_denominator());
}

/*
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
    Napi::FunctionReference cb;
} query_map_baton_t;
*/

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
/*
Napi::Value Map::queryMapPoint(Napi::CallbackInfo const& info)
{
    abstractQueryPoint(info,false);
    return;
}
*/
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
/*
Napi::Value Map::queryPoint(Napi::CallbackInfo const& info)
{
    abstractQueryPoint(info,true);
    return;
}

Napi::Value Map::abstractQueryPoint(Napi::CallbackInfo const& info, bool geo_coords)
{
    Napi::HandleScope scope(env);
    if (info.Length() < 3)
    {
        Napi::Error::New(env, "requires at least three arguments, a x,y query and a callback").ThrowAsJavaScriptException();

        return env.Undefined();
    }

    double x,y;
    if (!info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "x,y arguments must be numbers").ThrowAsJavaScriptException();

        return env.Undefined();
    }
    else
    {
        x = info[0].As<Napi::Number>().DoubleValue();
        y = info[1].As<Napi::Number>().DoubleValue();
    }

    Map* m = info.Holder().Unwrap<Map>();

    Napi::Object options = Napi::Object::New(env);
    int layer_idx = -1;

    if (info.Length() > 3)
    {
        // options object
        if (!info[2].IsObject()) {
            Napi::TypeError::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();

            return env.Undefined();
        }

        options = info[2].ToObject(Napi::GetCurrentContext());

        if ((options).Has(Napi::String::New(env, "layer")).FromMaybe(false))
        {
            std::vector<mapnik::layer> const& layers = m->map_->layers();
            Napi::Value layer_id = (options).Get(Napi::String::New(env, "layer"));
            if (! (layer_id.IsString() || layer_id.IsNumber()) ) {
                Napi::TypeError::New(env, "'layer' option required for map query and must be either a layer name(string) or layer index (integer)").ThrowAsJavaScriptException();

                return env.Undefined();
            }

            if (layer_id.IsString()) {
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
                    Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();

                    return env.Undefined();
                }
            }
            else if (layer_id.IsNumber())
            {
                layer_idx = layer_id.As<Napi::Number>().Int32Value();
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
                    Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();

                    return env.Undefined();
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
                    Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();

                    return env.Undefined();
                }
            }
        }
    }

    // ensure function callback
    Napi::Value callback = info[info.Length() - 1];
    if (!callback->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();

        return env.Undefined();
    }

    query_map_baton_t *closure = new query_map_baton_t();
    closure->request.data = closure;
    closure->m = m;
    closure->x = x;
    closure->y = y;
    closure->layer_idx = static_cast<std::size_t>(layer_idx);
    closure->geo_coords = geo_coords;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_QueryMap, (uv_after_work_cb)EIO_AfterQueryMap);
    m->Ref();
    return env.Undefined();
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    query_map_baton_t *closure = static_cast<query_map_baton_t *>(req->data);
    if (closure->error) {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else {
        std::size_t num_result = closure->featuresets.size();
        if (num_result >= 1)
        {
            Napi::Array a = Napi::Array::New(env, num_result);
            typedef std::map<std::string,mapnik::featureset_ptr> fs_itr;
            fs_itr::const_iterator it = closure->featuresets.begin();
            fs_itr::const_iterator end = closure->featuresets.end();
            unsigned idx = 0;
            for (; it != end; ++it)
            {
                Napi::Object obj = Napi::Object::New(env);
                (obj).Set(Napi::String::New(env, "layer"), Napi::String::New(env, it->first));
                (obj).Set(Napi::String::New(env, "featureset"), Featureset::NewInstance(it->second));
                (a).Set(idx, obj);
                ++idx;
            }
            closure->featuresets.clear();
            Napi::Value argv[2] = { env.Null(), a };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), env.Undefined() };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }

    closure->m->Unref();
    closure->cb.Reset();
    delete closure;
}
*/

/**
 * Get all of the currently-added layers in this map
 *
 * @memberof Map
 * @instance
 * @name layers
 * @returns {Array<mapnik.Layer>} layers
 */

Napi::Value Map::layers(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    std::vector<mapnik::layer> const& layers = map_->layers();
    std::size_t size = layers.size();
    Napi::Array arr = Napi::Array::New(env, size);

    for (std::size_t index = 0; index < size; ++index )
    {
        auto layer = std::make_shared<mapnik::layer>(layers[index]);
        Napi::Value arg = Napi::External<layer_ptr>::New(env, &layer);
        Napi::Object obj = Layer::constructor.New({arg});
        arr.Set(index, obj);
    }
    return scope.Escape(arr);
}

/**
 * Add a new layer to this map
 *
 * @memberof Map
 * @instance
 * @name add_layer
 * @param {mapnik.Layer} new layer
 */

Napi::Value Map::add_layer(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    if (!info[0].IsObject())
    {
        Napi::TypeError::New(env, "mapnik.Layer expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.InstanceOf(Layer::constructor.Value()))
    {
        Napi::TypeError::New(env, "mapnik.Layer expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Layer *layer = Napi::ObjectWrap<Layer>::Unwrap(obj);
    map_->add_layer(*layer->impl());
    return Napi::Boolean::New(env, true);
}

/**
 * Remove layer from this map
 *
 * @memberof Map
 * @instance
 * @name remove_layer
 * @param {number} layer index
 */
Napi::Value Map::remove_layer(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1)
    {
        Napi::Error::New(env, "Please provide layer index").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (!info[0].IsNumber())
    {
        Napi::TypeError::New(env, "index must be number").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::vector<mapnik::layer> const& layers = map_->layers();

    unsigned int index = info[0].As<Napi::Number>().Int32Value();

    if (index < layers.size())
    {
        map_->remove_layer(index);
        return Napi::Boolean::New(env, true);
    }
    Napi::TypeError::New(env, "invalid layer index").ThrowAsJavaScriptException();
    return env.Undefined();
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
Napi::Value Map::get_layer(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() != 1)
    {
        Napi::Error::New(env, "Please provide layer name or index").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::vector<mapnik::layer> const& layers = map_->layers();

    Napi::Value key = info[0];
    if (key.IsNumber())
    {
        unsigned int index = key.As<Napi::Number>().Int32Value();
        if (index < layers.size())
        {
            auto layer = std::make_shared<mapnik::layer>(layers[index]);
            Napi::Value arg = Napi::External<layer_ptr>::New(env, &layer);
            Napi::Object obj = Layer::constructor.New({arg});
            return scope.Escape(napi_value(obj)).ToObject();
        }
        else
        {
            Napi::TypeError::New(env, "invalid layer index").ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    else if (key.IsString())
    {
        std::string layer_name = key.As<Napi::String>();
        std::size_t index = 0;
        for (mapnik::layer const& lyr : layers)
        {
            if (lyr.name() == layer_name)
            {
                auto layer = std::make_shared<mapnik::layer>(layers[index]);
                Napi::Value arg = Napi::External<layer_ptr>::New(env, &layer);
                Napi::Object obj = Layer::constructor.New({arg});
                return scope.Escape(napi_value(obj)).ToObject();
            }
            ++index;
        }
        std::ostringstream s;
        s << "Layer name '" << layer_name << "' not found";
        Napi::TypeError::New(env, s.str()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::TypeError::New(env, "first argument must be either a layer name(string) or layer index (integer)")
        .ThrowAsJavaScriptException();
    return env.Undefined();
}

/**
 * Remove all layers and styles from this map
 *
 * @memberof Map
 * @instance
 * @name clear
 */
Napi::Value Map::clear(Napi::CallbackInfo const& info)
{
    map_->remove_all();
    return info.Env().Undefined();
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
Napi::Value Map::resize(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 2)
    {
        Napi::Error::New(env, "Please provide width and height").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (!info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "width and height must be integers").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    map_->resize(info[0].As<Napi::Number>().Int32Value(),info[1].As<Napi::Number>().Int32Value());
    return env.Undefined();
}

/*
typedef struct {
    uv_work_t request;
    Map *m;
    std::string stylesheet;
    std::string base_path;
    bool strict;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
} load_xml_baton_t;

*/
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

/*
Napi::Value Map::load(Napi::CallbackInfo const& info)
{
    if (info.Length() < 2) {
        Napi::Error::New(env, "please provide a stylesheet path, options, and callback").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure stylesheet path is a string
    Napi::Value stylesheet = info[0];
    if (!stylesheet.IsString()) {
        Napi::TypeError::New(env, "first argument must be a path to a mapnik stylesheet").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    if (!callback->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure options object
    if (!info[1].IsObject()) {
        Napi::TypeError::New(env, "options must be an object, eg {strict: true}").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object options = info[1].As<Napi::Object>();

    bool strict = false;
    Napi::String param = Napi::String::New(env, "strict");
    if ((options).Has(param).ToChecked())
    {
        Napi::Value param_val = (options).Get(param);
        if (!param_val->IsBoolean()) {
            Napi::TypeError::New(env, "'strict' must be a Boolean").ThrowAsJavaScriptException();
            return env.Null();
        }
        strict = param_val.As<Napi::Boolean>().Value();
    }

    Map* m = info.Holder().Unwrap<Map>();

    load_xml_baton_t *closure = new load_xml_baton_t();
    closure->request.data = closure;

    param = Napi::String::New(env, "base");
    if ((options).Has(param).ToChecked())
    {
        Napi::Value param_val = (options).Get(param);
        if (!param_val.IsString()) {
            Napi::TypeError::New(env, "'base' must be a string representing a filesystem path").ThrowAsJavaScriptException();
            return env.Null();
        }
        closure->base_path = TOSTR(param_val);
    }

    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);
    if (closure->error) {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else {
        Napi::Value argv[2] = { env.Null(), closure->m->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->cb.Reset();
    delete closure;
}
*/

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
/*
Napi::Value Map::loadSync(Napi::CallbackInfo const& info)
{
    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "first argument must be a path to a mapnik stylesheet").ThrowAsJavaScriptException();
        return env.Null();
    }

    Map* m = info.Holder().Unwrap<Map>();
    std::string stylesheet = TOSTR(info[0]);
    bool strict = false;
    std::string base_path;

    if (info.Length() > 2)
    {

        Napi::Error::New(env, "only accepts two arguments: a path to a mapnik stylesheet and an optional options object").ThrowAsJavaScriptException();
        return env.Null();
    }
    else if (info.Length() == 2)
    {
        // ensure options object
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "options must be an object, eg {strict: true}").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        Napi::String param = Napi::String::New(env, "strict");
        if ((options).Has(param).ToChecked())
        {
            Napi::Value param_val = (options).Get(param);
            if (!param_val->IsBoolean())
            {
                Napi::TypeError::New(env, "'strict' must be a Boolean").ThrowAsJavaScriptException();
                return env.Null();
            }
            strict = param_val.As<Napi::Boolean>().Value();
        }

        param = Napi::String::New(env, "base");
        if ((options).Has(param).ToChecked())
        {
            Napi::Value param_val = (options).Get(param);
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "'base' must be a string representing a filesystem path").ThrowAsJavaScriptException();
                return env.Null();
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
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
    return;
}
*/
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
 /*
Napi::Value Map::fromStringSync(Napi::CallbackInfo const& info)
{
    if (info.Length() < 1) {
        Napi::Error::New(env, "Accepts 2 arguments: stylesheet string and an optional options").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "first argument must be a mapnik stylesheet string").ThrowAsJavaScriptException();
        return env.Null();
    }


    // defaults
    bool strict = false;
    std::string base_path("");

    if (info.Length() >= 2) {
        // ensure options object
        if (!info[1].IsObject()) {
            Napi::TypeError::New(env, "options must be an object, eg {strict: true, base: \".\"'}").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        Napi::String param = Napi::String::New(env, "strict");
        if ((options).Has(param).ToChecked())
        {
            Napi::Value param_val = (options).Get(param);
            if (!param_val->IsBoolean()) {
                Napi::TypeError::New(env, "'strict' must be a Boolean").ThrowAsJavaScriptException();
                return env.Null();
            }
            strict = param_val.As<Napi::Boolean>().Value();
        }

        param = Napi::String::New(env, "base");
        if ((options).Has(param).ToChecked())
        {
            Napi::Value param_val = (options).Get(param);
            if (!param_val.IsString()) {
                Napi::TypeError::New(env, "'base' must be a string representing a filesystem path").ThrowAsJavaScriptException();
                return env.Null();
            }
            base_path = TOSTR(param_val);
        }
    }

    Map* m = info.Holder().Unwrap<Map>();

    std::string stylesheet = TOSTR(info[0]);

    try
    {
        mapnik::load_map_string(*m->map_,stylesheet,strict,base_path);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
    return;
}
 */
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
  /*
Napi::Value Map::fromString(Napi::CallbackInfo const& info)
{
    if (info.Length() < 2)
    {
        Napi::Error::New(env, "please provide a stylesheet string, options, and callback").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure stylesheet path is a string
    Napi::Value stylesheet = info[0];
    if (!stylesheet.IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to a mapnik stylesheet string").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure options object
    if (!info[1].IsObject())
    {
        Napi::TypeError::New(env, "options must be an object, eg {strict: true, base: \".\"'}").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object options = info[1].ToObject(Napi::GetCurrentContext());

    bool strict = false;
    Napi::String param = Napi::String::New(env, "strict");
    if ((options).Has(param).ToChecked())
    {
        Napi::Value param_val = (options).Get(param);
        if (!param_val->IsBoolean())
        {
            Napi::TypeError::New(env, "'strict' must be a Boolean").ThrowAsJavaScriptException();
            return env.Null();
        }
        strict = param_val.As<Napi::Boolean>().Value();
    }

    Map* m = info.Holder().Unwrap<Map>();

    load_xml_baton_t *closure = new load_xml_baton_t();
    closure->request.data = closure;

    param = Napi::String::New(env, "base");
    if ((options).Has(param).ToChecked())
    {
        Napi::Value param_val = (options).Get(param);
        if (!param_val.IsString())
        {
            Napi::TypeError::New(env, "'base' must be a string representing a filesystem path").ThrowAsJavaScriptException();
            return env.Null();
        }
        closure->base_path = TOSTR(param_val);
    }

    closure->stylesheet = TOSTR(stylesheet);
    closure->m = m;
    closure->strict = strict;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    load_xml_baton_t *closure = static_cast<load_xml_baton_t *>(req->data);
    if (closure->error) {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else {
        Napi::Value argv[2] = { env.Null(), closure->m->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->cb.Reset();
    delete closure;
}
  */
/**
 * Clone this map object, returning a value which can be changed
 * without mutating the original
 *
 * @instance
 * @name clone
 * @memberof Map
 * @returns {mapnik.Map} clone
 */

Napi::Value Map::clone(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    try
    {
        auto map = std::make_shared<mapnik::Map>(*map_);
        Napi::Value arg = Napi::External<map_ptr>::New(env, &map);
        Napi::Object obj = Map::constructor.New({arg});
        return scope.Escape(napi_value(obj)).ToObject();
    }
    catch (...)
    {
        Napi::Error::New(env, "Could not create new Map instance").ThrowAsJavaScriptException();
    }
    return env.Undefined();
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

Napi::Value Map::save(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to map.xml to save").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string filename = info[0].As<Napi::String>();
    bool explicit_defaults = false;
    try
    {
        mapnik::save_map(*map_, filename, explicit_defaults);
        return Napi::Boolean::New(env, true);
    }
    catch (...) {}
    return Napi::Boolean::New(env, false);
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
Napi::Value Map::toXML(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    bool explicit_defaults = false;
    try
    {
        std::string map_string = mapnik::save_map_to_string(*map_, explicit_defaults);
        return Napi::String::New(env, map_string);
    }
    catch (...) {}
    Napi::TypeError::New(env, "Failed to export to XML").ThrowAsJavaScriptException();
    return env.Undefined();
}

Napi::Value Map::zoomAll(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    try
    {
        map_->zoom_all();
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

Napi::Value Map::zoomToBox(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    double minx;
    double miny;
    double maxx;
    double maxy;

    if (info.Length() == 1)
    {
        if (!info[0].IsArray())
        {
            Napi::Error::New(env, "Must provide an array of: [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Array arr = info[0].As<Napi::Array>();
        if (arr.Length() != 4
            || !arr.Get(0u).IsNumber()
            || !arr.Get(1u).IsNumber()
            || !arr.Get(2u).IsNumber()
            || !arr.Get(3u).IsNumber())
        {
            Napi::Error::New(env, "Must provide an array of: [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        minx = arr.Get(0u).As<Napi::Number>().DoubleValue();
        miny = arr.Get(1u).As<Napi::Number>().DoubleValue();
        maxx = arr.Get(2u).As<Napi::Number>().DoubleValue();
        maxy = arr.Get(3u).As<Napi::Number>().DoubleValue();

    }
    else if (info.Length() != 4)
    {
        Napi::Error::New(env, "Must provide 4 arguments: minx,miny,maxx,maxy").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    else if (info[0].IsNumber() &&
             info[1].IsNumber() &&
             info[2].IsNumber() &&
             info[3].IsNumber())
    {
        minx = info[0].As<Napi::Number>().DoubleValue();
        miny = info[1].As<Napi::Number>().DoubleValue();
        maxx = info[2].As<Napi::Number>().DoubleValue();
        maxy = info[3].As<Napi::Number>().DoubleValue();
    }
    else
    {
        Napi::Error::New(env, "If you are providing 4 arguments: minx,miny,maxx,maxy - they must be all numbers")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    mapnik::box2d<double> box{minx,miny,maxx,maxy};
    map_->zoom_to_box(box);
    return env.Undefined();
}
/*
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
    Napi::FunctionReference cb;
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
    Napi::FunctionReference cb;
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
    Napi::FunctionReference cb;
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
*/
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

/*
Napi::Value Map::render(Napi::CallbackInfo const& info)
{
    // ensure at least 2 args
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "requires at least two arguments, a renderable mapnik object, and a callback").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure renderable object
    if (!info[0].IsObject()) {
        Napi::TypeError::New(env, "requires a renderable mapnik object to be passed as first argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure function callback
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    Map* m = info.Holder().Unwrap<Map>();

    try
    {
        // parse options

        // defaults
        int buffer_size = 0;
        double scale_factor = 1.0;
        double scale_denominator = 0.0;
        unsigned offset_x = 0;
        unsigned offset_y = 0;

        Napi::Object options = Napi::Object::New(env);

        if (info.Length() > 2) {

            // options object
            if (!info[1].IsObject()) {
                Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
                return env.Null();
            }

            options = info[1].ToObject(Napi::GetCurrentContext());

            if ((options).Has(Napi::String::New(env, "buffer_size")).FromMaybe(false)) {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "buffer_size"));
                if (!bind_opt.IsNumber()) {
                    Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                    return env.Null();
                }
                buffer_size = bind_opt.As<Napi::Number>().Int32Value();
            }

            if ((options).Has(Napi::String::New(env, "scale")).FromMaybe(false)) {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "scale"));
                if (!bind_opt.IsNumber()) {
                    Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                    return env.Null();
                }

                scale_factor = bind_opt.As<Napi::Number>().DoubleValue();
            }

            if ((options).Has(Napi::String::New(env, "scale_denominator")).FromMaybe(false)) {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "scale_denominator"));
                if (!bind_opt.IsNumber()) {
                    Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                    return env.Null();
                }

                scale_denominator = bind_opt.As<Napi::Number>().DoubleValue();
            }

            if ((options).Has(Napi::String::New(env, "offset_x")).FromMaybe(false)) {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_x"));
                if (!bind_opt.IsNumber()) {
                    Napi::TypeError::New(env, "optional arg 'offset_x' must be a number").ThrowAsJavaScriptException();
                    return env.Null();
                }

                offset_x = bind_opt.As<Napi::Number>().Int32Value();
            }

            if ((options).Has(Napi::String::New(env, "offset_y")).FromMaybe(false)) {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "offset_y"));
                if (!bind_opt.IsNumber()) {
                    Napi::TypeError::New(env, "optional arg 'offset_y' must be a number").ThrowAsJavaScriptException();
                    return env.Null();
                }

                offset_y = bind_opt.As<Napi::Number>().Int32Value();
            }
        }

        Napi::Object obj = info[0].ToObject(Napi::GetCurrentContext());

        if (Napi::New(env, Image::constructor)->HasInstance(obj)) {

            image_baton_t *closure = new image_baton_t();
            closure->request.data = closure;
            closure->m = m;
            closure->im = obj.Unwrap<Image>();
            closure->buffer_size = buffer_size;
            closure->scale_factor = scale_factor;
            closure->scale_denominator = scale_denominator;
            closure->offset_x = offset_x;
            closure->offset_y = offset_y;
            closure->error = false;

            if ((options).Has(Napi::String::New(env, "variables")).FromMaybe(false))
            {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "variables"));
                if (!bind_opt.IsObject())
                {
                    delete closure;
                    Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
                    return env.Null();
                }
                object_to_container(closure->variables,bind_opt->ToObject(Napi::GetCurrentContext()));
            }
            if (!m->acquire())
            {
                delete closure;
                Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
                return env.Null();
            }
            closure->cb.Reset(info[info.Length() - 1].As<Napi::Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderImage, (uv_after_work_cb)EIO_AfterRenderImage);
            closure->im->Ref();
        }
#if defined(GRID_RENDERER)
        else if (Napi::New(env, Grid::constructor)->HasInstance(obj)) {

            Grid * g = obj.Unwrap<Grid>();

            std::size_t layer_idx = 0;

            // grid requires special options for now
            if (!(options).Has(Napi::String::New(env, "layer")).FromMaybe(false)) {
                Napi::TypeError::New(env, "'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)").ThrowAsJavaScriptException();
                return env.Null();
            } else {

                std::vector<mapnik::layer> const& layers = m->map_->layers();

                Napi::Value layer_id = (options).Get(Napi::String::New(env, "layer"));
                if (! (layer_id.IsString() || layer_id.IsNumber()) ) {
                    Napi::TypeError::New(env, "'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)").ThrowAsJavaScriptException();
                    return env.Null();
                }

                if (layer_id.IsString()) {
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
                        Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();
                        return env.Null();
                    }
                } else { // IS NUMBER
                    layer_idx = layer_id.As<Napi::Number>().Int32Value();
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
                        Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();
                        return env.Null();
                    }
                }
            }

            if ((options).Has(Napi::String::New(env, "fields")).FromMaybe(false)) {

                Napi::Value param_val = (options).Get(Napi::String::New(env, "fields"));
                if (!param_val->IsArray()) {
                    Napi::TypeError::New(env, "option 'fields' must be an array of strings").ThrowAsJavaScriptException();
                    return env.Null();
                }
                Napi::Array a = param_val.As<Napi::Array>();
                unsigned int i = 0;
                unsigned int num_fields = a->Length();
                while (i < num_fields) {
                    Napi::Value name = (a).Get(i);
                    if (name.IsString()){
                        g->get()->add_field(TOSTR(name));
                    }
                    i++;
                }
            }

            grid_baton_t *closure = new grid_baton_t();

            if ((options).Has(Napi::String::New(env, "variables")).FromMaybe(false))
            {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "variables"));
                if (!bind_opt.IsObject())
                {
                    delete closure;
                    Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
                    return env.Null();
                }
                object_to_container(closure->variables,bind_opt->ToObject(Napi::GetCurrentContext()));
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
                Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
                return env.Null();
            }
            closure->cb.Reset(info[info.Length() - 1].As<Napi::Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderGrid, (uv_after_work_cb)EIO_AfterRenderGrid);
            closure->g->Ref();
        }
#endif
        else if (Napi::New(env, VectorTile::constructor)->HasInstance(obj))
        {

            vector_tile_baton_t *closure = new vector_tile_baton_t();

            if ((options).Has(Napi::String::New(env, "image_scaling")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "image_scaling"));
                if (!param_val.IsString())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'image_scaling' must be a string").ThrowAsJavaScriptException();
                    return env.Null();
                }
                std::string image_scaling = TOSTR(param_val);
                boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
                if (!method)
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->scaling_method = *method;
            }

            if ((options).Has(Napi::String::New(env, "image_format")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "image_format"));
                if (!param_val.IsString())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'image_format' must be a string").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->image_format = TOSTR(param_val);
            }

            if ((options).Has(Napi::String::New(env, "area_threshold")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "area_threshold"));
                if (!param_val.IsNumber())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'area_threshold' must be a number").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->area_threshold = param_val.As<Napi::Number>().DoubleValue();
                if (closure->area_threshold < 0.0)
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'area_threshold' must not be a negative number").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }

            if ((options).Has(Napi::String::New(env, "strictly_simple")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "strictly_simple"));
                if (!param_val->IsBoolean())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'strictly_simple' must be a boolean").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->strictly_simple = param_val.As<Napi::Boolean>().Value();
            }

            if ((options).Has(Napi::String::New(env, "multi_polygon_union")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "multi_polygon_union"));
                if (!param_val->IsBoolean())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'multi_polygon_union' must be a boolean").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->multi_polygon_union = param_val.As<Napi::Boolean>().Value();
            }

            if ((options).Has(Napi::String::New(env, "fill_type")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "fill_type"));
                if (!param_val.IsNumber())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'fill_type' must be an unsigned integer").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(param_val.As<Napi::Number>().Int32Value());
                if (closure->fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
                {
                    delete closure;
                    Napi::TypeError::New(env, "optional arg 'fill_type' out of possible range").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }

            if ((options).Has(Napi::String::New(env, "threading_mode")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "threading_mode"));
                if (!param_val.IsNumber())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'threading_mode' must be an unsigned integer").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->threading_mode = static_cast<std::launch>(param_val.As<Napi::Number>().Int32Value());
                if (closure->threading_mode != std::launch::async &&
                    closure->threading_mode != std::launch::deferred &&
                    closure->threading_mode != (std::launch::async | std::launch::deferred))
                {
                    delete closure;
                    Napi::TypeError::New(env, "optional arg 'threading_mode' value passed is invalid").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }

            if ((options).Has(Napi::String::New(env, "simplify_distance")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "simplify_distance"));
                if (!param_val.IsNumber())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'simplify_distance' must be an floating point number").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->simplify_distance = param_val.As<Napi::Number>().DoubleValue();
                if (closure->simplify_distance < 0)
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'simplify_distance' can not be negative").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }

            if ((options).Has(Napi::String::New(env, "variables")).FromMaybe(false))
            {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "variables"));
                if (!bind_opt.IsObject())
                {
                    delete closure;
                    Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
                    return env.Null();
                }
                object_to_container(closure->variables,bind_opt->ToObject(Napi::GetCurrentContext()));
            }

            if ((options).Has(Napi::String::New(env, "process_all_rings")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "process_all_rings"));
                if (!param_val->IsBoolean())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'process_all_rings' must be a boolean").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->process_all_rings = param_val.As<Napi::Boolean>().Value();
            }

            closure->request.data = closure;
            closure->m = m;
            closure->d = obj.Unwrap<VectorTile>();
            closure->scale_factor = scale_factor;
            closure->scale_denominator = scale_denominator;
            closure->offset_x = offset_x;
            closure->offset_y = offset_y;
            closure->error = false;
            if (!m->acquire())
            {
                delete closure;
                Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
                return env.Null();
            }
            closure->cb.Reset(info[info.Length() - 1].As<Napi::Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderVectorTile, (uv_after_work_cb)EIO_AfterRenderVectorTile);
            closure->d->Ref();
        }
        else
        {
            Napi::TypeError::New(env, "renderable mapnik object expected").ThrowAsJavaScriptException();
            return env.Null();
        }

        m->Ref();
        return;
    }
    catch (std::exception const& ex)
    {
        // I am not quite sure it is possible to put a test in to cover an exception here
        // LCOV_EXCL_START
        Napi::TypeError::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
// LCOV_EXCL_STOP
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    vector_tile_baton_t *closure = static_cast<vector_tile_baton_t *>(req->data);
    closure->m->release();

    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Napi::Value argv[2] = { env.Null(), closure->d->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);

    grid_baton_t *closure = static_cast<grid_baton_t *>(req->data);
    closure->m->release();

    if (closure->error) {
        // TODO - add more attributes
        // https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/Error
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else {
        Napi::Value argv[2] = { env.Null(), closure->g->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    image_baton_t *closure = static_cast<image_baton_t *>(req->data);
    closure->m->release();

    if (closure->error) {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else {
        Napi::Value argv[2] = { env.Null(), closure->im->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
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
    Napi::FunctionReference cb;
} render_file_baton_t;
*/

  /*
Napi::Value Map::renderFile(Napi::CallbackInfo const& info)
{
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "first argument must be a path to a file to save").ThrowAsJavaScriptException();
        return env.Null();
    }

    // defaults
    std::string format = "png";
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    palette_ptr palette;
    int buffer_size = 0;

    Napi::Value callback = info[info.Length()-1];

    if (!callback->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object options = Napi::Object::New(env);

    if (!info[1].IsFunction() && info[1].IsObject()) {
        options = info[1].ToObject(Napi::GetCurrentContext());
        if ((options).Has(Napi::String::New(env, "format")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "format"));
            if (!format_opt.IsString()) {
                Napi::TypeError::New(env, "'format' must be a String").ThrowAsJavaScriptException();
                return env.Null();
            }

            format = TOSTR(format_opt);
        }

        if ((options).Has(Napi::String::New(env, "palette")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "palette"));
            if (!format_opt.IsObject()) {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Null();
            }

            Napi::Object obj = format_opt->ToObject(Napi::GetCurrentContext());
            if (!Napi::New(env, Palette::constructor)->HasInstance(obj)) {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Null();
            }

            palette = obj)->palette(.Unwrap<Palette>();
        }
        if ((options).Has(Napi::String::New(env, "scale")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "scale"));
            if (!bind_opt.IsNumber()) {
                Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }

            scale_factor = bind_opt.As<Napi::Number>().DoubleValue();
        }

        if ((options).Has(Napi::String::New(env, "scale_denominator")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "scale_denominator"));
            if (!bind_opt.IsNumber()) {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }

            scale_denominator = bind_opt.As<Napi::Number>().DoubleValue();
        }

        if ((options).Has(Napi::String::New(env, "buffer_size")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "buffer_size"));
            if (!bind_opt.IsNumber()) {
                Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }

            buffer_size = bind_opt.As<Napi::Number>().Int32Value();
        }

    } else if (!info[1].IsFunction()) {
        Napi::TypeError::New(env, "optional argument must be an object").ThrowAsJavaScriptException();
        return env.Null();
    }

    Map* m = info.Holder().Unwrap<Map>();
    std::string output = TOSTR(info[0]);

    //maybe do this in the async part?
    if (format.empty()) {
        format = mapnik::guess_type(output);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << output << "\n";
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return env.Null();
        }
    }

    render_file_baton_t *closure = new render_file_baton_t();

    if ((options).Has(Napi::String::New(env, "variables")).FromMaybe(false))
    {
        Napi::Value bind_opt = (options).Get(Napi::String::New(env, "variables"));
        if (!bind_opt.IsObject())
        {
            delete closure;
            Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
            return env.Null();
        }
        object_to_container(closure->variables,bind_opt->ToObject(Napi::GetCurrentContext()));
    }

    if (format == "pdf" || format == "svg" || format == "ps" || format == "ARGB32" || format == "RGB24") {
#if defined(HAVE_CAIRO)
        closure->use_cairo = true;
#else
        delete closure;
        std::ostringstream s("");
        s << "Cairo backend is not available, cannot write to " << format << "\n";
        Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
        return env.Null();
#endif
    } else {
        closure->use_cairo = false;
    }

    if (!m->acquire())
    {
        delete closure;
        Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
        return env.Null();
    }
    closure->request.data = closure;

    closure->m = m;
    closure->scale_factor = scale_factor;
    closure->scale_denominator = scale_denominator;
    closure->buffer_size = buffer_size;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());

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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    render_file_baton_t *closure = static_cast<render_file_baton_t *>(req->data);
    closure->m->release();

    if (closure->error) {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else {
        Napi::Value argv[1] = { env.Null() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }

    closure->m->Unref();
    closure->cb.Reset();
    delete closure;

}
*/

// TODO - add support for grids
/*
Napi::Value Map::renderSync(Napi::CallbackInfo const& info)
{
    std::string format = "png";
    palette_ptr palette;
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    int buffer_size = 0;

    if (info.Length() >= 1)
    {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "first argument is optional, but if provided must be an object, eg. {format: 'pdf'}").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[0].ToObject(Napi::GetCurrentContext());
        if ((options).Has(Napi::String::New(env, "format")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "format"));
            if (!format_opt.IsString()) {
                Napi::TypeError::New(env, "'format' must be a String").ThrowAsJavaScriptException();
                return env.Null();
            }

            format = TOSTR(format_opt);
        }

        if ((options).Has(Napi::String::New(env, "palette")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "palette"));
            if (!format_opt.IsObject()) {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Null();
            }

            Napi::Object obj = format_opt->ToObject(Napi::GetCurrentContext());
            if (!Napi::New(env, Palette::constructor)->HasInstance(obj)) {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Null();
            }

            palette = obj)->palette(.Unwrap<Palette>();
        }
        if ((options).Has(Napi::String::New(env, "scale")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "scale"));
            if (!bind_opt.IsNumber()) {
                Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }

            scale_factor = bind_opt.As<Napi::Number>().DoubleValue();
        }
        if ((options).Has(Napi::String::New(env, "scale_denominator")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "scale_denominator"));
            if (!bind_opt.IsNumber()) {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }

            scale_denominator = bind_opt.As<Napi::Number>().DoubleValue();
        }
        if ((options).Has(Napi::String::New(env, "buffer_size")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "buffer_size"));
            if (!bind_opt.IsNumber()) {
                Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }

            buffer_size = bind_opt.As<Napi::Number>().Int32Value();
        }
    }

    Map* m = info.Holder().Unwrap<Map>();
    if (!m->acquire())
    {
        Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
        return env.Null();
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
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
    m->release();
    return Napi::Buffer::Copy(env, (char*)s.data(), s.size());
}

Napi::Value Map::renderFileSync(Napi::CallbackInfo const& info)
{
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "first argument must be a path to a file to save").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() > 2) {
        Napi::Error::New(env, "accepts two arguments, a required path to a file, an optional options object, eg. {format: 'pdf'}").ThrowAsJavaScriptException();
        return env.Null();
    }

    // defaults
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    int buffer_size = 0;
    std::string format = "png";
    palette_ptr palette;

    if (info.Length() >= 2){
        if (!info[1].IsObject()) {
            Napi::TypeError::New(env, "second argument is optional, but if provided must be an object, eg. {format: 'pdf'}").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[1].As<Napi::Object>();
        if ((options).Has(Napi::String::New(env, "format")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "format"));
            if (!format_opt.IsString()) {
                Napi::TypeError::New(env, "'format' must be a String").ThrowAsJavaScriptException();
                return env.Null();
            }

            format = TOSTR(format_opt);
        }

        if ((options).Has(Napi::String::New(env, "palette")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "palette"));
            if (!format_opt.IsObject()) {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Null();
            }

            Napi::Object obj = format_opt->ToObject(Napi::GetCurrentContext());
            if (!Napi::New(env, Palette::constructor)->HasInstance(obj)) {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Null();
            }

            palette = obj)->palette(.Unwrap<Palette>();
        }
        if ((options).Has(Napi::String::New(env, "scale")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "scale"));
            if (!bind_opt.IsNumber()) {
                Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }

            scale_factor = bind_opt.As<Napi::Number>().DoubleValue();
        }
        if ((options).Has(Napi::String::New(env, "scale_denominator")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "scale_denominator"));
            if (!bind_opt.IsNumber()) {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }

            scale_denominator = bind_opt.As<Napi::Number>().DoubleValue();
        }
        if ((options).Has(Napi::String::New(env, "buffer_size")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "buffer_size"));
            if (!bind_opt.IsNumber()) {
                Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                return env.Null();
            }

            buffer_size = bind_opt.As<Napi::Number>().Int32Value();
        }
    }

    Map* m = info.Holder().Unwrap<Map>();
    std::string output = TOSTR(info[0]);

    if (format.empty()) {
        format = mapnik::guess_type(output);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << output << "\n";
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return env.Null();
        }
    }
    if (!m->acquire())
    {
        Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
        return env.Null();
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
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return env.Null();
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
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
    m->release();
    return;
}
*/
