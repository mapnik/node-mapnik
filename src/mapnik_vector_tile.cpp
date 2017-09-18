#include "utils.hpp"
#include "mapnik_vector_tile.hpp"

#include "vector_tile_compression.hpp"
#include "vector_tile_composite.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_load_tile.hpp"
#include "object_to_container.hpp"

// mapnik
#include <mapnik/box2d.hpp>

// std
#include <set>                          // for set, etc
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

Nan::Persistent<v8::FunctionTemplate> VectorTile::constructor;

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
 * console.log(vt.x, vt.y, vt.z); // 9, 112, 195
 * console.log(vt.tileSize, vt.bufferSize); // 4096, 128
 */
void VectorTile::Initialize(v8::Local<v8::Object> target)
{
    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(VectorTile::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("VectorTile").ToLocalChecked());
    Nan::SetPrototypeMethod(lcons, "render", render);
    Nan::SetPrototypeMethod(lcons, "setData", setData);
    Nan::SetPrototypeMethod(lcons, "setDataSync", setDataSync);
    Nan::SetPrototypeMethod(lcons, "getData", getData);
    Nan::SetPrototypeMethod(lcons, "getDataSync", getDataSync);
    Nan::SetPrototypeMethod(lcons, "addData", addData);
    Nan::SetPrototypeMethod(lcons, "addDataSync", addDataSync);
    Nan::SetPrototypeMethod(lcons, "composite", composite);
    Nan::SetPrototypeMethod(lcons, "compositeSync", compositeSync);
    Nan::SetPrototypeMethod(lcons, "query", query);
    Nan::SetPrototypeMethod(lcons, "queryMany", queryMany);
    Nan::SetPrototypeMethod(lcons, "extent", extent);
    Nan::SetPrototypeMethod(lcons, "bufferedExtent", bufferedExtent);
    Nan::SetPrototypeMethod(lcons, "names", names);
    Nan::SetPrototypeMethod(lcons, "layer", layer);
    Nan::SetPrototypeMethod(lcons, "emptyLayers", emptyLayers);
    Nan::SetPrototypeMethod(lcons, "paintedLayers", paintedLayers);
    Nan::SetPrototypeMethod(lcons, "toJSON", toJSON);
    Nan::SetPrototypeMethod(lcons, "toGeoJSON", toGeoJSON);
    Nan::SetPrototypeMethod(lcons, "toGeoJSONSync", toGeoJSONSync);
    Nan::SetPrototypeMethod(lcons, "addGeoJSON", addGeoJSON);
    Nan::SetPrototypeMethod(lcons, "addImage", addImage);
    Nan::SetPrototypeMethod(lcons, "addImageSync", addImageSync);
    Nan::SetPrototypeMethod(lcons, "addImageBuffer", addImageBuffer);
    Nan::SetPrototypeMethod(lcons, "addImageBufferSync", addImageBufferSync);
#if BOOST_VERSION >= 105600
    Nan::SetPrototypeMethod(lcons, "reportGeometrySimplicity", reportGeometrySimplicity);
    Nan::SetPrototypeMethod(lcons, "reportGeometrySimplicitySync", reportGeometrySimplicitySync);
    Nan::SetPrototypeMethod(lcons, "reportGeometryValidity", reportGeometryValidity);
    Nan::SetPrototypeMethod(lcons, "reportGeometryValiditySync", reportGeometryValiditySync);
#endif // BOOST_VERSION >= 105600
    Nan::SetPrototypeMethod(lcons, "painted", painted);
    Nan::SetPrototypeMethod(lcons, "clear", clear);
    Nan::SetPrototypeMethod(lcons, "clearSync", clearSync);
    Nan::SetPrototypeMethod(lcons, "empty", empty);

    // properties
    ATTR(lcons, "x", get_tile_x, set_tile_x);
    ATTR(lcons, "y", get_tile_y, set_tile_y);
    ATTR(lcons, "z", get_tile_z, set_tile_z);
    ATTR(lcons, "tileSize", get_tile_size, set_tile_size);
    ATTR(lcons, "bufferSize", get_buffer_size, set_buffer_size);

    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(), "info", info);

    target->Set(Nan::New("VectorTile").ToLocalChecked(),lcons->GetFunction());
    constructor.Reset(lcons);
}

