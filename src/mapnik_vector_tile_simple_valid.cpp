#include "mapnik_vector_tile.hpp"

#if BOOST_VERSION >= 105800

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/geometry/is_simple.hpp>
#include <mapnik/geometry/is_valid.hpp>
#include <mapnik/geometry/reprojection.hpp>
#include <mapnik/util/feature_to_geojson.hpp>
#include <mapnik/projection.hpp>
// mapnik-vector-tile
#include "mapnik_vector_tile.hpp"
#include "vector_tile_compression.hpp"
#include "vector_tile_composite.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_load_tile.hpp"
#include "object_to_container.hpp"

namespace {

// LCOV_EXCL_START
struct not_simple_feature
{
    not_simple_feature(std::string const& layer_,
                       std::int64_t feature_id_)
        : layer(layer_),
          feature_id(feature_id_) {}
    std::string const layer;
    std::int64_t const feature_id;
};
// LCOV_EXCL_STOP

struct not_valid_feature
{
    not_valid_feature(std::string const& message_,
                      std::string const& layer_,
                      std::int64_t feature_id_,
                      std::string const& geojson_)
        : message(message_),
          layer(layer_),
          feature_id(feature_id_),
          geojson(geojson_) {}
    std::string const message;
    std::string const layer;
    std::int64_t const feature_id;
    std::string const geojson;
};

void layer_not_simple(protozero::pbf_reader const& layer_msg,
                      unsigned x,
                      unsigned y,
                      unsigned z,
                      std::vector<not_simple_feature>& errors)
{
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer_msg, x, y, z);
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
    if (fs && mapnik::is_valid(fs))
    {
        mapnik::feature_ptr feature;
        while ((feature = fs->next()))
        {
            if (!mapnik::geometry::is_simple(feature->get_geometry())) // NOLINT
            {
                // Right now we don't have an obvious way of bypassing our validation
                // process in JS, so let's skip testing this line
                // LCOV_EXCL_START
                errors.emplace_back(ds.get_name(), feature->id());
                // LCOV_EXCL_STOP
            }
        }
    }
}

struct visitor_geom_valid
{
    std::vector<not_valid_feature>& errors;
    mapnik::feature_ptr& feature;
    std::string const& layer_name;
    bool split_multi_features;

    visitor_geom_valid(std::vector<not_valid_feature>& errors_,
                       mapnik::feature_ptr& feature_,
                       std::string const& layer_name_,
                       bool split_multi_features_)
        : errors(errors_),
          feature(feature_),
          layer_name(layer_name_),
          split_multi_features(split_multi_features_) {}

    void operator()(mapnik::geometry::geometry_empty const&) {}

    template <typename T>
    void operator()(mapnik::geometry::point<T> const& geom)
    {
        std::string message;
        if (!mapnik::geometry::is_valid(geom, message)) // NOLINT
        {
            mapnik::feature_impl feature_new(feature->context(), feature->id());
            std::string result;
            std::string feature_str;
            result += "{\"type\":\"FeatureCollection\",\"features\":[";
            feature_new.set_data(feature->get_data());
            feature_new.set_geometry(mapnik::geometry::geometry<T>(geom));
            if (!mapnik::util::to_geojson(feature_str, feature_new))
            {
                // LCOV_EXCL_START
                throw std::runtime_error("Failed to generate GeoJSON geometry");
                // LCOV_EXCL_STOP
            }
            result += feature_str;
            result += "]}";
            errors.emplace_back(message,
                                layer_name,
                                feature->id(),
                                result);
        }
    }

