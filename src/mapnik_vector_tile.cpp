#include "utils.hpp"
#include "mapnik_map.hpp"
#include "mapnik_image.hpp"
#if defined(GRID_RENDERER)
#include "mapnik_grid.hpp"
#endif
#include "mapnik_feature.hpp"
#include "mapnik_cairo_surface.hpp"
#ifdef SVG_RENDERER
#include <mapnik/svg/output/svg_renderer.hpp>
#endif

#include "mapnik_vector_tile.hpp"
#include "vector_tile_compression.hpp"
#include "vector_tile_composite.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_load_tile.hpp"
#include "object_to_container.hpp"

// mapnik
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
#include <mapnik/datasource_cache.hpp>
#include <mapnik/geometry/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/geometry/is_simple.hpp>
#include <mapnik/geometry/is_valid.hpp>
#include <mapnik/geometry/reprojection.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/hit_test_filter.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/request.hpp>
#include <mapnik/scale_denominator.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/feature_to_geojson.hpp>
#include <mapnik/version.hpp>
#if defined(GRID_RENDERER)
#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp>  // for grid_renderer
#endif
#ifdef HAVE_CAIRO
#include <mapnik/cairo/cairo_renderer.hpp>
#include <cairo.h>
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif // CAIRO_HAS_SVG_SURFACE
#endif

// std
#include <set>                          // for set, etc
#include <sstream>                      // for operator<<, basic_ostream, etc
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
        auto num_points = geom.size();
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
    p2p_result operator() (mapnik::geometry::polygon<double> const& poly) const
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
                if (mapnik::detail::pip(pt0.x, pt0.y, pt1.x, pt1.y, x_,y_))
                {
                    inside = !inside;
                }
            }
            if (ring_index == 0 && !inside) return p2p;
        }
        if (inside) p2p.distance = 0;
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

Nan::Persistent<v8::FunctionTemplate> VectorTile::constructor;

/**
 * **`mapnik.VectorTile`**

 * A tile generator built according to the [Mapbox Vector Tile](https://github.com/mapbox/vector-tile-spec)
 * specification for compressed and simplified tiled vector data.
 * Learn more about vector tiles [here](https://www.mapbox.com/developers/vector-tiles/).
 *
 * @class VectorTile
 * @param {number} z - an integer zoom level
 * @param {number} x - an integer x coordinate
 * @param {number} y - an integer y coordinate
 * @property {number} x - horizontal axis position
 * @property {number} y - vertical axis position
 * @property {number} z - the zoom level
 * @property {number} tileSize - the size of the tile
 * @property {number} bufferSize - the size of the tile's buffer
 * @example
 * var vt = new mapnik.VectorTile(9,112,195);
 * console.log(vt.z, vt.x, vt.y); // 9, 112, 195
 * console.log(vt.tileSize, vt.bufferSize); // 4096, 128
 */
