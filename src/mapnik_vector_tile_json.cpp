#include "utils.hpp"

#define MAPNIK_VECTOR_TILE_LIBRARY
#include "mapnik_vector_tile.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_datasource_pbf.hpp"

// std
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

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
    v8::Local<v8::Array> operator() (mapnik::geometry::geometry_empty const &)
    {
        // Removed as it should be a bug if a vector tile has reached this point
        // therefore no known tests reach this point
        // LCOV_EXCL_START
        Nan::EscapableHandleScope scope;
        return scope.Escape(Nan::New<v8::Array>());
        // LCOV_EXCL_STOP
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::point<T> const & geom)
    {
        Nan::EscapableHandleScope scope;
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(2);
        arr->Set(0, Nan::New<v8::Number>(geom.x));
        arr->Set(1, Nan::New<v8::Number>(geom.y));
        return scope.Escape(arr);
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::line_string<T> const & geom)
    {
        Nan::EscapableHandleScope scope;
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return scope.Escape(Nan::New<v8::Array>());
            // LCOV_EXCL_STOP
        }
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(geom.size());
        std::uint32_t c = 0;
        for (auto const & pt : geom)
        {
            arr->Set(c++, (*this)(pt));
        }
        return scope.Escape(arr);
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::linear_ring<T> const & geom)
    {
        Nan::EscapableHandleScope scope;
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return scope.Escape(Nan::New<v8::Array>());
            // LCOV_EXCL_STOP
        }
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(geom.size());
        std::uint32_t c = 0;
        for (auto const & pt : geom)
        {
            arr->Set(c++, (*this)(pt));
        }
        return scope.Escape(arr);
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::multi_point<T> const & geom)
    {
        Nan::EscapableHandleScope scope;
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return scope.Escape(Nan::New<v8::Array>());
            // LCOV_EXCL_STOP
        }
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(geom.size());
        std::uint32_t c = 0;
        for (auto const & pt : geom)
        {
            arr->Set(c++, (*this)(pt));
        }
        return scope.Escape(arr);
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::multi_line_string<T> const & geom)
    {
        Nan::EscapableHandleScope scope;
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return scope.Escape(Nan::New<v8::Array>());
            // LCOV_EXCL_STOP
        }
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(geom.size());
        std::uint32_t c = 0;
        for (auto const & pt : geom)
        {
            arr->Set(c++, (*this)(pt));
        }
        return scope.Escape(arr);
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::polygon<T> const & geom)
    {
        Nan::EscapableHandleScope scope;
        if (geom.exterior_ring.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return scope.Escape(Nan::New<v8::Array>());
            // LCOV_EXCL_STOP
        }
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(1 + geom.interior_rings.size());
        std::uint32_t c = 0;
        arr->Set(c++, (*this)(geom.exterior_ring));
        for (auto const & pt : geom.interior_rings)
        {
            arr->Set(c++, (*this)(pt));
        }
        return scope.Escape(arr);
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::multi_polygon<T> const & geom)
    {
        Nan::EscapableHandleScope scope;
        if (geom.empty())
        {
            // Removed as it should be a bug if a vector tile has reached this point
            // therefore no known tests reach this point
            // LCOV_EXCL_START
            return scope.Escape(Nan::New<v8::Array>());
            // LCOV_EXCL_STOP
        }
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(geom.size());
        std::uint32_t c = 0;
        for (auto const & pt : geom)
        {
            arr->Set(c++, (*this)(pt));
        }
        return scope.Escape(arr);
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::geometry<T> const & geom)
    {
        // Removed as it should be a bug if a vector tile has reached this point
        // therefore no known tests reach this point
        // LCOV_EXCL_START
        Nan::EscapableHandleScope scope;
        return scope.Escape(mapnik::util::apply_visitor((*this), geom));
        // LCOV_EXCL_STOP
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::geometry_collection<T> const & geom)
    {
        // Removed as it should be a bug if a vector tile has reached this point
        // therefore no known tests reach this point
        // LCOV_EXCL_START
        Nan::EscapableHandleScope scope;
        if (geom.empty())
        {
            return scope.Escape(Nan::New<v8::Array>());
        }
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(geom.size());
        std::uint32_t c = 0;
        for (auto const & pt : geom)
        {
            arr->Set(c++, (*this)(pt));
        }
        return scope.Escape(arr);
        // LCOV_EXCL_STOP
    }
};

template <typename T>
v8::Local<v8::Array> geometry_to_array(mapnik::geometry::geometry<T> const & geom)
{
    Nan::EscapableHandleScope scope;
    return scope.Escape(mapnik::util::apply_visitor(geometry_array_visitor(), geom));
}

struct json_value_visitor
{
    v8::Local<v8::Object> & att_obj_;
    std::string const& name_;

    json_value_visitor(v8::Local<v8::Object> & att_obj,
                       std::string const& name)
        : att_obj_(att_obj),
          name_(name) {}

    void operator() (std::string const& val)
    {
        att_obj_->Set(Nan::New(name_).ToLocalChecked(), Nan::New(val).ToLocalChecked());
    }

    void operator() (bool const& val)
    {
        att_obj_->Set(Nan::New(name_).ToLocalChecked(), Nan::New<v8::Boolean>(val));
    }

    void operator() (int64_t const& val)
    {
        att_obj_->Set(Nan::New(name_).ToLocalChecked(), Nan::New<v8::Number>(val));
    }

    void operator() (uint64_t const& val)
    {
        // LCOV_EXCL_START
        att_obj_->Set(Nan::New(name_).ToLocalChecked(), Nan::New<v8::Number>(val));
        // LCOV_EXCL_STOP
    }