    template <typename T>
    void operator()(mapnik::geometry::multi_point<T> const& geom)
    {
        std::string message;
        if (!mapnik::geometry::is_valid(geom, message)) // NOLINT
        {
            mapnik::feature_impl feature_new(feature->context(), feature->id());
            std::string result;
            std::string feature_str;
            result += "{\"type\":\"FeatureCollection\",\"features\":[";
            feature_new.set_data(feature->get_data());
            feature_new.set_geometry(mapnik::geometry::geometry<T>(geom));
            if (!mapnik::util::to_geojson(feature_str, feature_new))
            {
                // LCOV_EXCL_START
                throw std::runtime_error("Failed to generate GeoJSON geometry");
                // LCOV_EXCL_STOP
            }
            result += feature_str;
            result += "]}";
            errors.emplace_back(message,
                                layer_name,
                                feature->id(),
                                result);
        }
    }

    template <typename T>
    void operator()(mapnik::geometry::line_string<T> const& geom)
    {
        std::string message;
        if (!mapnik::geometry::is_valid(geom, message)) // NOLINT
        {
            mapnik::feature_impl feature_new(feature->context(), feature->id());
            std::string result;
            std::string feature_str;
            result += "{\"type\":\"FeatureCollection\",\"features\":[";
            feature_new.set_data(feature->get_data());
            feature_new.set_geometry(mapnik::geometry::geometry<T>(geom));
            if (!mapnik::util::to_geojson(feature_str, feature_new))
            {
                // LCOV_EXCL_START
                throw std::runtime_error("Failed to generate GeoJSON geometry");
                // LCOV_EXCL_STOP
            }
            result += feature_str;
            result += "]}";
            errors.emplace_back(message,
                                layer_name,
                                feature->id(),
                                result);
        }
    }

    template <typename T>
    void operator()(mapnik::geometry::multi_line_string<T> const& geom)
    {
        if (split_multi_features)
        {
            for (auto const& ls : geom)
            {
                std::string message;
                if (!mapnik::geometry::is_valid(ls, message)) // NOLINT
                {
                    mapnik::feature_impl feature_new(feature->context(), feature->id());
                    std::string result;
                    std::string feature_str;
                    result += "{\"type\":\"FeatureCollection\",\"features\":[";
                    feature_new.set_data(feature->get_data());
                    feature_new.set_geometry(mapnik::geometry::geometry<T>(ls));
                    if (!mapnik::util::to_geojson(feature_str, feature_new))
                    {
                        // LCOV_EXCL_START
                        throw std::runtime_error("Failed to generate GeoJSON geometry");
                        // LCOV_EXCL_STOP
                    }
                    result += feature_str;
                    result += "]}";
                    errors.emplace_back(message,
                                        layer_name,
                                        feature->id(),
                                        result);
                }
            }
        }
        else
        {
            std::string message;
            if (!mapnik::geometry::is_valid(geom, message)) // NOLINT
            {
                mapnik::feature_impl feature_new(feature->context(), feature->id());
                std::string result;
                std::string feature_str;
                result += "{\"type\":\"FeatureCollection\",\"features\":[";
                feature_new.set_data(feature->get_data());
                feature_new.set_geometry(mapnik::geometry::geometry<T>(geom));
                if (!mapnik::util::to_geojson(feature_str, feature_new))
                {
                    // LCOV_EXCL_START
                    throw std::runtime_error("Failed to generate GeoJSON geometry");
                    // LCOV_EXCL_STOP
                }
                result += feature_str;
                result += "]}";
                errors.emplace_back(message,
                                    layer_name,
                                    feature->id(),
                                    result);
            }
        }
    }

    template <typename T>
    void operator()(mapnik::geometry::polygon<T> const& geom)
    {
        std::string message;
        if (!mapnik::geometry::is_valid(geom, message)) // NOLINT
        {
            mapnik::feature_impl feature_new(feature->context(), feature->id());
            std::string result;
            std::string feature_str;
            result += "{\"type\":\"FeatureCollection\",\"features\":[";
            feature_new.set_data(feature->get_data());
            feature_new.set_geometry(mapnik::geometry::geometry<T>(geom));
            if (!mapnik::util::to_geojson(feature_str, feature_new))
            {
                // LCOV_EXCL_START
                throw std::runtime_error("Failed to generate GeoJSON geometry");
                // LCOV_EXCL_STOP
            }
            result += feature_str;
            result += "]}";
            errors.emplace_back(message,
                                layer_name,
                                feature->id(),
                                result);
        }
    }

