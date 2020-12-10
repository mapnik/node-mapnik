#include "mapnik_map.hpp"
#include "mapnik_featureset.hpp"
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>

namespace detail {

struct AsyncQueryPoint : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncQueryPoint(map_ptr const& map, double x, double y, int layer_idx, bool geo_coords, Napi::Function const& callback)
        : Base(callback),
          map_(map),
          x_(x),
          y_(y),
          layer_idx_(layer_idx),
          geo_coords_(geo_coords) {}

    void Execute() override
    {
        try
        {
            std::vector<mapnik::layer> const& layers = map_->layers();
            if (layer_idx_ >= 0)
            {
                mapnik::featureset_ptr fs;
                if (geo_coords_)
                {
                    fs = map_->query_point(layer_idx_, x_, y_);
                }
                else
                {
                    fs = map_->query_map_point(layer_idx_, x_, y_);
                }
                mapnik::layer const& lyr = layers[layer_idx_];
                featuresets_.insert(std::make_pair(lyr.name(), fs));
            }
            else
            {
                // query all layers
                unsigned idx = 0;
                for (mapnik::layer const& lyr : layers)
                {
                    mapnik::featureset_ptr fs;
                    if (geo_coords_)
                    {
                        fs = map_->query_point(idx, x_, y_);
                    }
                    else
                    {
                        fs = map_->query_map_point(idx, x_, y_);
                    }
                    featuresets_.insert(std::make_pair(lyr.name(), fs));
                    ++idx;
                }
            }
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        std::size_t num_result = featuresets_.size();
        if (num_result >= 1)
        {
            Napi::Array arr = Napi::Array::New(env, num_result);
            typedef std::map<std::string, mapnik::featureset_ptr> fs_itr;
            fs_itr::iterator it = featuresets_.begin();
            fs_itr::iterator end = featuresets_.end();
            unsigned idx = 0;
            for (; it != end; ++it)
            {
                Napi::Object obj = Napi::Object::New(env);
                obj.Set("layer", Napi::String::New(env, it->first));
                Napi::Value arg = Napi::External<mapnik::featureset_ptr>::New(env, &it->second);
                obj.Set("featureset", Featureset::constructor.New({arg}));
                arr.Set(idx, obj);
                ++idx;
            }
            featuresets_.clear();
            return {env.Undefined(), arr};
        }
        return Base::GetResult(env);
    }

  private:
    map_ptr map_;
    double x_;
    double y_;
    int layer_idx_;
    bool geo_coords_;
    std::map<std::string, mapnik::featureset_ptr> featuresets_;
};

} // namespace detail

/**
 * Query a `Mapnik#Map` object to retrieve layer and feature data based on an
 * X and Y `Mapnik#Map` coordinates (use `Map#queryPoint` to query with geographic coordinates).
 *
 * @name queryMapPoint
 * @memberof Map
 * @instance
 * @param {number} x - x coordinate
 * @param {number} y - y coordinate
 * @param {Object} [options]
 * @param {String|number} [options.layer] - layer name (string) or index (positive integer, 0 index)
 * to query. If left blank, will query all layers.
 * @param {Function} callback
 * @returns {Array} array - An array of `Featureset` objects and layer names, which each contain their own
 * `Feature` objects.
 * @example
 * // iterate over the first layer returned and get all attribute information for each feature
 * map.queryMapPoint(10, 10, {layer: 0}, function(err, results) {
 *   if (err) throw err;
 *   console.log(results); // => [{"layer":"layer_name","featureset":{}}]
 *   var featureset = results[0].featureset;
 *   var attributes = [];
 *   var feature;
 *   while ((feature = featureset.next())) {
 *     attributes.push(feature.attributes());
 *   }
 *   console.log(attributes); // => [{"attr_key": "attr_value"}, {...}, {...}]
 * });
 *
 */

Napi::Value Map::queryMapPoint(Napi::CallbackInfo const& info)
{
    return query_point_impl(info, false);
}

