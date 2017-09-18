#include "utils.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_vector_tile.hpp"

#include "vector_tile_tile.hpp"
#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_projection.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_load_tile.hpp"

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/request.hpp>
#include <mapnik/hit_test_filter.hpp>

// std
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

namespace detail
{

struct p2p_result
{
    explicit p2p_result() :
      distance(-1),
      x_hit(0),
      y_hit(0) {}

    double distance;
    double x_hit;
    double y_hit;
};

struct p2p_distance
{
    p2p_distance(double x, double y)
     : x_(x),
       y_(y) {}

    p2p_result operator() (mapnik::geometry::geometry_empty const& ) const
    {
        p2p_result p2p;
        return p2p;
    }

    p2p_result operator() (mapnik::geometry::point<double> const& geom) const
    {
        p2p_result p2p;
        p2p.x_hit = geom.x;
        p2p.y_hit = geom.y;
        p2p.distance = mapnik::distance(geom.x, geom.y, x_, y_);
        return p2p;
    }
    p2p_result operator() (mapnik::geometry::multi_point<double> const& geom) const
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
    p2p_result operator() (mapnik::geometry::line_string<double> const& geom) const
    {
        p2p_result p2p;
        std::size_t num_points = geom.num_points();
        if (num_points > 1)
        {
            for (std::size_t i = 1; i < num_points; ++i)
            {
                auto const& pt0 = geom[i-1];
                auto const& pt1 = geom[i];
                double dist = mapnik::point_to_segment_distance(x_,y_,pt0.x,pt0.y,pt1.x,pt1.y);
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
    p2p_result operator() (mapnik::geometry::multi_line_string<double> const& geom) const
    {
        p2p_result p2p;
        for (auto const& line: geom)
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
    p2p_result operator() (mapnik::geometry::polygon<double> const& geom) const
    {
        auto const& exterior = geom.exterior_ring;
        std::size_t num_points = exterior.num_points();
        p2p_result p2p;
        if (num_points < 4)
        {
            return p2p;
        }
        bool inside = false;
        for (std::size_t i = 1; i < num_points; ++i)
        {
            auto const& pt0 = exterior[i-1];
            auto const& pt1 = exterior[i];
            // todo - account for tolerance
            if (mapnik::detail::pip(pt0.x,pt0.y,pt1.x,pt1.y,x_,y_))
            {
                inside = !inside;
            }
        }
        if (!inside)
        {
            return p2p;
        }
        for (auto const& ring :  geom.interior_rings)
        {
            std::size_t num_interior_points = ring.size();
            if (num_interior_points < 4)
            {
                continue;
            }
            for (std::size_t j = 1; j < num_interior_points; ++j)
            {
                auto const& pt0 = ring[j-1];
                auto const& pt1 = ring[j];
                if (mapnik::detail::pip(pt0.x,pt0.y,pt1.x,pt1.y,x_,y_))
                {
                    inside=!inside;
                }
            }
        }
        if (inside)
        {
            p2p.distance = 0;
        }
        return p2p;
    }
    p2p_result operator() (mapnik::geometry::multi_polygon<double> const& geom) const
    {
        p2p_result p2p;
        for (auto const& poly: geom)
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
    p2p_result operator() (mapnik::geometry::geometry_collection<double> const& collection) const
    {
        // There is no current way that a geometry collection could be returned from a vector tile.
        /* LCOV_EXCL_START */
        p2p_result p2p;
        for (auto const& geom: collection)
        {
            p2p_result p2p_sub = mapnik::util::apply_visitor((*this),geom);
            if (p2p_sub.distance >= 0 && (p2p.distance < 0 || p2p_sub.distance < p2p.distance))
            {
                p2p.x_hit = p2p_sub.x_hit;
                p2p.y_hit = p2p_sub.y_hit;
                p2p.distance = p2p_sub.distance;
            }
        }
        return p2p;
        /* LCOV_EXCL_STOP */
    }

    double x_;
    double y_;
};

}

detail::p2p_result path_to_point_distance(mapnik::geometry::geometry<double> const& geom, double x, double y)
{
    return mapnik::util::apply_visitor(detail::p2p_distance(x,y), geom);
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    double lon;
    double lat;
    double tolerance;
    bool error;
    std::vector<query_result> result;
    std::string layer_name;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_query_baton_t;

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
NAN_METHOD(VectorTile::query)
{
    if (info.Length() < 2 || !info[0]->IsNumber() || !info[1]->IsNumber())
    {
        Nan::ThrowError("expects lon,lat info");
        return;
    }
    double tolerance = 0.0; // meters
    std::string layer_name("");
    if (info.Length() > 2)
    {
        v8::Local<v8::Object> options = Nan::New<v8::Object>();
        if (!info[2]->IsObject())
        {
            Nan::ThrowTypeError("optional third argument must be an options object");
            return;
        }
        options = info[2]->ToObject();
        if (options->Has(Nan::New("tolerance").ToLocalChecked()))
        {
            v8::Local<v8::Value> tol = options->Get(Nan::New("tolerance").ToLocalChecked());
            if (!tol->IsNumber())
            {
                Nan::ThrowTypeError("tolerance value must be a number");
                return;
            }
            tolerance = tol->NumberValue();
        }
        if (options->Has(Nan::New("layer").ToLocalChecked()))
        {
            v8::Local<v8::Value> layer_id = options->Get(Nan::New("layer").ToLocalChecked());
            if (!layer_id->IsString())
            {
                Nan::ThrowTypeError("layer value must be a string");
                return;
            }
            layer_name = TOSTR(layer_id);
        }
    }

    double lon = info[0]->NumberValue();
    double lat = info[1]->NumberValue();
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    // If last argument is not a function go with sync call.
    if (!info[info.Length()-1]->IsFunction())
    {
        try
        {
            std::vector<query_result> result = _query(d, lon, lat, tolerance, layer_name);
            v8::Local<v8::Array> arr = _queryResultToV8(result);
            info.GetReturnValue().Set(arr);
            return;
        }
        catch (std::exception const& ex)
        {
            Nan::ThrowError(ex.what());
            return;
        }
    }
    else
    {
        v8::Local<v8::Value> callback = info[info.Length()-1];
        vector_tile_query_baton_t *closure = new vector_tile_query_baton_t();
        closure->request.data = closure;
        closure->lon = lon;
        closure->lat = lat;
        closure->tolerance = tolerance;
        closure->layer_name = layer_name;
        closure->d = d;
        closure->error = false;
        closure->cb.Reset(callback.As<v8::Function>());
        uv_queue_work(uv_default_loop(), &closure->request, EIO_Query, (uv_after_work_cb)EIO_AfterQuery);
        d->Ref();
        return;
    }
}

void VectorTile::EIO_Query(uv_work_t* req)
{
    vector_tile_query_baton_t *closure = static_cast<vector_tile_query_baton_t *>(req->data);
    try
    {
        closure->result = _query(closure->d, closure->lon, closure->lat, closure->tolerance, closure->layer_name);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterQuery(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_query_baton_t *closure = static_cast<vector_tile_query_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        std::vector<query_result> const& result = closure->result;
        v8::Local<v8::Array> arr = _queryResultToV8(result);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), arr };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

std::vector<query_result> VectorTile::_query(VectorTile* d, double lon, double lat, double tolerance, std::string const& layer_name)
{
    std::vector<query_result> arr;
    if (d->tile_->is_empty())
    {
        return arr;
    }

    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform tr(wgs84,merc);
    double x = lon;
    double y = lat;
    double z = 0;
    if (!tr.forward(x,y,z))
    {
        // THIS CAN NEVER BE REACHED CURRENTLY
        // internally lonlat2merc in mapnik can never return false.
        /* LCOV_EXCL_START */
        throw std::runtime_error("could not reproject lon/lat to mercator");
        /* LCOV_EXCL_STOP */
    }

    mapnik::coord2d pt(x,y);
    if (!layer_name.empty())
    {
        protozero::pbf_reader layer_msg;
        if (d->tile_->layer_reader(layer_name, layer_msg))
        {
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                                            layer_msg,
                                            d->tile_->x(),
                                            d->tile_->y(),
                                            d->tile_->z());
            mapnik::featureset_ptr fs = ds->features_at_point(pt, tolerance);
            if (fs && mapnik::is_valid(fs))
            {
                mapnik::feature_ptr feature;
                while ((feature = fs->next()))
                {
                    auto const& geom = feature->get_geometry();
                    auto p2p = path_to_point_distance(geom,x,y);
                    if (!tr.backward(p2p.x_hit,p2p.y_hit,z))
                    {
                        /* LCOV_EXCL_START */
                        throw std::runtime_error("could not reproject lon/lat to mercator");
                        /* LCOV_EXCL_STOP */
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
        protozero::pbf_reader item(d->tile_->get_reader());
        while (item.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
        {
            protozero::pbf_reader layer_msg = item.get_message();
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                                            layer_msg,
                                            d->tile_->x(),
                                            d->tile_->y(),
                                            d->tile_->z());
            mapnik::featureset_ptr fs = ds->features_at_point(pt,tolerance);
            if (fs && mapnik::is_valid(fs))
            {
                mapnik::feature_ptr feature;
                while ((feature = fs->next()))
                {
                    auto const& geom = feature->get_geometry();
                    auto p2p = path_to_point_distance(geom,x,y);
                    if (!tr.backward(p2p.x_hit,p2p.y_hit,z))
                    {
                        /* LCOV_EXCL_START */
                        throw std::runtime_error("could not reproject lon/lat to mercator");
                        /* LCOV_EXCL_STOP */
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
    std::sort(arr.begin(), arr.end(), _querySort);
    return arr;
}

bool VectorTile::_querySort(query_result const& a, query_result const& b)
{
    return a.distance < b.distance;
}

v8::Local<v8::Array> VectorTile::_queryResultToV8(std::vector<query_result> const& result)
{
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(result.size());
    std::size_t i = 0;
    for (auto const& item : result)
    {
        v8::Local<v8::Value> feat = Feature::NewInstance(item.feature);
        v8::Local<v8::Object> feat_obj = feat->ToObject();
        feat_obj->Set(Nan::New("layer").ToLocalChecked(),Nan::New<v8::String>(item.layer).ToLocalChecked());
        feat_obj->Set(Nan::New("distance").ToLocalChecked(),Nan::New<v8::Number>(item.distance));
        feat_obj->Set(Nan::New("x_hit").ToLocalChecked(),Nan::New<v8::Number>(item.x_hit));
        feat_obj->Set(Nan::New("y_hit").ToLocalChecked(),Nan::New<v8::Number>(item.y_hit));
        arr->Set(i++,feat);
    }
    return arr;
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    std::vector<query_lonlat> query;
    double tolerance;
    std::string layer_name;
    std::vector<std::string> fields;
    queryMany_result result;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_queryMany_baton_t;

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
NAN_METHOD(VectorTile::queryMany)
{
    if (info.Length() < 2 || !info[0]->IsArray())
    {
        Nan::ThrowError("expects lon,lat info + object with layer property referring to a layer name");
        return;
    }

    double tolerance = 0.0; // meters
    std::string layer_name("");
    std::vector<std::string> fields;
    std::vector<query_lonlat> query;

    // Convert v8 queryArray to a std vector
    v8::Local<v8::Array> queryArray = v8::Local<v8::Array>::Cast(info[0]);
    query.reserve(queryArray->Length());
    for (uint32_t p = 0; p < queryArray->Length(); ++p)
    {
        v8::Local<v8::Value> item = queryArray->Get(p);
        if (!item->IsArray())
        {
            Nan::ThrowError("non-array item encountered");
            return;
        }
        v8::Local<v8::Array> pair = v8::Local<v8::Array>::Cast(item);
        v8::Local<v8::Value> lon = pair->Get(0);
        v8::Local<v8::Value> lat = pair->Get(1);
        if (!lon->IsNumber() || !lat->IsNumber())
        {
            Nan::ThrowError("lng lat must be numbers");
            return;
        }
        query_lonlat lonlat;
        lonlat.lon = lon->NumberValue();
        lonlat.lat = lat->NumberValue();
        query.push_back(std::move(lonlat));
    }

    // Convert v8 options object to std params
    if (info.Length() > 1)
    {
        v8::Local<v8::Object> options = Nan::New<v8::Object>();
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second argument must be an options object");
            return;
        }
        options = info[1]->ToObject();
        if (options->Has(Nan::New("tolerance").ToLocalChecked()))
        {
            v8::Local<v8::Value> tol = options->Get(Nan::New("tolerance").ToLocalChecked());
            if (!tol->IsNumber())
            {
                Nan::ThrowTypeError("tolerance value must be a number");
                return;
            }
            tolerance = tol->NumberValue();
        }
        if (options->Has(Nan::New("layer").ToLocalChecked()))
        {
            v8::Local<v8::Value> layer_id = options->Get(Nan::New("layer").ToLocalChecked());
            if (!layer_id->IsString())
            {
                Nan::ThrowTypeError("layer value must be a string");
                return;
            }
            layer_name = TOSTR(layer_id);
        }
        if (options->Has(Nan::New("fields").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("fields").ToLocalChecked());
            if (!param_val->IsArray())
            {
                Nan::ThrowTypeError("option 'fields' must be an array of strings");
                return;
            }
            v8::Local<v8::Array> a = v8::Local<v8::Array>::Cast(param_val);
            unsigned int i = 0;
            unsigned int num_fields = a->Length();
            fields.reserve(num_fields);
            while (i < num_fields)
            {
                v8::Local<v8::Value> name = a->Get(i);
                if (name->IsString())
                {
                    fields.emplace_back(TOSTR(name));
                }
                ++i;
            }
        }
    }

    if (layer_name.empty())
    {
        Nan::ThrowTypeError("options.layer is required");
        return;
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.This());

    // If last argument is not a function go with sync call.
    if (!info[info.Length()-1]->IsFunction())
    {
        try
        {
            queryMany_result result;
            _queryMany(result, d, query, tolerance, layer_name, fields);
            v8::Local<v8::Object> result_obj = _queryManyResultToV8(result);
            info.GetReturnValue().Set(result_obj);
            return;
        }
        catch (std::exception const& ex)
        {
            Nan::ThrowError(ex.what());
            return;
        }
    }
    else
    {
        v8::Local<v8::Value> callback = info[info.Length()-1];
        vector_tile_queryMany_baton_t *closure = new vector_tile_queryMany_baton_t();
        closure->d = d;
        closure->query = query;
        closure->tolerance = tolerance;
        closure->layer_name = layer_name;
        closure->fields = fields;
        closure->error = false;
        closure->request.data = closure;
        closure->cb.Reset(callback.As<v8::Function>());
        uv_queue_work(uv_default_loop(), &closure->request, EIO_QueryMany, (uv_after_work_cb)EIO_AfterQueryMany);
        d->Ref();
        return;
    }
}

void VectorTile::_queryMany(queryMany_result & result,
                            VectorTile* d,
                            std::vector<query_lonlat> const& query,
                            double tolerance,
                            std::string const& layer_name,
                            std::vector<std::string> const& fields)
{
    protozero::pbf_reader layer_msg;
    if (!d->tile_->layer_reader(layer_name,layer_msg))
    {
        throw std::runtime_error("Could not find layer in vector tile");
    }

    std::map<unsigned,query_result> features;
    std::map<unsigned,std::vector<query_hit> > hits;

    // Reproject query => mercator points
    mapnik::box2d<double> bbox;
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform tr(wgs84,merc);
    std::vector<mapnik::coord2d> points;
    points.reserve(query.size());
    for (std::size_t p = 0; p < query.size(); ++p)
    {
        double x = query[p].lon;
        double y = query[p].lat;
        double z = 0;
        if (!tr.forward(x,y,z))
        {
            /* LCOV_EXCL_START */
            throw std::runtime_error("could not reproject lon/lat to mercator");
            /* LCOV_EXCL_STOP */
        }
        mapnik::coord2d pt(x,y);
        bbox.expand_to_include(pt);
        points.emplace_back(std::move(pt));
    }
    bbox.pad(tolerance);

    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                mapnik::vector_tile_impl::tile_datasource_pbf>(
                                    layer_msg,
                                    d->tile_->x(),
                                    d->tile_->y(),
                                    d->tile_->z());
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
                auto p2p = path_to_point_distance(geom,pt.x,pt.y);
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

                    std::map<unsigned,std::vector<query_hit> >::iterator hits_it;
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
    for (auto & hit : hits)
    {
        std::sort(hit.second.begin(), hit.second.end(), _queryManySort);
    }

    result.hits = std::move(hits);
    result.features = std::move(features);
    return;
}

bool VectorTile::_queryManySort(query_hit const& a, query_hit const& b)
{
    return a.distance < b.distance;
}

v8::Local<v8::Object> VectorTile::_queryManyResultToV8(queryMany_result const& result)
{
    v8::Local<v8::Object> results = Nan::New<v8::Object>();
    v8::Local<v8::Array> features = Nan::New<v8::Array>(result.features.size());
    v8::Local<v8::Array> hits = Nan::New<v8::Array>(result.hits.size());
    results->Set(Nan::New("hits").ToLocalChecked(), hits);
    results->Set(Nan::New("features").ToLocalChecked(), features);

    // result.features => features
    for (auto const& item : result.features)
    {
        v8::Local<v8::Value> feat = Feature::NewInstance(item.second.feature);
        v8::Local<v8::Object> feat_obj = feat->ToObject();
        feat_obj->Set(Nan::New("layer").ToLocalChecked(),Nan::New<v8::String>(item.second.layer).ToLocalChecked());
        features->Set(item.first, feat_obj);
    }

    // result.hits => hits
    for (auto const& hit : result.hits)
    {
        v8::Local<v8::Array> point_hits = Nan::New<v8::Array>(hit.second.size());
        std::size_t i = 0;
        for (auto const& h : hit.second)
        {
            v8::Local<v8::Object> hit_obj = Nan::New<v8::Object>();
            hit_obj->Set(Nan::New("distance").ToLocalChecked(), Nan::New<v8::Number>(h.distance));
            hit_obj->Set(Nan::New("feature_id").ToLocalChecked(), Nan::New<v8::Number>(h.feature_id));
            point_hits->Set(i++, hit_obj);
        }
        hits->Set(hit.first, point_hits);
    }

    return results;
}

void VectorTile::EIO_QueryMany(uv_work_t* req)
{
    vector_tile_queryMany_baton_t *closure = static_cast<vector_tile_queryMany_baton_t *>(req->data);
    try
    {
        _queryMany(closure->result, closure->d, closure->query, closure->tolerance, closure->layer_name, closure->fields);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterQueryMany(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_queryMany_baton_t *closure = static_cast<vector_tile_queryMany_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        queryMany_result result = closure->result;
        v8::Local<v8::Object> obj = _queryManyResultToV8(result);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), obj };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