    template <typename T>
    void operator()(mapnik::geometry::multi_polygon<T> const& geom)
    {
        if (split_multi_features)
        {
            for (auto const& poly : geom)
            {
                std::string message;
                if (!mapnik::geometry::is_valid(poly, message))
                {
                    mapnik::feature_impl feature_new(feature->context(), feature->id());
                    std::string result;
                    std::string feature_str;
                    result += "{\"type\":\"FeatureCollection\",\"features\":[";
                    feature_new.set_data(feature->get_data());
                    feature_new.set_geometry(mapnik::geometry::geometry<T>(poly));
                    if (!mapnik::util::to_geojson(feature_str, feature_new))
                    {
                        // LCOV_EXCL_START
                        throw std::runtime_error("Failed to generate GeoJSON geometry");
                        // LCOV_EXCL_STOP
                    }
                    result += feature_str;
                    result += "]}";
                    errors.emplace_back(message,
                                        layer_name,
                                        feature->id(),
                                        result);
                }
            }
        }
        else
        {
            std::string message;
            if (!mapnik::geometry::is_valid(geom, message))
            {
                mapnik::feature_impl feature_new(feature->context(), feature->id());
                std::string result;
                std::string feature_str;
                result += "{\"type\":\"FeatureCollection\",\"features\":[";
                feature_new.set_data(feature->get_data());
                feature_new.set_geometry(mapnik::geometry::geometry<T>(geom));
                if (!mapnik::util::to_geojson(feature_str, feature_new))
                {
                    // LCOV_EXCL_START
                    throw std::runtime_error("Failed to generate GeoJSON geometry");
                    // LCOV_EXCL_STOP
                }
                result += feature_str;
                result += "]}";
                errors.emplace_back(message,
                                    layer_name,
                                    feature->id(),
                                    result);
            }
        }
    }

    template <typename T>
    void operator()(mapnik::geometry::geometry_collection<T> const& geom)
    {
        // This should never be able to be reached.
        // LCOV_EXCL_START
        for (auto const& g : geom)
        {
            mapnik::util::apply_visitor((*this), g);
        }
        // LCOV_EXCL_STOP
    }
};

