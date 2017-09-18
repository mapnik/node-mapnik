#include "utils.hpp"
#include "mapnik_vector_tile.hpp"

#include "vector_tile_tile.hpp"
#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_processor.hpp"
#include "vector_tile_datasource_pbf.hpp"

// mapnik
#include <mapnik/datasource_cache.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/feature_to_geojson.hpp>

// std
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

bool layer_to_geojson(protozero::pbf_reader const& layer,
                      std::string & result,
                      unsigned x,
                      unsigned y,
                      unsigned z)
{
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer, x, y, z);
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,wgs84);
    // This mega box ensures we capture all features, including those
    // outside the tile extent. Geometries outside the tile extent are
    // likely when the vtile was created by clipping to a buffered extent
    mapnik::query q(mapnik::box2d<double>(std::numeric_limits<double>::lowest(),
                                          std::numeric_limits<double>::lowest(),
                                          std::numeric_limits<double>::max(),
                                          std::numeric_limits<double>::max()));
    mapnik::layer_descriptor ld = ds.get_descriptor();
    for (auto const& item : ld.get_descriptors())
    {
        q.add_property_name(item.get_name());
    }
    mapnik::featureset_ptr fs = ds.features(q);
    bool first = true;
    if (fs && mapnik::is_valid(fs))
    {
        mapnik::feature_ptr feature;
        while ((feature = fs->next()))
        {
            if (first)
            {
                first = false;
            }
            else
            {
                result += "\n,";
            }
            std::string feature_str;
            mapnik::feature_impl feature_new(feature->context(),feature->id());
            feature_new.set_data(feature->get_data());
            unsigned int n_err = 0;
            feature_new.set_geometry(mapnik::geometry::reproject_copy(feature->get_geometry(), prj_trans, n_err));
            if (!mapnik::util::to_geojson(feature_str, feature_new))
            {
                // LCOV_EXCL_START
                throw std::runtime_error("Failed to generate GeoJSON geometry");
                // LCOV_EXCL_STOP
            }
            result += feature_str;
        }
    }
    return !first;
}

/**
 * Syncronous version of {@link VectorTile}
 *
 * @memberof VectorTile
 * @instance
 * @name toGeoJSONSync
 * @param {string | number} [layer=__all__] Can be a zero-index integer representing
 * a layer or the string keywords `__array__` or `__all__` to get all layers in the form
 * of an array of GeoJSON `FeatureCollection`s or in the form of a single GeoJSON
 * `FeatureCollection` with all layers smooshed inside
 * @returns {string} stringified GeoJSON of all the features in this tile.
 * @example
 * var geojson = vectorTile.toGeoJSONSync();
 * geojson // stringified GeoJSON
 * JSON.parse(geojson); // GeoJSON object
 */
NAN_METHOD(VectorTile::toGeoJSONSync)
{
    info.GetReturnValue().Set(_toGeoJSONSync(info));
}

void write_geojson_array(std::string & result,
                         VectorTile * v)
{
    protozero::pbf_reader tile_msg = v->get_tile()->get_reader();
    result += "[";
    bool first = true;
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        if (first)
        {
            first = false;
        }
        else
        {
            result += ",";
        }
        auto data_view = tile_msg.get_view();
        protozero::pbf_reader layer_msg(data_view);
        protozero::pbf_reader name_msg(data_view);
        std::string layer_name;
        if (name_msg.next(mapnik::vector_tile_impl::Layer_Encoding::NAME))
        {
            layer_name = name_msg.get_string();
        }
        result += "{\"type\":\"FeatureCollection\",";
        result += "\"name\":\"" + layer_name + "\",\"features\":[";
        std::string features;
        bool hit = layer_to_geojson(layer_msg,
                                    features,
                                    v->get_tile()->x(),
                                    v->get_tile()->y(),
                                    v->get_tile()->z());
        if (hit)
        {
            result += features;
        }
        result += "]}";
    }
    result += "]";
}

void write_geojson_all(std::string & result,
                       VectorTile * v)
{
    protozero::pbf_reader tile_msg = v->get_tile()->get_reader();
    result += "{\"type\":\"FeatureCollection\",\"features\":[";
    bool first = true;
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        protozero::pbf_reader layer_msg(tile_msg.get_message());
        std::string features;
        bool hit = layer_to_geojson(layer_msg,
                                    features,
                                    v->get_tile()->x(),
                                    v->get_tile()->y(),
                                    v->get_tile()->z());
        if (hit)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                result += ",";
            }
            result += features;
        }
    }
    result += "]}";
}

