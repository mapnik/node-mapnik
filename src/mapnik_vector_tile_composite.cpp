#include "mapnik_vector_tile.hpp"
// mapnik-vector-tile
#include "vector_tile_composite.hpp"

using tile_type = mapnik::vector_tile_impl::merc_tile_ptr;

void _composite(tile_type target_tile,
                std::vector<tile_type>& vtiles,
                double scale_factor,
                unsigned offset_x,
                unsigned offset_y,
                double area_threshold,
                bool strictly_simple,
                bool multi_polygon_union,
                mapnik::vector_tile_impl::polygon_fill_type fill_type,
                double scale_denominator,
                bool reencode,
                boost::optional<mapnik::box2d<double>> const& max_extent,
                double simplify_distance,
                bool process_all_rings,
                std::string const& image_format,
                mapnik::scaling_method_e scaling_method,
                std::launch threading_mode)
{
    // create map
    mapnik::Map map(target_tile->size(), target_tile->size(), "epsg:3857");
    if (max_extent)
    {
        map.set_maximum_extent(*max_extent);
    }
    else
    {
        map.set_maximum_extent(target_tile->get_buffered_extent());
    }
    mapnik::vector_tile_impl::processor ren(map);
    ren.set_fill_type(fill_type);
    ren.set_simplify_distance(simplify_distance);
    ren.set_process_all_rings(process_all_rings);
    ren.set_multi_polygon_union(multi_polygon_union);
    ren.set_strictly_simple(strictly_simple);
    ren.set_area_threshold(area_threshold);
    ren.set_scale_factor(scale_factor);
    ren.set_scaling_method(scaling_method);
    ren.set_image_format(image_format);
    ren.set_threading_mode(threading_mode);

    mapnik::vector_tile_impl::composite(*target_tile,
                                        vtiles,
                                        map,
                                        ren,
                                        scale_denominator,
                                        offset_x,
                                        offset_y,
                                        reencode);
}

/**
 * Synchronous version of {@link #VectorTile.composite}
 *
 * @name compositeSync
 * @memberof VectorTile
 * @instance
 * @instance
 * @param {Array<mapnik.VectorTile>} array - an array of vector tile objects
 * @param {object} [options]
 * @example
 * var vt1 = new mapnik.VectorTile(0,0,0);
 * var vt2 = new mapnik.VectorTile(0,0,0);
 * var options = { ... };
 * vt1.compositeSync([vt2], options);
 *
 */
