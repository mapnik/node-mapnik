#include "mapnik_vector_tile.hpp"
#include "mapnik_feature.hpp"
// protozero
#include <protozero/pbf_reader.hpp>
// mapnik
#include <mapnik/geom_util.hpp>
#include <mapnik/hit_test_filter.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>
// mapnik-vector-tile
#include "mapnik_vector_tile.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_datasource_pbf.hpp"

namespace detail {

struct p2p_result
{
    double distance = -1;
    double x_hit = 0;
    double y_hit = 0;
};

struct p2p_distance
{
    p2p_distance(double x, double y)
        : x_(x),
          y_(y) {}

    p2p_result operator()(mapnik::geometry::geometry_empty const&) const
    {
        p2p_result p2p;
        return p2p;
    }

    p2p_result operator()(mapnik::geometry::point<double> const& geom) const
    {
        p2p_result p2p;
        p2p.x_hit = geom.x;
        p2p.y_hit = geom.y;
        p2p.distance = mapnik::distance(geom.x, geom.y, x_, y_);
        return p2p;
    }
    p2p_result operator()(mapnik::geometry::multi_point<double> const& geom) const
    {
        p2p_result p2p;
        for (auto const& pt : geom)
        {
            p2p_result p2p_sub = operator()(pt);
            if (p2p_sub.distance >= 0 && (p2p.distance < 0 || p2p_sub.distance < p2p.distance))
            {
                p2p.x_hit = p2p_sub.x_hit;
                p2p.y_hit = p2p_sub.y_hit;
                p2p.distance = p2p_sub.distance;
            }
        }
        return p2p;
    }
    p2p_result operator()(mapnik::geometry::line_string<double> const& geom) const
    {
        p2p_result p2p;
        auto num_points = geom.size();
        if (num_points > 1)
        {
            for (std::size_t i = 1; i < num_points; ++i)
            {
                auto const& pt0 = geom[i - 1];
                auto const& pt1 = geom[i];
                double dist = mapnik::point_to_segment_distance(x_, y_, pt0.x, pt0.y, pt1.x, pt1.y);
                if (dist >= 0 && (p2p.distance < 0 || dist < p2p.distance))
                {
                    p2p.x_hit = pt0.x;
                    p2p.y_hit = pt0.y;
                    p2p.distance = dist;
                }
            }
        }
        return p2p;
    }
    p2p_result operator()(mapnik::geometry::multi_line_string<double> const& geom) const
    {
        p2p_result p2p;
        for (auto const& line : geom)
        {
            p2p_result p2p_sub = operator()(line);
            if (p2p_sub.distance >= 0 && (p2p.distance < 0 || p2p_sub.distance < p2p.distance))
            {
                p2p.x_hit = p2p_sub.x_hit;
                p2p.y_hit = p2p_sub.y_hit;
                p2p.distance = p2p_sub.distance;
            }
        }
        return p2p;
    }
    p2p_result operator()(mapnik::geometry::polygon<double> const& poly) const
    {
        p2p_result p2p;
        std::size_t num_rings = poly.size();
        bool inside = false;
        for (std::size_t ring_index = 0; ring_index < num_rings; ++ring_index)
        {
            auto const& ring = poly[ring_index];
            auto num_points = ring.size();
            if (num_points < 4)
            {
                if (ring_index == 0) // exterior
                    return p2p;
                else // interior
                    continue;
            }
            for (std::size_t index = 1; index < num_points; ++index)
            {
                auto const& pt0 = ring[index - 1];
                auto const& pt1 = ring[index];
                // todo - account for tolerance
                if (mapnik::detail::pip(pt0.x, pt0.y, pt1.x, pt1.y, x_, y_))
                {
                    inside = !inside;
                }
            }
            if (ring_index == 0 && !inside) return p2p;
        }
        if (inside) p2p.distance = 0;
        return p2p;
    }