void VectorTile::Initialize(v8::Local<v8::Object> target)
{
    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(VectorTile::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("VectorTile").ToLocalChecked());
    Nan::SetPrototypeMethod(lcons, "render", render);
    Nan::SetPrototypeMethod(lcons, "setData", setData);
    Nan::SetPrototypeMethod(lcons, "setDataSync", setDataSync);
    Nan::SetPrototypeMethod(lcons, "getData", getData);
    Nan::SetPrototypeMethod(lcons, "getDataSync", getDataSync);
    Nan::SetPrototypeMethod(lcons, "addData", addData);
    Nan::SetPrototypeMethod(lcons, "addDataSync", addDataSync);
    Nan::SetPrototypeMethod(lcons, "composite", composite);
    Nan::SetPrototypeMethod(lcons, "compositeSync", compositeSync);
    Nan::SetPrototypeMethod(lcons, "query", query);
    Nan::SetPrototypeMethod(lcons, "queryMany", queryMany);
    Nan::SetPrototypeMethod(lcons, "extent", extent);
    Nan::SetPrototypeMethod(lcons, "bufferedExtent", bufferedExtent);
    Nan::SetPrototypeMethod(lcons, "names", names);
    Nan::SetPrototypeMethod(lcons, "layer", layer);
    Nan::SetPrototypeMethod(lcons, "emptyLayers", emptyLayers);
    Nan::SetPrototypeMethod(lcons, "paintedLayers", paintedLayers);
    Nan::SetPrototypeMethod(lcons, "toJSON", toJSON);
    Nan::SetPrototypeMethod(lcons, "toGeoJSON", toGeoJSON);
    Nan::SetPrototypeMethod(lcons, "toGeoJSONSync", toGeoJSONSync);
    Nan::SetPrototypeMethod(lcons, "addGeoJSON", addGeoJSON);
    Nan::SetPrototypeMethod(lcons, "addImage", addImage);
    Nan::SetPrototypeMethod(lcons, "addImageSync", addImageSync);
    Nan::SetPrototypeMethod(lcons, "addImageBuffer", addImageBuffer);
    Nan::SetPrototypeMethod(lcons, "addImageBufferSync", addImageBufferSync);
#if BOOST_VERSION >= 105600
    Nan::SetPrototypeMethod(lcons, "reportGeometrySimplicity", reportGeometrySimplicity);
    Nan::SetPrototypeMethod(lcons, "reportGeometrySimplicitySync", reportGeometrySimplicitySync);
    Nan::SetPrototypeMethod(lcons, "reportGeometryValidity", reportGeometryValidity);
    Nan::SetPrototypeMethod(lcons, "reportGeometryValiditySync", reportGeometryValiditySync);
#endif // BOOST_VERSION >= 105600
    Nan::SetPrototypeMethod(lcons, "painted", painted);
    Nan::SetPrototypeMethod(lcons, "clear", clear);
    Nan::SetPrototypeMethod(lcons, "clearSync", clearSync);
    Nan::SetPrototypeMethod(lcons, "empty", empty);

    // properties
    ATTR(lcons, "x", get_tile_x, set_tile_x);
    ATTR(lcons, "y", get_tile_y, set_tile_y);
    ATTR(lcons, "z", get_tile_z, set_tile_z);
    ATTR(lcons, "tileSize", get_tile_size, set_tile_size);
    ATTR(lcons, "bufferSize", get_buffer_size, set_buffer_size);

    Nan::SetMethod(Nan::GetFunction(lcons).ToLocalChecked().As<v8::Object>(), "info", info);
    
    Nan::Set(target, Nan::New("VectorTile").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

VectorTile::VectorTile(std::uint64_t z,
                       std::uint64_t x,
                       std::uint64_t y,
                       std::uint32_t tile_size,
                       std::int32_t buffer_size) :
    Nan::ObjectWrap(),
    tile_(std::make_shared<mapnik::vector_tile_impl::merc_tile>(x, y, z, tile_size, buffer_size))
{
}

// For some reason coverage never seems to be considered here even though
// I have tested it and it does print
/* LCOV_EXCL_START */
VectorTile::~VectorTile()
{
}
/* LCOV_EXCL_STOP */

NAN_METHOD(VectorTile::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = info[0].As<v8::External>();
        void* ptr = ext->Value();
        VectorTile* v =  static_cast<VectorTile*>(ptr);
        v->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }

    if (info.Length() < 3)
    {
        Nan::ThrowError("please provide a z, x, y");
        return;
    }

    if (!info[0]->IsNumber() ||
        !info[1]->IsNumber() ||
        !info[2]->IsNumber())
    {
        Nan::ThrowTypeError("required parameters (z, x, and y) must be a integers");
        return;
    }

    std::int64_t z = Nan::To<int>(info[0]).FromJust();
    std::int64_t x = Nan::To<int>(info[1]).FromJust();
    std::int64_t y = Nan::To<int>(info[2]).FromJust();
    if (z < 0 || x < 0 || y < 0)
    {
        Nan::ThrowTypeError("required parameters (z, x, and y) must be greater then or equal to zero");
        return;
    }
    std::int64_t max_at_zoom = pow(2,z);
    if (x >= max_at_zoom)
    {
        Nan::ThrowTypeError("required parameter x is out of range of possible values based on z value");
        return;
    }
    if (y >= max_at_zoom)
    {
        Nan::ThrowTypeError("required parameter y is out of range of possible values based on z value");
        return;
    }

    std::uint32_t tile_size = 4096;
    std::int32_t buffer_size = 128;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 3)
    {
        if (!info[3]->IsObject())
        {
            Nan::ThrowTypeError("optional fourth argument must be an options object");
            return;
        }
        options = info[3]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("tile_size").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("tile_size").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'tile_size' must be a number");
                return;
            }
            int tile_size_tmp = Nan::To<int>(opt).FromJust();
            if (tile_size_tmp <= 0)
            {
                Nan::ThrowTypeError("optional arg 'tile_size' must be greater then zero");
                return;
            }
            tile_size = tile_size_tmp;
        }
        if (Nan::Has(options, Nan::New("buffer_size").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> opt = Nan::Get(options, Nan::New("buffer_size").ToLocalChecked()).ToLocalChecked();
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }
            buffer_size = Nan::To<int>(opt).FromJust();
        }
    }
    if (static_cast<double>(tile_size) + (2 * buffer_size) <= 0)
    {
        Nan::ThrowError("too large of a negative buffer for tilesize");
        return;
    }

    VectorTile* d = new VectorTile(z, x, y, tile_size, buffer_size);

    d->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
}

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
        v8::Local<v8::Object> options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("area_threshold").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> area_thres = Nan::Get(options, Nan::New("area_threshold").ToLocalChecked()).ToLocalChecked();
            if (!area_thres->IsNumber())
            {
                Nan::ThrowTypeError("option 'area_threshold' must be an floating point number");
                return scope.Escape(Nan::Undefined());
            }
            area_threshold = Nan::To<double>(area_thres).FromJust();
            if (area_threshold < 0.0)
            {
                Nan::ThrowTypeError("option 'area_threshold' can not be negative");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (Nan::Has(options, Nan::New("simplify_distance").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("simplify_distance").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'simplify_distance' must be an floating point number");
                return scope.Escape(Nan::Undefined());
            }
            simplify_distance = Nan::To<double>(param_val).FromJust();
            if (simplify_distance < 0.0)
            {
                Nan::ThrowTypeError("option 'simplify_distance' can not be negative");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (Nan::Has(options, Nan::New("strictly_simple").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> strict_simp = Nan::Get(options, Nan::New("strictly_simple").ToLocalChecked()).ToLocalChecked();
            if (!strict_simp->IsBoolean())
            {
                Nan::ThrowTypeError("option 'strictly_simple' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            strictly_simple = Nan::To<bool>(strict_simp).FromJust();
        }
        if (Nan::Has(options, Nan::New("multi_polygon_union").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> mpu = Nan::Get(options, Nan::New("multi_polygon_union").ToLocalChecked()).ToLocalChecked();
            if (!mpu->IsBoolean())
            {
                Nan::ThrowTypeError("option 'multi_polygon_union' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            multi_polygon_union = Nan::To<bool>(mpu).FromJust();
        }
        if (Nan::Has(options, Nan::New("fill_type").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> ft = Nan::Get(options, Nan::New("fill_type").ToLocalChecked()).ToLocalChecked();
            if (!ft->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'fill_type' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(Nan::To<int>(ft).FromJust());
            if (fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
            {
                Nan::ThrowTypeError("optional arg 'fill_type' out of possible range");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (Nan::Has(options, Nan::New("threading_mode").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("threading_mode").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'threading_mode' must be an unsigned integer");
                return scope.Escape(Nan::Undefined());
            }
            threading_mode = static_cast<std::launch>(Nan::To<int>(param_val).FromJust());
            if (threading_mode != std::launch::async &&
                threading_mode != std::launch::deferred &&
                threading_mode != (std::launch::async | std::launch::deferred))
            {
                Nan::ThrowTypeError("optional arg 'threading_mode' is invalid");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            scale_factor = Nan::To<double>(bind_opt).FromJust();
            if (scale_factor <= 0.0)
            {
                Nan::ThrowTypeError("optional arg 'scale' must be greater then zero");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (Nan::Has(options, Nan::New("scale_denominator").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale_denominator").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            scale_denominator = Nan::To<double>(bind_opt).FromJust();
            if (scale_denominator < 0.0)
            {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be non negative number");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (Nan::Has(options, Nan::New("offset_x").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_x").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            offset_x = Nan::To<int>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("offset_y").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_y").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            offset_y = Nan::To<int>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("reencode").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> reencode_opt = Nan::Get(options, Nan::New("reencode").ToLocalChecked()).ToLocalChecked();
            if (!reencode_opt->IsBoolean())
            {
                Nan::ThrowTypeError("reencode value must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            reencode = Nan::To<bool>(reencode_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("max_extent").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> max_extent_opt = Nan::Get(options, Nan::New("max_extent").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> minx = Nan::Get(bbox, 0).ToLocalChecked();
            v8::Local<v8::Value> miny = Nan::Get(bbox, 1).ToLocalChecked();
            v8::Local<v8::Value> maxx = Nan::Get(bbox, 2).ToLocalChecked();
            v8::Local<v8::Value> maxy = Nan::Get(bbox, 3).ToLocalChecked();
            if (!minx->IsNumber() || !miny->IsNumber() || !maxx->IsNumber() || !maxy->IsNumber())
            {
                Nan::ThrowError("max_extent [minx,miny,maxx,maxy] must be numbers");
                return scope.Escape(Nan::Undefined());
            }
            max_extent = mapnik::box2d<double>(Nan::To<double>(minx).FromJust(),Nan::To<double>(miny).FromJust(),
                                               Nan::To<double>(maxx).FromJust(),Nan::To<double>(maxy).FromJust());
        }
        if (Nan::Has(options, Nan::New("process_all_rings").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("process_all_rings").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'process_all_rings' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            process_all_rings = Nan::To<bool>(param_val).FromJust();
        }

        if (Nan::Has(options, Nan::New("image_scaling").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_scaling").ToLocalChecked()).ToLocalChecked();
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

        if (Nan::Has(options, Nan::New("image_format").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_format").ToLocalChecked()).ToLocalChecked();
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
        v8::Local<v8::Value> val = Nan::Get(vtiles, j).ToLocalChecked();
        if (!val->IsObject())
        {
            Nan::ThrowTypeError("must provide an array of VectorTile objects");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> tile_obj = val->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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
        v8::Local<v8::Object> options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("area_threshold").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> area_thres = Nan::Get(options, Nan::New("area_threshold").ToLocalChecked()).ToLocalChecked();
            if (!area_thres->IsNumber())
            {
                Nan::ThrowTypeError("option 'area_threshold' must be a number");
                return;
            }
            area_threshold = Nan::To<double>(area_thres).FromJust();
            if (area_threshold < 0.0)
            {
                Nan::ThrowTypeError("option 'area_threshold' can not be negative");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("strictly_simple").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> strict_simp = Nan::Get(options, Nan::New("strictly_simple").ToLocalChecked()).ToLocalChecked();
            if (!strict_simp->IsBoolean())
            {
                Nan::ThrowTypeError("strictly_simple value must be a boolean");
                return;
            }
            strictly_simple = Nan::To<bool>(strict_simp).FromJust();
        }
        if (Nan::Has(options, Nan::New("multi_polygon_union").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> mpu = Nan::Get(options, Nan::New("multi_polygon_union").ToLocalChecked()).ToLocalChecked();
            if (!mpu->IsBoolean())
            {
                Nan::ThrowTypeError("multi_polygon_union value must be a boolean");
                return;
            }
            multi_polygon_union = Nan::To<bool>(mpu).FromJust();
        }
        if (Nan::Has(options, Nan::New("fill_type").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> ft = Nan::Get(options, Nan::New("fill_type").ToLocalChecked()).ToLocalChecked();
            if (!ft->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'fill_type' must be a number");
                return;
            }
            fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(Nan::To<int>(ft).FromJust());
            if (fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
            {
                Nan::ThrowTypeError("optional arg 'fill_type' out of possible range");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("threading_mode").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("threading_mode").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'threading_mode' must be an unsigned integer");
                return;
            }
            threading_mode = static_cast<std::launch>(Nan::To<int>(param_val).FromJust());
            if (threading_mode != std::launch::async &&
                threading_mode != std::launch::deferred &&
                threading_mode != (std::launch::async | std::launch::deferred))
            {
                Nan::ThrowTypeError("optional arg 'threading_mode' is not a valid value");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("simplify_distance").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("simplify_distance").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'simplify_distance' must be an floating point number");
                return;
            }
            simplify_distance = Nan::To<double>(param_val).FromJust();
            if (simplify_distance < 0.0)
            {
                Nan::ThrowTypeError("option 'simplify_distance' can not be negative");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return;
            }
            scale_factor = Nan::To<double>(bind_opt).FromJust();
            if (scale_factor < 0.0)
            {
                Nan::ThrowTypeError("option 'scale' can not be negative");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("scale_denominator").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale_denominator").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                return;
            }
            scale_denominator = Nan::To<double>(bind_opt).FromJust();
            if (scale_denominator < 0.0)
            {
                Nan::ThrowTypeError("option 'scale_denominator' can not be negative");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("offset_x").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_x").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
                return;
            }
            offset_x = Nan::To<int>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("offset_y").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("offset_y").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
                return;
            }
            offset_y = Nan::To<int>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("reencode").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> reencode_opt = Nan::Get(options, Nan::New("reencode").ToLocalChecked()).ToLocalChecked();
            if (!reencode_opt->IsBoolean())
            {
                Nan::ThrowTypeError("reencode value must be a boolean");
                return;
            }
            reencode = Nan::To<bool>(reencode_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("max_extent").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> max_extent_opt = Nan::Get(options, Nan::New("max_extent").ToLocalChecked()).ToLocalChecked();
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
            v8::Local<v8::Value> minx = Nan::Get(bbox, 0).ToLocalChecked();
            v8::Local<v8::Value> miny = Nan::Get(bbox, 1).ToLocalChecked();
            v8::Local<v8::Value> maxx = Nan::Get(bbox, 2).ToLocalChecked();
            v8::Local<v8::Value> maxy = Nan::Get(bbox, 3).ToLocalChecked();
            if (!minx->IsNumber() || !miny->IsNumber() || !maxx->IsNumber() || !maxy->IsNumber())
            {
                Nan::ThrowError("max_extent [minx,miny,maxx,maxy] must be numbers");
                return;
            }
            max_extent = mapnik::box2d<double>(Nan::To<double>(minx).FromJust(),Nan::To<double>(miny).FromJust(),
                                               Nan::To<double>(maxx).FromJust(),Nan::To<double>(maxy).FromJust());
        }
        if (Nan::Has(options, Nan::New("process_all_rings").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("process_all_rings").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean()) {
                Nan::ThrowTypeError("option 'process_all_rings' must be a boolean");
                return;
            }
            process_all_rings = Nan::To<bool>(param_val).FromJust();
        }

        if (Nan::Has(options, Nan::New("image_scaling").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_scaling").ToLocalChecked()).ToLocalChecked();
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

        if (Nan::Has(options, Nan::New("image_format").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_format").ToLocalChecked()).ToLocalChecked();
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
        v8::Local<v8::Value> val = Nan::Get(vtiles, j).ToLocalChecked();
        if (!val->IsObject())
        {
            delete closure;
            Nan::ThrowTypeError("must provide an array of VectorTile objects");
            return;
        }
        v8::Local<v8::Object> tile_obj = val->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
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
    Nan::AsyncResource async_resource(__func__);
    vector_tile_composite_baton_t *closure = static_cast<vector_tile_composite_baton_t *>(req->data);

    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->d->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    for (VectorTile* vt : closure->vtiles)
    {
        vt->Unref();
    }
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Get the extent of this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name extent
 * @returns {Array<number>} array of extent in the form of `[minx,miny,maxx,maxy]`
 * @example
 * var vt = new mapnik.VectorTile(9,112,195);
 * var extent = vt.extent();
 * console.log(extent); // [-11271098.44281895, 4696291.017841229, -11192826.925854929, 4774562.534805248]
 */
NAN_METHOD(VectorTile::extent)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
    mapnik::box2d<double> const& e = d->tile_->extent();
    Nan::Set(arr, 0, Nan::New<v8::Number>(e.minx()));
    Nan::Set(arr, 1, Nan::New<v8::Number>(e.miny()));
    Nan::Set(arr, 2, Nan::New<v8::Number>(e.maxx()));
    Nan::Set(arr, 3, Nan::New<v8::Number>(e.maxy()));
    info.GetReturnValue().Set(arr);
    return;
}

/**
 * Get the extent including the buffer of this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name bufferedExtent
 * @returns {Array<number>} extent - `[minx, miny, maxx, maxy]`
 * @example
 * var vt = new mapnik.VectorTile(9,112,195);
 * var extent = vt.bufferedExtent();
 * console.log(extent); // [-11273544.4277, 4693845.0329, -11190380.9409, 4777008.5197];
 */
NAN_METHOD(VectorTile::bufferedExtent)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
    mapnik::box2d<double> e = d->tile_->get_buffered_extent();
    Nan::Set(arr, 0, Nan::New<v8::Number>(e.minx()));
    Nan::Set(arr, 1, Nan::New<v8::Number>(e.miny()));
    Nan::Set(arr, 2, Nan::New<v8::Number>(e.maxx()));
    Nan::Set(arr, 3, Nan::New<v8::Number>(e.maxy()));
    info.GetReturnValue().Set(arr);
    return;
}

/**
 * Get the names of all of the layers in this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name names
 * @returns {Array<string>} layer names
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var data = fs.readFileSync('./path/to/data.mvt');
 * vt.addDataSync(data);
 * console.log(vt.names()); // ['layer-name', 'another-layer']
 */
NAN_METHOD(VectorTile::names)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::vector<std::string> const& names = d->tile_->get_layers();
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(names.size());
    unsigned idx = 0;
    for (std::string const& name : names)
    {
        Nan::Set(arr, idx++,Nan::New<v8::String>(name).ToLocalChecked());
    }
    info.GetReturnValue().Set(arr);
    return;
}

/**
 * Extract the layer by a given name to a new vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name layer
 * @param {string} layer_name - name of layer
 * @returns {mapnik.VectorTile} mapnik VectorTile object
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var data = fs.readFileSync('./path/to/data.mvt');
 * vt.addDataSync(data);
 * console.log(vt.names()); // ['layer-name', 'another-layer']
 * var vt2 = vt.layer('layer-name');
 * console.log(vt2.names()); // ['layer-name']
 */
NAN_METHOD(VectorTile::layer)
{
    if (info.Length() < 1)
    {
        Nan::ThrowError("first argument must be either a layer name");
        return;
    }
    v8::Local<v8::Value> layer_id = info[0];
    std::string layer_name;
    if (!layer_id->IsString())
    {
        Nan::ThrowTypeError("'layer' argument must be a layer name (string)");
        return;
    }
    layer_name = TOSTR(layer_id);
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!d->get_tile()->has_layer(layer_name))
    {
        Nan::ThrowTypeError("layer does not exist in vector tile");
        return;
    }
    VectorTile* v = new VectorTile(d->get_tile()->z(), d->get_tile()->x(), d->get_tile()->y(), d->tile_size(), d->buffer_size());
    protozero::pbf_reader tile_message(d->get_tile()->get_reader());
    while (tile_message.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        auto data_view = tile_message.get_view();
        protozero::pbf_reader layer_message(data_view);
        if (!layer_message.next(mapnik::vector_tile_impl::Layer_Encoding::NAME))
        {
            continue;
        }
        std::string name = layer_message.get_string();
        if (layer_name == name)
        {
            v->get_tile()->append_layer_buffer(data_view.data(), data_view.size(), layer_name);
            break;
        }
    }
    v8::Local<v8::Value> ext = Nan::New<v8::External>(v);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Layer instance");
    else info.GetReturnValue().Set(maybe_local.ToLocalChecked());
    return;
}

/**
 * Get the names of all of the empty layers in this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name emptyLayers
 * @returns {Array<string>} layer names
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var empty = vt.emptyLayers();
 * // assumes you have added data to your tile
 * console.log(empty); // ['layer-name', 'empty-layer']
 */
NAN_METHOD(VectorTile::emptyLayers)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::set<std::string> const& names = d->tile_->get_empty_layers();
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(names.size());
    unsigned idx = 0;
    for (std::string const& name : names)
    {
        Nan::Set(arr, idx++,Nan::New<v8::String>(name).ToLocalChecked());
    }
    info.GetReturnValue().Set(arr);
    return;
}

/**
 * Get the names of all of the painted layers in this vector tile. "Painted" is
 * a check to see if data exists in the source dataset in a tile.
 *
 * @memberof VectorTile
 * @instance
 * @name paintedLayers
 * @returns {Array<string>} layer names
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var painted = vt.paintedLayers();
 * // assumes you have added data to your tile
 * console.log(painted); // ['layer-name']
 */
NAN_METHOD(VectorTile::paintedLayers)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::set<std::string> const& names = d->tile_->get_painted_layers();
    v8::Local<v8::Array> arr = Nan::New<v8::Array>(names.size());
    unsigned idx = 0;
    for (std::string const& name : names)
    {
        Nan::Set(arr, idx++,Nan::New<v8::String>(name).ToLocalChecked());
    }
    info.GetReturnValue().Set(arr);
    return;
}

/**
 * Return whether this vector tile is empty - whether it has no
 * layers and no features
 *
 * @memberof VectorTile
 * @instance
 * @name empty
 * @returns {boolean} whether the layer is empty
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var empty = vt.empty();
 * console.log(empty); // true
 */
NAN_METHOD(VectorTile::empty)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(d->tile_->is_empty()));
}

/**
 * Get whether the vector tile has been painted. "Painted" is
 * a check to see if data exists in the source dataset in a tile.
 *
 * @memberof VectorTile
 * @instance
 * @name painted
 * @returns {boolean} painted
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var painted = vt.painted();
 * console.log(painted); // false
 */
NAN_METHOD(VectorTile::painted)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New(d->tile_->is_painted()));
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
        options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("tolerance").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> tol = Nan::Get(options, Nan::New("tolerance").ToLocalChecked()).ToLocalChecked();
            if (!tol->IsNumber())
            {
                Nan::ThrowTypeError("tolerance value must be a number");
                return;
            }
            tolerance = Nan::To<double>(tol).FromJust();
        }
        if (Nan::Has(options, Nan::New("layer").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> layer_id = Nan::Get(options, Nan::New("layer").ToLocalChecked()).ToLocalChecked();
            if (!layer_id->IsString())
            {
                Nan::ThrowTypeError("layer value must be a string");
                return;
            }
            layer_name = TOSTR(layer_id);
        }
    }

    double lon = Nan::To<double>(info[0]).FromJust();
    double lat = Nan::To<double>(info[1]).FromJust();
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
    Nan::AsyncResource async_resource(__func__);
    vector_tile_query_baton_t *closure = static_cast<vector_tile_query_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        std::vector<query_result> const& result = closure->result;
        v8::Local<v8::Array> arr = _queryResultToV8(result);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), arr };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
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
        v8::Local<v8::Object> feat_obj = feat->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        Nan::Set(feat_obj, Nan::New("layer").ToLocalChecked(),Nan::New<v8::String>(item.layer).ToLocalChecked());
        Nan::Set(feat_obj, Nan::New("distance").ToLocalChecked(),Nan::New<v8::Number>(item.distance));
        Nan::Set(feat_obj, Nan::New("x_hit").ToLocalChecked(),Nan::New<v8::Number>(item.x_hit));
        Nan::Set(feat_obj, Nan::New("y_hit").ToLocalChecked(),Nan::New<v8::Number>(item.y_hit));
        Nan::Set(arr, i++,feat);
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
        v8::Local<v8::Value> item = Nan::Get(queryArray, p).ToLocalChecked();
        if (!item->IsArray())
        {
            Nan::ThrowError("non-array item encountered");
            return;
        }
        v8::Local<v8::Array> pair = v8::Local<v8::Array>::Cast(item);
        v8::Local<v8::Value> lon = Nan::Get(pair, 0).ToLocalChecked();
        v8::Local<v8::Value> lat = Nan::Get(pair, 1).ToLocalChecked();
        if (!lon->IsNumber() || !lat->IsNumber())
        {
            Nan::ThrowError("lng lat must be numbers");
            return;
        }
        query_lonlat lonlat;
        lonlat.lon = Nan::To<double>(lon).FromJust();
        lonlat.lat = Nan::To<double>(lat).FromJust();
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
        options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("tolerance").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> tol = Nan::Get(options, Nan::New("tolerance").ToLocalChecked()).ToLocalChecked();
            if (!tol->IsNumber())
            {
                Nan::ThrowTypeError("tolerance value must be a number");
                return;
            }
            tolerance = Nan::To<double>(tol).FromJust();
        }
        if (Nan::Has(options, Nan::New("layer").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> layer_id = Nan::Get(options, Nan::New("layer").ToLocalChecked()).ToLocalChecked();
            if (!layer_id->IsString())
            {
                Nan::ThrowTypeError("layer value must be a string");
                return;
            }
            layer_name = TOSTR(layer_id);
        }
        if (Nan::Has(options, Nan::New("fields").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("fields").ToLocalChecked()).ToLocalChecked();
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
                v8::Local<v8::Value> name = Nan::Get(a, i).ToLocalChecked();
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
    Nan::Set(results, Nan::New("hits").ToLocalChecked(), hits);
    Nan::Set(results, Nan::New("features").ToLocalChecked(), features);

    // result.features => features
    for (auto const& item : result.features)
    {
        v8::Local<v8::Value> feat = Feature::NewInstance(item.second.feature);
        v8::Local<v8::Object> feat_obj = feat->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        Nan::Set(feat_obj, Nan::New("layer").ToLocalChecked(),Nan::New<v8::String>(item.second.layer).ToLocalChecked());
        Nan::Set(features, item.first, feat_obj);
    }

    // result.hits => hits
    for (auto const& hit : result.hits)
    {
        v8::Local<v8::Array> point_hits = Nan::New<v8::Array>(hit.second.size());
        std::size_t i = 0;
        for (auto const& h : hit.second)
        {
            v8::Local<v8::Object> hit_obj = Nan::New<v8::Object>();
            Nan::Set(hit_obj, Nan::New("distance").ToLocalChecked(), Nan::New<v8::Number>(h.distance));
            Nan::Set(hit_obj, Nan::New("feature_id").ToLocalChecked(), Nan::New<v8::Number>(h.feature_id));
            Nan::Set(point_hits, i++, hit_obj);
        }
        Nan::Set(hits, hit.first, point_hits);
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
    Nan::AsyncResource async_resource(__func__);
    vector_tile_queryMany_baton_t *closure = static_cast<vector_tile_queryMany_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        queryMany_result result = closure->result;
        v8::Local<v8::Object> obj = _queryManyResultToV8(result);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), obj };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

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
        Nan::Set(arr, 0, Nan::New<v8::Number>(geom.x));
        Nan::Set(arr, 1, Nan::New<v8::Number>(geom.y));
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
            Nan::Set(arr, c++, (*this)(pt));
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
            Nan::Set(arr, c++, (*this)(pt));
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
            Nan::Set(arr, c++, (*this)(pt));
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
            Nan::Set(arr, c++, (*this)(pt));
        }
        return scope.Escape(arr);
    }

    template <typename T>
    v8::Local<v8::Array> operator() (mapnik::geometry::polygon<T> const & poly)
    {
        Nan::EscapableHandleScope scope;
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(poly.size());
        std::uint32_t index = 0;

        for (auto const & ring : poly)
        {
            Nan::Set(arr, index++, (*this)(ring));
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
            Nan::Set(arr, c++, (*this)(pt));
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
            Nan::Set(arr, c++, (*this)(pt));
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
        Nan::Set(att_obj_, Nan::New(name_).ToLocalChecked(), Nan::New(val).ToLocalChecked());
    }

    void operator() (bool const& val)
    {
        Nan::Set(att_obj_, Nan::New(name_).ToLocalChecked(), Nan::New<v8::Boolean>(val));
    }

    void operator() (int64_t const& val)
    {
        Nan::Set(att_obj_, Nan::New(name_).ToLocalChecked(), Nan::New<v8::Number>(val));
    }

    void operator() (uint64_t const& val)
    {
        // LCOV_EXCL_START
        Nan::Set(att_obj_, Nan::New(name_).ToLocalChecked(), Nan::New<v8::Number>(val));
        // LCOV_EXCL_STOP
    }

    void operator() (double const& val)
    {
        Nan::Set(att_obj_, Nan::New(name_).ToLocalChecked(), Nan::New<v8::Number>(val));
    }

    void operator() (float const& val)
    {
        Nan::Set(att_obj_, Nan::New(name_).ToLocalChecked(), Nan::New<v8::Number>(val));
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
        v8::Local<v8::Object> options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::Has(options, Nan::New("decode_geometry").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("decode_geometry").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'decode_geometry' must be a boolean");
                return;
            }
            decode_geometry = Nan::To<bool>(param_val).FromJust();
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
                        Nan::Set(layer_obj, Nan::New("name").ToLocalChecked(), Nan::New<v8::String>(layer_msg.get_string()).ToLocalChecked());
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
                        Nan::Set(layer_obj, Nan::New("extent").ToLocalChecked(), Nan::New<v8::Integer>(layer_msg.get_uint32()));
                        break;
                    case mapnik::vector_tile_impl::Layer_Encoding::VERSION:
                        version = layer_msg.get_uint32();
                        Nan::Set(layer_obj, Nan::New("version").ToLocalChecked(), Nan::New<v8::Integer>(version));
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
                            Nan::Set(feature_obj, Nan::New("id").ToLocalChecked(),Nan::New<v8::Number>(feature_msg.get_uint64()));
                            break;
                        case mapnik::vector_tile_impl::Feature_Encoding::TAGS:
                            tag_itr = feature_msg.get_packed_uint32();
                            has_tags = true;
                            break;
                        case mapnik::vector_tile_impl::Feature_Encoding::TYPE:
                            geom_type_enum = feature_msg.get_enum();
                            has_geom_type = true;
                            Nan::Set(feature_obj, Nan::New("type").ToLocalChecked(),Nan::New<v8::Integer>(geom_type_enum));
                            break;
                        case mapnik::vector_tile_impl::Feature_Encoding::GEOMETRY:
                            geom_itr = feature_msg.get_packed_uint32();
                            has_geom = true;
                            break;
                        case mapnik::vector_tile_impl::Feature_Encoding::RASTER:
                        {
                            auto im_buffer = feature_msg.get_view();
                            Nan::Set(feature_obj, Nan::New("raster").ToLocalChecked(),
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
                Nan::Set(feature_obj, Nan::New("properties").ToLocalChecked(), att_obj);
                if (has_geom && has_geom_type)
                {
                    if (decode_geometry)
                    {
                        // Decode the geometry first into an int64_t mapnik geometry
                        mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
                        mapnik::geometry::geometry<std::int64_t> geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, geom_type_enum, version, 0, 0, 1.0, 1.0);
                        v8::Local<v8::Array> g_arr = geometry_to_array<std::int64_t>(geom);
                        Nan::Set(feature_obj, Nan::New("geometry").ToLocalChecked(), g_arr);
                        std::string geom_type = geometry_type_as_string(geom);
                        Nan::Set(feature_obj, Nan::New("geometry_type").ToLocalChecked(), Nan::New(geom_type).ToLocalChecked());
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
                            Nan::Set(g_arr, k, Nan::New<v8::Number>(geom_vec[k]));
                        }
                        Nan::Set(feature_obj, Nan::New("geometry").ToLocalChecked(), g_arr);
                    }
                }
                Nan::Set(f_arr, f_idx++, feature_obj);
            }
            Nan::Set(layer_obj, Nan::New("features").ToLocalChecked(), f_arr);
            Nan::Set(arr, l_idx++, layer_obj);
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

bool layer_to_geojson(protozero::pbf_reader const& layer,
                      std::string & result,
                      unsigned x,
                      unsigned y,
                      unsigned z)
{
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer, x, y, z);
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,wgs84);
    // This mega box ensures we capture all features, including those
    // outside the tile extent. Geometries outside the tile extent are
    // likely when the vtile was created by clipping to a buffered extent
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
    bool first = true;
    if (fs && mapnik::is_valid(fs))
    {
        mapnik::feature_ptr feature;
        while ((feature = fs->next()))
        {
            if (first)
            {
                first = false;
            }
            else
            {
                result += "\n,";
            }
            std::string feature_str;
            mapnik::feature_impl feature_new(feature->context(),feature->id());
            feature_new.set_data(feature->get_data());
            unsigned int n_err = 0;
            feature_new.set_geometry(mapnik::geometry::reproject_copy(feature->get_geometry(), prj_trans, n_err));
            if (!mapnik::util::to_geojson(feature_str, feature_new))
            {
                // LCOV_EXCL_START
                throw std::runtime_error("Failed to generate GeoJSON geometry");
                // LCOV_EXCL_STOP
            }
            result += feature_str;
        }
    }
    return !first;
}

/**
 * Syncronous version of {@link VectorTile}
 *
 * @memberof VectorTile
 * @instance
 * @name toGeoJSONSync
 * @param {string | number} [layer=__all__] Can be a zero-index integer representing
 * a layer or the string keywords `__array__` or `__all__` to get all layers in the form
 * of an array of GeoJSON `FeatureCollection`s or in the form of a single GeoJSON
 * `FeatureCollection` with all layers smooshed inside
 * @returns {string} stringified GeoJSON of all the features in this tile.
 * @example
 * var geojson = vectorTile.toGeoJSONSync('__all__');
 * geojson // stringified GeoJSON
 * JSON.parse(geojson); // GeoJSON object
 */
NAN_METHOD(VectorTile::toGeoJSONSync)
{
    info.GetReturnValue().Set(_toGeoJSONSync(info));
}

void write_geojson_array(std::string & result,
                         VectorTile * v)
{
    protozero::pbf_reader tile_msg = v->get_tile()->get_reader();
    result += "[";
    bool first = true;
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        if (first)
        {
            first = false;
        }
        else
        {
            result += ",";
        }
        auto data_view = tile_msg.get_view();
        protozero::pbf_reader layer_msg(data_view);
        protozero::pbf_reader name_msg(data_view);
        std::string layer_name;
        if (name_msg.next(mapnik::vector_tile_impl::Layer_Encoding::NAME))
        {
            layer_name = name_msg.get_string();
        }
        result += "{\"type\":\"FeatureCollection\",";
        result += "\"name\":\"" + layer_name + "\",\"features\":[";
        std::string features;
        bool hit = layer_to_geojson(layer_msg,
                                    features,
                                    v->get_tile()->x(),
                                    v->get_tile()->y(),
                                    v->get_tile()->z());
        if (hit)
        {
            result += features;
        }
        result += "]}";
    }
    result += "]";
}

void write_geojson_all(std::string & result,
                       VectorTile * v)
{
    protozero::pbf_reader tile_msg = v->get_tile()->get_reader();
    result += "{\"type\":\"FeatureCollection\",\"features\":[";
    bool first = true;
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        protozero::pbf_reader layer_msg(tile_msg.get_message());
        std::string features;
        bool hit = layer_to_geojson(layer_msg,
                                    features,
                                    v->get_tile()->x(),
                                    v->get_tile()->y(),
                                    v->get_tile()->z());
        if (hit)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                result += ",";
            }
            result += features;
        }
    }
    result += "]}";
}

bool write_geojson_layer_index(std::string & result,
                               std::size_t layer_idx,
                               VectorTile * v)
{
    protozero::pbf_reader layer_msg;
    if (v->get_tile()->layer_reader(layer_idx, layer_msg) &&
        v->get_tile()->get_layers().size() > layer_idx)
    {
        std::string layer_name = v->get_tile()->get_layers()[layer_idx];
        result += "{\"type\":\"FeatureCollection\",";
        result += "\"name\":\"" + layer_name + "\",\"features\":[";
        layer_to_geojson(layer_msg,
                         result,
                         v->get_tile()->x(),
                         v->get_tile()->y(),
                         v->get_tile()->z());
        result += "]}";
        return true;
    }
    // LCOV_EXCL_START
    return false;
    // LCOV_EXCL_STOP
}

bool write_geojson_layer_name(std::string & result,
                              std::string const& name,
                              VectorTile * v)
{
    protozero::pbf_reader layer_msg;
    if (v->get_tile()->layer_reader(name, layer_msg))
    {
        result += "{\"type\":\"FeatureCollection\",";
        result += "\"name\":\"" + name + "\",\"features\":[";
        layer_to_geojson(layer_msg,
                         result,
                         v->get_tile()->x(),
                         v->get_tile()->y(),
                         v->get_tile()->z());
        result += "]}";
        return true;
    }
    return false;
}

v8::Local<v8::Value> VectorTile::_toGeoJSONSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    if (info.Length() < 1)
    {
        Nan::ThrowError("first argument must be either a layer name (string) or layer index (integer)");
        return scope.Escape(Nan::Undefined());
    }
    v8::Local<v8::Value> layer_id = info[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) )
    {
        Nan::ThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        return scope.Escape(Nan::Undefined());
    }

    VectorTile* v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::string result;
    try
    {
        if (layer_id->IsString())
        {
            std::string layer_name = TOSTR(layer_id);
            if (layer_name == "__array__")
            {
                write_geojson_array(result, v);
            }
            else if (layer_name == "__all__")
            {
                write_geojson_all(result, v);
            }
            else
            {
                if (!write_geojson_layer_name(result, layer_name, v))
                {
                    std::string error_msg("Layer name '" + layer_name + "' not found");
                    Nan::ThrowTypeError(error_msg.c_str());
                    return scope.Escape(Nan::Undefined());
                }
            }
        }
        else if (layer_id->IsNumber())
        {
            int layer_idx = Nan::To<int>(layer_id).FromJust();
            if (layer_idx < 0)
            {
                Nan::ThrowTypeError("A layer index can not be negative");
                return scope.Escape(Nan::Undefined());
            }
            else if (layer_idx >= static_cast<int>(v->get_tile()->get_layers().size()))
            {
                Nan::ThrowTypeError("Layer index exceeds the number of layers in the vector tile.");
                return scope.Escape(Nan::Undefined());
            }
            if (!write_geojson_layer_index(result, layer_idx, v))
            {
                // LCOV_EXCL_START
                Nan::ThrowTypeError("Layer could not be retrieved (should have not reached here)");
                return scope.Escape(Nan::Undefined());
                // LCOV_EXCL_STOP
            }
        }
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        Nan::ThrowTypeError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
    return scope.Escape(Nan::New<v8::String>(result).ToLocalChecked());
}

enum geojson_write_type : std::uint8_t
{
    geojson_write_all = 0,
    geojson_write_array,
    geojson_write_layer_name,
    geojson_write_layer_index
};

struct to_geojson_baton
{
    uv_work_t request;
    VectorTile* v;
    bool error;
    std::string result;
    geojson_write_type type;
    int layer_idx;
    std::string layer_name;
    Nan::Persistent<v8::Function> cb;
};

/**
 * Get a [GeoJSON](http://geojson.org/) representation of this tile
 *
 * @memberof VectorTile
 * @instance
 * @name toGeoJSON
 * @param {string | number} [layer=__all__] Can be a zero-index integer representing
 * a layer or the string keywords `__array__` or `__all__` to get all layers in the form
 * of an array of GeoJSON `FeatureCollection`s or in the form of a single GeoJSON
 * `FeatureCollection` with all layers smooshed inside
 * @param {Function} callback - `function(err, geojson)`: a stringified
 * GeoJSON of all the features in this tile
 * @example
 * vectorTile.toGeoJSON('__all__',function(err, geojson) {
 *   if (err) throw err;
 *   console.log(geojson); // stringified GeoJSON
 *   console.log(JSON.parse(geojson)); // GeoJSON object
 * });
 */
NAN_METHOD(VectorTile::toGeoJSON)
{
    if ((info.Length() < 1) || !info[info.Length()-1]->IsFunction())
    {
        info.GetReturnValue().Set(_toGeoJSONSync(info));
        return;
    }
    to_geojson_baton *closure = new to_geojson_baton();
    closure->request.data = closure;
    closure->v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    closure->error = false;
    closure->layer_idx = 0;
    closure->type = geojson_write_all;

    v8::Local<v8::Value> layer_id = info[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) )
    {
        delete closure;
        Nan::ThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        return;
    }

    if (layer_id->IsString())
    {
        std::string layer_name = TOSTR(layer_id);
        if (layer_name == "__array__")
        {
            closure->type = geojson_write_array;
        }
        else if (layer_name == "__all__")
        {
            closure->type = geojson_write_all;
        }
        else
        {
            if (!closure->v->get_tile()->has_layer(layer_name))
            {
                delete closure;
                std::string error_msg("The layer does not contain the name: " + layer_name);
                Nan::ThrowTypeError(error_msg.c_str());
                return;
            }
            closure->layer_name = layer_name;
            closure->type = geojson_write_layer_name;
        }
    }
    else if (layer_id->IsNumber())
    {
        closure->layer_idx = Nan::To<int>(layer_id).FromJust();
        if (closure->layer_idx < 0)
        {
            delete closure;
            Nan::ThrowTypeError("A layer index can not be negative");
            return;
        }
        else if (closure->layer_idx >= static_cast<int>(closure->v->get_tile()->get_layers().size()))
        {
            delete closure;
            Nan::ThrowTypeError("Layer index exceeds the number of layers in the vector tile.");
            return;
        }
        closure->type = geojson_write_layer_index;
    }

    v8::Local<v8::Value> callback = info[info.Length()-1];
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, to_geojson, (uv_after_work_cb)after_to_geojson);
    closure->v->Ref();
    return;
}

void VectorTile::to_geojson(uv_work_t* req)
{
    to_geojson_baton *closure = static_cast<to_geojson_baton *>(req->data);
    try
    {
        switch (closure->type)
        {
            default:
            case geojson_write_all:
                write_geojson_all(closure->result, closure->v);
                break;
            case geojson_write_array:
                write_geojson_array(closure->result, closure->v);
                break;
            case geojson_write_layer_name:
                write_geojson_layer_name(closure->result, closure->layer_name, closure->v);
                break;
            case geojson_write_layer_index:
                write_geojson_layer_index(closure->result, closure->layer_idx, closure->v);
                break;
        }
    }
    catch (std::exception const& ex)
    {
        // There are currently no known ways to trigger this exception in testing. If it was
        // triggered this would likely be a bug in either mapnik or mapnik-vector-tile.
        // LCOV_EXCL_START
        closure->error = true;
        closure->result = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::after_to_geojson(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    to_geojson_baton *closure = static_cast<to_geojson_baton *>(req->data);
    if (closure->error)
    {
        // Because there are no known ways to trigger the exception path in to_geojson
        // there is no easy way to test this path currently
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->result.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::New<v8::String>(closure->result).ToLocalChecked() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->v->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Add features to this tile from a GeoJSON string. GeoJSON coordinates must be in the WGS84 longitude & latitude CRS
 * as specified in the [GeoJSON Specification](https://www.rfc-editor.org/rfc/rfc7946.txt).
 *
 * @memberof VectorTile
 * @instance
 * @name addGeoJSON
 * @param {string} geojson as a string
 * @param {string} name of the layer to be added
 * @param {Object} [options]
 * @param {number} [options.area_threshold=0.1] used to discard small polygons.
 * If a value is greater than `0` it will trigger polygons with an area smaller
 * than the value to be discarded. Measured in grid integers, not spherical mercator
 * coordinates.
 * @param {number} [options.simplify_distance=0.0] Simplification works to generalize
 * geometries before encoding into vector tiles.simplification distance The
 * `simplify_distance` value works in integer space over a 4096 pixel grid and uses
 * the [Douglas-Peucker algorithm](https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm).
 * @param {boolean} [options.strictly_simple=true] ensure all geometry is valid according to
 * OGC Simple definition
 * @param {boolean} [options.multi_polygon_union=false] union all multipolygons
 * @param {Object<mapnik.polygonFillType>} [options.fill_type=mapnik.polygonFillType.positive]
 * the fill type used in determining what are holes and what are outer rings. See the
 * [Clipper documentation](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/PolyFillType.htm)
 * to learn more about fill types.
 * @param {boolean} [options.process_all_rings=false] if `true`, don't assume winding order and ring order of
 * polygons are correct according to the [`2.0` Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec)
 * @example
 * var geojson = { ... };
 * var vt = mapnik.VectorTile(0,0,0);
 * vt.addGeoJSON(JSON.stringify(geojson), 'layer-name', {});
 */
NAN_METHOD(VectorTile::addGeoJSON)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsString())
    {
        Nan::ThrowError("first argument must be a GeoJSON string");
        return;
    }
    if (info.Length() < 2 || !info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return;
    }
    std::string geojson_string = TOSTR(info[0]);
    std::string geojson_name = TOSTR(info[1]);

    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    double area_threshold = 0.1;
    double simplify_distance = 0.0;
    bool strictly_simple = true;
    bool multi_polygon_union = false;
    mapnik::vector_tile_impl::polygon_fill_type fill_type = mapnik::vector_tile_impl::positive_fill;
    bool process_all_rings = false;

    if (info.Length() > 2)
    {
        // options object
        if (!info[2]->IsObject())
        {
            Nan::ThrowError("optional third argument must be an options object");
            return;
        }

        options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("area_threshold").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("area_threshold").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsNumber())
            {
                Nan::ThrowError("option 'area_threshold' must be a number");
                return;
            }
            area_threshold = Nan::To<int>(param_val).FromJust();
            if (area_threshold < 0.0)
            {
                Nan::ThrowError("option 'area_threshold' can not be negative");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("strictly_simple").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("strictly_simple").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'strictly_simple' must be a boolean");
                return;
            }
            strictly_simple = Nan::To<bool>(param_val).FromJust();
        }
        if (Nan::Has(options, Nan::New("multi_polygon_union").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> mpu = Nan::Get(options, Nan::New("multi_polygon_union").ToLocalChecked()).ToLocalChecked();
            if (!mpu->IsBoolean())
            {
                Nan::ThrowTypeError("multi_polygon_union value must be a boolean");
                return;
            }
            multi_polygon_union = Nan::To<bool>(mpu).FromJust();
        }
        if (Nan::Has(options, Nan::New("fill_type").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> ft = Nan::Get(options, Nan::New("fill_type").ToLocalChecked()).ToLocalChecked();
            if (!ft->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'fill_type' must be a number");
                return;
            }
            fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(Nan::To<int>(ft).FromJust());
            if (fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
            {
                Nan::ThrowTypeError("optional arg 'fill_type' out of possible range");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("simplify_distance").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("simplify_distance").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'simplify_distance' must be an floating point number");
                return;
            }
            simplify_distance = Nan::To<double>(param_val).FromJust();
            if (simplify_distance < 0.0)
            {
                Nan::ThrowTypeError("option 'simplify_distance' must be a positive number");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("process_all_rings").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("process_all_rings").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'process_all_rings' must be a boolean");
                return;
            }
            process_all_rings = Nan::To<bool>(param_val).FromJust();
        }
    }

    try
    {
        // create map object
        mapnik::Map map(d->tile_size(),d->tile_size(),"+init=epsg:3857");
        mapnik::parameters p;
        p["type"]="geojson";
        p["inline"]=geojson_string;
        mapnik::layer lyr(geojson_name,"+init=epsg:4326");
        lyr.set_datasource(mapnik::datasource_cache::instance().create(p));
        map.add_layer(lyr);

        mapnik::vector_tile_impl::processor ren(map);
        ren.set_area_threshold(area_threshold);
        ren.set_strictly_simple(strictly_simple);
        ren.set_simplify_distance(simplify_distance);
        ren.set_multi_polygon_union(multi_polygon_union);
        ren.set_fill_type(fill_type);
        ren.set_process_all_rings(process_all_rings);
        ren.update_tile(*d->get_tile());
        info.GetReturnValue().Set(Nan::True());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
}

/**
 * Add a {@link Image} as a tile layer (synchronous)
 *
 * @memberof VectorTile
 * @instance
 * @name addImageSync
 * @param {mapnik.Image} image
 * @param {string} name of the layer to be added
 * @param {Object} options
 * @param {string} [options.image_scaling=bilinear] can be any
 * of the <mapnik.imageScaling> methods
 * @param {string} [options.image_format=webp] or `jpeg`, `png`, `tiff`
 * @example
 * var vt = new mapnik.VectorTile(1, 0, 0, {
 *   tile_size:256
 * });
 * var im = new mapnik.Image(256, 256);
 * vt.addImageSync(im, 'layer-name', {
 *   image_format: 'jpeg',
 *   image_scaling: 'gaussian'
 * });
 */
NAN_METHOD(VectorTile::addImageSync)
{
    info.GetReturnValue().Set(_addImageSync(info));
}

v8::Local<v8::Value> VectorTile::_addImageSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowError("first argument must be an Image object");
        return scope.Escape(Nan::Undefined());
    }
    if (info.Length() < 2 || !info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return scope.Escape(Nan::Undefined());
    }
    std::string layer_name = TOSTR(info[1]);
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() ||
        obj->IsUndefined() ||
        !Nan::New(Image::constructor)->HasInstance(obj))
    {
        Nan::ThrowError("first argument must be an Image object");
        return scope.Escape(Nan::Undefined());
    }
    Image *im = Nan::ObjectWrap::Unwrap<Image>(obj);
    if (im->get()->width() <= 0 || im->get()->height() <= 0)
    {
        Nan::ThrowError("Image width and height must be greater then zero");
        return scope.Escape(Nan::Undefined());
    }

    std::string image_format = "webp";
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_BILINEAR;

    if (info.Length() > 2)
    {
        // options object
        if (!info[2]->IsObject())
        {
            Nan::ThrowError("optional third argument must be an options object");
            return scope.Escape(Nan::Undefined());
        }

        v8::Local<v8::Object> options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("image_scaling").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_scaling").ToLocalChecked()).ToLocalChecked();
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

        if (Nan::Has(options, Nan::New("image_format").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_format").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsString()) 
            {
                Nan::ThrowTypeError("option 'image_format' must be a string");
                return scope.Escape(Nan::Undefined());
            }
            image_format = TOSTR(param_val);
        }
    }
    mapnik::image_any im_copy = *im->get();
    std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(mapnik::parameters());
    mapnik::raster_ptr ras = std::make_shared<mapnik::raster>(d->get_tile()->extent(), im_copy, 1.0);
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    feature->set_raster(ras);
    ds->push(feature);
    ds->envelope(); // can be removed later, currently doesn't work with out this.
    ds->set_envelope(d->get_tile()->extent());
    try
    {
        // create map object
        mapnik::Map map(d->tile_size(),d->tile_size(),"+init=epsg:3857");
        mapnik::layer lyr(layer_name,"+init=epsg:3857");
        lyr.set_datasource(ds);
        map.add_layer(lyr);

        mapnik::vector_tile_impl::processor ren(map);
        ren.set_scaling_method(scaling_method);
        ren.set_image_format(image_format);
        ren.update_tile(*d->get_tile());
        info.GetReturnValue().Set(Nan::True());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    Image* im;
    std::string layer_name;
    std::string image_format;
    mapnik::scaling_method_e scaling_method;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_add_image_baton_t;

/**
 * Add a <mapnik.Image> as a tile layer (asynchronous)
 *
 * @memberof VectorTile
 * @instance
 * @name addImage
 * @param {mapnik.Image} image
 * @param {string} name of the layer to be added
 * @param {Object} [options]
 * @param {string} [options.image_scaling=bilinear] can be any
 * of the <mapnik.imageScaling> methods
 * @param {string} [options.image_format=webp] other options include `jpeg`, `png`, `tiff`
 * @example
 * var vt = new mapnik.VectorTile(1, 0, 0, {
 *   tile_size:256
 * });
 * var im = new mapnik.Image(256, 256);
 * vt.addImage(im, 'layer-name', {
 *   image_format: 'jpeg',
 *   image_scaling: 'gaussian'
 * }, function(err) {
 *   if (err) throw err;
 *   // your custom code using `vt`
 * });
 */
NAN_METHOD(VectorTile::addImage)
{
    // If last param is not a function assume sync
    if (info.Length() < 2)
    {
        Nan::ThrowError("addImage requires at least two parameters: an Image and a layer name");
        return;
    }
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length() - 1]->IsFunction())
    {
        info.GetReturnValue().Set(_addImageSync(info));
        return;
    }
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.This());
    if (!info[0]->IsObject())
    {
        Nan::ThrowError("first argument must be an Image object");
        return;
    }
    if (!info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return;
    }
    std::string layer_name = TOSTR(info[1]);
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() ||
        obj->IsUndefined() ||
        !Nan::New(Image::constructor)->HasInstance(obj))
    {
        Nan::ThrowError("first argument must be an Image object");
        return;
    }
    Image *im = Nan::ObjectWrap::Unwrap<Image>(obj);
    if (im->get()->width() <= 0 || im->get()->height() <= 0)
    {
        Nan::ThrowError("Image width and height must be greater then zero");
        return;
    }

    std::string image_format = "webp";
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_BILINEAR;

    if (info.Length() > 3)
    {
        // options object
        if (!info[2]->IsObject())
        {
            Nan::ThrowError("optional third argument must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("image_scaling").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_scaling").ToLocalChecked()).ToLocalChecked();
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

        if (Nan::Has(options, Nan::New("image_format").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("image_format").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsString()) 
            {
                Nan::ThrowTypeError("option 'image_format' must be a string");
                return;
            }
            image_format = TOSTR(param_val);
        }
    }
    vector_tile_add_image_baton_t *closure = new vector_tile_add_image_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->im = im;
    closure->scaling_method = scaling_method;
    closure->image_format = image_format;
    closure->layer_name = layer_name;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_AddImage, (uv_after_work_cb)EIO_AfterAddImage);
    d->Ref();
    im->Ref();
    return;
}

void VectorTile::EIO_AddImage(uv_work_t* req)
{
    vector_tile_add_image_baton_t *closure = static_cast<vector_tile_add_image_baton_t *>(req->data);

    try
    {
        mapnik::image_any im_copy = *closure->im->get();
        std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(mapnik::parameters());
        mapnik::raster_ptr ras = std::make_shared<mapnik::raster>(closure->d->get_tile()->extent(), im_copy, 1.0);
        mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
        feature->set_raster(ras);
        ds->push(feature);
        ds->envelope(); // can be removed later, currently doesn't work with out this.
        ds->set_envelope(closure->d->get_tile()->extent());
        // create map object
        mapnik::Map map(closure->d->tile_size(),closure->d->tile_size(),"+init=epsg:3857");
        mapnik::layer lyr(closure->layer_name,"+init=epsg:3857");
        lyr.set_datasource(ds);
        map.add_layer(lyr);

        mapnik::vector_tile_impl::processor ren(map);
        ren.set_scaling_method(closure->scaling_method);
        ren.set_image_format(closure->image_format);
        ren.update_tile(*closure->d->get_tile());
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterAddImage(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    vector_tile_add_image_baton_t *closure = static_cast<vector_tile_add_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }

    closure->d->Unref();
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Add raw image buffer as a new tile layer (synchronous)
 *
 * @memberof VectorTile
 * @instance
 * @name addImageBufferSync
 * @param {Buffer} buffer - raw data
 * @param {string} name - name of the layer to be added
 * @example
 * var vt = new mapnik.VectorTile(1, 0, 0, {
 *   tile_size: 256
 * });
 * var image_buffer = fs.readFileSync('./path/to/image.jpg');
 * vt.addImageBufferSync(image_buffer, 'layer-name');
 */
NAN_METHOD(VectorTile::addImageBufferSync)
{
    info.GetReturnValue().Set(_addImageBufferSync(info));
}

v8::Local<v8::Value> VectorTile::_addImageBufferSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    if (info.Length() < 2 || !info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return scope.Escape(Nan::Undefined());
    }
    std::string layer_name = TOSTR(info[1]);
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        Nan::ThrowError("cannot accept empty buffer as protobuf");
        return scope.Escape(Nan::Undefined());
    }
    try
    {
        add_image_buffer_as_tile_layer(*d->get_tile(), layer_name, node::Buffer::Data(obj), buffer_size);
    }
    catch (std::exception const& ex)
    {
        // no obvious way to get this to throw in JS under obvious conditions
        // but keep the standard exeption cache in C++
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    const char *data;
    size_t dataLength;
    std::string layer_name;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    Nan::Persistent<v8::Object> buffer;
} vector_tile_addimagebuffer_baton_t;


/**
 * Add an encoded image buffer as a layer
 *
 * @memberof VectorTile
 * @instance
 * @name addImageBuffer
 * @param {Buffer} buffer - raw image data
 * @param {string} name - name of the layer to be added
 * @param {Function} callback
 * @example
 * var vt = new mapnik.VectorTile(1, 0, 0, {
 *   tile_size: 256
 * });
 * var image_buffer = fs.readFileSync('./path/to/image.jpg'); // returns a buffer
 * vt.addImageBufferSync(image_buffer, 'layer-name', function(err) {
 *   if (err) throw err;
 *   // your custom code
 * });
 */
NAN_METHOD(VectorTile::addImageBuffer)
{
    if (info.Length() < 3)
    {
        info.GetReturnValue().Set(_addImageBufferSync(info));
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length() - 1]->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return;
    }
    if (info.Length() < 2 || !info[1]->IsString())
    {
        Nan::ThrowError("second argument must be a layer name (string)");
        return;
    }
    std::string layer_name = TOSTR(info[1]);
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return;
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    vector_tile_addimagebuffer_baton_t *closure = new vector_tile_addimagebuffer_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->layer_name = layer_name;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_AddImageBuffer, (uv_after_work_cb)EIO_AfterAddImageBuffer);
    d->Ref();
    return;
}

void VectorTile::EIO_AddImageBuffer(uv_work_t* req)
{
    vector_tile_addimagebuffer_baton_t *closure = static_cast<vector_tile_addimagebuffer_baton_t *>(req->data);

    try
    {
        add_image_buffer_as_tile_layer(*closure->d->get_tile(), closure->layer_name, closure->data, closure->dataLength);
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::EIO_AfterAddImageBuffer(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    vector_tile_addimagebuffer_baton_t *closure = static_cast<vector_tile_addimagebuffer_baton_t *>(req->data);
    if (closure->error)
    {
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Add raw data to this tile as a Buffer
 *
 * @memberof VectorTile
 * @instance
 * @name addDataSync
 * @param {Buffer} buffer - raw data
 * @param {object} [options]
 * @param {boolean} [options.validate=false] - If true does validity checks mvt schema (not geometries)
 * Will throw if anything invalid or unexpected is encountered in the data
 * @param {boolean} [options.upgrade=false] - If true will upgrade v1 tiles to adhere to the v2 specification
 * @example
 * var data_buffer = fs.readFileSync('./path/to/data.mvt'); // returns a buffer
 * // assumes you have created a vector tile object already
 * vt.addDataSync(data_buffer);
 * // your custom code
 */
NAN_METHOD(VectorTile::addDataSync)
{
    info.GetReturnValue().Set(_addDataSync(info));
}

v8::Local<v8::Value> VectorTile::_addDataSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        Nan::ThrowError("cannot accept empty buffer as protobuf");
        return scope.Escape(Nan::Undefined());
    }
    bool upgrade = false;
    bool validate = false;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 1)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("second arg must be a options object");
            return scope.Escape(Nan::Undefined());
        }
        options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New<v8::String>("validate").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("validate").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'validate' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            validate = Nan::To<bool>(param_val).FromJust();
        }
        if (Nan::Has(options, Nan::New<v8::String>("upgrade").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("upgrade").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'upgrade' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            upgrade = Nan::To<bool>(param_val).FromJust();
        }
    }
    try
    {
        merge_from_compressed_buffer(*d->get_tile(), node::Buffer::Data(obj), buffer_size, validate, upgrade);
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    const char *data;
    bool validate;
    bool upgrade;
    size_t dataLength;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    Nan::Persistent<v8::Object> buffer;
} vector_tile_adddata_baton_t;


/**
 * Add new vector tile data to an existing vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name addData
 * @param {Buffer} buffer - raw vector data
 * @param {object} [options]
 * @param {boolean} [options.validate=false] - If true does validity checks mvt schema (not geometries)
 * Will throw if anything invalid or unexpected is encountered in the data
 * @param {boolean} [options.upgrade=false] - If true will upgrade v1 tiles to adhere to the v2 specification
 * @param {Object} callback
 * @example
 * var data_buffer = fs.readFileSync('./path/to/data.mvt'); // returns a buffer
 * var vt = new mapnik.VectorTile(9,112,195);
 * vt.addData(data_buffer, function(err) {
 *   if (err) throw err;
 *   // your custom code
 * });
 */
NAN_METHOD(VectorTile::addData)
{
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length() - 1]->IsFunction())
    {
        info.GetReturnValue().Set(_addDataSync(info));
        return;
    }

    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return;
    }

    bool upgrade = false;
    bool validate = false;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 1)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("second arg must be a options object");
            return;
        }
        options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New<v8::String>("validate").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("validate").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'validate' must be a boolean");
                return;
            }
            validate = Nan::To<bool>(param_val).FromJust();
        }
        if (Nan::Has(options, Nan::New<v8::String>("upgrade").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("upgrade").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'upgrade' must be a boolean");
                return;
            }
            upgrade = Nan::To<bool>(param_val).FromJust();
        }
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    vector_tile_adddata_baton_t *closure = new vector_tile_adddata_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->validate = validate;
    closure->upgrade = upgrade;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_AddData, (uv_after_work_cb)EIO_AfterAddData);
    d->Ref();
    return;
}

void VectorTile::EIO_AddData(uv_work_t* req)
{
    vector_tile_adddata_baton_t *closure = static_cast<vector_tile_adddata_baton_t *>(req->data);

    if (closure->dataLength <= 0)
    {
        closure->error = true;
        closure->error_name = "cannot accept empty buffer as protobuf";
        return;
    }
    try
    {
        merge_from_compressed_buffer(*closure->d->get_tile(), closure->data, closure->dataLength, closure->validate, closure->upgrade);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterAddData(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    vector_tile_adddata_baton_t *closure = static_cast<vector_tile_adddata_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Replace the data in this vector tile with new raw data (synchronous). This function validates
 * geometry according to the [Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec).
 *
 * @memberof VectorTile
 * @instance
 * @name setDataSync
 * @param {Buffer} buffer - raw data
 * @param {object} [options]
 * @param {boolean} [options.validate=false] - If true does validity checks mvt schema (not geometries)
 * Will throw if anything invalid or unexpected is encountered in the data
 * @param {boolean} [options.upgrade=false] - If true will upgrade v1 tiles to adhere to the v2 specification
 * @example
 * var data = fs.readFileSync('./path/to/data.mvt');
 * vectorTile.setDataSync(data);
 * // your custom code
 */
NAN_METHOD(VectorTile::setDataSync)
{
    info.GetReturnValue().Set(_setDataSync(info));
}

v8::Local<v8::Value> VectorTile::_setDataSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        Nan::ThrowError("cannot accept empty buffer as protobuf");
        return scope.Escape(Nan::Undefined());
    }
    bool upgrade = false;
    bool validate = false;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 1)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("second arg must be a options object");
            return scope.Escape(Nan::Undefined());
        }
        options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New<v8::String>("validate").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("validate").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'validate' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            validate = Nan::To<bool>(param_val).FromJust();
        }
        if (Nan::Has(options, Nan::New<v8::String>("upgrade").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("upgrade").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'upgrade' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            upgrade = Nan::To<bool>(param_val).FromJust();
        }
    }
    try
    {
        d->clear();
        merge_from_compressed_buffer(*d->get_tile(), node::Buffer::Data(obj), buffer_size, validate, upgrade);
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    const char *data;
    bool validate;
    bool upgrade;
    size_t dataLength;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    Nan::Persistent<v8::Object> buffer;
} vector_tile_setdata_baton_t;


/**
 * Replace the data in this vector tile with new raw data
 *
 * @memberof VectorTile
 * @instance
 * @name setData
 * @param {Buffer} buffer - raw data
 * @param {object} [options]
 * @param {boolean} [options.validate=false] - If true does validity checks mvt schema (not geometries)
 * Will throw if anything invalid or unexpected is encountered in the data
 * @param {boolean} [options.upgrade=false] - If true will upgrade v1 tiles to adhere to the v2 specification
 * @param {Function} callback
 * @example
 * var data = fs.readFileSync('./path/to/data.mvt');
 * vectorTile.setData(data, function(err) {
 *   if (err) throw err;
 *   // your custom code
 * });
 */
NAN_METHOD(VectorTile::setData)
{
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length() - 1]->IsFunction())
    {
        info.GetReturnValue().Set(_setDataSync(info));
        return;
    }

    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return;
    }

    bool upgrade = false;
    bool validate = false;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() > 1)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("second arg must be a options object");
            return;
        }
        options = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New<v8::String>("validate").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("validate").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'validate' must be a boolean");
                return;
            }
            validate = Nan::To<bool>(param_val).FromJust();
        }
        if (Nan::Has(options, Nan::New<v8::String>("upgrade").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("upgrade").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'upgrade' must be a boolean");
                return;
            }
            upgrade = Nan::To<bool>(param_val).FromJust();
        }
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    vector_tile_setdata_baton_t *closure = new vector_tile_setdata_baton_t();
    closure->request.data = closure;
    closure->validate = validate;
    closure->upgrade = upgrade;
    closure->d = d;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    closure->buffer.Reset(obj.As<v8::Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_SetData, (uv_after_work_cb)EIO_AfterSetData);
    d->Ref();
    return;
}

