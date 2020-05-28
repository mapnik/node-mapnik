//
#include "mapnik_vector_tile.hpp"
// mapnik
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/feature_to_geojson.hpp>
//#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_compression.hpp"
#include "vector_tile_composite.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_load_tile.hpp"
#include "object_to_container.hpp"
// protozero
//#include <protozero/pbf_reader.hpp>


namespace {

struct geometry_type_name
{
    template <typename T>
    std::string operator () (T const& geom) const
    {
        return mapnik::util::apply_visitor(*this, geom);
    }

    std::string operator() (mapnik::geometry::geometry_empty const& ) const
    {
        // LCOV_EXCL_START
        return "Empty";
        // LCOV_EXCL_STOP
    }

    template <typename T>
    std::string operator () (mapnik::geometry::point<T> const&) const
    {
        return "Point";
    }

    template <typename T>
    std::string operator () (mapnik::geometry::line_string<T> const&) const
    {
        return "LineString";
    }

    template <typename T>
    std::string operator () (mapnik::geometry::polygon<T> const&) const
    {
        return "Polygon";
    }

    template <typename T>
    std::string operator () (mapnik::geometry::multi_point<T> const&) const
    {
        return "MultiPoint";
    }

    template <typename T>
    std::string operator () (mapnik::geometry::multi_line_string<T> const&) const
    {
        return "MultiLineString";
    }

    template <typename T>
    std::string operator () (mapnik::geometry::multi_polygon<T> const&) const
    {
        return "MultiPolygon";
    }

    template <typename T>
    std::string operator () (mapnik::geometry::geometry_collection<T> const&) const
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

    Napi::Array operator() (mapnik::geometry::geometry_empty const&)
    {
        // Removed as it should be a bug if a vector tile has reached this point
        // therefore no known tests reach this point
        // LCOV_EXCL_START
        return Napi::Array::New(env_);
        // LCOV_EXCL_STOP
    }

    template <typename T>
    Napi::Array operator() (mapnik::geometry::point<T> const& geom)
    {
        Napi::Array arr = Napi::Array::New(env_, 2);
        arr.Set(0u, Napi::Number::New(env_, geom.x));
        arr.Set(1u, Napi::Number::New(env_, geom.y));
        return arr;
    }

    template <typename T>
    Napi::Array operator() (mapnik::geometry::line_string<T> const & geom)
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
        for (auto const & pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator() (mapnik::geometry::linear_ring<T> const & geom)
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
        for (auto const & pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator() (mapnik::geometry::multi_point<T> const & geom)
    {
        Napi::EscapableHandleScope scope(env_);
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
        for (auto const & pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator() (mapnik::geometry::multi_line_string<T> const & geom)
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
        for (auto const & pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator() (mapnik::geometry::polygon<T> const & poly)
    {
        Napi::Array arr = Napi::Array::New(env_, poly.size());
        std::uint32_t index = 0;

        for (auto const & ring : poly)
        {
            arr.Set(index++, (*this)(ring));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator() (mapnik::geometry::multi_polygon<T> const & geom)
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
        for (auto const & pt : geom)
        {
            arr.Set(c++, (*this)(pt));
        }
        return arr;
    }

    template <typename T>
    Napi::Array operator() (mapnik::geometry::geometry<T> const & geom)
    {
        // Removed as it should be a bug if a vector tile has reached this point
        // therefore no known tests reach this point
        // LCOV_EXCL_START
        return mapnik::util::apply_visitor((*this), geom);
        // LCOV_EXCL_STOP
    }

    template <typename T>
    Napi::Array operator() (mapnik::geometry::geometry_collection<T> const & geom)
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
        for (auto const & pt : geom)
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
Napi::Array geometry_to_array(Napi::Env const& env, mapnik::geometry::geometry<T> const & geom)
{
    return mapnik::util::apply_visitor(geometry_array_visitor(env), geom);
}


struct json_value_visitor
{

    json_value_visitor(Napi::Env env, Napi::Object & att_obj,
                       std::string const& name)
        : env_(env), att_obj_(att_obj),
          name_(name) {}

    void operator() (std::string const& val)
    {
        att_obj_.Set(name_, val);
    }

    void operator() (bool const& val)
    {
        att_obj_.Set(name_, Napi::Boolean::New(env_, val));
    }

    void operator() (int64_t const& val)
    {
        att_obj_.Set(name_, Napi::Number::New(env_, val));
    }

    void operator() (uint64_t const& val)
    {
        // LCOV_EXCL_START
        att_obj_.Set(name_, Napi::Number::New(env_, val));
        // LCOV_EXCL_STOP
    }

    void operator() (double const& val)
    {
        att_obj_.Set(name_, Napi::Number::New(env_, val));
    }

    void operator() (float const& val)
    {
        att_obj_.Set(name_, Napi::Number::New(env_, val));
    }
    Napi::Env env_;
    Napi::Object & att_obj_;
    std::string const& name_;
};
}

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
                            switch(val_msg.tag())
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
                            feature_obj.Set("id",Napi::Number::New(env, feature_msg.get_uint64()));
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
                        case mapnik::vector_tile_impl::Feature_Encoding::RASTER:
                        {
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
                        for (std::size_t k = 0; k < geom_vec.size();++k)
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