    p2p_result operator()(mapnik::geometry::multi_polygon<double> const& geom) const
    {
        p2p_result p2p;
        for (auto const& poly : geom)
        {
            p2p_result p2p_sub = operator()(poly);
            if (p2p_sub.distance >= 0 && (p2p.distance < 0 || p2p_sub.distance < p2p.distance))
            {
                p2p.x_hit = p2p_sub.x_hit;
                p2p.y_hit = p2p_sub.y_hit;
                p2p.distance = p2p_sub.distance;
            }
        }
        return p2p;
    }
    p2p_result operator()(mapnik::geometry::geometry_collection<double> const& collection) const
    {
        // There is no current way that a geometry collection could be returned from a vector tile.
        // LCOV_EXCL_START
        p2p_result p2p;
        for (auto const& geom : collection)
        {
            p2p_result p2p_sub = mapnik::util::apply_visitor((*this), geom);
            if (p2p_sub.distance >= 0 && (p2p.distance < 0 || p2p_sub.distance < p2p.distance))
            {
                p2p.x_hit = p2p_sub.x_hit;
                p2p.y_hit = p2p_sub.y_hit;
                p2p.distance = p2p_sub.distance;
            }
        }
        return p2p;
        // LCOV_EXCL_STOP
    }

    double x_;
    double y_;
};

detail::p2p_result path_to_point_distance(mapnik::geometry::geometry<double> const& geom, double x, double y)
{
    return mapnik::util::apply_visitor(detail::p2p_distance(x, y), geom);
}

std::vector<query_result> _query(mapnik::vector_tile_impl::merc_tile_ptr const& tile, double lon, double lat, double tolerance, std::string const& layer_name)
{
    std::vector<query_result> arr;
    if (tile->is_empty())
    {
        return arr;
    }

    mapnik::projection wgs84("epsg:4326", true);
    mapnik::projection merc("epsg:3857", true);
    mapnik::proj_transform tr(wgs84, merc);
    double x = lon;
    double y = lat;
    double z = 0;
    if (!tr.forward(x, y, z))
    {
        // THIS CAN NEVER BE REACHED CURRENTLY
        // internally lonlat2merc in mapnik can never return false.
        // LCOV_EXCL_START
        throw std::runtime_error("could not reproject lon/lat to mercator");
        // LCOV_EXCL_STOP
    }

    mapnik::coord2d pt(x, y);
    if (!layer_name.empty())
    {
        protozero::pbf_reader layer_msg;
        if (tile->layer_reader(layer_name, layer_msg))
        {
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                layer_msg,
                tile->x(),
                tile->y(),
                tile->z());
            mapnik::featureset_ptr fs = ds->features_at_point(pt, tolerance);
            if (fs && mapnik::is_valid(fs))
            {
                mapnik::feature_ptr feature;
                while ((feature = fs->next()))
                {
                    auto const& geom = feature->get_geometry();
                    auto p2p = path_to_point_distance(geom, x, y);
                    if (!tr.backward(p2p.x_hit, p2p.y_hit, z))
                    {
                        // LCOV_EXCL_START
                        throw std::runtime_error("could not reproject lon/lat to mercator");
                        // LCOV_EXCL_STOP
                    }
                    if (p2p.distance >= 0 && p2p.distance <= tolerance)
                    {
                        query_result res;
                        res.x_hit = p2p.x_hit;
                        res.y_hit = p2p.y_hit;
                        res.distance = p2p.distance;
                        res.layer = layer_name;
                        res.feature = feature;
                        arr.push_back(std::move(res));
                    }
                }
            }
        }
    }
    else
    {
        protozero::pbf_reader item(tile->get_reader());
        while (item.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
        {
            protozero::pbf_reader layer_msg = item.get_message();
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                layer_msg,
                tile->x(),
                tile->y(),
                tile->z());
            mapnik::featureset_ptr fs = ds->features_at_point(pt, tolerance);
            if (fs && mapnik::is_valid(fs))
            {
                mapnik::feature_ptr feature;
                while ((feature = fs->next()))
                {
                    auto const& geom = feature->get_geometry();
                    auto p2p = path_to_point_distance(geom, x, y);
                    if (!tr.backward(p2p.x_hit, p2p.y_hit, z))
                    {
                        // LCOV_EXCL_START
                        throw std::runtime_error("could not reproject lon/lat to mercator");
                        // LCOV_EXCL_STOP
                    }
                    if (p2p.distance >= 0 && p2p.distance <= tolerance)
                    {
                        query_result res;
                        res.x_hit = p2p.x_hit;
                        res.y_hit = p2p.y_hit;
                        res.distance = p2p.distance;
                        res.layer = ds->get_name();
                        res.feature = feature;
                        arr.push_back(std::move(res));
                    }
                }
            }
        }
    }
    std::sort(arr.begin(), arr.end(), [](query_result const& a, query_result const& b) {
        return a.distance < b.distance;
    });
    return arr;
}

