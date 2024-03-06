//
#include "mapnik_vector_tile.hpp"

// mapnik
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/feature_to_geojson.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/geometry/reprojection.hpp>
#include <mapnik/datasource_cache.hpp>
// mapnik-vector-tile
#include "vector_tile_compression.hpp"
#include "vector_tile_composite.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_load_tile.hpp"
#include "object_to_container.hpp"

namespace {

struct geometry_type_name
{
    template <typename T>
    std::string operator()(T const& geom) const
    {
        return mapnik::util::apply_visitor(*this, geom);
    }

    std::string operator()(mapnik::geometry::geometry_empty const&) const
    {
        // LCOV_EXCL_START
        return "Empty";
        // LCOV_EXCL_STOP
    }

    template <typename T>
    std::string operator()(mapnik::geometry::point<T> const&) const
    {
        return "Point";
    }

    template <typename T>
    std::string operator()(mapnik::geometry::line_string<T> const&) const
    {
        return "LineString";
    }

    template <typename T>
    std::string operator()(mapnik::geometry::polygon<T> const&) const
    {
        return "Polygon";
    }

    template <typename T>
    std::string operator()(mapnik::geometry::multi_point<T> const&) const
    {
        return "MultiPoint";
    }

    template <typename T>
    std::string operator()(mapnik::geometry::multi_line_string<T> const&) const
    {
        return "MultiLineString";
    }

    template <typename T>
    std::string operator()(mapnik::geometry::multi_polygon<T> const&) const
    {
        return "MultiPolygon";
    }

    template <typename T>
    std::string operator()(mapnik::geometry::geometry_collection<T> const&) const
    {
        // LCOV_EXCL_START
        return "GeometryCollection";
        // LCOV_EXCL_STOP
    }
};

template <typename T>
static inline std::string geometry_type_as_string(T const& geom)
{
    return geometry_type_name()(geom);
}

struct geometry_array_visitor
{
    geometry_array_visitor(Napi::Env env)
        : env_(env) {}

    Napi::Array operator()(mapnik::geometry::geometry_empty const&)
    {
        // Removed as it should be a bug if a vector tile has reached this point
        // therefore no known tests reach this point
        // LCOV_EXCL_START
        return Napi::Array::New(env_);
        // LCOV_EXCL_STOP
    }

    template <typename T>
    Napi::Array operator()(mapnik::geometry::point<T> const& geom)
    {
        Napi::Array arr = Napi::Array::New(env_, 2);
        arr.Set(0u, Napi::Number::New(env_, geom.x));
        arr.Set(1u, Napi::Number::New(env_, geom.y));
        return arr;
    }