bool write_geojson_layer_index(std::string & result,
                               std::size_t layer_idx,
                               VectorTile * v)
{
    protozero::pbf_reader layer_msg;
    if (v->get_tile()->layer_reader(layer_idx, layer_msg) &&
        v->get_tile()->get_layers().size() > layer_idx)
    {
        std::string layer_name = v->get_tile()->get_layers()[layer_idx];
        result += "{\"type\":\"FeatureCollection\",";
        result += "\"name\":\"" + layer_name + "\",\"features\":[";
        layer_to_geojson(layer_msg,
                         result,
                         v->get_tile()->x(),
                         v->get_tile()->y(),
                         v->get_tile()->z());
        result += "]}";
        return true;
    }
    // LCOV_EXCL_START
    return false;
    // LCOV_EXCL_STOP
}

bool write_geojson_layer_name(std::string & result,
                              std::string const& name,
                              VectorTile * v)
{
    protozero::pbf_reader layer_msg;
    if (v->get_tile()->layer_reader(name, layer_msg))
    {
        result += "{\"type\":\"FeatureCollection\",";
        result += "\"name\":\"" + name + "\",\"features\":[";
        layer_to_geojson(layer_msg,
                         result,
                         v->get_tile()->x(),
                         v->get_tile()->y(),
                         v->get_tile()->z());
        result += "]}";
        return true;
    }
    return false;
}

v8::Local<v8::Value> VectorTile::_toGeoJSONSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    if (info.Length() < 1)
    {
        Nan::ThrowError("first argument must be either a layer name (string) or layer index (integer)");
        return scope.Escape(Nan::Undefined());
    }
    v8::Local<v8::Value> layer_id = info[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) )
    {
        Nan::ThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        return scope.Escape(Nan::Undefined());
    }

    VectorTile* v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::string result;
    try
    {
        if (layer_id->IsString())
        {
            std::string layer_name = TOSTR(layer_id);
            if (layer_name == "__array__")
            {
                write_geojson_array(result, v);
            }
            else if (layer_name == "__all__")
            {
                write_geojson_all(result, v);
            }
            else
            {
                if (!write_geojson_layer_name(result, layer_name, v))
                {
                    std::string error_msg("Layer name '" + layer_name + "' not found");
                    Nan::ThrowTypeError(error_msg.c_str());
                    return scope.Escape(Nan::Undefined());
                }
            }
        }
        else if (layer_id->IsNumber())
        {
            int layer_idx = layer_id->IntegerValue();
            if (layer_idx < 0)
            {
                Nan::ThrowTypeError("A layer index can not be negative");
                return scope.Escape(Nan::Undefined());
            }
            else if (layer_idx >= static_cast<int>(v->get_tile()->get_layers().size()))
            {
                Nan::ThrowTypeError("Layer index exceeds the number of layers in the vector tile.");
                return scope.Escape(Nan::Undefined());
            }
            if (!write_geojson_layer_index(result, layer_idx, v))
            {
                // LCOV_EXCL_START
                Nan::ThrowTypeError("Layer could not be retrieved (should have not reached here)");
                return scope.Escape(Nan::Undefined());
                // LCOV_EXCL_STOP
            }
        }
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        Nan::ThrowTypeError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
    return scope.Escape(Nan::New<v8::String>(result).ToLocalChecked());
}

enum geojson_write_type : std::uint8_t
{
    geojson_write_all = 0,
    geojson_write_array,
    geojson_write_layer_name,
    geojson_write_layer_index
};

struct to_geojson_baton
{
    uv_work_t request;
    VectorTile* v;
    bool error;
    std::string result;
    geojson_write_type type;
    int layer_idx;
    std::string layer_name;
    Nan::Persistent<v8::Function> cb;
};

/**
 * Get a [GeoJSON](http://geojson.org/) representation of this tile
 *
 * @memberof VectorTile
 * @instance
 * @name toGeoJSON
 * @param {string | number} [layer=__all__] Can be a zero-index integer representing
 * a layer or the string keywords `__array__` or `__all__` to get all layers in the form
 * of an array of GeoJSON `FeatureCollection`s or in the form of a single GeoJSON
 * `FeatureCollection` with all layers smooshed inside
 * @param {Function} callback - `function(err, geojson)`: a stringified
 * GeoJSON of all the features in this tile
 * @example
 * vectorTile.toGeoJSON(function(err, geojson) {
 *   if (err) throw err;
 *   console.log(geojson); // stringified GeoJSON
 *   console.log(JSON.parse(geojson)); // GeoJSON object
 * });
 */