void VectorTile::EIO_SetData(uv_work_t* req)
{
    vector_tile_setdata_baton_t *closure = static_cast<vector_tile_setdata_baton_t *>(req->data);

    if (closure->dataLength <= 0)
    {
        closure->error = true;
        closure->error_name = "cannot accept empty buffer as protobuf";
        return;
    }

    try
    {
        closure->d->clear();
        merge_from_compressed_buffer(*closure->d->get_tile(), closure->data, closure->dataLength, closure->validate, closure->upgrade);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterSetData(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    vector_tile_setdata_baton_t *closure = static_cast<vector_tile_setdata_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Get the data in this vector tile as a buffer (synchronous)
 *
 * @memberof VectorTile
 * @instance
 * @name getDataSync
 * @param {Object} [options]
 * @param {string} [options.compression=none] - can also be `gzip`
 * @param {boolean} [options.release=false] releases VT buffer
 * @param {int} [options.level=0] a number `0` (no compression) to `9` (best compression)
 * @param {string} options.strategy must be `FILTERED`, `HUFFMAN_ONLY`, `RLE`, `FIXED`, `DEFAULT`
 * @returns {Buffer} raw data
 * @example
 * var data = vt.getData({
 *   compression: 'gzip',
 *   level: 9,
 *   strategy: 'FILTERED'
 * });
 */
NAN_METHOD(VectorTile::getDataSync)
{
    info.GetReturnValue().Set(_getDataSync(info));
}

v8::Local<v8::Value> VectorTile::_getDataSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    bool compress = false;
    bool release = false;
    int level = Z_DEFAULT_COMPRESSION;
    int strategy = Z_DEFAULT_STRATEGY;

    v8::Local<v8::Object> options = Nan::New<v8::Object>();

    if (info.Length() > 0)
    {
        if (!info[0]->IsObject())
        {
            Nan::ThrowTypeError("first arg must be a options object");
            return scope.Escape(Nan::Undefined());
        }

        options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::Has(options, Nan::New<v8::String>("compression").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("compression").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'compression' must be a string, either 'gzip', or 'none' (default)");
                return scope.Escape(Nan::Undefined());
            }
            compress = std::string("gzip") == (TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()));
        }
        if (Nan::Has(options, Nan::New<v8::String>("release").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("release").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'release' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            release = Nan::To<bool>(param_val).FromJust();
        }
        if (Nan::Has(options, Nan::New<v8::String>("level").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("level").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return scope.Escape(Nan::Undefined());
            }
            level = Nan::To<int>(param_val).FromJust();
            if (level < 0 || level > 9)
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (Nan::Has(options, Nan::New<v8::String>("strategy").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("strategy").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return scope.Escape(Nan::Undefined());
            }
            else if (std::string("FILTERED") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_FILTERED;
            }
            else if (std::string("HUFFMAN_ONLY") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_HUFFMAN_ONLY;
            }
            else if (std::string("RLE") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_RLE;
            }
            else if (std::string("FIXED") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_FIXED;
            }
            else if (std::string("DEFAULT") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_DEFAULT_STRATEGY;
            }
            else
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return scope.Escape(Nan::Undefined());
            }
        }
    }

    try
    {
        std::size_t raw_size = d->tile_->size();
        if (raw_size <= 0)
        {
            return scope.Escape(Nan::NewBuffer(0).ToLocalChecked());
        }
        else
        {
            if (raw_size >= node::Buffer::kMaxLength) {
                // This is a valid test path, but I am excluding it from test coverage due to the
                // requirement of loading a very large object in memory in order to test it.
                // LCOV_EXCL_START
                std::ostringstream s;
                s << "Data is too large to convert to a node::Buffer ";
                s << "(" << raw_size << " raw bytes >= node::Buffer::kMaxLength)";
                Nan::ThrowTypeError(s.str().c_str());
                return scope.Escape(Nan::Undefined());
                // LCOV_EXCL_STOP
            }
            if (!compress)
            {
                if (release)
                {
                    return scope.Escape(node_mapnik::NewBufferFrom(d->tile_->release_buffer()).ToLocalChecked());
                }
                else
                {
                    return scope.Escape(Nan::CopyBuffer((char*)d->tile_->data(),raw_size).ToLocalChecked());
                }
            }
            else
            {
                std::unique_ptr<std::string> compressed = std::make_unique<std::string>();
                mapnik::vector_tile_impl::zlib_compress(d->tile_->data(), raw_size, *compressed, true, level, strategy);
                if (release)
                {
                    // To keep the same behaviour as a non compression release, we want to clear the VT buffer
                    d->tile_->clear();
                }
                return scope.Escape(node_mapnik::NewBufferFrom(std::move(compressed)).ToLocalChecked());
            }
        }
    }
    catch (std::exception const& ex)
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        Nan::ThrowTypeError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    bool error;
    std::unique_ptr<std::string> data;
    bool compress;
    bool release;
    int level;
    int strategy;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_get_data_baton_t;

/**
 * Get the data in this vector tile as a buffer (asynchronous)
 * @memberof VectorTile
 * @instance
 * @name getData
 * @param {Object} [options]
 * @param {string} [options.compression=none] compression type can also be `gzip`
 * @param {boolean} [options.release=false] releases VT buffer
 * @param {int} [options.level=0] a number `0` (no compression) to `9` (best compression)
 * @param {string} options.strategy must be `FILTERED`, `HUFFMAN_ONLY`, `RLE`, `FIXED`, `DEFAULT`
 * @param {Function} callback
 * @example
 * vt.getData({
 *   compression: 'gzip',
 *   level: 9,
 *   strategy: 'FILTERED'
 * }, function(err, data) {
 *   if (err) throw err;
 *   console.log(data); // buffer
 * });
 */
NAN_METHOD(VectorTile::getData)
{
    if (info.Length() == 0 || !info[info.Length()-1]->IsFunction())
    {
        info.GetReturnValue().Set(_getDataSync(info));
        return;
    }

    v8::Local<v8::Value> callback = info[info.Length()-1];
    bool compress = false;
    bool release = false;
    int level = Z_DEFAULT_COMPRESSION;
    int strategy = Z_DEFAULT_STRATEGY;

    v8::Local<v8::Object> options = Nan::New<v8::Object>();

    if (info.Length() > 1)
    {
        if (!info[0]->IsObject())
        {
            Nan::ThrowTypeError("first arg must be a options object");
            return;
        }

        options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::Has(options, Nan::New("compression").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("compression").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'compression' must be a string, either 'gzip', or 'none' (default)");
                return;
            }
            compress = std::string("gzip") == (TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()));
        }
        if (Nan::Has(options, Nan::New("release").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("release").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowTypeError("option 'release' must be a boolean");
                return;
            }
            release = Nan::To<bool>(param_val).FromJust();
        }
        if (Nan::Has(options, Nan::New("level").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("level").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return;
            }
            level = Nan::To<int>(param_val).FromJust();
            if (level < 0 || level > 9)
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("strategy").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("strategy").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return;
            }
            else if (std::string("FILTERED") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_FILTERED;
            }
            else if (std::string("HUFFMAN_ONLY") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_HUFFMAN_ONLY;
            }
            else if (std::string("RLE") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_RLE;
            }
            else if (std::string("FIXED") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_FIXED;
            }
            else if (std::string("DEFAULT") == TOSTR(param_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()))
            {
                strategy = Z_DEFAULT_STRATEGY;
            }
            else
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return;
            }
        }
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    vector_tile_get_data_baton_t *closure = new vector_tile_get_data_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->compress = compress;
    closure->release = release;
    closure->data = std::make_unique<std::string>();
    closure->level = level;
    closure->strategy = strategy;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, get_data, (uv_after_work_cb)after_get_data);
    d->Ref();
    return;
}