void layer_not_valid(protozero::pbf_reader& layer_msg,
                     unsigned x,
                     unsigned y,
                     unsigned z,
                     std::vector<not_valid_feature>& errors,
                     bool split_multi_features = false,
                     bool lat_lon = false,
                     bool web_merc = false)
{
    if (web_merc || lat_lon)
    {
        mapnik::vector_tile_impl::tile_datasource_pbf ds(layer_msg, x, y, z);
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
        if (fs && mapnik::is_valid(fs))
        {
            mapnik::feature_ptr feature;
            while ((feature = fs->next()))
            {
                if (lat_lon)
                {
                    mapnik::projection wgs84("epsg:4326", true);
                    mapnik::projection merc("epsg:3857", true);
                    mapnik::proj_transform prj_trans(merc, wgs84);
                    unsigned int n_err = 0;
                    mapnik::util::apply_visitor(
                        visitor_geom_valid(errors, feature, ds.get_name(), split_multi_features),
                        mapnik::geometry::reproject_copy(feature->get_geometry(), prj_trans, n_err));
                }
                else
                {
                    mapnik::util::apply_visitor(
                        visitor_geom_valid(errors, feature, ds.get_name(), split_multi_features),
                        feature->get_geometry());
                }
            }
        }
    }
    else
    {
        std::vector<protozero::pbf_reader> layer_features;
        std::uint32_t version = 1;
        std::string layer_name;
        while (layer_msg.next())
        {
            switch (layer_msg.tag())
            {
            case mapnik::vector_tile_impl::Layer_Encoding::NAME:
                layer_name = layer_msg.get_string();
                break;
            case mapnik::vector_tile_impl::Layer_Encoding::FEATURES:
                layer_features.push_back(layer_msg.get_message());
                break;
            case mapnik::vector_tile_impl::Layer_Encoding::VERSION:
                version = layer_msg.get_uint32();
                break;
            default:
                layer_msg.skip();
                break;
            }
        }
        for (auto feature_msg : layer_features)
        {
            mapnik::vector_tile_impl::GeometryPBF::pbf_itr geom_itr;
            bool has_geom = false;
            bool has_geom_type = false;
            std::int32_t geom_type_enum = 0;
            //std::uint64_t feature_id = 0;
            while (feature_msg.next())
            {
                switch (feature_msg.tag())
                {
                case mapnik::vector_tile_impl::Feature_Encoding::TYPE:
                    geom_type_enum = feature_msg.get_enum();
                    has_geom_type = true;
                    break;
                case mapnik::vector_tile_impl::Feature_Encoding::GEOMETRY:
                    geom_itr = feature_msg.get_packed_uint32();
                    has_geom = true;
                    break;
                default:
                    feature_msg.skip();
                    break;
                }
            }
            if (has_geom && has_geom_type)
            {
                // Decode the geometry first into an int64_t mapnik geometry
                mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
                mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx, 1));
                mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
                feature->set_geometry(mapnik::vector_tile_impl::decode_geometry<double>(geoms, geom_type_enum, version, 0.0, 0.0, 1.0, 1.0));
                mapnik::util::apply_visitor(
                    visitor_geom_valid(errors, feature, layer_name, split_multi_features),
                    feature->get_geometry());
            }
        }
    }
}

void vector_tile_not_simple(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                            std::vector<not_simple_feature>& errors)
{
    protozero::pbf_reader tile_msg(tile->get_reader());
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        protozero::pbf_reader layer_msg(tile_msg.get_message());
        layer_not_simple(layer_msg,
                         tile->x(),
                         tile->y(),
                         tile->z(),
                         errors);
    }
}

Napi::Array make_not_simple_array(Napi::Env env, std::vector<not_simple_feature>& errors)
{
    Napi::Array array = Napi::Array::New(env, errors.size());
    Napi::String layer_key = Napi::String::New(env, "layer");
    Napi::String feature_id_key = Napi::String::New(env, "featureId");
    std::uint32_t idx = 0;
    for (auto const& error : errors)
    {
        // LCOV_EXCL_START
        Napi::Object obj = Napi::Object::New(env);
        obj.Set(layer_key, Napi::String::New(env, error.layer));
        obj.Set(feature_id_key, Napi::Number::New(env, error.feature_id));
        array.Set(idx++, obj);
        // LCOV_EXCL_STOP
    }
    return array;
}

void vector_tile_not_valid(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                           std::vector<not_valid_feature>& errors,
                           bool split_multi_features = false,
                           bool lat_lon = false,
                           bool web_merc = false)
{
    protozero::pbf_reader tile_msg(tile->get_reader());
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        protozero::pbf_reader layer_msg(tile_msg.get_message());
        layer_not_valid(layer_msg,
                        tile->x(),
                        tile->y(),
                        tile->z(),
                        errors,
                        split_multi_features,
                        lat_lon,
                        web_merc);
    }
}

Napi::Array make_not_valid_array(Napi::Env env, std::vector<not_valid_feature>& errors)
{
    Napi::Array array = Napi::Array::New(env, errors.size());
    Napi::String layer_key = Napi::String::New(env, "layer");
    Napi::String feature_id_key = Napi::String::New(env, "featureId");
    Napi::String message_key = Napi::String::New(env, "message");
    Napi::String geojson_key = Napi::String::New(env, "geojson");
    std::size_t idx = 0;
    for (auto const& error : errors)
    {
        Napi::Object obj = Napi::Object::New(env);
        obj.Set(layer_key, Napi::String::New(env, error.layer));
        obj.Set(message_key, Napi::String::New(env, error.message));
        obj.Set(feature_id_key, Napi::Number::New(env, error.feature_id));
        obj.Set(geojson_key, Napi::String::New(env, error.geojson));
        array.Set(idx++, obj);
    }
    return array;
}

