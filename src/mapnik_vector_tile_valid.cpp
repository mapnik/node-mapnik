#include "utils.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_vector_tile.hpp"

#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_geometry_decoder.hpp"

// mapnik
#include <mapnik/datasource_cache.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/geometry_is_valid.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/feature_to_geojson.hpp>

// std
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

#if BOOST_VERSION >= 105800

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

struct visitor_geom_valid
{
    std::vector<not_valid_feature> & errors;
    mapnik::feature_ptr & feature;
    std::string const& layer_name;
    bool split_multi_features;

    visitor_geom_valid(std::vector<not_valid_feature> & errors_,
                       mapnik::feature_ptr & feature_,
                       std::string const& layer_name_,
                       bool split_multi_features_)
        : errors(errors_),
          feature(feature_),
          layer_name(layer_name_),
          split_multi_features(split_multi_features_) {}

    void operator() (mapnik::geometry::geometry_empty const&) {}

    template <typename T>
    void operator() (mapnik::geometry::point<T> const& geom)
    {
        std::string message;
        if (!mapnik::geometry::is_valid(geom, message))
        {
            if (!mapnik::geometry::is_valid(geom, message))
            {
                mapnik::feature_impl feature_new(feature->context(),feature->id());
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
    void operator() (mapnik::geometry::multi_point<T> const& geom)
    {
        std::string message;
        if (!mapnik::geometry::is_valid(geom, message))
        {
            if (!mapnik::geometry::is_valid(geom, message))
            {
                mapnik::feature_impl feature_new(feature->context(),feature->id());
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
    void operator() (mapnik::geometry::line_string<T> const& geom)
    {
        std::string message;
        if (!mapnik::geometry::is_valid(geom, message))
        {
            if (!mapnik::geometry::is_valid(geom, message))
            {
                mapnik::feature_impl feature_new(feature->context(),feature->id());
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
    void operator() (mapnik::geometry::multi_line_string<T> const& geom)
    {
        if (split_multi_features)
        {
            for (auto const& ls : geom)
            {
                std::string message;
                if (!mapnik::geometry::is_valid(ls, message))
                {
                    mapnik::feature_impl feature_new(feature->context(),feature->id());
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
            if (!mapnik::geometry::is_valid(geom, message))
            {
                mapnik::feature_impl feature_new(feature->context(),feature->id());
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
    void operator() (mapnik::geometry::polygon<T> const& geom)
    {
        std::string message;
        if (!mapnik::geometry::is_valid(geom, message))
        {
            if (!mapnik::geometry::is_valid(geom, message))
            {
                mapnik::feature_impl feature_new(feature->context(),feature->id());
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
    void operator() (mapnik::geometry::multi_polygon<T> const& geom)
    {
        if (split_multi_features)
        {
            for (auto const& poly : geom)
            {
                std::string message;
                if (!mapnik::geometry::is_valid(poly, message))
                {
                    mapnik::feature_impl feature_new(feature->context(),feature->id());
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
                mapnik::feature_impl feature_new(feature->context(),feature->id());
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
    void operator() (mapnik::geometry::geometry_collection<T> const& geom)
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

void layer_not_valid(protozero::pbf_reader & layer_msg,
               unsigned x,
               unsigned y,
               unsigned z,
               std::vector<not_valid_feature> & errors,
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
                    mapnik::projection wgs84("+init=epsg:4326",true);
                    mapnik::projection merc("+init=epsg:3857",true);
                    mapnik::proj_transform prj_trans(merc,wgs84);
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
            std::uint64_t feature_id = 0;
            while (feature_msg.next())
            {
                switch (feature_msg.tag())
                {
                    case mapnik::vector_tile_impl::Feature_Encoding::ID:
                        feature_id = feature_msg.get_uint64();
                        break;
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
                mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
                mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
                feature->set_geometry(mapnik::vector_tile_impl::decode_geometry<double>(geoms, geom_type_enum, version, 0.0, 0.0, 1.0, 1.0));
                mapnik::util::apply_visitor(
                        visitor_geom_valid(errors, feature, layer_name, split_multi_features),
                        feature->get_geometry());
            }
        }
    }
}

void vector_tile_not_valid(VectorTile * v,
                           std::vector<not_valid_feature> & errors,
                           bool split_multi_features = false,
                           bool lat_lon = false,
                           bool web_merc = false)
{
    protozero::pbf_reader tile_msg(v->get_tile()->get_reader());
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        protozero::pbf_reader layer_msg(tile_msg.get_message());
        layer_not_valid(layer_msg,
                        v->get_tile()->x(),
                        v->get_tile()->y(),
                        v->get_tile()->z(),
                        errors,
                        split_multi_features,
                        lat_lon,
                        web_merc);
    }
}

v8::Local<v8::Array> make_not_valid_array(std::vector<not_valid_feature> & errors)
{
    Nan::EscapableHandleScope scope;
    v8::Local<v8::Array> array = Nan::New<v8::Array>(errors.size());
    v8::Local<v8::String> layer_key = Nan::New<v8::String>("layer").ToLocalChecked();
    v8::Local<v8::String> feature_id_key = Nan::New<v8::String>("featureId").ToLocalChecked();
    v8::Local<v8::String> message_key = Nan::New<v8::String>("message").ToLocalChecked();
    v8::Local<v8::String> geojson_key = Nan::New<v8::String>("geojson").ToLocalChecked();
    std::uint32_t idx = 0;
    for (auto const& error : errors)
    {
        v8::Local<v8::Object> obj = Nan::New<v8::Object>();
        obj->Set(layer_key, Nan::New<v8::String>(error.layer).ToLocalChecked());
        obj->Set(message_key, Nan::New<v8::String>(error.message).ToLocalChecked());
        obj->Set(feature_id_key, Nan::New<v8::Number>(error.feature_id));
        obj->Set(geojson_key, Nan::New<v8::String>(error.geojson).ToLocalChecked());
        array->Set(idx++, obj);
    }
    return scope.Escape(array);
}

struct not_valid_baton
{
    uv_work_t request;
    VectorTile* v;
    bool error;
    bool split_multi_features;
    bool lat_lon;
    bool web_merc;
    std::vector<not_valid_feature> result;
    std::string err_msg;
    Nan::Persistent<v8::Function> cb;
};

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
NAN_METHOD(VectorTile::reportGeometryValiditySync)
{
    info.GetReturnValue().Set(_reportGeometryValiditySync(info));
}

v8::Local<v8::Value> VectorTile::_reportGeometryValiditySync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    bool split_multi_features = false;
    bool lat_lon = false;
    bool web_merc = false;
    if (info.Length() >= 1)
    {
        if (!info[0]->IsObject())
        {
            Nan::ThrowError("The first argument must be an object");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> options = info[0]->ToObject();

        if (options->Has(Nan::New("split_multi_features").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("split_multi_features").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'split_multi_features' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            split_multi_features = param_val->BooleanValue();
        }

        if (options->Has(Nan::New("lat_lon").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("lat_lon").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'lat_lon' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            lat_lon = param_val->BooleanValue();
        }

        if (options->Has(Nan::New("web_merc").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("web_merc").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'web_merc' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            web_merc = param_val->BooleanValue();
        }
    }
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    try
    {
        std::vector<not_valid_feature> errors;
        vector_tile_not_valid(d, errors, split_multi_features, lat_lon, web_merc);
        return scope.Escape(make_not_valid_array(errors));
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        // LCOV_EXCL_STOP
    }
    // LCOV_EXCL_START
    return scope.Escape(Nan::Undefined());
    // LCOV_EXCL_STOP
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
NAN_METHOD(VectorTile::reportGeometryValidity)
{
    if (info.Length() == 0 || (info.Length() == 1 && !info[0]->IsFunction()))
    {
        info.GetReturnValue().Set(_reportGeometryValiditySync(info));
        return;
    }
    bool split_multi_features = false;
    bool lat_lon = false;
    bool web_merc = false;
    if (info.Length() >= 2)
    {
        if (!info[0]->IsObject())
        {
            Nan::ThrowError("The first argument must be an object");
            return;
        }
        v8::Local<v8::Object> options = info[0]->ToObject();

        if (options->Has(Nan::New("split_multi_features").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("split_multi_features").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'split_multi_features' must be a boolean");
                return;
            }
            split_multi_features = param_val->BooleanValue();
        }

        if (options->Has(Nan::New("lat_lon").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("lat_lon").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'lat_lon' must be a boolean");
                return;
            }
            lat_lon = param_val->BooleanValue();
        }

        if (options->Has(Nan::New("web_merc").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("web_merc").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'web_merc' must be a boolean");
                return;
            }
            web_merc = param_val->BooleanValue();
        }
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    not_valid_baton *closure = new not_valid_baton();
    closure->request.data = closure;
    closure->v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    closure->error = false;
    closure->split_multi_features = split_multi_features;
    closure->lat_lon = lat_lon;
    closure->web_merc = web_merc;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_ReportGeometryValidity, (uv_after_work_cb)EIO_AfterReportGeometryValidity);
    closure->v->Ref();
    return;
}

void VectorTile::EIO_ReportGeometryValidity(uv_work_t* req)
{
    not_valid_baton *closure = static_cast<not_valid_baton *>(req->data);
    try
    {
        vector_tile_not_valid(closure->v, closure->result, closure->split_multi_features, closure->lat_lon, closure->web_merc);
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        closure->error = true;
        closure->err_msg = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::EIO_AfterReportGeometryValidity(uv_work_t* req)
{
    Nan::HandleScope scope;
    not_valid_baton *closure = static_cast<not_valid_baton *>(req->data);
    if (closure->error)
    {
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->err_msg.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else
    {
        v8::Local<v8::Array> array = make_not_valid_array(closure->result);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), array };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->v->Unref();
    closure->cb.Reset();
    delete closure;
}

#endif // BOOST_VERSION >= 1.58