void VectorTile::get_data(uv_work_t* req)
{
    vector_tile_get_data_baton_t *closure = static_cast<vector_tile_get_data_baton_t *>(req->data);
    try
    {
        // compress if requested
        if (closure->compress)
        {
            mapnik::vector_tile_impl::zlib_compress(closure->d->tile_->data(), closure->d->tile_->size(), *(closure->data), true, closure->level, closure->strategy);
        }
    }
    catch (std::exception const& ex)
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::after_get_data(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    vector_tile_get_data_baton_t *closure = static_cast<vector_tile_get_data_baton_t *>(req->data);
    if (closure->error)
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else if (!closure->data->empty())
    {
        if (closure->release)
        {
            // To keep the same behaviour as a non compression release, we want to clear the VT buffer
            closure->d->tile_->clear();
        }
        v8::Local<v8::Value> argv[2] = { Nan::Null(), 
                                         node_mapnik::NewBufferFrom(std::move(closure->data)).ToLocalChecked() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    else
    {
        std::size_t raw_size = closure->d->tile_->size();
        if (raw_size <= 0)
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::NewBuffer(0).ToLocalChecked() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
        else if (raw_size >= node::Buffer::kMaxLength)
        {
            // This is a valid test path, but I am excluding it from test coverage due to the
            // requirement of loading a very large object in memory in order to test it.
            // LCOV_EXCL_START
            std::ostringstream s;
            s << "Data is too large to convert to a node::Buffer ";
            s << "(" << raw_size << " raw bytes >= node::Buffer::kMaxLength)";
            v8::Local<v8::Value> argv[1] = { Nan::Error(s.str().c_str()) };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
            // LCOV_EXCL_STOP
        }
        else
        {
            if (closure->release)
            {
                v8::Local<v8::Value> argv[2] = { Nan::Null(), 
                                                 node_mapnik::NewBufferFrom(closure->d->tile_->release_buffer()).ToLocalChecked() };
                async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
            }
            else
            {
                v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::CopyBuffer((char*)closure->d->tile_->data(),raw_size).ToLocalChecked() };
                async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
            }
        }
    }

    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

struct dummy_surface {};

using surface_type = mapnik::util::variant
    <dummy_surface,
     Image *,
     CairoSurface *
#if defined(GRID_RENDERER)
     ,Grid *
#endif
     >;

struct ref_visitor
{
    void operator() (dummy_surface) {} // no-op
    template <typename SurfaceType>
    void operator() (SurfaceType * surface)
    {
        if (surface != nullptr)
        {
            surface->Ref();
        }
    }
};


struct deref_visitor
{
    void operator() (dummy_surface) {} // no-op
    template <typename SurfaceType>
    void operator() (SurfaceType * surface)
    {
        if (surface != nullptr)
        {
            surface->Unref();
        }
    }
};

struct vector_tile_render_baton_t
{
    uv_work_t request;
    Map* m;
    VectorTile * d;
    surface_type surface;
    mapnik::attributes variables;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    std::string result;
    std::size_t layer_idx;
    std::int64_t z;
    std::int64_t x;
    std::int64_t y;
    unsigned width;
    unsigned height;
    int buffer_size;
    double scale_factor;
    double scale_denominator;
    bool use_cairo;
    bool zxy_override;
    bool error;
    vector_tile_render_baton_t() :
        request(),
        m(nullptr),
        d(nullptr),
        surface(),
        variables(),
        error_name(),
        cb(),
        result(),
        layer_idx(0),
        z(0),
        x(0),
        y(0),
        width(0),
        height(0),
        buffer_size(0),
        scale_factor(1.0),
        scale_denominator(0.0),
        use_cairo(true),
        zxy_override(false),
        error(false)
        {}
};

struct baton_guard
{
    baton_guard(vector_tile_render_baton_t * baton) :
      baton_(baton),
      released_(false) {}

    ~baton_guard()
    {
        if (!released_) delete baton_;
    }

    void release()
    {
        released_ = true;
    }

    vector_tile_render_baton_t * baton_;
    bool released_;
};

/**
 * Render/write this vector tile to a surface/image, like a {@link Image}
 *
 * @name render
 * @memberof VectorTile
 * @instance
 * @param {mapnik.Map} map - mapnik map object
 * @param {mapnik.Image} surface - renderable surface object
 * @param {Object} [options]
 * @param {number} [options.z] an integer zoom level. Must be used with `x` and `y`
 * @param {number} [options.x] an integer x coordinate. Must be used with `y` and `z`.
 * @param {number} [options.y] an integer y coordinate. Must be used with `x` and `z`
 * @param {number} [options.buffer_size] the size of the tile's buffer
 * @param {number} [options.scale] floating point scale factor size to used
 * for rendering
 * @param {number} [options.scale_denominator] An floating point `scale_denominator`
 * to be used by Mapnik when matching zoom filters. If provided this overrides the
 * auto-calculated scale_denominator that is based on the map dimensions and bbox.
 * Do not set this option unless you know what it means.
 * @param {Object} [options.variables] Mapnik 3.x ONLY: A javascript object
 * containing key value pairs that should be passed into Mapnik as variables
 * for rendering and for datasource queries. For example if you passed
 * `vtile.render(map,image,{ variables : {zoom:1} },cb)` then the `@zoom`
 * variable would be usable in Mapnik symbolizers like `line-width:"@zoom"`
 * and as a token in Mapnik postgis sql sub-selects like
 * `(select * from table where some_field > @zoom)` as tmp
 * @param {string} [options.renderer] must be `cairo` or `svg`
 * @param {string|number} [options.layer] option required for grid rendering
 * and must be either a layer name (string) or layer index (integer)
 * @param {Array<string>} [options.fields] must be an array of strings
 * @param {Function} callback
 * @example
 * var vt = new mapnik.VectorTile(0,0,0);
 * var tileSize = vt.tileSize;
 * var map = new mapnik.Map(tileSize, tileSize);
 * vt.render(map, new mapnik.Image(256,256), function(err, image) {
 *   if (err) throw err;
 *   // save the rendered image to an existing image file somewhere
 *   // see mapnik.Image for available methods
 *   image.save('./path/to/image/file.png', 'png32');
 * });
 */
NAN_METHOD(VectorTile::render)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("mapnik.Map expected as first arg");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Map::constructor)->HasInstance(obj))
    {
        Nan::ThrowTypeError("mapnik.Map expected as first arg");
        return;
    }

    Map *m = Nan::ObjectWrap::Unwrap<Map>(obj);
    if (info.Length() < 2 || !info[1]->IsObject())
    {
        Nan::ThrowTypeError("a renderable mapnik object is expected as second arg");
        return;
    }
    v8::Local<v8::Object> im_obj = info[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    vector_tile_render_baton_t *closure = new vector_tile_render_baton_t();
    baton_guard guard(closure);
    v8::Local<v8::Object> options = Nan::New<v8::Object>();

    if (info.Length() > 2)
    {
        bool set_x = false;
        bool set_y = false;
        bool set_z = false;
        if (!info[2]->IsObject())
        {
            Nan::ThrowTypeError("optional third argument must be an options object");
            return;
        }
        options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("z").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("z").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'z' must be a number");
                return;
            }
            closure->z = Nan::To<int>(bind_opt).FromJust();
            set_z = true;
            closure->zxy_override = true;
        }
        if (Nan::Has(options, Nan::New("x").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("x").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'x' must be a number");
                return;
            }
            closure->x = Nan::To<int>(bind_opt).FromJust();
            set_x = true;
            closure->zxy_override = true;
        }
        if (Nan::Has(options, Nan::New("y").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("y").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'y' must be a number");
                return;
            }
            closure->y = Nan::To<int>(bind_opt).FromJust();
            set_y = true;
            closure->zxy_override = true;
        }

        if (closure->zxy_override)
        {
            if (!set_z || !set_x || !set_y)
            {
                Nan::ThrowTypeError("original args 'z', 'x', and 'y' must all be used together");
                return;
            }
            if (closure->x < 0 || closure->y < 0 || closure->z < 0)
            {
                Nan::ThrowTypeError("original args 'z', 'x', and 'y' can not be negative");
                return;
            }
            std::int64_t max_at_zoom = pow(2,closure->z);
            if (closure->x >= max_at_zoom)
            {
                Nan::ThrowTypeError("required parameter x is out of range of possible values based on z value");
                return;
            }
            if (closure->y >= max_at_zoom)
            {
                Nan::ThrowTypeError("required parameter y is out of range of possible values based on z value");
                return;
            }
        }

        if (Nan::Has(options, Nan::New("buffer_size").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("buffer_size").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }
            closure->buffer_size = Nan::To<int>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("scale").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return;
            }
            closure->scale_factor = Nan::To<double>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("scale_denominator").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("scale_denominator").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                return;
            }
            closure->scale_denominator = Nan::To<double>(bind_opt).FromJust();
        }
        if (Nan::Has(options, Nan::New("variables").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("variables").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsObject())
            {
                Nan::ThrowTypeError("optional arg 'variables' must be an object");
                return;
            }
            object_to_container(closure->variables,bind_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked());
        }
    }

    closure->layer_idx = 0;
    if (Nan::New(Image::constructor)->HasInstance(im_obj))
    {
        Image *im = Nan::ObjectWrap::Unwrap<Image>(im_obj);
        closure->width = im->get()->width();
        closure->height = im->get()->height();
        closure->surface = im;
    }
    else if (Nan::New(CairoSurface::constructor)->HasInstance(im_obj))
    {
        CairoSurface *c = Nan::ObjectWrap::Unwrap<CairoSurface>(im_obj);
        closure->width = c->width();
        closure->height = c->height();
        closure->surface = c;
        if (Nan::Has(options, Nan::New("renderer").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> renderer = Nan::Get(options, Nan::New("renderer").ToLocalChecked()).ToLocalChecked();
            if (!renderer->IsString() )
            {
                Nan::ThrowError("'renderer' option must be a string of either 'svg' or 'cairo'");
                return;
            }
            std::string renderer_name = TOSTR(renderer);
            if (renderer_name == "cairo")
            {
                closure->use_cairo = true;
            }
            else if (renderer_name == "svg")
            {
                closure->use_cairo = false;
            }
            else
            {
                Nan::ThrowError("'renderer' option must be a string of either 'svg' or 'cairo'");
                return;
            }
        }
    }
#if defined(GRID_RENDERER)
    else if (Nan::New(Grid::constructor)->HasInstance(im_obj))
    {
        Grid *g = Nan::ObjectWrap::Unwrap<Grid>(im_obj);
        closure->width = g->get()->width();
        closure->height = g->get()->height();
        closure->surface = g;

        std::size_t layer_idx = 0;

        // grid requires special options for now
        if (!Nan::Has(options, Nan::New("layer").ToLocalChecked()).FromMaybe(false))
        {
            Nan::ThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
            return;
        }
        else
        {
            std::vector<mapnik::layer> const& layers = m->get()->layers();
            v8::Local<v8::Value> layer_id = Nan::Get(options, Nan::New("layer").ToLocalChecked()).ToLocalChecked();
            if (layer_id->IsString())
            {
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
                    Nan::ThrowTypeError(s.str().c_str());
                    return;
                }
            }
            else if (layer_id->IsNumber())
            {
                layer_idx = Nan::To<int>(layer_id).FromJust();
                std::size_t layer_num = layers.size();
                if (layer_idx >= layer_num)
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
                    Nan::ThrowTypeError(s.str().c_str());
                    return;
                }
            }
            else
            {
                Nan::ThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
                return;
            }
        }
        if (Nan::Has(options, Nan::New("fields").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("fields").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsArray())
            {
                Nan::ThrowTypeError("option 'fields' must be an array of strings");
                return;
            }
            v8::Local<v8::Array> a = v8::Local<v8::Array>::Cast(param_val);
            unsigned int i = 0;
            unsigned int num_fields = a->Length();
            while (i < num_fields)
            {
                v8::Local<v8::Value> name = Nan::Get(a, i).ToLocalChecked();
                if (name->IsString())
                {
                    g->get()->add_field(TOSTR(name));
                }
                ++i;
            }
        }
        closure->layer_idx = layer_idx;
    }