struct AsyncGeometrySimple : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncGeometrySimple(mapnik::vector_tile_impl::merc_tile_ptr const& tile, Napi::Function const& callback)
        : Base(callback),
          tile_(tile) {}

    void Execute() override
    {
        try
        {
            vector_tile_not_simple(tile_, result_);
        }
        catch (std::exception const& ex)
        {
            // LCOV_EXCL_START
            SetError(ex.what());
            // LCOV_EXCL_STOP
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        Napi::Array array = make_not_simple_array(env, result_);
        return {env.Undefined(), array};
    }

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    std::vector<not_simple_feature> result_;
};

struct AsyncGeometryValid : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncGeometryValid(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                       bool split_multi_features, bool lat_lon, bool web_merc,
                       Napi::Function const& callback)
        : Base(callback),
          tile_(tile),
          split_multi_features_(split_multi_features),
          lat_lon_(lat_lon),
          web_merc_(web_merc)
    {
    }

    void Execute() override
    {
        try
        {
            vector_tile_not_valid(tile_, result_, split_multi_features_, lat_lon_, web_merc_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        Napi::Array array = make_not_valid_array(env, result_);
        return {env.Undefined(), array};
    }

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    bool split_multi_features_;
    bool lat_lon_;
    bool web_merc_;
    std::vector<not_valid_feature> result_;
};

} // namespace

/**
 * Count the number of geometries that are not [OGC simple]{@link http://www.iso.org/iso/catalogue_detail.htm?csnumber=40114}
 *
 * @memberof VectorTile
 * @instance
 * @name reportGeometrySimplicitySync
 * @returns {number} number of features that are not simple
 * @example
 * var simple = vectorTile.reportGeometrySimplicitySync();
 * console.log(simple); // array of non-simple geometries and their layer info
 * console.log(simple.length); // number
 */
Napi::Value VectorTile::reportGeometrySimplicitySync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    try
    {
        std::vector<not_simple_feature> errors;
        vector_tile_not_simple(tile_, errors);
        return scope.Escape(make_not_simple_array(env, errors));
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

        // LCOV_EXCL_STOP
    }
    // LCOV_EXCL_START
    return env.Undefined();
    // LCOV_EXCL_STOP
}

/**
 * Count the number of geometries that are not [OGC valid]{@link http://postgis.net/docs/using_postgis_dbmanagement.html#OGC_Validity}
 *
 * @memberof VectorTile
 * @instance
 * @name reportGeometryValiditySync
 * @param {object} [options]
 * @param {boolean} [options.split_multi_features=false] - If true does validity checks on multi geometries part by part
 * Normally the validity of multipolygons and multilinestrings is done together against
 * all the parts of the geometries. Changing this to true checks the validity of multipolygons
 * and multilinestrings for each part they contain, rather then as a group.
 * @param {boolean} [options.lat_lon=false] - If true results in EPSG:4326
 * @param {boolean} [options.web_merc=false] - If true results in EPSG:3857
 * @returns {number} number of features that are not valid
 * @example
 * var valid = vectorTile.reportGeometryValiditySync();
 * console.log(valid); // array of invalid geometries and their layer info
 * console.log(valid.length); // number
 */