Napi::Value VectorTile::compositeSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsArray())
    {
        Napi::TypeError::New(env, "must provide an array of VectorTile objects and an optional options object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Array vtiles = info[0].As<Napi::Array>();
    unsigned num_tiles = vtiles.Length();
    if (num_tiles < 1)
    {
        Napi::TypeError::New(env, "must provide an array with at least one VectorTile object and an optional options object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // options needed for re-rendering tiles
    // unclear yet to what extent these need to be user
    // driven, but we expose here to avoid hardcoding
    double scale_factor = 1.0;
    unsigned offset_x = 0;
    unsigned offset_y = 0;
    double area_threshold = 0.1;
    bool strictly_simple = true;
    bool multi_polygon_union = false;
    mapnik::vector_tile_impl::polygon_fill_type fill_type = mapnik::vector_tile_impl::positive_fill;
    double scale_denominator = 0.0;
    bool reencode = false;
    boost::optional<mapnik::box2d<double>> max_extent;
    double simplify_distance = 0.0;
    bool process_all_rings = false;
    std::string image_format = "webp";
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_BILINEAR;
    std::launch threading_mode = std::launch::deferred;

    if (info.Length() > 1)
    {
        // options object
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("area_threshold"))
        {
            Napi::Value area_thres = options.Get("area_threshold");
            if (!area_thres.IsNumber())
            {
                Napi::TypeError::New(env, "option 'area_threshold' must be an floating point number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            area_threshold = area_thres.As<Napi::Number>().DoubleValue();
            if (area_threshold < 0.0)
            {
                Napi::TypeError::New(env, "option 'area_threshold' can not be negative").ThrowAsJavaScriptException();
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
                Napi::TypeError::New(env, "option 'simplify_distance' can not be negative").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("strictly_simple"))
        {
            Napi::Value strict_simp = options.Get("strictly_simple");
            if (!strict_simp.IsBoolean())
            {
                Napi::TypeError::New(env, "option 'strictly_simple' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            strictly_simple = strict_simp.As<Napi::Boolean>();
        }
        if (options.Has("multi_polygon_union"))
        {
            Napi::Value mpu = options.Get("multi_polygon_union");
            if (!mpu.IsBoolean())
            {
                Napi::TypeError::New(env, "option 'multi_polygon_union' must be a boolean").ThrowAsJavaScriptException();
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
        if (options.Has("threading_mode"))
        {
            Napi::Value param_val = options.Get("threading_mode");
            if (!param_val.IsNumber())
            {
                Napi::TypeError::New(env, "option 'threading_mode' must be an unsigned integer").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            threading_mode = static_cast<std::launch>(param_val.As<Napi::Number>().Int32Value());
            if (threading_mode != std::launch::async &&
                threading_mode != std::launch::deferred &&
                threading_mode != (std::launch::async | std::launch::deferred))
            {
                Napi::TypeError::New(env, "optional arg 'threading_mode' is invalid").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("scale"))
        {
            Napi::Value bind_opt = options.Get("scale");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_factor = bind_opt.As<Napi::Number>().DoubleValue();
            if (scale_factor <= 0.0)
            {
                Napi::TypeError::New(env, "optional arg 'scale' must be greater than zero").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("scale_denominator"))
        {
            Napi::Value bind_opt = options.Get("scale_denominator");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();

                return env.Undefined();
            }
            scale_denominator = bind_opt.As<Napi::Number>().DoubleValue();
            if (scale_denominator < 0.0)
            {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be non negative number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("offset_x"))
        {
            Napi::Value bind_opt = options.Get("offset_x");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'offset_x' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            offset_x = bind_opt.As<Napi::Number>().Int32Value();
        }
        if (options.Has("offset_y"))
        {
            Napi::Value bind_opt = options.Get("offset_y");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'offset_y' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            offset_y = bind_opt.As<Napi::Number>().Int32Value();
        }
        if (options.Has("reencode"))
        {
            Napi::Value reencode_opt = options.Get("reencode");
            if (!reencode_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "reencode value must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            reencode = reencode_opt.As<Napi::Boolean>();
        }
        if (options.Has("max_extent"))
        {
            Napi::Value max_extent_opt = options.Get("max_extent");
            if (!max_extent_opt.IsArray())
            {
                Napi::TypeError::New(env, "max_extent value must be an array of [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Array bbox = max_extent_opt.As<Napi::Array>();
            auto len = bbox.Length();
            if (len != 4)
            {
                Napi::TypeError::New(env, "max_extent value must be an array of [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Value minx = bbox.Get(0u);
            Napi::Value miny = bbox.Get(1u);
            Napi::Value maxx = bbox.Get(2u);
            Napi::Value maxy = bbox.Get(3u);
            if (!minx.IsNumber() || !miny.IsNumber() || !maxx.IsNumber() || !maxy.IsNumber())
            {
                Napi::Error::New(env, "max_extent [minx,miny,maxx,maxy] must be numbers").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            max_extent = mapnik::box2d<double>(minx.As<Napi::Number>().DoubleValue(), miny.As<Napi::Number>().DoubleValue(),
                                               maxx.As<Napi::Number>().DoubleValue(), maxy.As<Napi::Number>().DoubleValue());
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

        if (options.Has("image_scaling"))
        {
            Napi::Value param_val = options.Get("image_scaling");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'image_scaling' must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::string image_scaling = param_val.As<Napi::String>();
            boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
            if (!method)
            {
                Napi::TypeError::New(env, "option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scaling_method = *method;
        }

        if (options.Has("image_format"))
        {
            Napi::Value param_val = options.Get("image_format");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'image_format' must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            image_format = param_val.As<Napi::String>();
        }
    }

    std::vector<tile_type> vtiles_vec;
    vtiles_vec.reserve(num_tiles);
    for (std::size_t j = 0; j < num_tiles; ++j)
    {
        Napi::Value val = (vtiles).Get(j);
        if (!val.IsObject())
        {
            Napi::TypeError::New(env, "must provide an array of VectorTile objects").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object tile_obj = val.As<Napi::Object>();
        if (!tile_obj.InstanceOf(VectorTile::constructor.Value()))
        {
            Napi::TypeError::New(env, "must provide an array of VectorTile objects").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        vtiles_vec.push_back(Napi::ObjectWrap<VectorTile>::Unwrap(tile_obj)->tile_);
    }
    try
    {
        _composite(tile_,
                   vtiles_vec,
                   scale_factor,
                   offset_x,
                   offset_y,
                   area_threshold,
                   strictly_simple,
                   multi_polygon_union,
                   fill_type,
                   scale_denominator,
                   reencode,
                   max_extent,
                   simplify_distance,
                   process_all_rings,
                   image_format,
                   scaling_method,
                   threading_mode);
    }
    catch (std::exception const& ex)
    {
        Napi::TypeError::New(env, ex.what()).ThrowAsJavaScriptException();

        return env.Undefined();
    }

    return env.Undefined();
}

namespace {

struct AsyncCompositeVectorTile : Napi::AsyncWorker
{
    AsyncCompositeVectorTile(tile_type const& tile,
                             std::vector<tile_type> const& vtiles,
                             double scale_factor,
                             unsigned offset_x,
                             unsigned offset_y,
                             double area_threshold,
                             bool strictly_simple,
                             bool multi_polygon_union,
                             mapnik::vector_tile_impl::polygon_fill_type fill_type,
                             double scale_denominator,
                             bool reencode,
                             boost::optional<mapnik::box2d<double>> max_extent,
                             double simplify_distance,
                             bool process_all_rings,
                             std::string const& image_format,
                             mapnik::scaling_method_e scaling_method,
                             std::launch threading_mode,
                             Napi::Function const& callback)
        : Napi::AsyncWorker(callback),
          tile_(tile),
          vtiles_(vtiles),
          scale_factor_(scale_factor),
          offset_x_(offset_x),
          offset_y_(offset_y),
          area_threshold_(area_threshold),
          strictly_simple_(strictly_simple),
          multi_polygon_union_(multi_polygon_union),
          fill_type_(fill_type),
          scale_denominator_(scale_denominator),
          reencode_(reencode),
          max_extent_(max_extent),
          simplify_distance_(simplify_distance),
          process_all_rings_(process_all_rings),
          image_format_(image_format),
          scaling_method_(scaling_method),
          threading_mode_(threading_mode)
    {
    }

    void Execute() override
    {
        try
        {
            _composite(tile_,
                       vtiles_,
                       scale_factor_,
                       offset_x_,
                       offset_y_,
                       area_threshold_,
                       strictly_simple_,
                       multi_polygon_union_,
                       fill_type_,
                       scale_denominator_,
                       reencode_,
                       max_extent_,
                       simplify_distance_,
                       process_all_rings_,
                       image_format_,
                       scaling_method_,
                       threading_mode_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        Napi::Value arg = Napi::External<mapnik::vector_tile_impl::merc_tile_ptr>::New(env, &tile_);
        Napi::Object obj = VectorTile::constructor.New({arg});
        return {env.Undefined(), napi_value(obj)};
    }

  private:
    tile_type tile_;
    std::vector<tile_type> vtiles_;
    double scale_factor_;
    unsigned offset_x_;
    unsigned offset_y_;
    double area_threshold_;
    bool strictly_simple_;
    bool multi_polygon_union_;
    mapnik::vector_tile_impl::polygon_fill_type fill_type_;
    double scale_denominator_;
    bool reencode_;
    boost::optional<mapnik::box2d<double>> max_extent_;
    double simplify_distance_;
    bool process_all_rings_;
    std::string image_format_;
    mapnik::scaling_method_e scaling_method_;
    std::launch threading_mode_;
};

} // namespace

/**
 * Composite an array of vector tiles into one vector tile
 *
 * @name composite
 * @memberof VectorTile
 * @instance
 * @param {Array<mapnik.VectorTile>} array - an array of vector tile objects
 * @param {object} [options]
 * @param {float} [options.scale_factor=1.0]
 * @param {number} [options.offset_x=0]
 * @param {number} [options.offset_y=0]
 * @param {float} [options.area_threshold=0.1] - used to discard small polygons.
 * If a value is greater than `0` it will trigger polygons with an area smaller
 * than the value to be discarded. Measured in grid integers, not spherical mercator
 * coordinates.
 * @param {boolean} [options.strictly_simple=true] - ensure all geometry is valid according to
 * OGC Simple definition
 * @param {boolean} [options.multi_polygon_union=false] - union all multipolygons
 * @param {Object<mapnik.polygonFillType>} [options.fill_type=mapnik.polygonFillType.positive]
 * the fill type used in determining what are holes and what are outer rings. See the
 * [Clipper documentation](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/PolyFillType.htm)
 * to learn more about fill types.
 * @param {float} [options.scale_denominator=0.0]
 * @param {boolean} [options.reencode=false]
 * @param {Array<number>} [options.max_extent=minx,miny,maxx,maxy]
 * @param {float} [options.simplify_distance=0.0] - Simplification works to generalize
 * geometries before encoding into vector tiles.simplification distance The
 * `simplify_distance` value works in integer space over a 4096 pixel grid and uses
 * the [Douglas-Peucker algorithm](https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm).
 * @param {boolean} [options.process_all_rings=false] - if `true`, don't assume winding order and ring order of
 * polygons are correct according to the [`2.0` Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec)
 * @param {string} [options.image_format=webp] or `jpeg`, `png`, `tiff`
 * @param {string} [options.scaling_method=bilinear] - can be any
 * of the <mapnik.imageScaling> methods
 * @param {string} [options.threading_mode=deferred]
 * @param {Function} callback - `function(err)`
 * @example
 * var vt1 = new mapnik.VectorTile(0,0,0);
 * var vt2 = new mapnik.VectorTile(0,0,0);
 * var options = {
 *   scale: 1.0,
 *   offset_x: 0,
 *   offset_y: 0,
 *   area_threshold: 0.1,
 *   strictly_simple: false,
 *   multi_polygon_union: true,
 *   fill_type: mapnik.polygonFillType.nonZero,
 *   process_all_rings:false,
 *   scale_denominator: 0.0,
 *   reencode: true
 * }
 * // add vt2 to vt1 tile
 * vt1.composite([vt2], options, function(err) {
 *   if (err) throw err;
 *   // your custom code with `vt1`
 * });
 *
 */
Napi::Value VectorTile::composite(Napi::CallbackInfo const& info)
{
    if (info.Length() < 2 || !info[info.Length() - 1].IsFunction())
    {
        return compositeSync(info);
    }

    Napi::Env env = info.Env();
    if (!info[0].IsArray())
    {
        Napi::TypeError::New(env, "must provide an array of VectorTile objects and an optional options object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Array vtiles = info[0].As<Napi::Array>();
    unsigned num_tiles = vtiles.Length();
    if (num_tiles < 1)
    {
        Napi::TypeError::New(env, "must provide an array with at least one VectorTile object and an optional options object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // options needed for re-rendering tiles
    // unclear yet to what extent these need to be user
    // driven, but we expose here to avoid hardcoding
    double scale_factor = 1.0;
    unsigned offset_x = 0;
    unsigned offset_y = 0;
    double area_threshold = 0.1;
    bool strictly_simple = true;
    bool multi_polygon_union = false;
    mapnik::vector_tile_impl::polygon_fill_type fill_type = mapnik::vector_tile_impl::positive_fill;
    double scale_denominator = 0.0;
    bool reencode = false;
    boost::optional<mapnik::box2d<double>> max_extent;
    double simplify_distance = 0.0;
    bool process_all_rings = false;
    std::string image_format = "webp";
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_BILINEAR;
    std::launch threading_mode = std::launch::deferred;
    std::string merc_srs("epsg:3857");

    if (info.Length() > 2)
    {
        // options object
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("area_threshold"))
        {
            Napi::Value area_thres = options.Get("area_threshold");
            if (!area_thres.IsNumber())
            {
                Napi::TypeError::New(env, "option 'area_threshold' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            area_threshold = area_thres.As<Napi::Number>().DoubleValue();
            if (area_threshold < 0.0)
            {
                Napi::TypeError::New(env, "option 'area_threshold' can not be negative").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("strictly_simple"))
        {
            Napi::Value strict_simp = options.Get("strictly_simple");
            if (!strict_simp.IsBoolean())
            {
                Napi::TypeError::New(env, "strictly_simple value must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            strictly_simple = strict_simp.As<Napi::Boolean>();
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
        if (options.Has("threading_mode"))
        {
            Napi::Value param_val = options.Get("threading_mode");
            if (!param_val.IsNumber())
            {
                Napi::TypeError::New(env, "option 'threading_mode' must be an unsigned integer").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            threading_mode = static_cast<std::launch>(param_val.As<Napi::Number>().Int32Value());
            if (threading_mode != std::launch::async &&
                threading_mode != std::launch::deferred &&
                threading_mode != (std::launch::async | std::launch::deferred))
            {
                Napi::TypeError::New(env, "optional arg 'threading_mode' is not a valid value").ThrowAsJavaScriptException();
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
                Napi::TypeError::New(env, "option 'simplify_distance' can not be negative").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }

        if (options.Has("scale"))
        {
            Napi::Value bind_opt = options.Get("scale");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_factor = bind_opt.As<Napi::Number>().DoubleValue();
            if (scale_factor < 0.0)
            {
                Napi::TypeError::New(env, "option 'scale' can not be negative").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("scale_denominator"))
        {
            Napi::Value bind_opt = options.Get("scale_denominator");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_denominator = bind_opt.As<Napi::Number>().DoubleValue();
            if (scale_denominator < 0.0)
            {
                Napi::TypeError::New(env, "option 'scale_denominator' can not be negative").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("offset_x"))
        {
            Napi::Value bind_opt = options.Get("offset_x");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'offset_x' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            offset_x = bind_opt.As<Napi::Number>().Int32Value();
        }
        if (options.Has("offset_y"))
        {
            Napi::Value bind_opt = options.Get("offset_y");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'offset_y' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            offset_y = bind_opt.As<Napi::Number>().Int32Value();
        }
        if (options.Has("reencode"))
        {
            Napi::Value reencode_opt = options.Get("reencode");
            if (!reencode_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "reencode value must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            reencode = reencode_opt.As<Napi::Boolean>();
        }
        if (options.Has("max_extent"))
        {
            Napi::Value max_extent_opt = options.Get("max_extent");
            if (!max_extent_opt.IsArray())
            {
                Napi::TypeError::New(env, "max_extent value must be an array of [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Array bbox = max_extent_opt.As<Napi::Array>();
            auto len = bbox.Length();
            if (!(len == 4))
            {
                Napi::TypeError::New(env, "max_extent value must be an array of [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Value minx = bbox.Get(0u);
            Napi::Value miny = bbox.Get(1u);
            Napi::Value maxx = bbox.Get(2u);
            Napi::Value maxy = bbox.Get(3u);
            if (!minx.IsNumber() || !miny.IsNumber() || !maxx.IsNumber() || !maxy.IsNumber())
            {
                Napi::Error::New(env, "max_extent [minx,miny,maxx,maxy] must be numbers").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            max_extent = mapnik::box2d<double>(minx.As<Napi::Number>().DoubleValue(), miny.As<Napi::Number>().DoubleValue(),
                                               maxx.As<Napi::Number>().DoubleValue(), maxy.As<Napi::Number>().DoubleValue());
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

        if (options.Has("image_scaling"))
        {
            Napi::Value param_val = options.Get("image_scaling");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'image_scaling' must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::string image_scaling = param_val.As<Napi::String>();
            boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
            if (!method)
            {
                Napi::TypeError::New(env, "option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scaling_method = *method;
        }

        if (options.Has("image_format"))
        {
            Napi::Value param_val = options.Get("image_format");
            if (!param_val.IsString())
            {
                Napi::TypeError::New(env, "option 'image_format' must be a string").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            image_format = param_val.As<Napi::String>();
        }
    }

    Napi::Value callback = info[info.Length() - 1];
    std::vector<tile_type> vtiles_vec;
    for (std::size_t j = 0; j < num_tiles; ++j)
    {
        Napi::Value val = vtiles.Get(j);
        if (!val.IsObject())
        {
            Napi::TypeError::New(env, "must provide an array of VectorTile objects").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object tile_obj = val.As<Napi::Object>();
        if (!tile_obj.InstanceOf(VectorTile::constructor.Value()))
        {
            Napi::TypeError::New(env, "must provide an array of VectorTile objects").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        vtiles_vec.push_back(Napi::ObjectWrap<VectorTile>::Unwrap(tile_obj)->tile_);
    }

    auto* worker = new AsyncCompositeVectorTile{tile_,
                                                vtiles_vec,
                                                scale_factor,
                                                offset_x,
                                                offset_y,
                                                area_threshold,
                                                strictly_simple,
                                                multi_polygon_union,
                                                fill_type,
                                                scale_denominator,
                                                reencode,
                                                max_extent,
                                                simplify_distance,
                                                process_all_rings,
                                                image_format,
                                                scaling_method,
                                                threading_mode,
                                                callback.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}