#endif
    else
    {
        Nan::ThrowTypeError("renderable mapnik object expected as second arg");
        return;
    }
    closure->request.data = closure;
    closure->d = d;
    closure->m = m;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderTile, (uv_after_work_cb)EIO_AfterRenderTile);
    mapnik::util::apply_visitor(ref_visitor(), closure->surface);
    m->Ref();
    d->Ref();
    guard.release();
    return;
}

template <typename Renderer> void process_layers(Renderer & ren,
                                            mapnik::request const& m_req,
                                            mapnik::projection const& map_proj,
                                            std::vector<mapnik::layer> const& layers,
                                            double scale_denom,
                                            std::string const& map_srs,
                                            vector_tile_render_baton_t *closure)
{
    for (auto const& lyr : layers)
    {
        if (lyr.visible(scale_denom))
        {
            protozero::pbf_reader layer_msg;
            if (closure->d->get_tile()->layer_reader(lyr.name(), layer_msg))
            {
                mapnik::layer lyr_copy(lyr);
                lyr_copy.set_srs(map_srs);
                std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                                mapnik::vector_tile_impl::tile_datasource_pbf>(
                                                    layer_msg,
                                                    closure->d->get_tile()->x(),
                                                    closure->d->get_tile()->y(),
                                                    closure->d->get_tile()->z());
                ds->set_envelope(m_req.get_buffered_extent());
                lyr_copy.set_datasource(ds);
                std::set<std::string> names;
                ren.apply_to_layer(lyr_copy,
                                   ren,
                                   map_proj,
                                   m_req.scale(),
                                   scale_denom,
                                   m_req.width(),
                                   m_req.height(),
                                   m_req.extent(),
                                   m_req.buffer_size(),
                                   names);
            }
        }
    }
}

