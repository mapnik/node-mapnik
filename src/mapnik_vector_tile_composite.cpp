#include "utils.hpp"
#include "mapnik_vector_tile.hpp"

#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_composite.hpp"
#include "vector_tile_processor.hpp"

// mapnik
#include <mapnik/box2d.hpp>

// std
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

void _composite(VectorTile* target_vt,
                std::vector<VectorTile*> & vtiles,
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
    mapnik::Map map(target_vt->tile_size(),target_vt->tile_size(),"+init=epsg:3857");
    if (max_extent)
    {
        map.set_maximum_extent(*max_extent);
    }
    else
    {
        map.set_maximum_extent(target_vt->get_tile()->get_buffered_extent());
    }

    std::vector<mapnik::vector_tile_impl::merc_tile_ptr> merc_vtiles;
    for (VectorTile* vt : vtiles)
    {
        merc_vtiles.push_back(vt->get_tile());
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

    mapnik::vector_tile_impl::composite(*target_vt->get_tile(),
                                        merc_vtiles,
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
NAN_METHOD(VectorTile::compositeSync)
{
    info.GetReturnValue().Set(_compositeSync(info));
}

v8::Local<v8::Value> VectorTile::_compositeSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    if (info.Length() < 1 || !info[0]->IsArray())
    {
        Nan::ThrowTypeError("must provide an array of VectorTile objects and an optional options object");
        return scope.Escape(Nan::Undefined());
    }
    v8::Local<v8::Array> vtiles = info[0].As<v8::Array>();
    unsigned num_tiles = vtiles->Length();
    if (num_tiles < 1)
    {
        Nan::ThrowTypeError("must provide an array with at least one VectorTile object and an optional options object");
        return scope.Escape(Nan::Undefined());
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
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second argument must be an options object");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("area_threshold").ToLocalChecked()))
        {
            v8::Local<v8::Value> area_thres = options->Get(Nan::New("area_threshold").ToLocalChecked());
            if (!area_thres->IsNumber())
            {
                Nan::ThrowTypeError("option 'area_threshold' must be an floating point number");
                return scope.Escape(Nan::Undefined());
            }
            area_threshold = area_thres->NumberValue();
            if (area_threshold < 0.0)
            {
                Nan::ThrowTypeError("option 'area_threshold' can not be negative");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (options->Has(Nan::New("simplify_distance").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("simplify_distance").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'simplify_distance' must be an floating point number");
                return scope.Escape(Nan::Undefined());
            }
            simplify_distance = param_val->NumberValue();
            if (simplify_distance < 0.0)
            {
                Nan::ThrowTypeError("option 'simplify_distance' can not be negative");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (options->Has(Nan::New("strictly_simple").ToLocalChecked()))
        {
            v8::Local<v8::Value> strict_simp = options->Get(Nan::New("strictly_simple").ToLocalChecked());
            if (!strict_simp->IsBoolean())
            {
                Nan::ThrowTypeError("option 'strictly_simple' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            strictly_simple = strict_simp->BooleanValue();
        }
        if (options->Has(Nan::New("multi_polygon_union").ToLocalChecked()))
        {
            v8::Local<v8::Value> mpu = options->Get(Nan::New("multi_polygon_union").ToLocalChecked());
            if (!mpu->IsBoolean())
            {
                Nan::ThrowTypeError("option 'multi_polygon_union' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            multi_polygon_union = mpu->BooleanValue();
        }
        if (options->Has(Nan::New("fill_type").ToLocalChecked()))
        {
            v8::Local<v8::Value> ft = options->Get(Nan::New("fill_type").ToLocalChecked());
            if (!ft->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'fill_type' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(ft->IntegerValue());
            if (fill_type < 0 || fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
            {
                Nan::ThrowTypeError("optional arg 'fill_type' out of possible range");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (options->Has(Nan::New("threading_mode").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("threading_mode").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'threading_mode' must be an unsigned integer");
                return scope.Escape(Nan::Undefined());
            }
            threading_mode = static_cast<std::launch>(param_val->IntegerValue());
            if (threading_mode != std::launch::async &&
                threading_mode != std::launch::deferred &&
                threading_mode != (std::launch::async | std::launch::deferred))
            {
                Nan::ThrowTypeError("optional arg 'threading_mode' is invalid");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (options->Has(Nan::New("scale").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            scale_factor = bind_opt->NumberValue();
            if (scale_factor <= 0.0)
            {
                Nan::ThrowTypeError("optional arg 'scale' must be greater then zero");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (options->Has(Nan::New("scale_denominator").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("scale_denominator").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            scale_denominator = bind_opt->NumberValue();
            if (scale_denominator < 0.0)
            {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be non negative number");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (options->Has(Nan::New("offset_x").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_x").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            offset_x = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("offset_y").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_y").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            offset_y = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("reencode").ToLocalChecked()))
        {
            v8::Local<v8::Value> reencode_opt = options->Get(Nan::New("reencode").ToLocalChecked());
            if (!reencode_opt->IsBoolean())
            {
                Nan::ThrowTypeError("reencode value must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            reencode = reencode_opt->BooleanValue();
        }
        if (options->Has(Nan::New("max_extent").ToLocalChecked()))
        {
            v8::Local<v8::Value> max_extent_opt = options->Get(Nan::New("max_extent").ToLocalChecked());
            if (!max_extent_opt->IsArray())
            {
                Nan::ThrowTypeError("max_extent value must be an array of [minx,miny,maxx,maxy]");
                return scope.Escape(Nan::Undefined());
            }
            v8::Local<v8::Array> bbox = max_extent_opt.As<v8::Array>();
            auto len = bbox->Length();
            if (!(len == 4))
            {
                Nan::ThrowTypeError("max_extent value must be an array of [minx,miny,maxx,maxy]");
                return scope.Escape(Nan::Undefined());
            }
            v8::Local<v8::Value> minx = bbox->Get(0);
            v8::Local<v8::Value> miny = bbox->Get(1);
            v8::Local<v8::Value> maxx = bbox->Get(2);
            v8::Local<v8::Value> maxy = bbox->Get(3);
            if (!minx->IsNumber() || !miny->IsNumber() || !maxx->IsNumber() || !maxy->IsNumber())
            {
                Nan::ThrowError("max_extent [minx,miny,maxx,maxy] must be numbers");
                return scope.Escape(Nan::Undefined());
            }
            max_extent = mapnik::box2d<double>(minx->NumberValue(),miny->NumberValue(),
                                               maxx->NumberValue(),maxy->NumberValue());
        }
        if (options->Has(Nan::New("process_all_rings").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("process_all_rings").ToLocalChecked());
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'process_all_rings' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            process_all_rings = param_val->BooleanValue();
        }

        if (options->Has(Nan::New("image_scaling").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("image_scaling").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'image_scaling' must be a string");
                return scope.Escape(Nan::Undefined());
            }
            std::string image_scaling = TOSTR(param_val);
            boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
            if (!method)
            {
                Nan::ThrowTypeError("option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')");
                return scope.Escape(Nan::Undefined());
            }
            scaling_method = *method;
        }

        if (options->Has(Nan::New("image_format").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("image_format").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'image_format' must be a string");
                return scope.Escape(Nan::Undefined());
            }
            image_format = TOSTR(param_val);
        }
    }
    VectorTile* target_vt = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::vector<VectorTile*> vtiles_vec;
    vtiles_vec.reserve(num_tiles);
    for (unsigned j=0;j < num_tiles;++j)
    {
        v8::Local<v8::Value> val = vtiles->Get(j);
        if (!val->IsObject())
        {
            Nan::ThrowTypeError("must provide an array of VectorTile objects");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> tile_obj = val->ToObject();
        if (tile_obj->IsNull() || tile_obj->IsUndefined() || !Nan::New(VectorTile::constructor)->HasInstance(tile_obj))
        {
            Nan::ThrowTypeError("must provide an array of VectorTile objects");
            return scope.Escape(Nan::Undefined());
        }
        vtiles_vec.push_back(Nan::ObjectWrap::Unwrap<VectorTile>(tile_obj));
    }
    try
    {
        _composite(target_vt,
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
        Nan::ThrowTypeError(ex.what());
        return scope.Escape(Nan::Undefined());
    }

    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    double scale_factor;
    unsigned offset_x;
    unsigned offset_y;
    double area_threshold;
    double scale_denominator;
    std::vector<VectorTile*> vtiles;
    bool error;
    bool strictly_simple;
    bool multi_polygon_union;
    mapnik::vector_tile_impl::polygon_fill_type fill_type;
    bool reencode;
    boost::optional<mapnik::box2d<double>> max_extent;
    double simplify_distance;
    bool process_all_rings;
    std::string image_format;
    mapnik::scaling_method_e scaling_method;
    std::launch threading_mode;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_composite_baton_t;

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
NAN_METHOD(VectorTile::composite)
{
    if ((info.Length() < 2) || !info[info.Length()-1]->IsFunction())
    {
        info.GetReturnValue().Set(_compositeSync(info));
        return;
    }
    if (!info[0]->IsArray())
    {
        Nan::ThrowTypeError("must provide an array of VectorTile objects and an optional options object");
        return;
    }
    v8::Local<v8::Array> vtiles = info[0].As<v8::Array>();
    unsigned num_tiles = vtiles->Length();
    if (num_tiles < 1)
    {
        Nan::ThrowTypeError("must provide an array with at least one VectorTile object and an optional options object");
        return;
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
    std::string merc_srs("+init=epsg:3857");

    if (info.Length() > 2)
    {
        // options object
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second argument must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("area_threshold").ToLocalChecked()))
        {
            v8::Local<v8::Value> area_thres = options->Get(Nan::New("area_threshold").ToLocalChecked());
            if (!area_thres->IsNumber())
            {
                Nan::ThrowTypeError("option 'area_threshold' must be a number");
                return;
            }
            area_threshold = area_thres->NumberValue();
            if (area_threshold < 0.0)
            {
                Nan::ThrowTypeError("option 'area_threshold' can not be negative");
                return;
            }
        }
        if (options->Has(Nan::New("strictly_simple").ToLocalChecked()))
        {
            v8::Local<v8::Value> strict_simp = options->Get(Nan::New("strictly_simple").ToLocalChecked());
            if (!strict_simp->IsBoolean())
            {
                Nan::ThrowTypeError("strictly_simple value must be a boolean");
                return;
            }
            strictly_simple = strict_simp->BooleanValue();
        }
        if (options->Has(Nan::New("multi_polygon_union").ToLocalChecked()))
        {
            v8::Local<v8::Value> mpu = options->Get(Nan::New("multi_polygon_union").ToLocalChecked());
            if (!mpu->IsBoolean())
            {
                Nan::ThrowTypeError("multi_polygon_union value must be a boolean");
                return;
            }
            multi_polygon_union = mpu->BooleanValue();
        }
        if (options->Has(Nan::New("fill_type").ToLocalChecked()))
        {
            v8::Local<v8::Value> ft = options->Get(Nan::New("fill_type").ToLocalChecked());
            if (!ft->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'fill_type' must be a number");
                return;
            }
            fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(ft->IntegerValue());
            if (fill_type < 0 || fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
            {
                Nan::ThrowTypeError("optional arg 'fill_type' out of possible range");
                return;
            }
        }
        if (options->Has(Nan::New("threading_mode").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("threading_mode").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'threading_mode' must be an unsigned integer");
                return;
            }
            threading_mode = static_cast<std::launch>(param_val->IntegerValue());
            if (threading_mode != std::launch::async &&
                threading_mode != std::launch::deferred &&
                threading_mode != (std::launch::async | std::launch::deferred))
            {
                Nan::ThrowTypeError("optional arg 'threading_mode' is not a valid value");
                return;
            }
        }
        if (options->Has(Nan::New("simplify_distance").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("simplify_distance").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'simplify_distance' must be an floating point number");
                return;
            }
            simplify_distance = param_val->NumberValue();
            if (simplify_distance < 0.0)
            {
                Nan::ThrowTypeError("option 'simplify_distance' can not be negative");
                return;
            }
        }
        if (options->Has(Nan::New("scale").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return;
            }
            scale_factor = bind_opt->NumberValue();
            if (scale_factor < 0.0)
            {
                Nan::ThrowTypeError("option 'scale' can not be negative");
                return;
            }
        }
        if (options->Has(Nan::New("scale_denominator").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("scale_denominator").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                return;
            }
            scale_denominator = bind_opt->NumberValue();
            if (scale_denominator < 0.0)
            {
                Nan::ThrowTypeError("option 'scale_denominator' can not be negative");
                return;
            }
        }
        if (options->Has(Nan::New("offset_x").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_x").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
                return;
            }
            offset_x = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("offset_y").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_y").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
                return;
            }
            offset_y = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("reencode").ToLocalChecked()))
        {
            v8::Local<v8::Value> reencode_opt = options->Get(Nan::New("reencode").ToLocalChecked());
            if (!reencode_opt->IsBoolean())
            {
                Nan::ThrowTypeError("reencode value must be a boolean");
                return;
            }
            reencode = reencode_opt->BooleanValue();
        }
        if (options->Has(Nan::New("max_extent").ToLocalChecked()))
        {
            v8::Local<v8::Value> max_extent_opt = options->Get(Nan::New("max_extent").ToLocalChecked());
            if (!max_extent_opt->IsArray())
            {
                Nan::ThrowTypeError("max_extent value must be an array of [minx,miny,maxx,maxy]");
                return;
            }
            v8::Local<v8::Array> bbox = max_extent_opt.As<v8::Array>();
            auto len = bbox->Length();
            if (!(len == 4))
            {
                Nan::ThrowTypeError("max_extent value must be an array of [minx,miny,maxx,maxy]");
                return;
            }
            v8::Local<v8::Value> minx = bbox->Get(0);
            v8::Local<v8::Value> miny = bbox->Get(1);
            v8::Local<v8::Value> maxx = bbox->Get(2);
            v8::Local<v8::Value> maxy = bbox->Get(3);
            if (!minx->IsNumber() || !miny->IsNumber() || !maxx->IsNumber() || !maxy->IsNumber())
            {
                Nan::ThrowError("max_extent [minx,miny,maxx,maxy] must be numbers");
                return;
            }
            max_extent = mapnik::box2d<double>(minx->NumberValue(),miny->NumberValue(),
                                               maxx->NumberValue(),maxy->NumberValue());
        }
        if (options->Has(Nan::New("process_all_rings").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("process_all_rings").ToLocalChecked());
            if (!param_val->IsBoolean()) {
                Nan::ThrowTypeError("option 'process_all_rings' must be a boolean");
                return;
            }
            process_all_rings = param_val->BooleanValue();
        }

        if (options->Has(Nan::New("image_scaling").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("image_scaling").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'image_scaling' must be a string");
                return;
            }
            std::string image_scaling = TOSTR(param_val);
            boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
            if (!method)
            {
                Nan::ThrowTypeError("option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')");
                return;
            }
            scaling_method = *method;
        }

        if (options->Has(Nan::New("image_format").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("image_format").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'image_format' must be a string");
                return;
            }
            image_format = TOSTR(param_val);
        }
    }

    v8::Local<v8::Value> callback = info[info.Length()-1];
    vector_tile_composite_baton_t *closure = new vector_tile_composite_baton_t();
    closure->request.data = closure;
    closure->offset_x = offset_x;
    closure->offset_y = offset_y;
    closure->strictly_simple = strictly_simple;
    closure->fill_type = fill_type;
    closure->multi_polygon_union = multi_polygon_union;
    closure->area_threshold = area_threshold;
    closure->scale_factor = scale_factor;
    closure->scale_denominator = scale_denominator;
    closure->reencode = reencode;
    closure->max_extent = max_extent;
    closure->simplify_distance = simplify_distance;
    closure->process_all_rings = process_all_rings;
    closure->scaling_method = scaling_method;
    closure->image_format = image_format;
    closure->threading_mode = threading_mode;
    closure->d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    closure->error = false;
    closure->vtiles.reserve(num_tiles);
    for (unsigned j=0;j < num_tiles;++j)
    {
        v8::Local<v8::Value> val = vtiles->Get(j);
        if (!val->IsObject())
        {
            delete closure;
            Nan::ThrowTypeError("must provide an array of VectorTile objects");
            return;
        }
        v8::Local<v8::Object> tile_obj = val->ToObject();
        if (tile_obj->IsNull() || tile_obj->IsUndefined() || !Nan::New(VectorTile::constructor)->HasInstance(tile_obj))
        {
            delete closure;
            Nan::ThrowTypeError("must provide an array of VectorTile objects");
            return;
        }
        VectorTile* vt = Nan::ObjectWrap::Unwrap<VectorTile>(tile_obj);
        vt->Ref();
        closure->vtiles.push_back(vt);
    }
    closure->d->Ref();
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Composite, (uv_after_work_cb)EIO_AfterComposite);
    return;
}

void VectorTile::EIO_Composite(uv_work_t* req)
{
    vector_tile_composite_baton_t *closure = static_cast<vector_tile_composite_baton_t *>(req->data);
    try
    {
        _composite(closure->d,
                   closure->vtiles,
                   closure->scale_factor,
                   closure->offset_x,
                   closure->offset_y,
                   closure->area_threshold,
                   closure->strictly_simple,
                   closure->multi_polygon_union,
                   closure->fill_type,
                   closure->scale_denominator,
                   closure->reencode,
                   closure->max_extent,
                   closure->simplify_distance,
                   closure->process_all_rings,
                   closure->image_format,
                   closure->scaling_method,
                   closure->threading_mode);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterComposite(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_composite_baton_t *closure = static_cast<vector_tile_composite_baton_t *>(req->data);

    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->d->handle() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    for (VectorTile* vt : closure->vtiles)
    {
        vt->Unref();
    }
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