NAN_METHOD(VectorTile::toGeoJSON)
{
    if ((info.Length() < 1) || !info[info.Length()-1]->IsFunction())
    {
        info.GetReturnValue().Set(_toGeoJSONSync(info));
        return;
    }
    to_geojson_baton *closure = new to_geojson_baton();
    closure->request.data = closure;
    closure->v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    closure->error = false;
    closure->layer_idx = 0;
    closure->type = geojson_write_all;

    v8::Local<v8::Value> layer_id = info[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) )
    {
        delete closure;
        Nan::ThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        return;
    }

    if (layer_id->IsString())
    {
        std::string layer_name = TOSTR(layer_id);
        if (layer_name == "__array__")
        {
            closure->type = geojson_write_array;
        }
        else if (layer_name == "__all__")
        {
            closure->type = geojson_write_all;
        }
        else
        {
            if (!closure->v->get_tile()->has_layer(layer_name))
            {
                delete closure;
                std::string error_msg("The layer does not contain the name: " + layer_name);
                Nan::ThrowTypeError(error_msg.c_str());
                return;
            }
            closure->layer_name = layer_name;
            closure->type = geojson_write_layer_name;
        }
    }
    else if (layer_id->IsNumber())
    {
        closure->layer_idx = layer_id->IntegerValue();
        if (closure->layer_idx < 0)
        {
            delete closure;
            Nan::ThrowTypeError("A layer index can not be negative");
            return;
        }
        else if (closure->layer_idx >= static_cast<int>(closure->v->get_tile()->get_layers().size()))
        {
            delete closure;
            Nan::ThrowTypeError("Layer index exceeds the number of layers in the vector tile.");
            return;
        }
        closure->type = geojson_write_layer_index;
    }

    v8::Local<v8::Value> callback = info[info.Length()-1];
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, to_geojson, (uv_after_work_cb)after_to_geojson);
    closure->v->Ref();
    return;
}