void VectorTile::EIO_RenderTile(uv_work_t* req)
{
    vector_tile_render_baton_t *closure = static_cast<vector_tile_render_baton_t *>(req->data);

    try
    {
        mapnik::Map const& map_in = *closure->m->get();
        mapnik::box2d<double> map_extent;
        if (closure->zxy_override)
        {
            map_extent = mapnik::vector_tile_impl::tile_mercator_bbox(closure->x,closure->y,closure->z);
        }
        else
        {
            map_extent = mapnik::vector_tile_impl::tile_mercator_bbox(closure->d->get_tile()->x(),
                                                                      closure->d->get_tile()->y(),
                                                                      closure->d->get_tile()->z());
        }
        mapnik::request m_req(closure->width, closure->height, map_extent);
        m_req.set_buffer_size(closure->buffer_size);
        mapnik::projection map_proj(map_in.srs(),true);
        double scale_denom = closure->scale_denominator;
        if (scale_denom <= 0.0)
        {
            scale_denom = mapnik::scale_denominator(m_req.scale(),map_proj.is_geographic());
        }
        scale_denom *= closure->scale_factor;
        std::vector<mapnik::layer> const& layers = map_in.layers();
#if defined(GRID_RENDERER)
        // render grid for layer
        if (closure->surface.is<Grid *>())
        {
            Grid * g = mapnik::util::get<Grid *>(closure->surface);
            mapnik::grid_renderer<mapnik::grid> ren(map_in,
                                                    m_req,
                                                    closure->variables,
                                                    *(g->get()),
                                                    closure->scale_factor);
            ren.start_map_processing(map_in);

            mapnik::layer const& lyr = layers[closure->layer_idx];
            if (lyr.visible(scale_denom))
            {
                protozero::pbf_reader layer_msg;
                if (closure->d->get_tile()->layer_reader(lyr.name(),layer_msg))
                {
                    // copy field names
                    std::set<std::string> attributes = g->get()->get_fields();

                    // todo - make this a static constant
                    std::string known_id_key = "__id__";
                    if (attributes.find(known_id_key) != attributes.end())
                    {
                        attributes.erase(known_id_key);
                    }
                    std::string join_field = g->get()->get_key();
                    if (known_id_key != join_field &&
                        attributes.find(join_field) == attributes.end())
                    {
                        attributes.insert(join_field);
                    }

                    mapnik::layer lyr_copy(lyr);
                    lyr_copy.set_srs(map_in.srs());
                    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                                        layer_msg,
                                                        closure->d->get_tile()->x(),
                                                        closure->d->get_tile()->y(),
                                                        closure->d->get_tile()->z());
                    ds->set_envelope(m_req.get_buffered_extent());
                    lyr_copy.set_datasource(ds);
                    ren.apply_to_layer(lyr_copy,
                                       ren,
                                       map_proj,
                                       m_req.scale(),
                                       scale_denom,
                                       m_req.width(),
                                       m_req.height(),
                                       m_req.extent(),
                                       m_req.buffer_size(),
                                       attributes);
                }
                ren.end_map_processing(map_in);
            }
        }
        else
#endif
        if (closure->surface.is<CairoSurface *>())
        {
            CairoSurface * c = mapnik::util::get<CairoSurface *>(closure->surface);
            if (closure->use_cairo)
            {
#if defined(HAVE_CAIRO)
                mapnik::cairo_surface_ptr surface;
                // TODO - support any surface type
                surface = mapnik::cairo_surface_ptr(cairo_svg_surface_create_for_stream(
                                                       (cairo_write_func_t)c->write_callback,
                                                       (void*)(&c->ss_),
                                                       static_cast<double>(c->width()),
                                                       static_cast<double>(c->height())
                                                    ),mapnik::cairo_surface_closer());
                mapnik::cairo_ptr c_context = mapnik::create_context(surface);
                mapnik::cairo_renderer<mapnik::cairo_ptr> ren(map_in,m_req,
                                                                closure->variables,
                                                                c_context,closure->scale_factor);
                ren.start_map_processing(map_in);
                process_layers(ren,m_req,map_proj,layers,scale_denom,map_in.srs(),closure);
                ren.end_map_processing(map_in);
#else
                closure->error = true;
                closure->error_name = "no support for rendering svg with cairo backend";
#endif
            }
            else
            {
#if defined(SVG_RENDERER)
                typedef mapnik::svg_renderer<std::ostream_iterator<char> > svg_ren;
                std::ostream_iterator<char> output_stream_iterator(c->ss_);
                svg_ren ren(map_in, m_req,
                            closure->variables,
                            output_stream_iterator, closure->scale_factor);
                ren.start_map_processing(map_in);
                process_layers(ren,m_req,map_proj,layers,scale_denom,map_in.srs(),closure);
                ren.end_map_processing(map_in);
#else
                closure->error = true;
                closure->error_name = "no support for rendering svg with native svg backend (-DSVG_RENDERER)";
#endif
            }
        }
        // render all layers with agg
        else if (closure->surface.is<Image *>())
        {
            Image * js_image = mapnik::util::get<Image *>(closure->surface);
            mapnik::image_any & im = *(js_image->get());
            if (im.is<mapnik::image_rgba8>())
            {
                mapnik::image_rgba8 & im_data = mapnik::util::get<mapnik::image_rgba8>(im);
                mapnik::agg_renderer<mapnik::image_rgba8> ren(map_in,m_req,
                                                        closure->variables,
                                                        im_data,closure->scale_factor);
                ren.start_map_processing(map_in);
                process_layers(ren,m_req,map_proj,layers,scale_denom,map_in.srs(),closure);
                ren.end_map_processing(map_in);
            }
            else
            {
                throw std::runtime_error("This image type is not currently supported for rendering.");
            }
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterRenderTile(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    vector_tile_render_baton_t *closure = static_cast<vector_tile_render_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        if (closure->surface.is<Image *>())
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), mapnik::util::get<Image *>(closure->surface)->handle() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
#if defined(GRID_RENDERER)
        else if (closure->surface.is<Grid *>())
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), mapnik::util::get<Grid *>(closure->surface)->handle() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
#endif
        else if (closure->surface.is<CairoSurface *>())
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), mapnik::util::get<CairoSurface *>(closure->surface)->handle() };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }

    mapnik::util::apply_visitor(deref_visitor(), closure->surface);
    closure->m->Unref();
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Remove all data from this vector tile (synchronously)
 * @name clearSync
 * @memberof VectorTile
 * @instance
 * @example
 * vt.clearSync();
 * console.log(vt.getData().length); // 0
 */