Napi::Value VectorTile::reportGeometryValiditySync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    bool split_multi_features = false;
    bool lat_lon = false;
    bool web_merc = false;
    if (info.Length() >= 1)
    {
        if (!info[0].IsObject())
        {
            Napi::Error::New(env, "The first argument must be an object").ThrowAsJavaScriptException();

            return env.Undefined();
        }
        Napi::Object options = info[0].As<Napi::Object>();

        if (options.Has("split_multi_features"))
        {
            Napi::Value param_val = options.Get("split_multi_features");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'split_multi_features' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            split_multi_features = param_val.As<Napi::Boolean>();
        }

        if (options.Has("lat_lon"))
        {
            Napi::Value param_val = options.Get("lat_lon");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'lat_lon' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            lat_lon = param_val.As<Napi::Boolean>();
        }

        if (options.Has("web_merc"))
        {
            Napi::Value param_val = options.Get("web_merc");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'web_merc' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            web_merc = param_val.As<Napi::Boolean>();
        }
    }

    try
    {
        std::vector<not_valid_feature> errors;
        vector_tile_not_valid(tile_, errors, split_multi_features, lat_lon, web_merc);
        return scope.Escape(make_not_valid_array(env, errors));
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

/**
 * Count the number of geometries that are not [OGC simple]{@link http://www.iso.org/iso/catalogue_detail.htm?csnumber=40114}
 *
 * @memberof VectorTile
 * @instance
 * @name reportGeometrySimplicity
 * @param {Function} callback
 * @example
 * vectorTile.reportGeometrySimplicity(function(err, simple) {
 *   if (err) throw err;
 *   console.log(simple); // array of non-simple geometries and their layer info
 *   console.log(simple.length); // number
 * });
 */
Napi::Value VectorTile::reportGeometrySimplicity(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return reportGeometrySimplicitySync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!callback.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto* worker = new AsyncGeometrySimple(tile_, callback.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

/**
 * Count the number of geometries that are not [OGC valid]{@link http://postgis.net/docs/using_postgis_dbmanagement.html#OGC_Validity}
 *
 * @memberof VectorTile
 * @instance
 * @name reportGeometryValidity
 * @param {object} [options]
 * @param {boolean} [options.split_multi_features=false] - If true does validity checks on multi geometries part by part
 * Normally the validity of multipolygons and multilinestrings is done together against
 * all the parts of the geometries. Changing this to true checks the validity of multipolygons
 * and multilinestrings for each part they contain, rather then as a group.
 * @param {boolean} [options.lat_lon=false] - If true results in EPSG:4326
 * @param {boolean} [options.web_merc=false] - If true results in EPSG:3857
 * @param {Function} callback
 * @example
 * vectorTile.reportGeometryValidity(function(err, valid) {
 *   console.log(valid); // array of invalid geometries and their layer info
 *   console.log(valid.length); // number
 * });
 */

Napi::Value VectorTile::reportGeometryValidity(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0 || (info.Length() == 1 && !info[0].IsFunction()))
    {
        return reportGeometryValiditySync(info);
    }
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    bool split_multi_features = false;
    bool lat_lon = false;
    bool web_merc = false;
    if (info.Length() >= 2)
    {
        if (!info[0].IsObject())
        {
            Napi::Error::New(env, "The first argument must be an object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[0].As<Napi::Object>();

        if (options.Has("split_multi_features"))
        {
            Napi::Value param_val = options.Get("split_multi_features");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'split_multi_features' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            split_multi_features = param_val.As<Napi::Boolean>();
        }

        if (options.Has("lat_lon"))
        {
            Napi::Value param_val = options.Get("lat_lon");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'lat_lon' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            lat_lon = param_val.As<Napi::Boolean>();
        }

        if (options.Has("web_merc"))
        {
            Napi::Value param_val = options.Get("web_merc");
            if (!param_val.IsBoolean())
            {
                Napi::Error::New(env, "option 'web_merc' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            web_merc = param_val.As<Napi::Boolean>();
        }
    }
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!callback.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto* worker = new AsyncGeometryValid(tile_, split_multi_features, lat_lon, web_merc, callback.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

#endif // BOOST_VERSION >= 1.58
