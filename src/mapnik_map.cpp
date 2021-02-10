#include "mapnik_map.hpp"
#include "utils.hpp"
#include "mapnik_color.hpp"   // for Color, Color::constructor
#include "mapnik_image.hpp"   // for Image, Image::constructor
#include "mapnik_layer.hpp"   // for Layer, Layer::constructor
#include "mapnik_palette.hpp" // for palette_ptr, Palette, etc
// mapnik
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>    // for layer
#include <mapnik/save_map.hpp> // for save_map, etc
// stl
#include <sstream> // for basic_ostringstream, etc

Napi::FunctionReference Map::constructor;

Napi::Object Map::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Map", {
            InstanceMethod<&Map::fonts>("fonts", prop_attr),
            InstanceMethod<&Map::fontFiles>("fontFiles", prop_attr),
            InstanceMethod<&Map::fontDirectory>("fontDirectory", prop_attr),
            InstanceMethod<&Map::memoryFonts>("memoryFonts", prop_attr),
            InstanceMethod<&Map::registerFonts>("registerFonts", prop_attr),
            InstanceMethod<&Map::loadFonts>("loadFonts", prop_attr),
            InstanceMethod<&Map::load>("load", prop_attr),
            InstanceMethod<&Map::loadSync>("loadSync", prop_attr),
            InstanceMethod<&Map::fromStringSync>("fromStringSync", prop_attr),
            InstanceMethod<&Map::fromString>("fromString", prop_attr),
            InstanceMethod<&Map::clone>("clone", prop_attr),
            InstanceMethod<&Map::save>("save", prop_attr),
            InstanceMethod<&Map::clear>("clear", prop_attr),
            InstanceMethod<&Map::toXML>("toXML", prop_attr),
            InstanceMethod<&Map::resize>("resize", prop_attr),
            InstanceMethod<&Map::render>("render", prop_attr),
            InstanceMethod<&Map::renderSync>("renderSync", prop_attr),
            InstanceMethod<&Map::renderFile>("renderFile", prop_attr),
            InstanceMethod<&Map::renderFileSync>("renderFileSync", prop_attr),
            InstanceMethod<&Map::zoomAll>("zoomAll", prop_attr),
            InstanceMethod<&Map::zoomToBox>("zoomToBox", prop_attr),
            InstanceMethod<&Map::scale>("scale", prop_attr),
            InstanceMethod<&Map::scaleDenominator>("scaleDenominator", prop_attr),
            InstanceMethod<&Map::queryPoint>("queryPoint", prop_attr),
            InstanceMethod<&Map::queryMapPoint>("queryMapPoint", prop_attr),
            InstanceMethod<&Map::add_layer>("add_layer", prop_attr),
            InstanceMethod<&Map::remove_layer>("remove_layer", prop_attr),
            InstanceMethod<&Map::get_layer>("get_layer", prop_attr),
            InstanceMethod<&Map::layers>("layers", prop_attr),
            // accessors
            InstanceAccessor<&Map::srs, &Map::srs>("srs", prop_attr),
            InstanceAccessor<&Map::width, &Map::width>("width", prop_attr),
            InstanceAccessor<&Map::height, &Map::height>("height", prop_attr),
            InstanceAccessor<&Map::bufferSize, &Map::bufferSize>("bufferSize", prop_attr),
            InstanceAccessor<&Map::extent, &Map::extent>("extent", prop_attr),
            InstanceAccessor<&Map::bufferedExtent>("bufferedExtent", prop_attr),
            InstanceAccessor<&Map::maximumExtent, &Map::maximumExtent>("maximumExtent", prop_attr),
            InstanceAccessor<&Map::background, &Map::background>("background", prop_attr),
            InstanceAccessor<&Map::parameters, &Map::parameters>("parameters", prop_attr),
            InstanceAccessor<&Map::aspect_fix_mode, &Map::aspect_fix_mode>("aspect_fix_mode", prop_attr)
        });
    // clang-format on
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
 * typically used with 'epsg:3857'
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
 * var map = new mapnik.Map(600, 400, 'epsg:3857');
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
 * //   srs: 'epsg:3857'
 * // }
 */

Map::Map(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Map>(info)
{
    Napi::Env env = info.Env();
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
        map_ = std::make_shared<mapnik::Map>(info[0].As<Napi::Number>().Int32Value(), info[1].As<Napi::Number>().Int32Value());
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
    boost::optional<mapnik::box2d<double>> const& e = map_->maximum_extent();
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
    boost::optional<mapnik::box2d<double>> const& e = map_->get_buffered_extent();
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
        return scope.Escape(obj);
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

// parameters
Napi::Value Map::parameters(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Object obj = Napi::Object::New(env);
    mapnik::parameters const& params = map_->get_extra_parameters();
    for (auto const& p : params)
    {
        node_mapnik::params_to_object(env, obj, p.first, p.second);
    }
    return scope.Escape(obj);
}

void Map::parameters(Napi::CallbackInfo const& info, Napi::Value const& value)
{
    Napi::Env env = info.Env();
    if (!value.IsObject())
    {
        Napi::TypeError::New(env, "object expected for map.parameters").ThrowAsJavaScriptException();
        return;
    }
    mapnik::parameters params;

    Napi::Object obj = value.As<Napi::Object>();
    Napi::Array names = obj.GetPropertyNames();
    std::size_t length = names.Length();
    for (std::size_t index = 0; index < length; ++index)
    {
        std::string name = names.Get(index).ToString();
        Napi::Value val = obj.Get(name);

        if (val.IsString())
        {
            params[name] = val.As<Napi::String>().Utf8Value();
        }
        else if (val.IsNumber())
        {
            double num = val.As<Napi::Number>().DoubleValue();
            // todo - round
            if (num == val.As<Napi::Number>().Int32Value())
            {
                params[name] = static_cast<node_mapnik::value_integer>(num);
            }
            else
            {
                params[name] = num;
            }
        }
        else if (val.IsBoolean())
        {
            params[name] = val.As<Napi::Boolean>().Value();
        }
    }
    map_->set_extra_parameters(params);
}

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
    return env.Undefined();
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

    for (std::size_t index = 0; index < size; ++index)
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
    Layer* layer = Napi::ObjectWrap<Layer>::Unwrap(obj);
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
            return scope.Escape(obj);
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
                return scope.Escape(obj);
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

    map_->resize(info[0].As<Napi::Number>().Int32Value(), info[1].As<Napi::Number>().Int32Value());
    return env.Undefined();
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

Napi::Value Map::clone(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    try
    {
        auto map = std::make_shared<mapnik::Map>(*map_);
        Napi::Value arg = Napi::External<map_ptr>::New(env, &map);
        Napi::Object obj = Map::constructor.New({arg});
        return scope.Escape(obj);
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
    catch (...)
    {
    }
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
    catch (...)
    {
    }
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
        if (arr.Length() != 4 || !arr.Get(0u).IsNumber() || !arr.Get(1u).IsNumber() || !arr.Get(2u).IsNumber() || !arr.Get(3u).IsNumber())
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

    mapnik::box2d<double> box{minx, miny, maxx, maxy};
    map_->zoom_to_box(box);
    return env.Undefined();
}