NAN_METHOD(VectorTile::clearSync)
{
    info.GetReturnValue().Set(_clearSync(info));
}

v8::Local<v8::Value> VectorTile::_clearSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    d->clear();
    return scope.Escape(Nan::Undefined());
}

typedef struct
{
    uv_work_t request;
    VectorTile* d;
    std::string format;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} clear_vector_tile_baton_t;

/**
 * Remove all data from this vector tile
 *
 * @memberof VectorTile
 * @instance
 * @name clear
 * @param {Function} callback
 * @example
 * vt.clear(function(err) {
 *   if (err) throw err;
 *   console.log(vt.getData().length); // 0
 * });
 */
NAN_METHOD(VectorTile::clear)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    if (info.Length() == 0)
    {
        info.GetReturnValue().Set(_clearSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }
    clear_vector_tile_baton_t *closure = new clear_vector_tile_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Clear, (uv_after_work_cb)EIO_AfterClear);
    d->Ref();
    return;
}

void VectorTile::EIO_Clear(uv_work_t* req)
{
    clear_vector_tile_baton_t *closure = static_cast<clear_vector_tile_baton_t *>(req->data);
    try
    {
        closure->d->clear();
    }
    catch(std::exception const& ex)
    {
        // No reason this should ever throw an exception, not currently testable.
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::EIO_AfterClear(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    clear_vector_tile_baton_t *closure = static_cast<clear_vector_tile_baton_t *>(req->data);
    if (closure->error)
    {
        // No reason this should ever throw an exception, not currently testable.
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

#if BOOST_VERSION >= 105800

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
               std::vector<not_simple_feature> & errors)
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
            if (!mapnik::geometry::is_simple(feature->get_geometry()))
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

void vector_tile_not_simple(VectorTile * v,
                            std::vector<not_simple_feature> & errors)
{
    protozero::pbf_reader tile_msg(v->get_tile()->get_reader());
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        protozero::pbf_reader layer_msg(tile_msg.get_message());
        layer_not_simple(layer_msg,
                         v->get_tile()->x(),
                         v->get_tile()->y(),
                         v->get_tile()->z(),
                         errors);
    }
}

v8::Local<v8::Array> make_not_simple_array(std::vector<not_simple_feature> & errors)
{
    Nan::EscapableHandleScope scope;
    v8::Local<v8::Array> array = Nan::New<v8::Array>(errors.size());
    v8::Local<v8::String> layer_key = Nan::New<v8::String>("layer").ToLocalChecked();
    v8::Local<v8::String> feature_id_key = Nan::New<v8::String>("featureId").ToLocalChecked();
    std::uint32_t idx = 0;
    for (auto const& error : errors)
    {
        // LCOV_EXCL_START
        v8::Local<v8::Object> obj = Nan::New<v8::Object>();
        Nan::Set(obj, layer_key, Nan::New<v8::String>(error.layer).ToLocalChecked());
        Nan::Set(obj, feature_id_key, Nan::New<v8::Number>(error.feature_id));
        Nan::Set(array, idx++, obj);
        // LCOV_EXCL_STOP
    }
    return scope.Escape(array);
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
        Nan::Set(obj, layer_key, Nan::New<v8::String>(error.layer).ToLocalChecked());
        Nan::Set(obj, message_key, Nan::New<v8::String>(error.message).ToLocalChecked());
        Nan::Set(obj, feature_id_key, Nan::New<v8::Number>(error.feature_id));
        Nan::Set(obj, geojson_key, Nan::New<v8::String>(error.geojson).ToLocalChecked());
        Nan::Set(array, idx++, obj);
    }
    return scope.Escape(array);
}


struct not_simple_baton
{
    uv_work_t request;
    VectorTile* v;
    bool error;
    std::vector<not_simple_feature> result;
    std::string err_msg;
    Nan::Persistent<v8::Function> cb;
};

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
NAN_METHOD(VectorTile::reportGeometrySimplicitySync)
{
    info.GetReturnValue().Set(_reportGeometrySimplicitySync(info));
}

v8::Local<v8::Value> VectorTile::_reportGeometrySimplicitySync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    try
    {
        std::vector<not_simple_feature> errors;
        vector_tile_not_simple(d, errors);
        return scope.Escape(make_not_simple_array(errors));
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
        v8::Local<v8::Object> options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::Has(options, Nan::New("split_multi_features").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("split_multi_features").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'split_multi_features' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            split_multi_features = Nan::To<bool>(param_val).FromJust();
        }

        if (Nan::Has(options, Nan::New("lat_lon").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("lat_lon").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'lat_lon' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            lat_lon = Nan::To<bool>(param_val).FromJust();
        }

        if (Nan::Has(options, Nan::New("web_merc").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("web_merc").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'web_merc' must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            web_merc = Nan::To<bool>(param_val).FromJust();
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
NAN_METHOD(VectorTile::reportGeometrySimplicity)
{
    if (info.Length() == 0)
    {
        info.GetReturnValue().Set(_reportGeometrySimplicitySync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    not_simple_baton *closure = new not_simple_baton();
    closure->request.data = closure;
    closure->v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_ReportGeometrySimplicity, (uv_after_work_cb)EIO_AfterReportGeometrySimplicity);
    closure->v->Ref();
    return;
}

void VectorTile::EIO_ReportGeometrySimplicity(uv_work_t* req)
{
    not_simple_baton *closure = static_cast<not_simple_baton *>(req->data);
    try
    {
        vector_tile_not_simple(closure->v, closure->result);
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        closure->error = true;
        closure->err_msg = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::EIO_AfterReportGeometrySimplicity(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    not_simple_baton *closure = static_cast<not_simple_baton *>(req->data);
    if (closure->error)
    {
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->err_msg.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else
    {
        v8::Local<v8::Array> array = make_not_simple_array(closure->result);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), array };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->v->Unref();
    closure->cb.Reset();
    delete closure;
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
        v8::Local<v8::Object> options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::Has(options, Nan::New("split_multi_features").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("split_multi_features").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'split_multi_features' must be a boolean");
                return;
            }
            split_multi_features = Nan::To<bool>(param_val).FromJust();
        }

        if (Nan::Has(options, Nan::New("lat_lon").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("lat_lon").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'lat_lon' must be a boolean");
                return;
            }
            lat_lon = Nan::To<bool>(param_val).FromJust();
        }

        if (Nan::Has(options, Nan::New("web_merc").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> param_val = Nan::Get(options, Nan::New("web_merc").ToLocalChecked()).ToLocalChecked();
            if (!param_val->IsBoolean())
            {
                Nan::ThrowError("option 'web_merc' must be a boolean");
                return;
            }
            web_merc = Nan::To<bool>(param_val).FromJust();
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
    Nan::AsyncResource async_resource(__func__);
    not_valid_baton *closure = static_cast<not_valid_baton *>(req->data);
    if (closure->error)
    {
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->err_msg.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else
    {
        v8::Local<v8::Array> array = make_not_valid_array(closure->result);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), array };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->v->Unref();
    closure->cb.Reset();
    delete closure;
}

#endif // BOOST_VERSION >= 1.58

NAN_GETTER(VectorTile::get_tile_x)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->x()));
}

NAN_GETTER(VectorTile::get_tile_y)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->y()));
}

NAN_GETTER(VectorTile::get_tile_z)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->z()));
}