    void operator() (double const& val)
    {
        att_obj_->Set(Nan::New(name_).ToLocalChecked(), Nan::New<v8::Number>(val));
    }

    void operator() (float const& val)
    {
        att_obj_->Set(Nan::New(name_).ToLocalChecked(), Nan::New<v8::Number>(val));
    }
};

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
NAN_METHOD(VectorTile::toJSON)
{
    bool decode_geometry = false;
    if (info.Length() >= 1)
    {
        if (!info[0]->IsObject())
        {
            Nan::ThrowError("The first argument must be an object");
            return;
        }
        v8::Local<v8::Object> options = info[0]->ToObject();

        if (options->Has(Nan::New("decode_geometry").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("decode_geometry").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'decode_geometry' must be a boolean");
                return;
            }
            decode_geometry = param_val->BooleanValue();
        }
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    try
    {
        protozero::pbf_reader tile_msg = d->tile_->get_reader();
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(d->tile_->get_layers().size());
        std::size_t l_idx = 0;
        while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
        {
            protozero::pbf_reader layer_msg = tile_msg.get_message();
            v8::Local<v8::Object> layer_obj = Nan::New<v8::Object>();
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
                        layer_obj->Set(Nan::New("name").ToLocalChecked(), Nan::New<v8::String>(layer_msg.get_string()).ToLocalChecked());
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
                        layer_obj->Set(Nan::New("extent").ToLocalChecked(), Nan::New<v8::Integer>(layer_msg.get_uint32()));
                        break;
                    case mapnik::vector_tile_impl::Layer_Encoding::VERSION:
                        version = layer_msg.get_uint32();
                        layer_obj->Set(Nan::New("version").ToLocalChecked(), Nan::New<v8::Integer>(version));
                        break;
                    default:
                        // LCOV_EXCL_START
                        layer_msg.skip();
                        break;
                        // LCOV_EXCL_STOP
                }
            }
            v8::Local<v8::Array> f_arr = Nan::New<v8::Array>(layer_features.size());
            std::size_t f_idx = 0;
            for (auto feature_msg : layer_features)
            {
                v8::Local<v8::Object> feature_obj = Nan::New<v8::Object>();
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
                            feature_obj->Set(Nan::New("id").ToLocalChecked(),Nan::New<v8::Number>(feature_msg.get_uint64()));
                            break;
                        case mapnik::vector_tile_impl::Feature_Encoding::TAGS:
                            tag_itr = feature_msg.get_packed_uint32();
                            has_tags = true;
                            break;
                        case mapnik::vector_tile_impl::Feature_Encoding::TYPE:
                            geom_type_enum = feature_msg.get_enum();
                            has_geom_type = true;
                            feature_obj->Set(Nan::New("type").ToLocalChecked(),Nan::New<v8::Integer>(geom_type_enum));
                            break;
                        case mapnik::vector_tile_impl::Feature_Encoding::GEOMETRY:
                            geom_itr = feature_msg.get_packed_uint32();
                            has_geom = true;
                            break;
                        case mapnik::vector_tile_impl::Feature_Encoding::RASTER:
                        {
                            auto im_buffer = feature_msg.get_view();
                            feature_obj->Set(Nan::New("raster").ToLocalChecked(),
                                             Nan::CopyBuffer(im_buffer.data(), im_buffer.size()).ToLocalChecked());
                            break;
                        }
                        default:
                            // LCOV_EXCL_START
                            feature_msg.skip();
                            break;
                            // LCOV_EXCL_STOP
                    }
                }
                v8::Local<v8::Object> att_obj = Nan::New<v8::Object>();
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
                            json_value_visitor vv(att_obj, name);
                            mapnik::util::apply_visitor(vv, val);
                        }
                    }
                }
                feature_obj->Set(Nan::New("properties").ToLocalChecked(),att_obj);
                if (has_geom && has_geom_type)
                {
                    if (decode_geometry)
                    {
                        // Decode the geometry first into an int64_t mapnik geometry
                        mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
                        mapnik::geometry::geometry<std::int64_t> geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, geom_type_enum, version, 0, 0, 1.0, 1.0);
                        v8::Local<v8::Array> g_arr = geometry_to_array<std::int64_t>(geom);
                        feature_obj->Set(Nan::New("geometry").ToLocalChecked(),g_arr);
                        std::string geom_type = geometry_type_as_string(geom);
                        feature_obj->Set(Nan::New("geometry_type").ToLocalChecked(),Nan::New(geom_type).ToLocalChecked());
                    }
                    else
                    {
                        std::vector<std::uint32_t> geom_vec;
                        for (auto _i = geom_itr.begin(); _i != geom_itr.end(); ++_i)
                        {
                            geom_vec.push_back(*_i);
                        }
                        v8::Local<v8::Array> g_arr = Nan::New<v8::Array>(geom_vec.size());
                        for (std::size_t k = 0; k < geom_vec.size();++k)
                        {
                            g_arr->Set(k,Nan::New<v8::Number>(geom_vec[k]));
                        }
                        feature_obj->Set(Nan::New("geometry").ToLocalChecked(),g_arr);
                    }
                }
                f_arr->Set(f_idx++,feature_obj);
            }
            layer_obj->Set(Nan::New("features").ToLocalChecked(), f_arr);
            arr->Set(l_idx++, layer_obj);
        }
        info.GetReturnValue().Set(arr);
        return;
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return;
        // LCOV_EXCL_STOP
    }
}

