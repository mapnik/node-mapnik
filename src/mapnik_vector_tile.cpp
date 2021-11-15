#include "utils.hpp"
#include "mapnik_map.hpp"
#include "mapnik_image.hpp"
#if defined(GRID_RENDERER)
#include "mapnik_grid.hpp"
#endif
#include "mapnik_feature.hpp"
#include "mapnik_cairo_surface.hpp"
#ifdef SVG_RENDERER
#include <mapnik/svg/output/svg_renderer.hpp>
#endif

#include "mapnik_vector_tile.hpp"
#include <mapnik/image_any.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/request.hpp>
#include <mapnik/scale_denominator.hpp>
#include <mapnik/version.hpp>
#if defined(GRID_RENDERER)
#include <mapnik/grid/grid.hpp>          // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp> // for grid_renderer
#endif
#ifdef HAVE_CAIRO
#include <mapnik/cairo/cairo_renderer.hpp>
#include <cairo.h>
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif // CAIRO_HAS_SVG_SURFACE
#endif

// std
#include <set>       // for set, etc
#include <sstream>   // for operator<<, basic_ostream, etc
#include <string>    // for string, char_traits, etc
#include <exception> // for exception
#include <vector>    // for vector

Napi::FunctionReference VectorTile::constructor;

Napi::Object VectorTile::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "VectorTile", {
            InstanceAccessor<&VectorTile::get_tile_x, &VectorTile::set_tile_x>("x", prop_attr),
            InstanceAccessor<&VectorTile::get_tile_y, &VectorTile::set_tile_y>("y", prop_attr),
            InstanceAccessor<&VectorTile::get_tile_z, &VectorTile::set_tile_z>("z", prop_attr),
            InstanceAccessor<&VectorTile::get_tile_size, &VectorTile::set_tile_size>("tileSize", prop_attr),
            InstanceAccessor<&VectorTile::get_buffer_size, &VectorTile::set_buffer_size>("bufferSize", prop_attr),
            InstanceMethod<&VectorTile::render>("render", prop_attr),
            InstanceMethod<&VectorTile::setData>("setData", prop_attr),
            InstanceMethod<&VectorTile::setDataSync>("setDataSync", prop_attr),
            InstanceMethod<&VectorTile::getData>("getData", prop_attr),
            InstanceMethod<&VectorTile::getDataSync>("getDataSync", prop_attr),
            InstanceMethod<&VectorTile::addData>("addData", prop_attr),
            InstanceMethod<&VectorTile::addDataSync>("addDataSync", prop_attr),
            InstanceMethod<&VectorTile::composite>("composite", prop_attr),
            InstanceMethod<&VectorTile::compositeSync>("compositeSync", prop_attr),
            InstanceMethod<&VectorTile::query>("query", prop_attr),
            InstanceMethod<&VectorTile::queryMany>("queryMany", prop_attr),
            InstanceMethod<&VectorTile::extent>("extent", prop_attr),
            InstanceMethod<&VectorTile::bufferedExtent>("bufferedExtent", prop_attr),
            InstanceMethod<&VectorTile::names>("names", prop_attr),
            InstanceMethod<&VectorTile::layer>("layer", prop_attr),
            InstanceMethod<&VectorTile::emptyLayers>("emptyLayers", prop_attr),
            InstanceMethod<&VectorTile::paintedLayers>("paintedLayers", prop_attr),
            InstanceMethod<&VectorTile::toJSON>("toJSON", prop_attr),
            InstanceMethod<&VectorTile::toGeoJSON>("toGeoJSON", prop_attr),
            InstanceMethod<&VectorTile::toGeoJSONSync>("toGeoJSONSync", prop_attr),
            InstanceMethod<&VectorTile::addGeoJSON>("addGeoJSON", prop_attr),
            InstanceMethod<&VectorTile::addImage>("addImage", prop_attr),
            InstanceMethod<&VectorTile::addImageSync>("addImageSync", prop_attr),
            InstanceMethod<&VectorTile::addImageBuffer>("addImageBuffer", prop_attr),
            InstanceMethod<&VectorTile::addImageBufferSync>("addImageBufferSync", prop_attr),
#if BOOST_VERSION >= 105600
            InstanceMethod<&VectorTile::reportGeometrySimplicity>("reportGeometrySimplicity", prop_attr),
            InstanceMethod<&VectorTile::reportGeometrySimplicitySync>("reportGeometrySimplicitySync", prop_attr),
            InstanceMethod<&VectorTile::reportGeometryValidity>("reportGeometryValidity", prop_attr),
            InstanceMethod<&VectorTile::reportGeometryValiditySync>("reportGeometryValiditySync", prop_attr),
#endif
            InstanceMethod<&VectorTile::painted>("painted", prop_attr),
            InstanceMethod<&VectorTile::clear>("clear", prop_attr),
            InstanceMethod<&VectorTile::clearSync>("clearSync", prop_attr),
            InstanceMethod<&VectorTile::empty>("empty", prop_attr),
            // static methods
            StaticMethod<&VectorTile::info>("info", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("VectorTile", func);
    return exports;
}

/**
 * **`mapnik.VectorTile`**

 * A tile generator built according to the [Mapbox Vector Tile](https://github.com/mapbox/vector-tile-spec)
 * specification for compressed and simplified tiled vector data.
 * Learn more about vector tiles [here](https://www.mapbox.com/developers/vector-tiles/).
 *
 * @class VectorTile
 * @param {number} z - an integer zoom level
 * @param {number} x - an integer x coordinate
 * @param {number} y - an integer y coordinate
 * @property {number} x - horizontal axis position
 * @property {number} y - vertical axis position
 * @property {number} z - the zoom level
 * @property {number} tileSize - the size of the tile
 * @property {number} bufferSize - the size of the tile's buffer
 * @example
 * var vt = new mapnik.VectorTile(9,112,195);
 * console.log(vt.z, vt.x, vt.y); // 9, 112, 195
 * console.log(vt.tileSize, vt.bufferSize); // 4096, 128
 */

VectorTile::VectorTile(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<VectorTile>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 1 && info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<mapnik::vector_tile_impl::merc_tile_ptr>>();
        if (ext) tile_ = *ext.Data();
        return;
    }

    if (info.Length() < 3)
    {
        Napi::Error::New(env, "please provide a z, x, y").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsNumber() ||
        !info[1].IsNumber() ||
        !info[2].IsNumber())
    {
        Napi::TypeError::New(env, "required parameters (z, x, and y) must be integers").ThrowAsJavaScriptException();
        return;
    }

    std::int64_t z = info[0].As<Napi::Number>().Int64Value();
    std::int64_t x = info[1].As<Napi::Number>().Int64Value();
    std::int64_t y = info[2].As<Napi::Number>().Int64Value();
    if (z < 0 || x < 0 || y < 0)
    {
        Napi::TypeError::New(env, "required parameters (z, x, and y) must be greater than or equal to zero").ThrowAsJavaScriptException();
        return;
    }
    std::int64_t max_at_zoom = pow(2, z);
    if (x >= max_at_zoom)
    {
        Napi::TypeError::New(env, "required parameter x is out of range of possible values based on z value").ThrowAsJavaScriptException();
        return;
    }
    if (y >= max_at_zoom)
    {
        Napi::TypeError::New(env, "required parameter y is out of range of possible values based on z value").ThrowAsJavaScriptException();
        return;
    }

    std::uint32_t tile_size = 4096;
    std::int32_t buffer_size = 128;
    Napi::Object options = Napi::Object::New(env);
    if (info.Length() > 3)
    {
        if (!info[3].IsObject())
        {
            Napi::TypeError::New(env, "optional fourth argument must be an options object").ThrowAsJavaScriptException();
            return;
        }
        options = info[3].As<Napi::Object>();
        if (options.Has("tile_size"))
        {
            Napi::Value opt = options.Get("tile_size");
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'tile_size' must be a number").ThrowAsJavaScriptException();
                return;
            }
            int tile_size_tmp = opt.As<Napi::Number>().Int32Value();
            if (tile_size_tmp <= 0)
            {
                Napi::TypeError::New(env, "optional arg 'tile_size' must be greater than zero").ThrowAsJavaScriptException();
                return;
            }
            tile_size = tile_size_tmp;
        }
        if (options.Has("buffer_size"))
        {
            Napi::Value opt = options.Get("buffer_size");
            if (!opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                return;
            }
            buffer_size = opt.As<Napi::Number>().Int32Value();
        }
    }
    if (static_cast<double>(tile_size) + (2 * buffer_size) <= 0)
    {
        Napi::Error::New(env, "too large of a negative buffer for tilesize").ThrowAsJavaScriptException();
        return;
    }
    tile_ = std::make_shared<mapnik::vector_tile_impl::merc_tile>(x, y, z, tile_size, buffer_size);
}