NAN_GETTER(VectorTile::get_tile_size)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->tile_size()));
}

NAN_GETTER(VectorTile::get_buffer_size)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(d->tile_->buffer_size()));
}

NAN_SETTER(VectorTile::set_tile_x)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = Nan::To<int>(value).FromJust();
        if (val < 0)
        {
            Nan::ThrowError("tile x coordinate must be greater then or equal to zero");
            return;
        }
        d->tile_->x(val);
    }
}

NAN_SETTER(VectorTile::set_tile_y)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = Nan::To<int>(value).FromJust();
        if (val < 0)
        {
            Nan::ThrowError("tile y coordinate must be greater then or equal to zero");
            return;
        }
        d->tile_->y(val);
    }
}

NAN_SETTER(VectorTile::set_tile_z)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = Nan::To<int>(value).FromJust();
        if (val < 0)
        {
            Nan::ThrowError("tile z coordinate must be greater then or equal to zero");
            return;
        }
        d->tile_->z(val);
    }
}

NAN_SETTER(VectorTile::set_tile_size)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = Nan::To<int>(value).FromJust();
        if (val <= 0)
        {
            Nan::ThrowError("tile size must be greater then zero");
            return;
        }
        d->tile_->tile_size(val);
    }
}

NAN_SETTER(VectorTile::set_buffer_size)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        int val = Nan::To<int>(value).FromJust();
        if (static_cast<int>(d->tile_size()) + (2 * val) <= 0)
        {
            Nan::ThrowError("too large of a negative buffer for tilesize");
            return;
        }
        d->tile_->buffer_size(val);
    }
}

typedef struct {
    uv_work_t request;
    const char *data;
    size_t dataLength;
    v8::Local<v8::Object> obj;
    bool error;
    std::string error_str;
    Nan::Persistent<v8::Object> buffer;
    Nan::Persistent<v8::Function> cb;
} vector_tile_info_baton_t;

/**
 * Return an object containing information about a vector tile buffer. Useful for
 * debugging `.mvt` files with errors.
 *
 * @name info
 * @param {Buffer} buffer - vector tile buffer
 * @returns {Object} json object with information about the vector tile buffer
 * @static
 * @memberof VectorTile
 * @instance
 * @example
 * var buffer = fs.readFileSync('./path/to/tile.mvt');
 * var info = mapnik.VectorTile.info(buffer);
 * console.log(info);
 * // { layers:
 * //   [ { name: 'world',
 * //      features: 1,
 * //      point_features: 0,
 * //      linestring_features: 0,
 * //      polygon_features: 1,
 * //      unknown_features: 0,
 * //      raster_features: 0,
 * //      version: 2 },
 * //    { name: 'world2',
 * //      features: 1,
 * //      point_features: 0,
 * //      linestring_features: 0,
 * //      polygon_features: 1,
 * //      unknown_features: 0,
 * //      raster_features: 0,
 * //      version: 2 } ],
 * //    errors: false }
 */
NAN_METHOD(VectorTile::info)
{
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("must provide a buffer argument");
        return;
    }

    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
        return;
    }

    v8::Local<v8::Object> out = Nan::New<v8::Object>();
    v8::Local<v8::Array> layers = Nan::New<v8::Array>();
    std::set<mapnik::vector_tile_impl::validity_error> errors;
    bool has_errors = false;
    std::size_t layers_size = 0;
    bool first_layer = true;
    std::set<std::string> layer_names_set;
    std::uint32_t version = 1;
    protozero::pbf_reader tile_msg;
    std::string decompressed;
    try
    {
        if (mapnik::vector_tile_impl::is_gzip_compressed(node::Buffer::Data(obj),node::Buffer::Length(obj)) ||
            mapnik::vector_tile_impl::is_zlib_compressed(node::Buffer::Data(obj),node::Buffer::Length(obj)))
        {
            mapnik::vector_tile_impl::zlib_decompress(node::Buffer::Data(obj), node::Buffer::Length(obj), decompressed);
            tile_msg = protozero::pbf_reader(decompressed);
        }
        else
        {
            tile_msg = protozero::pbf_reader(node::Buffer::Data(obj),node::Buffer::Length(obj));
        }
        while (tile_msg.next())
        {
            switch (tile_msg.tag())
            {
                case mapnik::vector_tile_impl::Tile_Encoding::LAYERS:
                    {
                        v8::Local<v8::Object> layer_obj = Nan::New<v8::Object>();
                        std::uint64_t point_feature_count = 0;
                        std::uint64_t line_feature_count = 0;
                        std::uint64_t polygon_feature_count = 0;
                        std::uint64_t unknown_feature_count = 0;
                        std::uint64_t raster_feature_count = 0;
                        auto layer_view = tile_msg.get_view();
                        protozero::pbf_reader layer_props_msg(layer_view);
                        auto layer_info = mapnik::vector_tile_impl::get_layer_name_and_version(layer_props_msg);
                        std::string const& layer_name = layer_info.first;
                        std::uint32_t layer_version = layer_info.second;
                        std::set<mapnik::vector_tile_impl::validity_error> layer_errors;
                        if (version > 2 || version < 1)
                        {
                            layer_errors.insert(mapnik::vector_tile_impl::LAYER_HAS_UNSUPPORTED_VERSION);
                        }
                        protozero::pbf_reader layer_msg(layer_view);
                        mapnik::vector_tile_impl::layer_is_valid(layer_msg,
                                                                 layer_errors,
                                                                 point_feature_count,
                                                                 line_feature_count,
                                                                 polygon_feature_count,
                                                                 unknown_feature_count,
                                                                 raster_feature_count);
                        std::uint64_t feature_count = point_feature_count +
                                                      line_feature_count +
                                                      polygon_feature_count +
                                                      unknown_feature_count +
                                                      raster_feature_count;
                        if (!layer_name.empty())
                        {
                            auto p = layer_names_set.insert(layer_name);
                            if (!p.second)
                            {
                                errors.insert(mapnik::vector_tile_impl::TILE_REPEATED_LAYER_NAMES);
                            }
                            Nan::Set(layer_obj, Nan::New("name").ToLocalChecked(), Nan::New<v8::String>(layer_name).ToLocalChecked());
                        }
                        Nan::Set(layer_obj, Nan::New("features").ToLocalChecked(), Nan::New<v8::Number>(feature_count));
                        Nan::Set(layer_obj, Nan::New("point_features").ToLocalChecked(), Nan::New<v8::Number>(point_feature_count));
                        Nan::Set(layer_obj, Nan::New("linestring_features").ToLocalChecked(), Nan::New<v8::Number>(line_feature_count));
                        Nan::Set(layer_obj, Nan::New("polygon_features").ToLocalChecked(), Nan::New<v8::Number>(polygon_feature_count));
                        Nan::Set(layer_obj, Nan::New("unknown_features").ToLocalChecked(), Nan::New<v8::Number>(unknown_feature_count));
                        Nan::Set(layer_obj, Nan::New("raster_features").ToLocalChecked(), Nan::New<v8::Number>(raster_feature_count));
                        Nan::Set(layer_obj, Nan::New("version").ToLocalChecked(), Nan::New<v8::Number>(layer_version));
                        if (!layer_errors.empty())
                        {
                            has_errors = true;
                            v8::Local<v8::Array> err_arr = Nan::New<v8::Array>();
                            std::size_t i = 0;
                            for (auto const& e : layer_errors)
                            {
                                Nan::Set(err_arr, i++, Nan::New<v8::String>(mapnik::vector_tile_impl::validity_error_to_string(e)).ToLocalChecked());
                            }
                            Nan::Set(layer_obj, Nan::New("errors").ToLocalChecked(), err_arr);
                        }
                        if (first_layer)
                        {
                            version = layer_version;
                        }
                        else
                        {
                            if (version != layer_version)
                            {
                                errors.insert(mapnik::vector_tile_impl::TILE_HAS_DIFFERENT_VERSIONS);
                            }
                        }
                        first_layer = false;
                        Nan::Set(layers, layers_size++, layer_obj);
                    }
                    break;
                default:
                    errors.insert(mapnik::vector_tile_impl::TILE_HAS_UNKNOWN_TAG);
                    tile_msg.skip();
                    break;
            }
        }
    }
    catch (...)
    {
        errors.insert(mapnik::vector_tile_impl::INVALID_PBF_BUFFER);
    }
    Nan::Set(out, Nan::New("layers").ToLocalChecked(), layers);
    has_errors = has_errors || !errors.empty();
    Nan::Set(out, Nan::New("errors").ToLocalChecked(),  Nan::New<v8::Boolean>(has_errors));
    if (!errors.empty())
    {
        v8::Local<v8::Array> err_arr = Nan::New<v8::Array>();
        std::size_t i = 0;
        for (auto const& e : errors)
        {
            Nan::Set(err_arr, i++, Nan::New<v8::String>(mapnik::vector_tile_impl::validity_error_to_string(e)).ToLocalChecked());
        }
        Nan::Set(out, Nan::New("tile_errors").ToLocalChecked(), err_arr);
    }
    info.GetReturnValue().Set(out);
    return;
}
