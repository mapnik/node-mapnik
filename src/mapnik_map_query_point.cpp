#include "mapnik_map.hpp"

/*
typedef struct {
    uv_work_t request;
    Map *m;
    std::map<std::string,mapnik::featureset_ptr> featuresets;
    int layer_idx;
    bool geo_coords;
    double x;
    double y;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
} query_map_baton_t;
*/

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
    //abstractQueryPoint(info,false);
    return info.Env().Undefined();
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
    //abstractQueryPoint(info,true);
    return info.Env().Undefined();
}
/*
Napi::Value Map::abstractQueryPoint(Napi::CallbackInfo const& info, bool geo_coords)
{
    Napi::HandleScope scope(env);
    if (info.Length() < 3)
    {
        Napi::Error::New(env, "requires at least three arguments, a x,y query and a callback").ThrowAsJavaScriptException();

        return env.Undefined();
    }

    double x,y;
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

    Map* m = info.Holder().Unwrap<Map>();

    Napi::Object options = Napi::Object::New(env);
    int layer_idx = -1;

    if (info.Length() > 3)
    {
        // options object
        if (!info[2].IsObject()) {
            Napi::TypeError::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();

            return env.Undefined();
        }

        options = info[2].ToObject(Napi::GetCurrentContext());

        if ((options).Has(Napi::String::New(env, "layer")).FromMaybe(false))
        {
            std::vector<mapnik::layer> const& layers = m->map_->layers();
            Napi::Value layer_id = (options).Get(Napi::String::New(env, "layer"));
            if (! (layer_id.IsString() || layer_id.IsNumber()) ) {
                Napi::TypeError::New(env, "'layer' option required for map query and must be either a layer name(string) or layer index (integer)").ThrowAsJavaScriptException();

                return env.Undefined();
            }

            if (layer_id.IsString()) {
                bool found = false;
                unsigned int idx(0);
                std::string layer_name = TOSTR(layer_id);
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

                if (layer_idx < 0) {
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
                    Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();

                    return env.Undefined();
                } else if (layer_idx >= static_cast<int>(layer_num)) {
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
                    Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();

                    return env.Undefined();
                }
            }
        }
    }

    // ensure function callback
    Napi::Value callback = info[info.Length() - 1];
    if (!callback->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();

        return env.Undefined();
    }

    query_map_baton_t *closure = new query_map_baton_t();
    closure->request.data = closure;
    closure->m = m;
    closure->x = x;
    closure->y = y;
    closure->layer_idx = static_cast<std::size_t>(layer_idx);
    closure->geo_coords = geo_coords;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_QueryMap, (uv_after_work_cb)EIO_AfterQueryMap);
    m->Ref();
    return env.Undefined();
}

void Map::EIO_QueryMap(uv_work_t* req)
{
    query_map_baton_t *closure = static_cast<query_map_baton_t *>(req->data);

    try
    {
        std::vector<mapnik::layer> const& layers = closure->m->map_->layers();
        if (closure->layer_idx >= 0)
        {
            mapnik::featureset_ptr fs;
            if (closure->geo_coords)
            {
                fs = closure->m->map_->query_point(closure->layer_idx,
                                                   closure->x,
                                                   closure->y);
            }
            else
            {
                fs = closure->m->map_->query_map_point(closure->layer_idx,
                                                       closure->x,
                                                       closure->y);
            }
            mapnik::layer const& lyr = layers[closure->layer_idx];
            closure->featuresets.insert(std::make_pair(lyr.name(),fs));
        }
        else
        {
            // query all layers
            unsigned idx = 0;
            for (mapnik::layer const& lyr : layers)
            {
                mapnik::featureset_ptr fs;
                if (closure->geo_coords)
                {
                    fs = closure->m->map_->query_point(idx,
                                                       closure->x,
                                                       closure->y);
                }
                else
                {
                    fs = closure->m->map_->query_map_point(idx,
                                                           closure->x,
                                                           closure->y);
                }
                closure->featuresets.insert(std::make_pair(lyr.name(),fs));
                ++idx;
            }
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterQueryMap(uv_work_t* req)
{
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    query_map_baton_t *closure = static_cast<query_map_baton_t *>(req->data);
    if (closure->error) {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else {
        std::size_t num_result = closure->featuresets.size();
        if (num_result >= 1)
        {
            Napi::Array a = Napi::Array::New(env, num_result);
            typedef std::map<std::string,mapnik::featureset_ptr> fs_itr;
            fs_itr::const_iterator it = closure->featuresets.begin();
            fs_itr::const_iterator end = closure->featuresets.end();
            unsigned idx = 0;
            for (; it != end; ++it)
            {
                Napi::Object obj = Napi::Object::New(env);
                (obj).Set(Napi::String::New(env, "layer"), Napi::String::New(env, it->first));
                (obj).Set(Napi::String::New(env, "featureset"), Featureset::NewInstance(it->second));
                (a).Set(idx, obj);
                ++idx;
            }
            closure->featuresets.clear();
            Napi::Value argv[2] = { env.Null(), a };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), env.Undefined() };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }

    closure->m->Unref();
    closure->cb.Reset();
    delete closure;
}
*/