/**
 * Get the extent of this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name extent
 * @returns {Array<number>} array of extent in the form of `[minx,miny,maxx,maxy]`
 * @example
 * var vt = new mapnik.VectorTile(9,112,195);
 * var extent = vt.extent();
 * console.log(extent); // [-11271098.44281895, 4696291.017841229, -11192826.925854929, 4774562.534805248]
 */
Napi::Value VectorTile::extent(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Array arr = Napi::Array::New(env, 4);
    mapnik::box2d<double> const& e = tile_->extent();
    arr.Set(0u, Napi::Number::New(env, e.minx()));
    arr.Set(1u, Napi::Number::New(env, e.miny()));
    arr.Set(2u, Napi::Number::New(env, e.maxx()));
    arr.Set(3u, Napi::Number::New(env, e.maxy()));
    return scope.Escape(arr);
}

/**
 * Get the extent including the buffer of this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name bufferedExtent
 * @returns {Array<number>} extent - `[minx, miny, maxx, maxy]`
 * @example
 * var vt = new mapnik.VectorTile(9,112,195);
 * var extent = vt.bufferedExtent();
 * console.log(extent); // [-11273544.4277, 4693845.0329, -11190380.9409, 4777008.5197];
 */
Napi::Value VectorTile::bufferedExtent(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Array arr = Napi::Array::New(env, 4);
    mapnik::box2d<double> e = tile_->get_buffered_extent();
    arr.Set(0u, Napi::Number::New(env, e.minx()));
    arr.Set(1u, Napi::Number::New(env, e.miny()));
    arr.Set(2u, Napi::Number::New(env, e.maxx()));
    arr.Set(3u, Napi::Number::New(env, e.maxy()));
    return scope.Escape(arr);
}