VectorTile::VectorTile(std::uint64_t z,
                       std::uint64_t x,
                       std::uint64_t y,
                       std::uint32_t tile_size,
                       std::int32_t buffer_size) :
    Nan::ObjectWrap(),
    tile_(std::make_shared<mapnik::vector_tile_impl::merc_tile>(x, y, z, tile_size, buffer_size))
{
}

// For some reason coverage never seems to be considered here even though
// I have tested it and it does print
/* LCOV_EXCL_START */
VectorTile::~VectorTile()
{
}
/* LCOV_EXCL_STOP */

NAN_METHOD(VectorTile::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = info[0].As<v8::External>();
        void* ptr = ext->Value();
        VectorTile* v =  static_cast<VectorTile*>(ptr);
        v->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }

    if (info.Length() < 3)
    {
        Nan::ThrowError("please provide a z, x, y");
        return;
    }

    if (!info[0]->IsNumber() ||
        !info[1]->IsNumber() ||
        !info[2]->IsNumber())
    {
        Nan::ThrowTypeError("required parameters (z, x, and y) must be a integers");
        return;
    }

    std::int64_t z = info[0]->IntegerValue();
    std::int64_t x = info[1]->IntegerValue();
    std::int64_t y = info[2]->IntegerValue();
    if (z < 0 || x < 0 || y < 0)
    {
        Nan::ThrowTypeError("required parameters (z, x, and y) must be greater then or equal to zero");
        return;
    }
    std::int64_t max_at_zoom = pow(2,z);
    if (x >= max_at_zoom)
    {
        Nan::ThrowTypeError("required parameter x is out of range of possible values based on z value");
        return;
    }
    if (y >= max_at_zoom)
    {
        Nan::ThrowTypeError("required parameter y is out of range of possible values based on z value");
        return;
    }

    std::uint32_t tile_size = 4096;
    std::int32_t buffer_size = 128;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 3)
    {
        if (!info[3]->IsObject())
        {
            Nan::ThrowTypeError("optional fourth argument must be an options object");
            return;
        }
        options = info[3]->ToObject();
        if (options->Has(Nan::New("tile_size").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("tile_size").ToLocalChecked());
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'tile_size' must be a number");
                return;
            }
            int tile_size_tmp = opt->IntegerValue();
            if (tile_size_tmp <= 0)
            {
                Nan::ThrowTypeError("optional arg 'tile_size' must be greater then zero");
                return;
            }
            tile_size = tile_size_tmp;
        }
        if (options->Has(Nan::New("buffer_size").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("buffer_size").ToLocalChecked());
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }
            buffer_size = opt->IntegerValue();
        }
    }
    if (static_cast<double>(tile_size) + (2 * buffer_size) <= 0)
    {
        Nan::ThrowError("too large of a negative buffer for tilesize");
        return;
    }

    VectorTile* d = new VectorTile(z, x, y, tile_size, buffer_size);

    d->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
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
NAN_METHOD(VectorTile::extent)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
    mapnik::box2d<double> const& e = d->tile_->extent();
    arr->Set(0, Nan::New<v8::Number>(e.minx()));
    arr->Set(1, Nan::New<v8::Number>(e.miny()));
    arr->Set(2, Nan::New<v8::Number>(e.maxx()));
    arr->Set(3, Nan::New<v8::Number>(e.maxy()));
    info.GetReturnValue().Set(arr);
    return;
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
NAN_METHOD(VectorTile::bufferedExtent)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
    mapnik::box2d<double> e = d->tile_->get_buffered_extent();
    arr->Set(0, Nan::New<v8::Number>(e.minx()));
    arr->Set(1, Nan::New<v8::Number>(e.miny()));
    arr->Set(2, Nan::New<v8::Number>(e.maxx()));
    arr->Set(3, Nan::New<v8::Number>(e.maxy()));
    info.GetReturnValue().Set(arr);
    return;
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
NAN_METHOD(VectorTile::names)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::vector<std::string> const& names = d->tile_->get_layers();
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(names.size());
    unsigned idx = 0;
    for (std::string const& name : names)
    {
        arr->Set(idx++,Nan::New<v8::String>(name).ToLocalChecked());
    }
    info.GetReturnValue().Set(arr);
    return;
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
NAN_METHOD(VectorTile::layer)
{
    if (info.Length() < 1)
    {
        Nan::ThrowError("first argument must be either a layer name");
        return;
    }
    v8::Local<v8::Value> layer_id = info[0];
    std::string layer_name;
    if (!layer_id->IsString())
    {
        Nan::ThrowTypeError("'layer' argument must be a layer name (string)");
        return;
    }
    layer_name = TOSTR(layer_id);
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!d->get_tile()->has_layer(layer_name))
    {
        Nan::ThrowTypeError("layer does not exist in vector tile");
        return;
    }
    protozero::pbf_reader layer_msg;
    VectorTile* v = new VectorTile(d->get_tile()->z(), d->get_tile()->x(), d->get_tile()->y(), d->tile_size(), d->buffer_size());
    protozero::pbf_reader tile_message(d->get_tile()->get_reader());
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
            v->get_tile()->append_layer_buffer(data_view.data(), data_view.size(), layer_name);
            break;
        }
    }
    v8::Local<v8::Value> ext = Nan::New<v8::External>(v);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Layer instance");
    else info.GetReturnValue().Set(maybe_local.ToLocalChecked());
    return;
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
NAN_METHOD(VectorTile::emptyLayers)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::set<std::string> const& names = d->tile_->get_empty_layers();
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(names.size());
    unsigned idx = 0;
    for (std::string const& name : names)
    {
        arr->Set(idx++,Nan::New<v8::String>(name).ToLocalChecked());
    }
    info.GetReturnValue().Set(arr);
    return;
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
NAN_METHOD(VectorTile::paintedLayers)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::set<std::string> const& names = d->tile_->get_painted_layers();
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(names.size());
    unsigned idx = 0;
    for (std::string const& name : names)
    {
        arr->Set(idx++,Nan::New<v8::String>(name).ToLocalChecked());
    }
    info.GetReturnValue().Set(arr);
    return;
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
NAN_METHOD(VectorTile::empty)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(d->tile_->is_empty()));
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
NAN_METHOD(VectorTile::painted)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New(d->tile_->is_painted()));
}