/**
 * Query a `Mapnik#Map` object to retrieve layer and feature data based on geographic
 * coordinates of the source data (use `Map#queryMapPoint` to query with XY coordinates).
 *
 * @name queryPoint
 * @memberof Map
 * @instance
 * @param {number} x - x geographic coordinate (CRS based on source data)
 * @param {number} y - y geographic coordinate (CRS based on source data)
 * @param {Object} [options]
 * @param {String|number} [options.layer] - layer name (string) or index (positive integer, 0 index)
 * to query. If left blank, will query all layers.
 * @param {Function} callback
 * @returns {Array} array - An array of `Featureset` objects and layer names, which each contain their own
 * `Feature` objects.
 * @example
 * // query based on web mercator coordinates
 * map.queryMapPoint(-12957605.0331, 5518141.9452, {layer: 0}, function(err, results) {
 *   if (err) throw err;
 *   console.log(results); // => [{"layer":"layer_name","featureset":{}}]
 *   var featureset = results[0].featureset;
 *   var attributes = [];
 *   var feature;
 *   while ((feature = featureset.next())) {
 *     attributes.push(feature.attributes());
 *   }
 *   console.log(attributes); // => [{"attr_key": "attr_value"}, {...}, {...}]
 * });
 *
 */

Napi::Value Map::queryPoint(Napi::CallbackInfo const& info)
{
    return query_point_impl(info, true);
}

Napi::Value Map::query_point_impl(Napi::CallbackInfo const& info, bool geo_coords)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3)
    {
        Napi::Error::New(env, "requires at least three arguments, a x,y query and a callback")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    double x;
    double y;
    if (!info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "x,y arguments must be numbers").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    else
    {
        x = info[0].As<Napi::Number>().DoubleValue();
        y = info[1].As<Napi::Number>().DoubleValue();
    }

    Napi::Object options = Napi::Object::New(env);
    int layer_idx = -1;

    if (info.Length() > 3)
    {
        // options object
        if (!info[2].IsObject())
        {
            Napi::TypeError::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        options = info[2].As<Napi::Object>();
        if (options.Has("layer"))
        {
            std::vector<mapnik::layer> const& layers = map_->layers();
            Napi::Value layer_id = options.Get("layer");
            if (!(layer_id.IsString() || layer_id.IsNumber()))
            {
                Napi::TypeError::New(env, "'layer' option required for map query and must be either a layer name(string) or layer index (integer)").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            if (layer_id.IsString())
            {
                bool found = false;
                unsigned int idx(0);
                std::string layer_name = layer_id.As<Napi::String>();
                for (mapnik::layer const& lyr : layers)
                {
                    if (lyr.name() == layer_name)
                    {
                        found = true;
                        layer_idx = idx;
                        break;
                    }
                    ++idx;
                }
                if (!found)
                {
                    std::ostringstream s;
                    s << "Layer name '" << layer_name << "' not found";
                    Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
            else if (layer_id.IsNumber())
            {
                layer_idx = layer_id.As<Napi::Number>().Int32Value();
                std::size_t layer_num = layers.size();

                if (layer_idx < 0)
                {
                    std::ostringstream s;
                    s << "Zero-based layer index '" << layer_idx << "' not valid"
                      << " must be a positive integer, ";
                    if (layer_num > 0)
                    {
                        s << "only '" << layer_num << "' layers exist in map";
                    }
                    else
                    {
                        s << "no layers found in map";
                    }
                    Napi::TypeError::New(env, s.str()).ThrowAsJavaScriptException();
                    return env.Undefined();
                }
                else if (layer_idx >= static_cast<int>(layer_num))
                {
                    std::ostringstream s;
                    s << "Zero-based layer index '" << layer_idx << "' not valid, ";
                    if (layer_num > 0)
                    {
                        s << "only '" << layer_num << "' layers exist in map";
                    }
                    else
                    {
                        s << "no layers found in map";
                    }
                    Napi::TypeError::New(env, s.str()).ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
        }
    }
    // ensure function callback
    Napi::Value callback = info[info.Length() - 1];
    if (!callback.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto* worker = new detail::AsyncQueryPoint(map_, x, y, layer_idx, geo_coords, callback.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}