/**
 * Get the names of all of the layers in this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name names
 * @returns {Array<string>} layer names
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var data = fs.readFileSync('./path/to/data.mvt');
 * vt.addDataSync(data);
 * console.log(vt.names()); // ['layer-name', 'another-layer']
 */
Napi::Value VectorTile::names(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::vector<std::string> const& names = tile_->get_layers();
    Napi::Array arr = Napi::Array::New(env, names.size());
    std::size_t idx = 0;
    for (std::string const& name : names)
    {
        arr.Set(idx++, name);
    }
    return scope.Escape(arr);
}

/**
 * Extract the layer by a given name to a new vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name layer
 * @param {string} layer_name - name of layer
 * @returns {mapnik.VectorTile} mapnik VectorTile object
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var data = fs.readFileSync('./path/to/data.mvt');
 * vt.addDataSync(data);
 * console.log(vt.names()); // ['layer-name', 'another-layer']
 * var vt2 = vt.layer('layer-name');
 * console.log(vt2.names()); // ['layer-name']
 */
Napi::Value VectorTile::layer(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() < 1)
    {
        Napi::Error::New(env, "first argument must be either a layer name").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Value layer_id = info[0];
    if (!layer_id.IsString())
    {
        Napi::TypeError::New(env, "'layer' argument must be a layer name (string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string layer_name = layer_id.As<Napi::String>();
    if (!tile_->has_layer(layer_name))
    {
        Napi::TypeError::New(env, "layer does not exist in vector tile").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    //VectorTile* v = new VectorTile(d->get_tile()->z(), d->get_tile()->x(), d->get_tile()->y(), d->tile_size(), d->buffer_size());
    auto new_tile = std::make_shared<mapnik::vector_tile_impl::merc_tile>(tile_->x(), tile_->y(), tile_->z(), tile_->size(), tile_->buffer_size());
    protozero::pbf_reader tile_message(tile_->get_reader());
    while (tile_message.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        auto data_view = tile_message.get_view();
        protozero::pbf_reader layer_message(data_view);
        if (!layer_message.next(mapnik::vector_tile_impl::Layer_Encoding::NAME))
        {
            continue;
        }
        std::string name = layer_message.get_string();
        if (layer_name == name)
        {
            new_tile->append_layer_buffer(data_view.data(), data_view.size(), layer_name);
            break;
        }
    }
    Napi::Value arg = Napi::External<mapnik::vector_tile_impl::merc_tile_ptr>::New(env, &new_tile);
    Napi::Object obj = VectorTile::constructor.New({arg});
    return scope.Escape(obj);
}

/**
 * Get the names of all of the empty layers in this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name emptyLayers
 * @returns {Array<string>} layer names
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var empty = vt.emptyLayers();
 * // assumes you have added data to your tile
 * console.log(empty); // ['layer-name', 'empty-layer']
 */
Napi::Value VectorTile::emptyLayers(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::set<std::string> const& names = tile_->get_empty_layers();
    Napi::Array arr = Napi::Array::New(env, names.size());
    std::size_t idx = 0;
    for (std::string const& name : names)
    {
        arr.Set(idx++, name);
    }
    return scope.Escape(arr);
}

/**
 * Get the names of all of the painted layers in this vector tile. "Painted" is
 * a check to see if data exists in the source dataset in a tile.
 *
 * @memberof VectorTile
 * @instance
 * @name paintedLayers
 * @returns {Array<string>} layer names
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var painted = vt.paintedLayers();
 * // assumes you have added data to your tile
 * console.log(painted); // ['layer-name']
 */
Napi::Value VectorTile::paintedLayers(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::set<std::string> const& names = tile_->get_painted_layers();
    Napi::Array arr = Napi::Array::New(env, names.size());
    std::size_t idx = 0;
    for (std::string const& name : names)
    {
        arr.Set(idx++, name);
    }
    return scope.Escape(arr);
}

/**
 * Return whether this vector tile is empty - whether it has no
 * layers and no features
 *
 * @memberof VectorTile
 * @instance
 * @name empty
 * @returns {boolean} whether the layer is empty
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var empty = vt.empty();
 * console.log(empty); // true
 */
Napi::Value VectorTile::empty(Napi::CallbackInfo const& info)
{
    return Napi::Boolean::New(info.Env(), tile_->is_empty());
}

/**
 * Get whether the vector tile has been painted. "Painted" is
 * a check to see if data exists in the source dataset in a tile.
 *
 * @memberof VectorTile
 * @instance
 * @name painted
 * @returns {boolean} painted
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var painted = vt.painted();
 * console.log(painted); // false
 */

Napi::Value VectorTile::painted(Napi::CallbackInfo const& info)
{
    return Napi::Boolean::New(info.Env(), tile_->is_painted());
}

// accessors
Napi::Value VectorTile::get_tile_x(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), tile_->x());
}

Napi::Value VectorTile::get_tile_y(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), tile_->y());
}