NAN_GETTER(VectorTile::get_tile_x)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->x()));
}

NAN_GETTER(VectorTile::get_tile_y)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->y()));
}

NAN_GETTER(VectorTile::get_tile_z)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->z()));
}

NAN_GETTER(VectorTile::get_tile_size)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->tile_size()));
}

NAN_GETTER(VectorTile::get_buffer_size)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->buffer_size()));
}

NAN_SETTER(VectorTile::set_tile_x)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = value->IntegerValue();
        if (val < 0)
        {
            Nan::ThrowError("tile x coordinate must be greater then or equal to zero");
            return;
        }
        d->tile_->x(val);
    }
}

NAN_SETTER(VectorTile::set_tile_y)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = value->IntegerValue();
        if (val < 0)
        {
            Nan::ThrowError("tile y coordinate must be greater then or equal to zero");
            return;
        }
        d->tile_->y(val);
    }
}

NAN_SETTER(VectorTile::set_tile_z)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = value->IntegerValue();
        if (val < 0)
        {
            Nan::ThrowError("tile z coordinate must be greater then or equal to zero");
            return;
        }
        d->tile_->z(val);
    }
}

NAN_SETTER(VectorTile::set_tile_size)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = value->IntegerValue();
        if (val <= 0)
        {
            Nan::ThrowError("tile size must be greater then zero");
            return;
        }
        d->tile_->tile_size(val);
    }
}

NAN_SETTER(VectorTile::set_buffer_size)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = value->IntegerValue();
        if (static_cast<int>(d->tile_size()) + (2 * val) <= 0)
        {
            Nan::ThrowError("too large of a negative buffer for tilesize");
            return;
        }
        d->tile_->buffer_size(val);
    }
}

namespace mapnik
{

namespace vector_tile_impl
{

template
mapnik::geometry::geometry<std::int64_t> decode_geometry<std::int64_t>(GeometryPBF & paths,
                                                         std::int32_t geom_type,
                                                         unsigned version,
                                                         std::int64_t tile_x,
                                                         std::int64_t tile_y,
                                                         double scale_x,
                                                         double scale_y);

template mapnik::geometry::geometry<double> decode_geometry<double>(GeometryPBF & paths,
                                                                    int32_t geom_type,
                                                                    unsigned version,
                                                                    double tile_x,
                                                                    double tile_y,
                                                                    double scale_x,
                                                                    double scale_y);

}

}