    template <typename T>
    Napi::Array operator()(mapnik::geometry::line_string<T> const& geom)
    {
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return Napi::Array::New(env_);
            // LCOV_EXCL_STOP
        }
        Napi::Array arr = Napi::Array::New(env_, geom.size());
        std::uint32_t c = 0;
        for (auto const& pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator()(mapnik::geometry::linear_ring<T> const& geom)
    {
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return Napi::Array::New(env_);
            // LCOV_EXCL_STOP
        }
        Napi::Array arr = Napi::Array::New(env_, geom.size());
        std::uint32_t c = 0;
        for (auto const& pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator()(mapnik::geometry::multi_point<T> const& geom)
    {
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return Napi::Array::New(env_);
            // LCOV_EXCL_STOP
        }
        Napi::Array arr = Napi::Array::New(env_, geom.size());
        std::uint32_t c = 0;
        for (auto const& pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator()(mapnik::geometry::multi_line_string<T> const& geom)
    {
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return Napi::Array::New(env_);
            // LCOV_EXCL_STOP
        }
        Napi::Array arr = Napi::Array::New(env_, geom.size());
        std::uint32_t c = 0;
        for (auto const& pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator()(mapnik::geometry::polygon<T> const& poly)
    {
        Napi::Array arr = Napi::Array::New(env_, poly.size());
        std::uint32_t index = 0;

        for (auto const& ring : poly)
        {
            arr.Set(index++, (*this)(ring));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator()(mapnik::geometry::multi_polygon<T> const& geom)
    {
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return Napi::Array::New(env_);
            // LCOV_EXCL_STOP
        }
        Napi::Array arr = Napi::Array::New(env_, geom.size());
        std::uint32_t c = 0;
        for (auto const& pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator()(mapnik::geometry::geometry<T> const& geom)
    {
        // Removed as it should be a bug if a vector tile has reached this point
        // therefore no known tests reach this point
        // LCOV_EXCL_START
        return mapnik::util::apply_visitor((*this), geom);
        // LCOV_EXCL_STOP
    }

    template <typename T>
    Napi::Array operator()(mapnik::geometry::geometry_collection<T> const& geom)
    {
        // Removed as it should be a bug if a vector tile has reached this point
        // therefore no known tests reach this point
        // LCOV_EXCL_START
        if (geom.empty())
        {
            return Napi::Array::New(env_);
        }
        Napi::Array arr = Napi::Array::New(env_, geom.size());
        std::uint32_t c = 0;
        for (auto const& pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
        // LCOV_EXCL_STOP
    }

  private:
    Napi::Env env_;
};

template <typename T>
Napi::Array geometry_to_array(Napi::Env const& env, mapnik::geometry::geometry<T> const& geom)
{
    return mapnik::util::apply_visitor(geometry_array_visitor(env), geom);
}

struct json_value_visitor
{

    json_value_visitor(Napi::Env env, Napi::Object& att_obj,
                       std::string const& name)
        : env_(env), att_obj_(att_obj), name_(name) {}

    void operator()(std::string const& val)
    {
        att_obj_.Set(name_, val);
    }

    void operator()(bool const& val)
    {
        att_obj_.Set(name_, Napi::Boolean::New(env_, val));
    }

    void operator()(int64_t const& val)
    {
        att_obj_.Set(name_, Napi::Number::New(env_, val));
    }

    void operator()(uint64_t const& val)
    {
        // LCOV_EXCL_START
        att_obj_.Set(name_, Napi::Number::New(env_, val));
        // LCOV_EXCL_STOP
    }

    void operator()(double const& val)
    {
        att_obj_.Set(name_, Napi::Number::New(env_, val));
    }

    void operator()(float const& val)
    {
        att_obj_.Set(name_, Napi::Number::New(env_, val));
    }
    Napi::Env env_;
    Napi::Object& att_obj_;
    std::string const& name_;
};

enum geojson_write_type : std::uint8_t
{
    geojson_write_all = 0,
    geojson_write_array,
    geojson_write_layer_name,
    geojson_write_layer_index
};

bool layer_to_geojson(protozero::pbf_reader const& layer,
                      std::string& result,
                      unsigned x,
                      unsigned y,
                      unsigned z)
{
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer, x, y, z);
    mapnik::projection wgs84("epsg:4326", true);
    mapnik::projection merc("epsg:3857", true);
    mapnik::proj_transform prj_trans(merc, wgs84);
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
            mapnik::feature_impl feature_new(feature->context(), feature->id());
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
void write_geojson_array(std::string& result,
                         mapnik::vector_tile_impl::merc_tile_ptr const& tile)
{
    protozero::pbf_reader tile_msg = tile->get_reader();
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
                                    tile->x(),
                                    tile->y(),
                                    tile->z());
        if (hit)
        {
            result += features;
        }
        result += "]}";
    }
    result += "]";
}

void write_geojson_all(std::string& result,
                       mapnik::vector_tile_impl::merc_tile_ptr const& tile)
{
    protozero::pbf_reader tile_msg = tile->get_reader();
    result += "{\"type\":\"FeatureCollection\",\"features\":[";
    bool first = true;
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        protozero::pbf_reader layer_msg(tile_msg.get_message());
        std::string features;
        bool hit = layer_to_geojson(layer_msg,
                                    features,
                                    tile->x(),
                                    tile->y(),
                                    tile->z());
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

bool write_geojson_layer_index(std::string& result,
                               std::size_t layer_idx,
                               mapnik::vector_tile_impl::merc_tile_ptr const& tile)
{
    protozero::pbf_reader layer_msg;
    if (tile->layer_reader(layer_idx, layer_msg) &&
        tile->get_layers().size() > layer_idx)
    {
        std::string layer_name = tile->get_layers()[layer_idx];
        result += "{\"type\":\"FeatureCollection\",";
        result += "\"name\":\"" + layer_name + "\",\"features\":[";
        layer_to_geojson(layer_msg,
                         result,
                         tile->x(),
                         tile->y(),
                         tile->z());
        result += "]}";
        return true;
    }
    // LCOV_EXCL_START
    return false;
    // LCOV_EXCL_STOP
}

bool write_geojson_layer_name(std::string& result,
                              std::string const& name,
                              mapnik::vector_tile_impl::merc_tile_ptr const& tile)
{
    protozero::pbf_reader layer_msg;
    if (tile->layer_reader(name, layer_msg))
    {
        result += "{\"type\":\"FeatureCollection\",";
        result += "\"name\":\"" + name + "\",\"features\":[";
        layer_to_geojson(layer_msg,
                         result,
                         tile->x(),
                         tile->y(),
                         tile->z());
        result += "]}";
        return true;
    }
    return false;
}

struct AsyncToGeoJSON : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncToGeoJSON(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                   geojson_write_type type, int layer_idx, std::string const& layer_name,
                   Napi::Function const& callback)
        : Base(callback),
          tile_(tile),
          type_(type),
          layer_idx_(layer_idx),
          layer_name_(layer_name)
    {
    }

    void Execute() override
    {
        try
        {
            switch (type_)
            {
            default:
            case geojson_write_all:
                write_geojson_all(result_, tile_);
                break;
            case geojson_write_array:
                write_geojson_array(result_, tile_);
                break;
            case geojson_write_layer_name:
                write_geojson_layer_name(result_, layer_name_, tile_);
                break;
            case geojson_write_layer_index:
                write_geojson_layer_index(result_, layer_idx_, tile_);
                break;
            }
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        return {env.Undefined(), Napi::String::New(env, result_)};
    }

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    geojson_write_type type_;
    int layer_idx_;
    std::string layer_name_;
    std::string result_;
};

} // namespace

/**
 * Get a JSON representation of this tile
 *
 * @memberof VectorTile
 * @instance
 * @name toJSON
 * @param {Object} [options]
 * @param {boolean} [options.decode_geometry=false] return geometry as integers
 * relative to the tile grid
 * @returns {Object} json representation of this tile with name, extent,
 * version, and feature properties
 * @example
 * var vt = mapnik.VectorTile(10,131,242);
 * var buffer = fs.readFileSync('./path/to/data.mvt');
 * vt.setData(buffer);
 * var json = vectorTile.toJSON();
 * console.log(json);
 * // {
 * //   name: 'layer-name',
 * //   extent: 4096,
 * //   version: 2,
 * //   features: [ ... ] // array of objects
 * // }
 */
Napi::Value VectorTile::toJSON(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    bool decode_geometry = false;
    if (info.Length() >= 1)
    {
        if (!info[0].IsObject())
        {
            Napi::Error::New(env, "The first argument must be an object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[0].As<Napi::Object>();
        if (options.Has("decode_geometry"))
        {
            Napi::Value param_val = options.Get("decode_geometry");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'decode_geometry' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            decode_geometry = param_val.As<Napi::Boolean>();
        }
    }

    try
    {
        protozero::pbf_reader tile_msg = tile_->get_reader();
        Napi::Array arr = Napi::Array::New(env, tile_->get_layers().size());
        std::size_t l_idx = 0;
        while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
        {
            protozero::pbf_reader layer_msg = tile_msg.get_message();
            Napi::Object layer_obj = Napi::Object::New(env);
            std::vector<std::string> layer_keys;
            mapnik::vector_tile_impl::layer_pbf_attr_type layer_values;
            std::vector<protozero::pbf_reader> layer_features;
            protozero::pbf_reader val_msg;
            std::uint32_t version = 1;
            while (layer_msg.next())
            {
                switch (layer_msg.tag())
                {
                case mapnik::vector_tile_impl::Layer_Encoding::NAME:
                    layer_obj.Set("name", layer_msg.get_string());
                    break;
                case mapnik::vector_tile_impl::Layer_Encoding::FEATURES:
                    layer_features.push_back(layer_msg.get_message());
                    break;
                case mapnik::vector_tile_impl::Layer_Encoding::KEYS:
                    layer_keys.push_back(layer_msg.get_string());
                    break;
                case mapnik::vector_tile_impl::Layer_Encoding::VALUES:
                    val_msg = layer_msg.get_message();
                    while (val_msg.next())
                    {
                        switch (val_msg.tag())
                        {
                        case mapnik::vector_tile_impl::Value_Encoding::STRING:
                            layer_values.push_back(val_msg.get_string());
                            break;
                        case mapnik::vector_tile_impl::Value_Encoding::FLOAT:
                            layer_values.push_back(val_msg.get_float());
                            break;
                        case mapnik::vector_tile_impl::Value_Encoding::DOUBLE:
                            layer_values.push_back(val_msg.get_double());
                            break;
                        case mapnik::vector_tile_impl::Value_Encoding::INT:
                            layer_values.push_back(val_msg.get_int64());
                            break;
                        case mapnik::vector_tile_impl::Value_Encoding::UINT:
                            // LCOV_EXCL_START
                            layer_values.push_back(val_msg.get_uint64());
                            break;
                            // LCOV_EXCL_STOP
                        case mapnik::vector_tile_impl::Value_Encoding::SINT:
                            // LCOV_EXCL_START
                            layer_values.push_back(val_msg.get_sint64());
                            break;
                            // LCOV_EXCL_STOP
                        case mapnik::vector_tile_impl::Value_Encoding::BOOL:
                            layer_values.push_back(val_msg.get_bool());
                            break;
                        default:
                            // LCOV_EXCL_START
                            val_msg.skip();
                            break;
                            // LCOV_EXCL_STOP
                        }
                    }
                    break;
                case mapnik::vector_tile_impl::Layer_Encoding::EXTENT:
                    layer_obj.Set("extent", Napi::Number::New(env, layer_msg.get_uint32()));
                    break;
                case mapnik::vector_tile_impl::Layer_Encoding::VERSION:
                    version = layer_msg.get_uint32();
                    layer_obj.Set("version", Napi::Number::New(env, version));
                    break;
                default:
                    // LCOV_EXCL_START
                    layer_msg.skip();
                    break;
                    // LCOV_EXCL_STOP
                }
            }
            Napi::Array f_arr = Napi::Array::New(env, layer_features.size());
            std::size_t f_idx = 0;
            for (auto feature_msg : layer_features)
            {
                Napi::Object feature_obj = Napi::Object::New(env);
                mapnik::vector_tile_impl::GeometryPBF::pbf_itr geom_itr;
                mapnik::vector_tile_impl::GeometryPBF::pbf_itr tag_itr;
                bool has_geom = false;
                bool has_geom_type = false;
                bool has_tags = false;
                std::int32_t geom_type_enum = 0;
                while (feature_msg.next())
                {
                    switch (feature_msg.tag())
                    {
                    case mapnik::vector_tile_impl::Feature_Encoding::ID:
                        feature_obj.Set("id", Napi::Number::New(env, feature_msg.get_uint64()));
                        break;
                    case mapnik::vector_tile_impl::Feature_Encoding::TAGS:
                        tag_itr = feature_msg.get_packed_uint32();
                        has_tags = true;
                        break;
                    case mapnik::vector_tile_impl::Feature_Encoding::TYPE:
                        geom_type_enum = feature_msg.get_enum();
                        has_geom_type = true;
                        feature_obj.Set("type", Napi::Number::New(env, geom_type_enum));
                        break;
                    case mapnik::vector_tile_impl::Feature_Encoding::GEOMETRY:
                        geom_itr = feature_msg.get_packed_uint32();
                        has_geom = true;
                        break;
                    case mapnik::vector_tile_impl::Feature_Encoding::RASTER: {
                        auto im_buffer = feature_msg.get_view();
                        feature_obj.Set("raster",
                                        Napi::Buffer<char>::Copy(env, im_buffer.data(), im_buffer.size()));
                        break;
                    }
                    default:
                        // LCOV_EXCL_START
                        feature_msg.skip();
                        break;
                        // LCOV_EXCL_STOP
                    }
                }
                Napi::Object att_obj = Napi::Object::New(env);
                if (has_tags)
                {
                    for (auto _i = tag_itr.begin(); _i != tag_itr.end();)
                    {
                        std::size_t key_name = *(_i++);
                        if (_i == tag_itr.end())
                        {
                            break;
                        }
                        std::size_t key_value = *(_i++);
                        if (key_name < layer_keys.size() &&
                            key_value < layer_values.size())
                        {
                            std::string const& name = layer_keys.at(key_name);
                            mapnik::vector_tile_impl::pbf_attr_value_type val = layer_values.at(key_value);
                            json_value_visitor vv(env, att_obj, name);
                            mapnik::util::apply_visitor(vv, val);
                        }
                    }
                }
                feature_obj.Set("properties", att_obj);
                if (has_geom && has_geom_type)
                {
                    if (decode_geometry)
                    {
                        // Decode the geometry first into an int64_t mapnik geometry
                        mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
                        mapnik::geometry::geometry<std::int64_t> geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, geom_type_enum, version, 0, 0, 1.0, 1.0);
                        Napi::Array g_arr = geometry_to_array<std::int64_t>(env, geom);
                        feature_obj.Set("geometry", g_arr);
                        std::string geom_type = geometry_type_as_string(geom);
                        feature_obj.Set("geometry_type", geom_type);
                    }
                    else
                    {
                        std::vector<std::uint32_t> geom_vec;
                        for (auto _i = geom_itr.begin(); _i != geom_itr.end(); ++_i)
                        {
                            geom_vec.push_back(*_i);
                        }
                        Napi::Array g_arr = Napi::Array::New(env, geom_vec.size());
                        for (std::size_t k = 0; k < geom_vec.size(); ++k)
                        {
                            g_arr.Set(k, Napi::Number::New(env, geom_vec[k]));
                        }
                        feature_obj.Set("geometry", g_arr);
                    }
                }
                f_arr.Set(f_idx++, feature_obj);
            }
            layer_obj.Set("features", f_arr);
            arr.Set(l_idx++, layer_obj);
        }
        return scope.Escape(arr);
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
        // LCOV_EXCL_STOP
    }
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
 * var geojson = vectorTile.toGeoJSONSync('__all__');
 * geojson // stringified GeoJSON
 * JSON.parse(geojson); // GeoJSON object
 */
Napi::Value VectorTile::toGeoJSONSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() == 0)
    {
        Napi::Error::New(env, "first argument must be either a layer name (string) or layer index (integer)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Value layer_id = info[0];
    if (!(layer_id.IsString() || layer_id.IsNumber()))
    {
        Napi::TypeError::New(env, "'layer' argument must be either a layer name (string) or layer index (integer)").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string result;
    try
    {
        if (layer_id.IsString())
        {
            std::string layer_name = layer_id.As<Napi::String>();
            if (layer_name == "__array__")
            {
                write_geojson_array(result, tile_);
            }
            else if (layer_name == "__all__")
            {
                write_geojson_all(result, tile_);
            }
            else
            {
                if (!write_geojson_layer_name(result, layer_name, tile_))
                {
                    std::string error_msg("Layer name '" + layer_name + "' not found");
                    Napi::TypeError::New(env, error_msg.c_str()).ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
        }
        else if (layer_id.IsNumber())
        {
            int layer_idx = layer_id.As<Napi::Number>().Int32Value();
            if (layer_idx < 0)
            {
                Napi::TypeError::New(env, "A layer index can not be negative").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            else if (layer_idx >= static_cast<int>(tile_->get_layers().size()))
            {
                Napi::TypeError::New(env, "Layer index exceeds the number of layers in the vector tile.").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            if (!write_geojson_layer_index(result, layer_idx, tile_))
            {
                // LCOV_EXCL_START
                Napi::TypeError::New(env, "Layer could not be retrieved (should have not reached here)").ThrowAsJavaScriptException();
                return env.Undefined();
                // LCOV_EXCL_STOP
            }
        }
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        Napi::TypeError::New(env, ex.what()).ThrowAsJavaScriptException();

        return env.Undefined();
        // LCOV_EXCL_STOP
    }
    return scope.Escape(Napi::String::New(env, result));
}

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
 * vectorTile.toGeoJSON('__all__',function(err, geojson) {
 *   if (err) throw err;
 *   console.log(geojson); // stringified GeoJSON
 *   console.log(JSON.parse(geojson)); // GeoJSON object
 * });
 */

Napi::Value VectorTile::toGeoJSON(Napi::CallbackInfo const& info)
{
    if ((info.Length() < 1) || !info[info.Length() - 1].IsFunction())
    {
        return toGeoJSONSync(info);
    }
    Napi::Env env = info.Env();

    Napi::Value layer_id = info[0];
    if (!(layer_id.IsString() || layer_id.IsNumber()))
    {
        Napi::TypeError::New(env, "'layer' argument must be either a layer name (string) or layer index (integer)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    geojson_write_type type{geojson_write_all};
    int layer_idx = 0;
    std::string layer_name{};

    if (layer_id.IsString())
    {
        layer_name = layer_id.As<Napi::String>();
        if (layer_name == "__array__")
        {
            type = geojson_write_array;
        }
        else if (layer_name == "__all__")
        {
            type = geojson_write_all;
        }
        else
        {
            if (!tile_->has_layer(layer_name))
            {
                std::string error_msg("The layer does not contain the name: " + layer_name);
                Napi::TypeError::New(env, error_msg.c_str()).ThrowAsJavaScriptException();
                return env.Undefined();
            }
            type = geojson_write_layer_name;
        }
    }
    else if (layer_id.IsNumber())
    {
        layer_idx = layer_id.As<Napi::Number>().Int32Value();
        if (layer_idx < 0)
        {
            Napi::TypeError::New(env, "A layer index can not be negative").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        else if (layer_idx >= static_cast<int>(tile_->get_layers().size()))
        {
            Napi::TypeError::New(env, "Layer index exceeds the number of layers in the vector tile.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        type = geojson_write_layer_index;
    }

    Napi::Value callback = info[info.Length() - 1];
    auto* worker = new AsyncToGeoJSON(tile_, type, layer_idx, layer_name, callback.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
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
Napi::Value VectorTile::addGeoJSON(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::Error::New(env, "first argument must be a GeoJSON string").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (info.Length() < 2 || !info[1].IsString())
    {
        Napi::Error::New(env, "second argument must be a layer name (string)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string geojson_string = info[0].As<Napi::String>();
    std::string geojson_name = info[1].As<Napi::String>();

    Napi::Object options = Napi::Object::New(env);
    double area_threshold = 0.1;
    double simplify_distance = 0.0;
    bool strictly_simple = true;
    bool multi_polygon_union = false;
    mapnik::vector_tile_impl::polygon_fill_type fill_type = mapnik::vector_tile_impl::positive_fill;
    bool process_all_rings = false;

    if (info.Length() > 2)
    {
        // options object
        if (!info[2].IsObject())
        {
            Napi::Error::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        options = info[2].As<Napi::Object>();
        if (options.Has("area_threshold"))
        {
            Napi::Value param_val = options.Get("area_threshold");
            if (!param_val.IsNumber())
            {
                Napi::Error::New(env, "option 'area_threshold' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            area_threshold = param_val.As<Napi::Number>().Int32Value();
            if (area_threshold < 0.0)
            {
                Napi::Error::New(env, "option 'area_threshold' can not be negative").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("strictly_simple"))
        {
            Napi::Value param_val = options.Get("strictly_simple");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'strictly_simple' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            strictly_simple = param_val.As<Napi::Boolean>();
        }
        if (options.Has("multi_polygon_union"))
        {
            Napi::Value mpu = options.Get("multi_polygon_union");
            if (!mpu.IsBoolean())
            {
                Napi::TypeError::New(env, "multi_polygon_union value must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            multi_polygon_union = mpu.As<Napi::Boolean>();
        }
        if (options.Has("fill_type"))
        {
            Napi::Value ft = options.Get("fill_type");
            if (!ft.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'fill_type' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(ft.As<Napi::Number>().Int32Value());
            if (fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
            {
                Napi::TypeError::New(env, "optional arg 'fill_type' out of possible range").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("simplify_distance"))
        {
            Napi::Value param_val = options.Get("simplify_distance");
            if (!param_val.IsNumber())
            {
                Napi::TypeError::New(env, "option 'simplify_distance' must be an floating point number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            simplify_distance = param_val.As<Napi::Number>().DoubleValue();
            if (simplify_distance < 0.0)
            {
                Napi::TypeError::New(env, "option 'simplify_distance' must be a positive number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("process_all_rings"))
        {
            Napi::Value param_val = options.Get("process_all_rings");
            if (!param_val.IsBoolean())
            {
                Napi::TypeError::New(env, "option 'process_all_rings' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            process_all_rings = param_val.As<Napi::Boolean>();
        }
    }

    try
    {
        // create map object
        auto tile_size = tile_->tile_size();
        mapnik::Map map(tile_size, tile_size, "epsg:3857");
        mapnik::parameters p;
        p["type"] = "geojson";
        p["inline"] = geojson_string;
        mapnik::layer lyr(geojson_name, "epsg:4326");
        lyr.set_datasource(mapnik::datasource_cache::instance().create(p));
        map.add_layer(lyr);

        mapnik::vector_tile_impl::processor ren(map);
        ren.set_area_threshold(area_threshold);
        ren.set_strictly_simple(strictly_simple);
        ren.set_simplify_distance(simplify_distance);
        ren.set_multi_polygon_union(multi_polygon_union);
        ren.set_fill_type(fill_type);
        ren.set_process_all_rings(process_all_rings);
        ren.update_tile(*tile_);
        return Napi::Boolean::New(env, true);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}