Napi::Array _queryResultToV8(Napi::Env env, std::vector<query_result>& result)
{
    Napi::Array arr = Napi::Array::New(env, result.size());
    std::size_t i = 0;
    for (auto& item : result)
    {
        Napi::Value arg = Napi::External<mapnik::feature_ptr>::New(env, &item.feature);
        Napi::Object feat_obj = Feature::constructor.New({arg});
        feat_obj.Set("layer", item.layer);
        feat_obj.Set("distance", Napi::Number::New(env, item.distance));
        feat_obj.Set("x_hit", Napi::Number::New(env, item.x_hit));
        feat_obj.Set("y_hit", Napi::Number::New(env, item.y_hit));
        arr.Set(i++, feat_obj);
    }
    return arr;
}

struct AsyncQuery : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncQuery(mapnik::vector_tile_impl::merc_tile_ptr const& tile, double lon, double lat, double tolerance,
               std::string layer_name, Napi::Function const& callback)
        : Base(callback),
          tile_(tile),
          lon_(lon),
          lat_(lat),
          tolerance_(tolerance),
          layer_name_(layer_name)
    {
    }

    void Execute() override
    {
        try
        {
            result_ = _query(tile_, lon_, lat_, tolerance_, layer_name_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        Napi::Array arr = _queryResultToV8(env, result_);
        return {env.Undefined(), arr};
    }

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    double lon_;
    double lat_;
    double tolerance_;
    std::string layer_name_;
    std::vector<query_result> result_;
};

// Query many

Napi::Object _queryManyResultToV8(Napi::Env env, queryMany_result& result)
{
    Napi::Object results = Napi::Object::New(env);
    Napi::Array features = Napi::Array::New(env, result.features.size());
    Napi::Array hits = Napi::Array::New(env, result.hits.size());
    results.Set("hits", hits);
    results.Set("features", features);

    // result.features => features
    for (auto& item : result.features)
    {
        Napi::Value arg = Napi::External<mapnik::feature_ptr>::New(env, &item.second.feature);
        Napi::Object feat_obj = Feature::constructor.New({arg});
        feat_obj.Set("layer", item.second.layer);
        features.Set(item.first, feat_obj);
    }

    // result.hits => hits
    for (auto const& hit : result.hits)
    {
        Napi::Array point_hits = Napi::Array::New(env, hit.second.size());
        std::size_t i = 0;
        for (auto const& h : hit.second)
        {
            Napi::Object hit_obj = Napi::Object::New(env);
            hit_obj.Set("distance", Napi::Number::New(env, h.distance));
            hit_obj.Set("feature_id", Napi::Number::New(env, h.feature_id));
            point_hits.Set(i++, hit_obj);
        }
        hits.Set(hit.first, point_hits);
    }
    return results;
}

void _queryMany(queryMany_result& result,
                mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                std::vector<query_lonlat> const& query,
                double tolerance,
                std::string const& layer_name,
                std::vector<std::string> const& fields)
{
    protozero::pbf_reader layer_msg;
    if (!tile->layer_reader(layer_name, layer_msg))
    {
        throw std::runtime_error("Could not find layer in vector tile");
    }

    std::map<unsigned, query_result> features;
    std::map<unsigned, std::vector<query_hit>> hits;

    // Reproject query => mercator points
    mapnik::box2d<double> bbox;
    mapnik::projection wgs84("epsg:4326", true);
    mapnik::projection merc("epsg:3857", true);
    mapnik::proj_transform tr(wgs84, merc);
    std::vector<mapnik::coord2d> points;
    points.reserve(query.size());
    for (std::size_t p = 0; p < query.size(); ++p)
    {
        double x = query[p].lon;
        double y = query[p].lat;
        double z = 0;
        if (!tr.forward(x, y, z))
        {
            // LCOV_EXCL_START
            throw std::runtime_error("could not reproject lon/lat to mercator");
            // LCOV_EXCL_STOP
        }
        mapnik::coord2d pt(x, y);
        bbox.expand_to_include(pt);
        points.emplace_back(std::move(pt));
    }
    bbox.pad(tolerance);

    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
        mapnik::vector_tile_impl::tile_datasource_pbf>(
        layer_msg,
        tile->x(),
        tile->y(),
        tile->z());
    mapnik::query q(bbox);
    if (fields.empty())
    {
        // request all data attributes
        auto fields2 = ds->get_descriptor().get_descriptors();
        for (auto const& field : fields2)
        {
            q.add_property_name(field.get_name());
        }
    }
    else
    {
        for (std::string const& name : fields)
        {
            q.add_property_name(name);
        }
    }
    mapnik::featureset_ptr fs = ds->features(q);

    if (fs && mapnik::is_valid(fs))
    {
        mapnik::feature_ptr feature;
        unsigned idx = 0;
        while ((feature = fs->next()))
        {
            unsigned has_hit = 0;
            for (std::size_t p = 0; p < points.size(); ++p)
            {
                mapnik::coord2d const& pt = points[p];
                auto const& geom = feature->get_geometry();
                auto p2p = path_to_point_distance(geom, pt.x, pt.y);
                if (p2p.distance >= 0 && p2p.distance <= tolerance)
                {
                    has_hit = 1;
                    query_result res;
                    res.feature = feature;
                    res.distance = 0;
                    res.layer = ds->get_name();

                    query_hit hit;
                    hit.distance = p2p.distance;
                    hit.feature_id = idx;

                    features.insert(std::make_pair(idx, res));

                    std::map<unsigned, std::vector<query_hit>>::iterator hits_it;
                    hits_it = hits.find(p);
                    if (hits_it == hits.end())
                    {
                        std::vector<query_hit> pointHits;
                        pointHits.reserve(1);
                        pointHits.push_back(std::move(hit));
                        hits.insert(std::make_pair(p, pointHits));
                    }
                    else
                    {
                        hits_it->second.push_back(std::move(hit));
                    }
                }
            }
            if (has_hit > 0)
            {
                idx++;
            }
        }
    }

    // Sort each group of hits by distance.
    for (auto& hit : hits)
    {
        std::sort(hit.second.begin(), hit.second.end(), [](auto const& a, auto const& b) {
            return a.distance < b.distance;
        });
    }

    result.hits = std::move(hits);
    result.features = std::move(features);
}

struct AsyncQueryMany : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncQueryMany(mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                   std::vector<query_lonlat> const& query, double tolerance,
                   std::string layer_name, std::vector<std::string> const& fields, Napi::Function const& callback)
        : Base(callback),
          tile_(tile),
          query_(query),
          tolerance_(tolerance),
          layer_name_(layer_name),
          fields_(fields)
    {
    }

    void Execute() override
    {
        try
        {
            _queryMany(result_, tile_, query_, tolerance_, layer_name_, fields_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        Napi::Object obj = _queryManyResultToV8(env, result_);
        return {env.Undefined(), obj};
    }

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    std::vector<query_lonlat> query_;
    double tolerance_;
    std::string layer_name_;
    std::vector<std::string> fields_;
    queryMany_result result_;
};

} // namespace detail

/**
 * Query a vector tile by longitude and latitude and get an array of
 * features in the vector tile that exist in relation to those coordinates.
 *
 * A note on `tolerance`: If you provide a positive value for tolerance you
 * are saying that you'd like features returned in the query results that might
 * not exactly intersect with a given lon/lat. The higher the tolerance the
 * slower the query will run because it will do more work by comparing your query
 * lon/lat against more potential features. However, this is an important parameter
 * because vector tile storage, by design, results in reduced precision of coordinates.
 * The amount of precision loss depends on the zoom level of a given vector tile
 * and how aggressively it was simplified during encoding. So if you want at
 * least one match - say the closest single feature to your query lon/lat - is is
 * not possible to know the smallest tolerance that will work without experimentation.
 * In general be prepared to provide a high tolerance (1-100) for low zoom levels
 * while you should be okay with a low tolerance (1-10) at higher zoom levels and
 * with vector tiles that are storing less simplified geometries. The units tolerance
 * should be expressed in depend on the coordinate system of the underlying data.
 * In the case of vector tiles this is spherical mercator so the units are meters.
 * For points any features will be returned that contain a point which is, by distance
 * in meters, not greater than the tolerance value. For lines any features will be
 * returned that have a segment which is, by distance in meters, not greater than
 * the tolerance value. For polygons tolerance is not supported which means that
 * your lon/lat must fall inside a feature's polygon otherwise that feature will
 * not be matched.
 *
 * @memberof VectorTile
 * @instance
 * @name query
 * @param {number} longitude - longitude
 * @param {number} latitude - latitude
 * @param {Object} [options]
 * @param {number} [options.tolerance=0] include features a specific distance from the
 * lon/lat query in the response
 * @param {string} [options.layer] layer - Pass a layer name to restrict
 * the query results to a single layer in the vector tile. Get all possible
 * layer names in the vector tile with {@link VectorTile#names}
 * @param {Function} callback(err, features)
 * @returns {Array<mapnik.Feature>} an array of {@link mapnik.Feature} objects
 * @example
 * vt.query(139.61, 37.17, {tolerance: 0}, function(err, features) {
 *   if (err) throw err;
 *   console.log(features); // array of objects
 *   console.log(features.length) // 1
 *   console.log(features[0].id()) // 89
 *   console.log(features[0].geometry().type()); // 'Polygon'
 *   console.log(features[0].distance); // 0
 *   console.log(features[0].layer); // 'layer name'
 * });
 */
Napi::Value VectorTile::query(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::Error::New(env, "expects lon,lat info").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    double tolerance = 0.0; // meters
    std::string layer_name("");
    if (info.Length() > 2)
    {
        Napi::Object options = Napi::Object::New(env);
        if (!info[2].IsObject())
        {
            Napi::TypeError::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        options = info[2].As<Napi::Object>();
        if (options.Has("tolerance"))
        {
            Napi::Value tol = options.Get("tolerance");
            if (!tol.IsNumber())
            {
                Napi::TypeError::New(env, "tolerance value must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            tolerance = tol.As<Napi::Number>().DoubleValue();
        }
        if (options.Has("layer"))
        {
            Napi::Value layer_id = options.Get("layer");
            if (!layer_id.IsString())
            {
                Napi::TypeError::New(env, "layer value must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            layer_name = layer_id.As<Napi::String>();
        }
    }

    double lon = info[0].As<Napi::Number>().DoubleValue();
    double lat = info[1].As<Napi::Number>().DoubleValue();

    // If last argument is not a function go with sync call.
    if (!info[info.Length() - 1].IsFunction())
    {
        try
        {
            std::vector<query_result> result = detail::_query(tile_, lon, lat, tolerance, layer_name);
            Napi::Array arr = detail::_queryResultToV8(env, result);
            return arr; // Escape ? FIXME
        }
        catch (std::exception const& ex)
        {
            Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    else
    {
        Napi::Value callback = info[info.Length() - 1];
        auto* worker = new detail::AsyncQuery(tile_, lon, lat, tolerance, layer_name, callback.As<Napi::Function>());
        worker->Queue();
    }
    return env.Undefined();
}

/**
 * Query a vector tile by multiple sets of latitude/longitude pairs.
 * Just like <mapnik.VectorTile.query> but with more points to search.
 *
 * @memberof VectorTile
 * @instance
 * @name queryMany
 * @param {array<number>} array - `longitude` and `latitude` array pairs [[lon1,lat1], [lon2,lat2]]
 * @param {Object} options
 * @param {number} [options.tolerance=0] include features a specific distance from the
 * lon/lat query in the response. Read more about tolerance at {@link VectorTile#query}.
 * @param {string} options.layer - layer name
 * @param {Array<string>} [options.fields] - array of field names
 * @param {Function} [callback] - `function(err, results)`
 * @returns {Object} The response has contains two main objects: `hits` and `features`.
 * The number of hits returned will correspond to the number of lon/lats queried and will
 * be returned in the order of the query. Each hit returns 1) a `distance` and a 2) `feature_id`.
 * The `distance` is number of meters the queried lon/lat is from the object in the vector tile.
 * The `feature_id` is the corresponding object in features object.
 *
 * The values for the query is contained in the features object. Use attributes() to extract a value.
 * @example
 * vt.queryMany([[139.61, 37.17], [140.64, 38.1]], {tolerance: 0}, function(err, results) {
 *   if (err) throw err;
 *   console.log(results.hits); //
 *   console.log(results.features); // array of feature objects
 *   if (features.length) {
 *     console.log(results.features[0].layer); // 'layer-name'
 *     console.log(results.features[0].distance, features[0].x_hit, features[0].y_hit); // 0, 0, 0
 *   }
 * });
 */
Napi::Value VectorTile::queryMany(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if (info.Length() < 2 || !info[0].IsArray())
    {
        Napi::Error::New(env, "expects lon,lat info + object with layer property referring to a layer name")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    double tolerance = 0.0; // meters
    std::string layer_name("");
    std::vector<std::string> fields;
    std::vector<query_lonlat> query;

    // Convert v8 queryArray to a std vector
    Napi::Array queryArray = info[0].As<Napi::Array>();
    std::size_t length = queryArray.Length();
    query.reserve(length);
    for (std::size_t p = 0; p < length; ++p)
    {
        Napi::Value item = queryArray.Get(p);
        if (!item.IsArray())
        {
            Napi::Error::New(env, "non-array item encountered").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Array pair = item.As<Napi::Array>();
        Napi::Value lon = pair.Get(0u);
        Napi::Value lat = pair.Get(1u);
        if (!lon.IsNumber() || !lat.IsNumber())
        {
            Napi::Error::New(env, "lng lat must be numbers").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        query_lonlat lonlat;
        lonlat.lon = lon.As<Napi::Number>().DoubleValue();
        lonlat.lat = lat.As<Napi::Number>().DoubleValue();
        query.push_back(std::move(lonlat));
    }

    // Convert v8 options object to std params
    if (info.Length() > 1)
    {
        Napi::Object options = Napi::Object::New(env);
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        options = info[1].As<Napi::Object>();
        if (options.Has("tolerance"))
        {
            Napi::Value tol = options.Get("tolerance");
            if (!tol.IsNumber())
            {
                Napi::TypeError::New(env, "tolerance value must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            tolerance = tol.As<Napi::Number>().DoubleValue();
        }
        if (options.Has("layer"))
        {
            Napi::Value layer_id = options.Get("layer");
            if (!layer_id.IsString())
            {
                Napi::TypeError::New(env, "layer value must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            layer_name = layer_id.As<Napi::String>();
        }
        if (options.Has("fields"))
        {
            Napi::Value param_val = options.Get("fields");
            if (!param_val.IsArray())
            {
                Napi::TypeError::New(env, "option 'fields' must be an array of strings").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Array a = param_val.As<Napi::Array>();
            std::size_t i = 0;
            std::size_t num_fields = a.Length();
            fields.reserve(num_fields);
            while (i < num_fields)
            {
                Napi::Value name = a.Get(i);
                if (name.IsString())
                {
                    fields.emplace_back(name.As<Napi::String>().Utf8Value());
                }
                ++i;
            }
        }
    }

    if (layer_name.empty())
    {
        Napi::TypeError::New(env, "options.layer is required").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // If last argument is not a function go with sync call.
    if (!info[info.Length() - 1].IsFunction())
    {
        try
        {
            queryMany_result result;
            detail::_queryMany(result, tile_, query, tolerance, layer_name, fields);
            Napi::Object result_obj = detail::_queryManyResultToV8(env, result);
            return scope.Escape(result_obj);
        }
        catch (std::exception const& ex)
        {
            Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    else
    {
        Napi::Value callback = info[info.Length() - 1];
        auto* worker = new detail::AsyncQueryMany(tile_, query, tolerance, layer_name,
                                                  fields, callback.As<Napi::Function>());
        worker->Queue();
        return env.Undefined();
    }
}