void VectorTile::to_geojson(uv_work_t* req)
{
    to_geojson_baton *closure = static_cast<to_geojson_baton *>(req->data);
    try
    {
        switch (closure->type)
        {
            default:
            case geojson_write_all:
                write_geojson_all(closure->result, closure->v);
                break;
            case geojson_write_array:
                write_geojson_array(closure->result, closure->v);
                break;
            case geojson_write_layer_name:
                write_geojson_layer_name(closure->result, closure->layer_name, closure->v);
                break;
            case geojson_write_layer_index:
                write_geojson_layer_index(closure->result, closure->layer_idx, closure->v);
                break;
        }
    }
    catch (std::exception const& ex)
    {
        // There are currently no known ways to trigger this exception in testing. If it was
        // triggered this would likely be a bug in either mapnik or mapnik-vector-tile.
        // LCOV_EXCL_START
        closure->error = true;
        closure->result = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::after_to_geojson(uv_work_t* req)
{
    Nan::HandleScope scope;
    to_geojson_baton *closure = static_cast<to_geojson_baton *>(req->data);
    if (closure->error)
    {
        // Because there are no known ways to trigger the exception path in to_geojson
        // there is no easy way to test this path currently
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->result.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::New<v8::String>(closure->result).ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->v->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Add features to this tile from a GeoJSON string. GeoJSON coordinates must be in the WGS84 longitude & latitude CRS
 * as specified in the [GeoJSON Specification](https://www.rfc-editor.org/rfc/rfc7946.txt).
 *
 * @memberof VectorTile
 * @instance
 * @name addGeoJSON
 * @param {string} geojson as a string
 * @param {string} name of the layer to be added
 * @param {Object} [options]
 * @param {number} [options.area_threshold=0.1] used to discard small polygons.
 * If a value is greater than `0` it will trigger polygons with an area smaller
 * than the value to be discarded. Measured in grid integers, not spherical mercator
 * coordinates.
 * @param {number} [options.simplify_distance=0.0] Simplification works to generalize
 * geometries before encoding into vector tiles.simplification distance The
 * `simplify_distance` value works in integer space over a 4096 pixel grid and uses
 * the [Douglas-Peucker algorithm](https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm).
 * @param {boolean} [options.strictly_simple=true] ensure all geometry is valid according to
 * OGC Simple definition
 * @param {boolean} [options.multi_polygon_union=false] union all multipolygons
 * @param {Object<mapnik.polygonFillType>} [options.fill_type=mapnik.polygonFillType.positive]
 * the fill type used in determining what are holes and what are outer rings. See the
 * [Clipper documentation](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/PolyFillType.htm)
 * to learn more about fill types.
 * @param {boolean} [options.process_all_rings=false] if `true`, don't assume winding order and ring order of
 * polygons are correct according to the [`2.0` Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec)
 * @example
 * var geojson = { ... };
 * var vt = mapnik.VectorTile(0,0,0);
 * vt.addGeoJSON(JSON.stringify(geojson), 'layer-name', {});
 */
NAN_METHOD(VectorTile::addGeoJSON)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsString())
    {
        Nan::ThrowError("first argument must be a GeoJSON string");
        return;
    }
    if (info.Length() < 2 || !info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return;
    }
    std::string geojson_string = TOSTR(info[0]);
    std::string geojson_name = TOSTR(info[1]);

    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    double area_threshold = 0.1;
    double simplify_distance = 0.0;
    bool strictly_simple = true;
    bool multi_polygon_union = false;
    mapnik::vector_tile_impl::polygon_fill_type fill_type = mapnik::vector_tile_impl::positive_fill;
    bool process_all_rings = false;

    if (info.Length() > 2)
    {
        // options object
        if (!info[2]->IsObject())
        {
            Nan::ThrowError("optional third argument must be an options object");
            return;
        }

        options = info[2]->ToObject();
        if (options->Has(Nan::New("area_threshold").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("area_threshold").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowError("option 'area_threshold' must be a number");
                return;
            }
            area_threshold = param_val->IntegerValue();
            if (area_threshold < 0.0)
            {
                Nan::ThrowError("option 'area_threshold' can not be negative");
                return;
            }
        }
        if (options->Has(Nan::New("strictly_simple").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("strictly_simple").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'strictly_simple' must be a boolean");
                return;
            }
            strictly_simple = param_val->BooleanValue();
        }
        if (options->Has(Nan::New("multi_polygon_union").ToLocalChecked()))
        {
            v8::Local<v8::Value> mpu = options->Get(Nan::New("multi_polygon_union").ToLocalChecked());
            if (!mpu->IsBoolean())
            {
                Nan::ThrowTypeError("multi_polygon_union value must be a boolean");
                return;
            }
            multi_polygon_union = mpu->BooleanValue();
        }
        if (options->Has(Nan::New("fill_type").ToLocalChecked()))
        {
            v8::Local<v8::Value> ft = options->Get(Nan::New("fill_type").ToLocalChecked());
            if (!ft->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'fill_type' must be a number");
                return;
            }
            fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(ft->IntegerValue());
            if (fill_type < 0 || fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
            {
                Nan::ThrowTypeError("optional arg 'fill_type' out of possible range");
                return;
            }
        }
        if (options->Has(Nan::New("simplify_distance").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("simplify_distance").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'simplify_distance' must be an floating point number");
                return;
            }
            simplify_distance = param_val->NumberValue();
            if (simplify_distance < 0.0)
            {
                Nan::ThrowTypeError("option 'simplify_distance' must be a positive number");
                return;
            }
        }
        if (options->Has(Nan::New("process_all_rings").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("process_all_rings").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'process_all_rings' must be a boolean");
                return;
            }
            process_all_rings = param_val->BooleanValue();
        }
    }

    try
    {
        // create map object
        mapnik::Map map(d->tile_size(),d->tile_size(),"+init=epsg:3857");
        mapnik::parameters p;
        p["type"]="geojson";
        p["inline"]=geojson_string;
        mapnik::layer lyr(geojson_name,"+init=epsg:4326");
        lyr.set_datasource(mapnik::datasource_cache::instance().create(p));
        map.add_layer(lyr);

        mapnik::vector_tile_impl::processor ren(map);
        ren.set_area_threshold(area_threshold);
        ren.set_strictly_simple(strictly_simple);
        ren.set_simplify_distance(simplify_distance);
        ren.set_multi_polygon_union(multi_polygon_union);
        ren.set_fill_type(fill_type);
        ren.set_process_all_rings(process_all_rings);
        ren.update_tile(*d->get_tile());
        info.GetReturnValue().Set(Nan::True());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
}