Napi::Value VectorTile::get_tile_z(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), tile_->z());
}

Napi::Value VectorTile::get_tile_size(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), tile_->tile_size());
}

Napi::Value VectorTile::get_buffer_size(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), tile_->buffer_size());
}

void VectorTile::set_tile_x(Napi::CallbackInfo const& info, const Napi::Value& value)
{
    Napi::Env env = info.Env();
    if (!value.IsNumber())
    {
        Napi::Error::New(env, "Must provide a number").ThrowAsJavaScriptException();
    }
    else
    {
        int val = value.As<Napi::Number>().Int32Value();
        if (val < 0)
        {
            Napi::Error::New(env, "tile x coordinate must be greater than or equal to zero").ThrowAsJavaScriptException();
            return;
        }
        tile_->x(val);
    }
}

void VectorTile::set_tile_y(Napi::CallbackInfo const& info, const Napi::Value& value)
{
    Napi::Env env = info.Env();
    if (!value.IsNumber())
    {
        Napi::Error::New(env, "Must provide a number").ThrowAsJavaScriptException();
    }
    else
    {
        int val = value.As<Napi::Number>().Int32Value();
        if (val < 0)
        {
            Napi::Error::New(env, "tile y coordinate must be greater than or equal to zero").ThrowAsJavaScriptException();
            return;
        }
        tile_->y(val);
    }
}

void VectorTile::set_tile_z(Napi::CallbackInfo const& info, const Napi::Value& value)
{
    Napi::Env env = info.Env();
    if (!value.IsNumber())
    {
        Napi::Error::New(env, "Must provide a number").ThrowAsJavaScriptException();
    }
    else
    {
        int val = value.As<Napi::Number>().Int32Value();
        if (val < 0)
        {
            Napi::Error::New(env, "tile z coordinate must be greater than or equal to zero").ThrowAsJavaScriptException();
            return;
        }
        tile_->z(val);
    }
}

void VectorTile::set_tile_size(Napi::CallbackInfo const& info, const Napi::Value& value)
{
    Napi::Env env = info.Env();
    if (!value.IsNumber())
    {
        Napi::Error::New(env, "Must provide a number").ThrowAsJavaScriptException();
    }
    else
    {
        int val = value.As<Napi::Number>().Int32Value();
        if (val <= 0)
        {
            Napi::Error::New(env, "tile size must be greater than zero").ThrowAsJavaScriptException();
            return;
        }
        tile_->tile_size(val);
    }
}

void VectorTile::set_buffer_size(Napi::CallbackInfo const& info, const Napi::Value& value)
{
    Napi::Env env = info.Env();
    if (!value.IsNumber())
    {
        Napi::Error::New(env, "Must provide a number").ThrowAsJavaScriptException();
    }
    else
    {
        int val = value.As<Napi::Number>().Int32Value();
        if (static_cast<int>(tile_->tile_size()) + (2 * val) <= 0)
        {
            Napi::Error::New(env, "too large of a negative buffer for tilesize").ThrowAsJavaScriptException();
            return;
        }
        tile_->buffer_size(val);
    }
}
