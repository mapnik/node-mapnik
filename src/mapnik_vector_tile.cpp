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
#include "vector_tile_projection.hpp"
#include "vector_tile_datasource.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_util.hpp"
#include "vector_tile.pb.h"
#include "object_to_container.hpp"

#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/version.hpp>
#include <mapnik/request.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
#if defined(GRID_RENDERER)
#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp>  // for grid_renderer
#endif
#include <mapnik/box2d.hpp>
#include <mapnik/scale_denominator.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/feature_to_geojson.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geometry_is_simple.hpp>
#include <mapnik/geometry_is_valid.hpp>
#ifdef HAVE_CAIRO
#include <mapnik/cairo/cairo_renderer.hpp>
#include <cairo.h>
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif // CAIRO_HAS_SVG_SURFACE
#endif

#include <set>                          // for set, etc
#include <sstream>                      // for operator<<, basic_ostream, etc
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

#include <protozero/pbf_reader.hpp>

// addGeoJSON
#include "vector_tile_compression.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"
#include <mapnik/datasource_cache.hpp>
#include <mapnik/hit_test_filter.hpp>


#include <google/protobuf/io/coded_stream.h>
#include "vector_tile.pb.h"

namespace detail {

inline void add_tile(std::string & buffer, vector_tile::Tile const& tile)
{
    std::string new_message;
    if (!tile.SerializeToString(&new_message))
    {
        /* LCOV_EXCL_START */
        throw std::runtime_error("could not serialize new data for vt");
        /* LCOV_EXCL_END */
    }
    if (!new_message.empty())
    {
        buffer.append(new_message.data(),new_message.size());
    }
}

inline vector_tile::Tile get_tile(std::string const& buffer)
{
    vector_tile::Tile tile;
    if (!tile.ParseFromArray(buffer.data(), buffer.size()))
    {
        throw std::runtime_error("could not parse as proto from buf");
    }
    return tile;
}

bool pbf_layer_match(protozero::pbf_reader const& layer_msg, std::string const& layer_name)
{
    protozero::pbf_reader lay(layer_msg);
    while (lay.next(1)) {
        if (lay.get_string() == layer_name) {
            return true;
        }
    }
    return false;
}

bool pbf_get_layer(std::string const& tile_buffer,
                   std::string const& layer_name,
                   protozero::pbf_reader & layer_msg)
{
    protozero::pbf_reader item(tile_buffer.data(),tile_buffer.size());
    while (item.next(3)) {
        layer_msg = item.get_message();
        if (pbf_layer_match(layer_msg,layer_name))
        {
            return true;
        }
    }
    return false;
}

bool lazy_empty(std::string const& buffer)
{
    std::size_t bytes = buffer.size();
    if (bytes > 0)
    {
        protozero::pbf_reader item(buffer.data(),bytes);
        while (item.next(3)) {
            protozero::pbf_reader layer_msg = item.get_message();
            while (layer_msg.next(2)) {
                // we hit a feature, assume we've got data
                return false;
            }
        }
    }
    return true;
}

std::vector<std::string> lazy_names(std::string const& buffer)
{
    std::vector<std::string> names;
    std::size_t bytes = buffer.size();
    if (bytes > 0)
    {
        protozero::pbf_reader item(buffer.data(),bytes);
        while (item.next(3)) {
            protozero::pbf_reader layer_msg = item.get_message();
            while (layer_msg.next(1)) {
                names.emplace_back(layer_msg.get_string());
            }
        }
    }
    return names;
}

struct p2p_distance
{
    p2p_distance(double x, double y)
     : x_(x),
       y_(y) {}

    double operator() (mapnik::geometry::geometry_empty const& ) const
    {
        // There is no current way that a geometry empty could be returned from a vector tile.
        /* LCOV_EXCL_START */
        return -1;
        /* LCOV_EXCL_END */
    }

    double operator() (mapnik::geometry::point<double> const& geom) const
    {
        return mapnik::distance(geom.x, geom.y, x_, y_);
    }
    double operator() (mapnik::geometry::multi_point<double> const& geom) const
    {
        double distance = -1;
        for (auto const& pt : geom)
        {
            double dist = operator()(pt);
            if (dist >= 0 && (distance < 0 || dist < distance)) distance = dist;
        }
        return distance;
    }
    double operator() (mapnik::geometry::line_string<double> const& geom) const
    {
        double distance = -1;
        std::size_t num_points = geom.num_points();
        if (num_points > 1)
        {
            for (std::size_t i = 1; i < num_points; ++i)
            {
                auto const& pt0 = geom[i-1];
                auto const& pt1 = geom[i];
                double dist = mapnik::point_to_segment_distance(x_,y_,pt0.x,pt0.y,pt1.x,pt1.y);
                if (dist >= 0 && (distance < 0 || dist < distance)) distance = dist;
            }
        }
        return distance;
    }
    double operator() (mapnik::geometry::multi_line_string<double> const& geom) const
    {
        double distance = -1;
        for (auto const& line: geom)
        {
            double dist = operator()(line);
            if (dist >= 0 && (distance < 0 || dist < distance)) distance = dist;
        }
        return distance;
    }
    double operator() (mapnik::geometry::polygon<double> const& geom) const
    {
        auto const& exterior = geom.exterior_ring;
        std::size_t num_points = exterior.num_points();
        if (num_points < 4) return -1;
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
        if (!inside) return -1;
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
        return inside ? 0 : -1;
    }
    double operator() (mapnik::geometry::multi_polygon<double> const& geom) const
    {
        double distance = -1;
        for (auto const& poly: geom)
        {
            double dist = operator()(poly);
            if (dist >= 0 && (distance < 0 || dist < distance)) distance = dist;
        }
        return distance;
    }
    double operator() (mapnik::geometry::geometry_collection<double> const& collection) const
    {
        // There is no current way that a geometry collection could be returned from a vector tile.
        /* LCOV_EXCL_START */
        double distance = -1;
        for (auto const& geom: collection)
        {
            double dist = mapnik::util::apply_visitor((*this),geom);
            if (dist >= 0 && (distance < 0 || dist < distance)) distance = dist;
        }
        return distance;
        /* LCOV_EXCL_END */
    }

    double x_;
    double y_;
};

}

double path_to_point_distance(mapnik::geometry::geometry<double> const& geom, double x, double y)
{
    return mapnik::util::apply_visitor(detail::p2p_distance(x,y), geom);
}

Nan::Persistent<v8::FunctionTemplate> VectorTile::constructor;

/**
 * A generator for the [Mapbox Vector Tile](https://www.mapbox.com/developers/vector-tiles/)
 * specification of compressed and simplified tiled vector data
 *
 * @name mapnik.VectorTile
 * @class
 * @param {number} z
 * @param {number} x
 * @param {number} y
 * @example
 * var vtile = new mapnik.VectorTile(9,112,195);
 */
void VectorTile::Initialize(v8::Local<v8::Object> target) {
    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(VectorTile::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("VectorTile").ToLocalChecked());
    Nan::SetPrototypeMethod(lcons, "render", render);
    Nan::SetPrototypeMethod(lcons, "setData", setData);
    Nan::SetPrototypeMethod(lcons, "setDataSync", setDataSync);
    Nan::SetPrototypeMethod(lcons, "getData", getData);
    Nan::SetPrototypeMethod(lcons, "getDataSync", getDataSync);
    Nan::SetPrototypeMethod(lcons, "parse", parse);
    Nan::SetPrototypeMethod(lcons, "parseSync", parseSync);
    Nan::SetPrototypeMethod(lcons, "addData", addData);
    Nan::SetPrototypeMethod(lcons, "composite", composite);
    Nan::SetPrototypeMethod(lcons, "compositeSync", compositeSync);
    Nan::SetPrototypeMethod(lcons, "query", query);
    Nan::SetPrototypeMethod(lcons, "queryMany", queryMany);
    Nan::SetPrototypeMethod(lcons, "names", names);
    Nan::SetPrototypeMethod(lcons, "toJSON", toJSON);
    Nan::SetPrototypeMethod(lcons, "toGeoJSON", toGeoJSON);
    Nan::SetPrototypeMethod(lcons, "toGeoJSONSync", toGeoJSONSync);
    Nan::SetPrototypeMethod(lcons, "addGeoJSON", addGeoJSON);
    Nan::SetPrototypeMethod(lcons, "addImage", addImage);
#if BOOST_VERSION >= 105600
    Nan::SetPrototypeMethod(lcons, "reportGeometrySimplicity", reportGeometrySimplicity);
    Nan::SetPrototypeMethod(lcons, "reportGeometrySimplicitySync", reportGeometrySimplicitySync);
    Nan::SetPrototypeMethod(lcons, "reportGeometryValidity", reportGeometryValidity);
    Nan::SetPrototypeMethod(lcons, "reportGeometryValiditySync", reportGeometryValiditySync);
#endif // BOOST_VERSION >= 105600

    // common to mapnik.Image
    Nan::SetPrototypeMethod(lcons, "width", width);
    Nan::SetPrototypeMethod(lcons, "height", height);
    Nan::SetPrototypeMethod(lcons, "painted", painted);
    Nan::SetPrototypeMethod(lcons, "clear", clear);
    Nan::SetPrototypeMethod(lcons, "clearSync", clearSync);
    Nan::SetPrototypeMethod(lcons, "empty", empty);
    Nan::SetPrototypeMethod(lcons, "isSolid", isSolid);
    Nan::SetPrototypeMethod(lcons, "isSolidSync", isSolidSync);
    target->Set(Nan::New("VectorTile").ToLocalChecked(),lcons->GetFunction());
    constructor.Reset(lcons);
}

VectorTile::VectorTile(int z, int x, int y, unsigned w, unsigned h) :
    Nan::ObjectWrap(),
    z_(z),
    x_(x),
    y_(y),
    buffer_(),
    width_(w),
    height_(h) {}

// For some reason coverage never seems to be considered here even though
// I have tested it and it does print
/* LCOV_EXCL_START */
VectorTile::~VectorTile() { }
/* LCOV_EXCL_END */

NAN_METHOD(VectorTile::New)
{
    if (!info.IsConstructCall()) {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info.Length() >= 3)
    {
        if (!info[0]->IsNumber() ||
            !info[1]->IsNumber() ||
            !info[2]->IsNumber())
        {
            Nan::ThrowTypeError("required info (z, x, and y) must be a integers");
            return;
        }
        unsigned width = 256;
        unsigned height = 256;
        v8::Local<v8::Object> options = Nan::New<v8::Object>();
        if (info.Length() > 3) {
            if (!info[3]->IsObject())
            {
                Nan::ThrowTypeError("optional fourth argument must be an options object");
                return;
            }
            options = info[3]->ToObject();
            if (options->Has(Nan::New("width").ToLocalChecked())) {
                v8::Local<v8::Value> opt = options->Get(Nan::New("width").ToLocalChecked());
                if (!opt->IsNumber())
                {
                    Nan::ThrowTypeError("optional arg 'width' must be a number");
                    return;
                }
                width = opt->IntegerValue();
            }
            if (options->Has(Nan::New("height").ToLocalChecked())) {
                v8::Local<v8::Value> opt = options->Get(Nan::New("height").ToLocalChecked());
                if (!opt->IsNumber())
                {
                    Nan::ThrowTypeError("optional arg 'height' must be a number");
                    return;
                }
                height = opt->IntegerValue();
            }
        }

        VectorTile* d = new VectorTile(info[0]->IntegerValue(),
                                   info[1]->IntegerValue(),
                                   info[2]->IntegerValue(),
                                   width,height);

        d->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    else
    {
        Nan::ThrowError("please provide a z, x, y");
        return;
    }
    return;
}

void VectorTile::parse_proto()
{
    std::size_t bytes = buffer_.size();
    if (bytes == 0)
    {
        throw std::runtime_error("cannot parse 0 length buffer as protobuf");
    }
    vector_tile::Tile throwaway_tile;
    if (throwaway_tile.ParseFromArray(buffer_.data(), buffer_.size()))
    {
        throwaway_tile.Clear();
    }
    else
    {
        throwaway_tile.Clear();
        throw std::runtime_error("could not parse buffer as protobuf");
    }
}

void _composite(VectorTile* target_vt,
                std::vector<VectorTile*> & vtiles,
                unsigned path_multiplier,
                int buffer_size,
                double scale_factor,
                unsigned offset_x,
                unsigned offset_y,
                double area_threshold,
                bool strictly_simple,
                double scale_denominator)
{
    vector_tile::Tile new_tiledata;
    std::string merc_srs("+init=epsg:3857");
    mapnik::box2d<double> max_extent(-20037508.34,-20037508.34,20037508.34,20037508.34);
    if (target_vt->width() <= 0 || target_vt->height() <= 0)
    {
        throw std::runtime_error("Vector tile width and height must be great than zero");
    }
    for (VectorTile* vt : vtiles)
    {
        if (vt->width() <= 0 || vt->height() <= 0)
        {
            throw std::runtime_error("Vector tile width and height must be great than zero");
        }
        // TODO - handle name clashes
        if (target_vt->z_ == vt->z_ &&
            target_vt->x_ == vt->x_ &&
            target_vt->y_ == vt->y_)
        {
            int bytes = static_cast<int>(vt->buffer_.size());
            if (bytes > 0) {
                target_vt->buffer_.append(vt->buffer_.data(),vt->buffer_.size());
            }
        }
        else
        {
            new_tiledata.Clear();
            // set up to render to new vtile
            typedef mapnik::vector_tile_impl::backend_pbf backend_type;
            typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
            backend_type backend(new_tiledata, path_multiplier);

            // get mercator extent of target tile
            mapnik::vector_tile_impl::spherical_mercator merc(target_vt->width());
            double minx,miny,maxx,maxy;
            merc.xyz(target_vt->x_,target_vt->y_,target_vt->z_,minx,miny,maxx,maxy);
            mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
            // create request
            mapnik::request m_req(target_vt->width(),target_vt->height(),map_extent);
            m_req.set_buffer_size(buffer_size);
            // create map
            mapnik::Map map(target_vt->width(),target_vt->height(),merc_srs);
            map.set_maximum_extent(max_extent);
            protozero::pbf_reader message(vt->buffer_.data(),vt->buffer_.size());
            while (message.next(3)) {
                protozero::pbf_reader layer_msg = message.get_message();
                auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                            layer_msg,
                            vt->x_,
                            vt->y_,
                            vt->z_,
                            vt->width()
                            );
                mapnik::layer lyr(ds->get_name(),merc_srs);
                ds->set_envelope(m_req.get_buffered_extent());
                lyr.set_datasource(ds);
                map.add_layer(lyr);
            }
            if (!map.layers().empty())
            {
                renderer_type ren(backend,
                                  map,
                                  m_req,
                                  scale_factor,
                                  offset_x,
                                  offset_y,
                                  area_threshold,
                                  strictly_simple);
                ren.apply(scale_denominator);
            }

            std::string new_message;
            if (!new_tiledata.SerializeToString(&new_message))
            {
                /* The only time this could possible be reached it seems is
                if there is a protobuf that is attempted to be serialized that is
                larger then two GBs, see link below:
                https://github.com/google/protobuf/blob/6ef984af4b0c63c1c33127a12dcfc8e6359f0c9e/src/google/protobuf/message_lite.cc#L293-L300
                */
                /* LCOV_EXCL_START */
                throw std::runtime_error("could not serialize new data for vt");
                /* LCOV_EXCL_END */
            }
            if (!new_message.empty())
            {
                target_vt->buffer_.append(new_message.data(),new_message.size());
            }
        }
    }
}

/**
 * Composite an array of vector tiles into one vector tile
 *
 * @memberof mapnik.VectorTile
 * @name compositeSync
 * @instance
 * @param {v8::Array<mapnik.VectorTile>} vector tiles
 */
NAN_METHOD(VectorTile::compositeSync)
{
    info.GetReturnValue().Set(_compositeSync(info));
}

v8::Local<v8::Value> VectorTile::_compositeSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    if (info.Length() < 1 || !info[0]->IsArray()) {
        Nan::ThrowTypeError("must provide an array of VectorTile objects and an optional options object");
        return scope.Escape(Nan::Undefined());
    }
    v8::Local<v8::Array> vtiles = info[0].As<v8::Array>();
    unsigned num_tiles = vtiles->Length();
    if (num_tiles < 1) {
        Nan::ThrowTypeError("must provide an array with at least one VectorTile object and an optional options object");
        return scope.Escape(Nan::Undefined());
    }

    // options needed for re-rendering tiles
    // unclear yet to what extent these need to be user
    // driven, but we expose here to avoid hardcoding
    unsigned path_multiplier = 16;
    int buffer_size = 1;
    double scale_factor = 1.0;
    unsigned offset_x = 0;
    unsigned offset_y = 0;
    double area_threshold = 0.1;
    bool strictly_simple = false;
    double scale_denominator = 0.0;

    if (info.Length() > 1) {
        // options object
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second argument must be an options object");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("path_multiplier").ToLocalChecked())) {

            v8::Local<v8::Value> param_val = options->Get(Nan::New("path_multiplier").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'path_multiplier' must be an unsigned integer");
                return scope.Escape(Nan::Undefined());
            }
            path_multiplier = param_val->NumberValue();
        }
        if (options->Has(Nan::New("area_threshold").ToLocalChecked()))
        {
            v8::Local<v8::Value> area_thres = options->Get(Nan::New("area_threshold").ToLocalChecked());
            if (!area_thres->IsNumber())
            {
                Nan::ThrowTypeError("area_threshold value must be a number");
                return scope.Escape(Nan::Undefined());
            }
            area_threshold = area_thres->NumberValue();
        }
        if (options->Has(Nan::New("strictly_simple").ToLocalChecked()))
        {
            v8::Local<v8::Value> strict_simp = options->Get(Nan::New("strictly_simple").ToLocalChecked());
            if (!strict_simp->IsBoolean())
            {
                Nan::ThrowTypeError("strictly_simple value must be a boolean");
                return scope.Escape(Nan::Undefined());
            }
            strictly_simple = strict_simp->BooleanValue();
        }
        if (options->Has(Nan::New("buffer_size").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("buffer_size").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("scale").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            scale_factor = bind_opt->NumberValue();
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
        }
        if (options->Has(Nan::New("offset_x").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_x").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            offset_x = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("offset_y").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_y").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
                return scope.Escape(Nan::Undefined());
            }
            offset_y = bind_opt->IntegerValue();
        }
    }
    VectorTile* target_vt = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    std::vector<VectorTile*> vtiles_vec;
    vtiles_vec.reserve(num_tiles);
    for (unsigned j=0;j < num_tiles;++j) {
        v8::Local<v8::Value> val = vtiles->Get(j);
        if (!val->IsObject()) {
            Nan::ThrowTypeError("must provide an array of VectorTile objects");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> tile_obj = val->ToObject();
        if (tile_obj->IsNull() || tile_obj->IsUndefined() || !Nan::New(VectorTile::constructor)->HasInstance(tile_obj)) {
            Nan::ThrowTypeError("must provide an array of VectorTile objects");
            return scope.Escape(Nan::Undefined());
        }
        vtiles_vec.push_back(Nan::ObjectWrap::Unwrap<VectorTile>(tile_obj));
    }
    try
    {
        _composite(target_vt,
                   vtiles_vec,
                   path_multiplier,
                   buffer_size,
                   scale_factor,
                   offset_x,
                   offset_y,
                   area_threshold,
                   strictly_simple,
                   scale_denominator);
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowTypeError(ex.what());
        return scope.Escape(Nan::Undefined());
    }

    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    unsigned path_multiplier;
    int buffer_size;
    double scale_factor;
    unsigned offset_x;
    unsigned offset_y;
    double area_threshold;
    double scale_denominator;
    std::vector<VectorTile*> vtiles;
    bool error;
    bool strictly_simple;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_composite_baton_t;

NAN_METHOD(VectorTile::composite)
{
    if ((info.Length() < 2) || !info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_compositeSync(info));
        return;
    }
    if (!info[0]->IsArray()) {
        Nan::ThrowTypeError("must provide an array of VectorTile objects and an optional options object");
        return;
    }
    v8::Local<v8::Array> vtiles = info[0].As<v8::Array>();
    unsigned num_tiles = vtiles->Length();
    if (num_tiles < 1) {
        Nan::ThrowTypeError("must provide an array with at least one VectorTile object and an optional options object");
        return;
    }

    // options needed for re-rendering tiles
    // unclear yet to what extent these need to be user
    // driven, but we expose here to avoid hardcoding
    unsigned path_multiplier = 16;
    int buffer_size = 1;
    double scale_factor = 1.0;
    unsigned offset_x = 0;
    unsigned offset_y = 0;
    double area_threshold = 0.1;
    bool strictly_simple = false;
    double scale_denominator = 0.0;
    // not options yet, likely should never be....
    mapnik::box2d<double> max_extent(-20037508.34,-20037508.34,20037508.34,20037508.34);
    std::string merc_srs("+init=epsg:3857");

    if (info.Length() > 2) {
        // options object
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second argument must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("path_multiplier").ToLocalChecked())) {

            v8::Local<v8::Value> param_val = options->Get(Nan::New("path_multiplier").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'path_multiplier' must be an unsigned integer");
                return;
            }
            path_multiplier = param_val->NumberValue();
        }
        if (options->Has(Nan::New("area_threshold").ToLocalChecked()))
        {
            v8::Local<v8::Value> area_thres = options->Get(Nan::New("area_threshold").ToLocalChecked());
            if (!area_thres->IsNumber())
            {
                Nan::ThrowTypeError("area_threshold value must be a number");
                return;
            }
            area_threshold = area_thres->NumberValue();
        }
        if (options->Has(Nan::New("strictly_simple").ToLocalChecked())) {
            v8::Local<v8::Value> strict_simp = options->Get(Nan::New("strictly_simple").ToLocalChecked());
            if (!strict_simp->IsBoolean())
            {
                Nan::ThrowTypeError("strictly_simple value must be a boolean");
                return;
            }
            strictly_simple = strict_simp->BooleanValue();
        }
        if (options->Has(Nan::New("buffer_size").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("buffer_size").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }
            buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("scale").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return;
            }
            scale_factor = bind_opt->NumberValue();
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
        }
        if (options->Has(Nan::New("offset_x").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_x").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
                return;
            }
            offset_x = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("offset_y").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_y").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
                return;
            }
            offset_y = bind_opt->IntegerValue();
        }
    }

    v8::Local<v8::Value> callback = info[info.Length()-1];
    vector_tile_composite_baton_t *closure = new vector_tile_composite_baton_t();
    closure->request.data = closure;
    closure->offset_x = offset_x;
    closure->offset_y = offset_y;
    closure->strictly_simple = strictly_simple;
    closure->area_threshold = area_threshold;
    closure->path_multiplier = path_multiplier;
    closure->buffer_size = buffer_size;
    closure->scale_factor = scale_factor;
    closure->scale_denominator = scale_denominator;
    closure->d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    closure->error = false;
    closure->vtiles.reserve(num_tiles);
    for (unsigned j=0;j < num_tiles;++j) {
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
                   closure->path_multiplier,
                   closure->buffer_size,
                   closure->scale_factor,
                   closure->offset_x,
                   closure->offset_y,
                   closure->area_threshold,
                   closure->strictly_simple,
                   closure->scale_denominator);
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


/**
 * Get the names of all of the layers in this vector tile
 *
 * @memberof mapnik.VectorTile
 * @name names
 * @instance
 * @param {v8::Array<string>} layer names
 */
NAN_METHOD(VectorTile::names)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    int raw_size = d->buffer_.size();
    if (raw_size > 0)
    {
        try
        {
            std::vector<std::string> names = detail::lazy_names(d->buffer_);
            v8::Local<v8::Array> arr = Nan::New<v8::Array>(names.size());
            unsigned idx = 0;
            for (std::string const& name : names)
            {
                arr->Set(idx++,Nan::New<v8::String>(name).ToLocalChecked());
            }
            info.GetReturnValue().Set(arr);
            return;
        }
        catch (std::exception const& ex)
        {
            Nan::ThrowError(ex.what());
            return;
        }
    }
    info.GetReturnValue().Set(Nan::New<v8::Array>(0));
}

/**
 * Return whether this vector tile is empty - whether it has no
 * layers and no features
 *
 * @memberof mapnik.VectorTile
 * @name empty
 * @instance
 * @param {boolean} whether the layer is empty
 */
NAN_METHOD(VectorTile::empty)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    int raw_size = d->buffer_.size();
    if (raw_size > 0)
    {
        try
        {
            info.GetReturnValue().Set(Nan::New<v8::Boolean>(detail::lazy_empty(d->buffer_)));
            return;
        }
        catch (std::exception const& ex)
        {
            Nan::ThrowError(ex.what());
            return;
        }
    }
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(true));
}

/**
 * Get the vector tile's width
 *
 * @memberof mapnik.VectorTile
 * @name width
 * @instance
 * @param {number} width
 */
NAN_METHOD(VectorTile::width)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Integer>(d->width()));
}

/**
 * Get the vector tile's height
 *
 * @memberof mapnik.VectorTile
 * @name height
 * @instance
 * @param {number} height
 */
NAN_METHOD(VectorTile::height)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Integer>(d->height()));
}

/**
 * Get whether the vector tile has been painted
 *
 * @memberof mapnik.VectorTile
 * @name painted
 * @instance
 * @param {boolean} painted
 */
NAN_METHOD(VectorTile::painted)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    info.GetReturnValue().Set(Nan::New(d->painted()));
}

typedef struct {
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
 * Query a vector tile by longitude and latitude
 *
 * @memberof mapnik.VectorTile
 * @name query
 * @instance
 * @param {number} longitude
 * @param {number} latitude
 * @param {Object} [options={tolerance:0}] tolerance: allow results that
 * are not exactly on this longitude, latitude position.
 * @param {Function} callback
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
    if (!info[info.Length()-1]->IsFunction()) {
        try  {
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
    } else {
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
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        std::vector<query_result> result = closure->result;
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
    if (d->width() <= 0 || d->height() <= 0)
    {
        throw std::runtime_error("Can not query a vector tile with width or height of zero");
    }
    std::vector<query_result> arr;
    std::size_t bytes = d->buffer_.size();
    if (bytes <= 0)
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
        /* LCOV_EXCL_END */
    }

    mapnik::coord2d pt(x,y);
    if (!layer_name.empty())
    {
        protozero::pbf_reader layer_msg;
        if (detail::pbf_get_layer(d->buffer_,layer_name,layer_msg))
        {
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                                            layer_msg,
                                            d->x_,
                                            d->y_,
                                            d->z_,
                                            d->width()
                                            );
            mapnik::featureset_ptr fs = ds->features_at_point(pt,tolerance);
            if (fs)
            {
                mapnik::feature_ptr feature;
                while ((feature = fs->next()))
                {
                    auto const& geom = feature->get_geometry();
                    double distance = path_to_point_distance(geom,x,y);
                    if (distance >= 0 && distance <= tolerance)
                    {
                        query_result res;
                        res.distance = distance;
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
        protozero::pbf_reader item(d->buffer_.data(),bytes);
        while (item.next(3)) {
            protozero::pbf_reader layer_msg = item.get_message();
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                                            layer_msg,
                                            d->x_,
                                            d->y_,
                                            d->z_,
                                            d->width()
                                            );
            mapnik::featureset_ptr fs = ds->features_at_point(pt,tolerance);
            if (fs)
            {
                mapnik::feature_ptr feature;
                while ((feature = fs->next()))
                {
                    auto const& geom = feature->get_geometry();
                    double distance = path_to_point_distance(geom,x,y);
                    if (distance >= 0 && distance <= tolerance)
                    {
                        query_result res;
                        res.distance = distance;
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

bool VectorTile::_querySort(query_result const& a, query_result const& b) {
    return a.distance < b.distance;
}

v8::Local<v8::Array> VectorTile::_queryResultToV8(std::vector<query_result> const& result)
{
    v8::Local<v8::Array> arr = Nan::New<v8::Array>();
    std::size_t i = 0;
    for (auto const& item : result)
    {
        v8::Local<v8::Value> feat = Feature::NewInstance(item.feature);
        v8::Local<v8::Object> feat_obj = feat->ToObject();
        feat_obj->Set(Nan::New("layer").ToLocalChecked(),Nan::New<v8::String>(item.layer).ToLocalChecked());
        feat_obj->Set(Nan::New("distance").ToLocalChecked(),Nan::New<v8::Number>(item.distance));
        arr->Set(i++,feat);
    }
    return arr;
}

typedef struct {
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
        if (options->Has(Nan::New("fields").ToLocalChecked())) {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("fields").ToLocalChecked());
            if (!param_val->IsArray()) {
                Nan::ThrowTypeError("option 'fields' must be an array of strings");
                return;
            }
            v8::Local<v8::Array> a = v8::Local<v8::Array>::Cast(param_val);
            unsigned int i = 0;
            unsigned int num_fields = a->Length();
            fields.reserve(num_fields);
            while (i < num_fields) {
                v8::Local<v8::Value> name = a->Get(i);
                if (name->IsString()){
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
    if (!info[info.Length()-1]->IsFunction()) {
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
    } else {
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

void VectorTile::_queryMany(queryMany_result & result, VectorTile* d, std::vector<query_lonlat> const& query, double tolerance, std::string const& layer_name, std::vector<std::string> const& fields)
{
    protozero::pbf_reader layer_msg;
    if (detail::pbf_get_layer(d->buffer_,layer_name,layer_msg))
    {
        std::map<unsigned,query_result> features;
        std::map<unsigned,std::vector<query_hit> > hits;

        // Reproject query => mercator points
        mapnik::box2d<double> bbox;
        mapnik::projection wgs84("+init=epsg:4326",true);
        mapnik::projection merc("+init=epsg:3857",true);
        mapnik::proj_transform tr(wgs84,merc);
        std::vector<mapnik::coord2d> points;
        points.reserve(query.size());
        for (std::size_t p = 0; p < query.size(); ++p) {
            double x = query[p].lon;
            double y = query[p].lat;
            double z = 0;
            if (!tr.forward(x,y,z))
            {
                /* LCOV_EXCL_START */
                throw std::runtime_error("could not reproject lon/lat to mercator");
                /* LCOV_EXCL_END */
            }
            mapnik::coord2d pt(x,y);
            bbox.expand_to_include(pt);
            points.emplace_back(std::move(pt));
        }
        bbox.pad(tolerance);

        std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer_msg,
                                        d->x_,
                                        d->y_,
                                        d->z_,
                                        d->width()
                                        );
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

        if (fs)
        {
            mapnik::feature_ptr feature;
            unsigned idx = 0;
            while ((feature = fs->next()))
            {
                unsigned has_hit = 0;
                for (std::size_t p = 0; p < points.size(); ++p) {
                    mapnik::coord2d const& pt = points[p];
                    auto const& geom = feature->get_geometry();
                    double distance = path_to_point_distance(geom,pt.x,pt.y);
                    if (distance >= 0 && distance <= tolerance)
                    {
                        has_hit = 1;
                        query_result res;
                        res.feature = feature;
                        res.distance = 0;
                        res.layer = ds->get_name();

                        query_hit hit;
                        hit.distance = distance;
                        hit.feature_id = idx;

                        features.insert(std::make_pair(idx, res));

                        std::map<unsigned,std::vector<query_hit> >::iterator hits_it;
                        hits_it = hits.find(p);
                        if (hits_it == hits.end()) {
                            std::vector<query_hit> pointHits;
                            pointHits.reserve(1);
                            pointHits.push_back(std::move(hit));
                            hits.insert(std::make_pair(p, pointHits));
                        } else {
                            hits_it->second.push_back(std::move(hit));
                        }
                    }
                }
                if (has_hit > 0) {
                    idx++;
                }
            }
        }

        // Sort each group of hits by distance.
        for (auto & hit : hits) {
            std::sort(hit.second.begin(), hit.second.end(), _queryManySort);
        }

        result.hits = std::move(hits);
        result.features = std::move(features);
        return;
    }
    throw std::runtime_error("Could not find layer in vector tile");
}

bool VectorTile::_queryManySort(query_hit const& a, query_hit const& b) {
    return a.distance < b.distance;
}

v8::Local<v8::Object> VectorTile::_queryManyResultToV8(queryMany_result const& result) {
    v8::Local<v8::Object> results = Nan::New<v8::Object>();
    v8::Local<v8::Array> features = Nan::New<v8::Array>();
    v8::Local<v8::Array> hits = Nan::New<v8::Array>();
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
    if (closure->error) {
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
        // LCOV_EXCL_END
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
        // LCOV_EXCL_END
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
        // LCOV_EXCL_END
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
            // LCOV_EXCL_END
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
            // LCOV_EXCL_END
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
            // LCOV_EXCL_END
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
            // LCOV_EXCL_END
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
            // LCOV_EXCL_END
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
            // LCOV_EXCL_END
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
        // LCOV_EXCL_END
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
        // LCOV_EXCL_END
    }
};

template <typename T>
v8::Local<v8::Array> geometry_to_array(mapnik::geometry::geometry<T> const & geom)
{
    Nan::EscapableHandleScope scope;
    return scope.Escape(mapnik::util::apply_visitor(geometry_array_visitor(), geom));
}

/**
 * Get a JSON representation of this tile
 *
 * @memberof mapnik.VectorTile
 * @name toJSON
 * @instance
 * @returns {Object} json representation of this tile with name, extent,
 * and version properties
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

        if (options->Has(Nan::New("decode_geometry").ToLocalChecked())) {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("decode_geometry").ToLocalChecked());
            if (!param_val->IsBoolean()) {
                Nan::ThrowError("option 'decode_geometry' must be a boolean");
                return;
            }
            decode_geometry = param_val->BooleanValue();
        }
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    try
    {

        vector_tile::Tile tiledata = detail::get_tile(d->buffer_);
        v8::Local<v8::Array> arr = Nan::New<v8::Array>(tiledata.layers_size());
        for (int i=0; i < tiledata.layers_size(); ++i)
        {
            vector_tile::Tile_Layer const& layer = tiledata.layers(i);
            v8::Local<v8::Object> layer_obj = Nan::New<v8::Object>();
            layer_obj->Set(Nan::New("name").ToLocalChecked(), Nan::New<v8::String>(layer.name()).ToLocalChecked());
            layer_obj->Set(Nan::New("extent").ToLocalChecked(), Nan::New<v8::Integer>(layer.extent()));
            layer_obj->Set(Nan::New("version").ToLocalChecked(), Nan::New<v8::Integer>(layer.version()));

            v8::Local<v8::Array> f_arr = Nan::New<v8::Array>(layer.features_size());
            for (int j=0; j < layer.features_size(); ++j)
            {
                v8::Local<v8::Object> feature_obj = Nan::New<v8::Object>();
                vector_tile::Tile_Feature const& f = layer.features(j);
                if (f.has_id())
                {
                    feature_obj->Set(Nan::New("id").ToLocalChecked(),Nan::New<v8::Number>(f.id()));
                }
                if (f.has_raster())
                {
                    std::string const& raster = f.raster();
                    feature_obj->Set(Nan::New("raster").ToLocalChecked(),Nan::CopyBuffer((char*)raster.data(),raster.size()).ToLocalChecked());
                }
                feature_obj->Set(Nan::New("type").ToLocalChecked(),Nan::New<v8::Integer>(f.type()));
                if (decode_geometry)
                {
                    // Decode the geometry first into an int64_t mapnik geometry
                    mapnik::vector_tile_impl::Geometry<std::int64_t> geoms(f, 0, 0, 1.0, 1.0);
                    mapnik::geometry::geometry<std::int64_t> geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, f.type());
                    v8::Local<v8::Array> g_arr = geometry_to_array<std::int64_t>(geom);
                    feature_obj->Set(Nan::New("geometry").ToLocalChecked(),g_arr);
                    std::string geom_type = geometry_type_as_string(geom);
                    feature_obj->Set(Nan::New("geometry_type").ToLocalChecked(),Nan::New(geom_type).ToLocalChecked());
                }
                else
                {
                    v8::Local<v8::Array> g_arr = Nan::New<v8::Array>(f.geometry_size());
                    for (int k = 0; k < f.geometry_size();++k)
                    {
                        g_arr->Set(k,Nan::New<v8::Number>(f.geometry(k)));
                    }
                    feature_obj->Set(Nan::New("geometry").ToLocalChecked(),g_arr);
                }
                v8::Local<v8::Object> att_obj = Nan::New<v8::Object>();
                for (int m = 0; m < f.tags_size(); m += 2)
                {
                    std::size_t key_name = f.tags(m);
                    std::size_t key_value = f.tags(m + 1);
                    if (key_name < static_cast<std::size_t>(layer.keys_size())
                        && key_value < static_cast<std::size_t>(layer.values_size()))
                    {
                        std::string const& name = layer.keys(key_name);
                        vector_tile::Tile_Value const& value = layer.values(key_value);
                        if (value.has_string_value())
                        {
                            att_obj->Set(Nan::New(name).ToLocalChecked(), Nan::New(value.string_value()).ToLocalChecked());
                        }
                        else if (value.has_int_value())
                        {
                            att_obj->Set(Nan::New(name).ToLocalChecked(), Nan::New<v8::Number>(value.int_value()));
                        }
                        else if (value.has_double_value())
                        {
                            att_obj->Set(Nan::New(name).ToLocalChecked(), Nan::New<v8::Number>(value.double_value()));
                        }
                        // The following lines are not currently supported by mapnik-vector-tiles
                        // therefore these lines are not currently testable.
                        /* LCOV_EXCL_START */
                        else if (value.has_float_value())
                        {
                            att_obj->Set(Nan::New(name).ToLocalChecked(), Nan::New<v8::Number>(value.float_value()));
                        }
                        else if (value.has_bool_value())
                        {
                            att_obj->Set(Nan::New(name).ToLocalChecked(), Nan::New<v8::Boolean>(value.bool_value()));
                        }
                        else if (value.has_sint_value())
                        {
                            att_obj->Set(Nan::New(name).ToLocalChecked(), Nan::New<v8::Number>(value.sint_value()));
                        }
                        else if (value.has_uint_value())
                        {
                            att_obj->Set(Nan::New(name).ToLocalChecked(), Nan::New<v8::Number>(value.uint_value()));
                        }
                        else
                        {
                            att_obj->Set(Nan::New(name).ToLocalChecked(), Nan::Undefined());
                        }
                        /* LCOV_EXCL_END */
                    }
                }
                feature_obj->Set(Nan::New("properties").ToLocalChecked(),att_obj);
                f_arr->Set(j,feature_obj);
            }
            layer_obj->Set(Nan::New("features").ToLocalChecked(), f_arr);
            arr->Set(i, layer_obj);
        }
        info.GetReturnValue().Set(arr);
        return;
    } catch (std::exception const& ex) {
        Nan::ThrowError(ex.what());
        return;
    }
}

bool layer_to_geojson(vector_tile::Tile_Layer const& layer,
                      std::string & result,
                      unsigned x,
                      unsigned y,
                      unsigned z,
                      unsigned width)
{
    mapnik::vector_tile_impl::tile_datasource ds(layer,
                                                 x,
                                                 y,
                                                 z,
                                                 width);
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,wgs84);
    mapnik::query q(ds.envelope());
    mapnik::layer_descriptor ld = ds.get_descriptor();
    for (auto const& item : ld.get_descriptors())
    {
        q.add_property_name(item.get_name());
    }
    mapnik::featureset_ptr fs = ds.features(q);
    bool first = true;
    if (fs)
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
                // LCOV_EXCL_END
            }
            result += feature_str;
        }
    }
    return !first;
}

/**
 * Get a [GeoJSON](http://geojson.org/) representation of this tile
 *
 * @memberof mapnik.VectorTile
 * @name toGeoJSONSync
 * @instance
 * @returns {string} stringified GeoJSON of all the features in this tile.
 */
NAN_METHOD(VectorTile::toGeoJSONSync)
{
    info.GetReturnValue().Set(_toGeoJSONSync(info));
}

void handle_to_geojson_args(v8::Local<v8::Value> const& layer_id,
                            vector_tile::Tile const& tiledata,
                            bool & all_array,
                            bool & all_flattened,
                            std::string & error_msg,
                            int & layer_idx)
{
    unsigned layer_num = tiledata.layers_size();
    if (layer_id->IsString())
    {
        std::string layer_name = TOSTR(layer_id);
        if (layer_name == "__array__")
        {
            all_array = true;
        }
        else if (layer_name == "__all__")
        {
            all_flattened = true;
        }
        else
        {
            bool found = false;
            unsigned int idx(0);
            for (unsigned i=0; i < layer_num; ++i)
            {
                vector_tile::Tile_Layer const& layer = tiledata.layers(i);
                if (layer.name() == layer_name)
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
                error_msg = s.str();
            }
        }
    }
    else if (layer_id->IsNumber())
    {
        layer_idx = layer_id->IntegerValue();
        if (layer_idx < 0) {
            std::ostringstream s;
            s << "Zero-based layer index '" << layer_idx << "' not valid"
              << " must be a positive integer";
            if (layer_num > 0)
            {
                s << "only '" << layer_num << "' layers exist in map";
            }
            else
            {
                s << "no layers found in map";
            }
            error_msg = s.str();
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
            error_msg = s.str();
        }
    } else {
        // This should never be reached as info should have been caught earlier
        // LCOV_EXCL_START
        error_msg = "layer id must be a string or index number";
        // LCOV_EXCL_END
    }
}

void write_geojson_to_string(std::string & result,
                             bool array,
                             bool all,
                             int layer_idx,
                             VectorTile * v)
{
    vector_tile::Tile tiledata = detail::get_tile(v->buffer_);
    if (array)
    {
        unsigned layer_num = tiledata.layers_size();
        result += "[";
        bool first = true;
        for (unsigned i=0;i<layer_num;++i)
        {
            vector_tile::Tile_Layer const& layer = tiledata.layers(i);
            if (first) first = false;
            else result += ",";
            result += "{\"type\":\"FeatureCollection\",";
            result += "\"name\":\"" + layer.name() + "\",\"features\":[";
            std::string features;
            bool hit = layer_to_geojson(layer,features,v->x_,v->y_,v->z_,v->width());
            if (hit)
            {
                result += features;
            }
            result += "]}";
        }
        result += "]";
    }
    else
    {
        if (all)
        {
            result += "{\"type\":\"FeatureCollection\",\"features\":[";
            bool first = true;
            unsigned layer_num = tiledata.layers_size();
            for (unsigned i=0;i<layer_num;++i)
            {
                vector_tile::Tile_Layer const& layer = tiledata.layers(i);
                std::string features;
                bool hit = layer_to_geojson(layer,features,v->x_,v->y_,v->z_,v->width());
                if (hit)
                {
                    if (first) first = false;
                    else result += ",";
                    result += features;
                }
            }
            result += "]}";
        }
        else
        {
            vector_tile::Tile_Layer const& layer = tiledata.layers(layer_idx);
            result += "{\"type\":\"FeatureCollection\",";
            result += "\"name\":\"" + layer.name() + "\",\"features\":[";
            layer_to_geojson(layer,result,v->x_,v->y_,v->z_,v->width());
            result += "]}";
        }
    }
}

v8::Local<v8::Value> VectorTile::_toGeoJSONSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    if (info.Length() < 1) {
        Nan::ThrowError("first argument must be either a layer name (string) or layer index (integer)");
        return scope.Escape(Nan::Undefined());
    }
    v8::Local<v8::Value> layer_id = info[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
        Nan::ThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        return scope.Escape(Nan::Undefined());
    }

    VectorTile* v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    int layer_idx = -1;
    bool all_array = false;
    bool all_flattened = false;
    std::string error_msg;
    std::string result;
    try
    {
        vector_tile::Tile tiledata = detail::get_tile(v->buffer_);
        handle_to_geojson_args(layer_id,
                               tiledata,
                               all_array,
                               all_flattened,
                               error_msg,
                               layer_idx);
        if (!error_msg.empty())
        {
            Nan::ThrowTypeError(error_msg.c_str());
            return scope.Escape(Nan::Undefined());
        }
        write_geojson_to_string(result,all_array,all_flattened,layer_idx,v);
    }
    catch (std::exception const& ex)
    {
        // There are currently no known ways to trigger this exception in testing. If it was
        // triggered this would likely be a bug in either mapnik or mapnik-vector-tile.
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_END
    }
    return scope.Escape(Nan::New<v8::String>(result).ToLocalChecked());
}

struct to_geojson_baton {
    uv_work_t request;
    VectorTile* v;
    bool error;
    std::string result;
    int layer_idx;
    bool all_array;
    bool all_flattened;
    Nan::Persistent<v8::Function> cb;
};

/**
 * Get a [GeoJSON](http://geojson.org/) representation of this tile
 *
 * @memberof mapnik.VectorTile
 * @name toGeoJSON
 * @instance
 * @param {Function} callback
 * @returns {string} stringified GeoJSON of all the features in this tile.
 */
NAN_METHOD(VectorTile::toGeoJSON)
{
    if ((info.Length() < 1) || !info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_toGeoJSONSync(info));
        return;
    }
    to_geojson_baton *closure = new to_geojson_baton();
    closure->request.data = closure;
    closure->v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    closure->error = false;
    closure->layer_idx = -1;
    closure->all_array = false;
    closure->all_flattened = false;

    std::string error_msg;

    v8::Local<v8::Value> layer_id = info[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
        delete closure;
        Nan::ThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        return;
    }

    try
    {
        vector_tile::Tile tiledata = detail::get_tile(closure->v->buffer_);

        handle_to_geojson_args(layer_id,
                               tiledata,
                               closure->all_array,
                               closure->all_flattened,
                               error_msg,
                               closure->layer_idx);
        if (!error_msg.empty())
        {
            delete closure;
            Nan::ThrowTypeError(error_msg.c_str());
            return;
        }
    }
    catch (std::exception const& ex)
    {
        delete closure;
        Nan::ThrowTypeError(error_msg.c_str());
        return;
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
        write_geojson_to_string(closure->result,closure->all_array,closure->all_flattened,closure->layer_idx,closure->v);
    }
    catch (std::exception const& ex)
    {
        // There are currently no known ways to trigger this exception in testing. If it was
        // triggered this would likely be a bug in either mapnik or mapnik-vector-tile.
        // LCOV_EXCL_START
        closure->error = true;
        closure->result = ex.what();
        // LCOV_EXCL_END
    }
}

void VectorTile::after_to_geojson(uv_work_t* req)
{
    Nan::HandleScope scope;
    to_geojson_baton *closure = static_cast<to_geojson_baton *>(req->data);
    if (closure->error)
    {
        // Because there are no known ways to trigger the exception path in to_geojson
        // there is no easy way to test this path currently
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->result.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::New<v8::String>(closure->result).ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->v->Unref();
    closure->cb.Reset();
    delete closure;
}


NAN_METHOD(VectorTile::parseSync)
{
    info.GetReturnValue().Set(_parseSync(info));
}

v8::Local<v8::Value> VectorTile::_parseSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    try
    {
        d->parse_proto();
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_parse_baton_t;

NAN_METHOD(VectorTile::parse)
{
    if (info.Length() == 0) {
        info.GetReturnValue().Set(_parseSync(info));
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    vector_tile_parse_baton_t *closure = new vector_tile_parse_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Parse, (uv_after_work_cb)EIO_AfterParse);
    d->Ref();
    return;
}

void VectorTile::EIO_Parse(uv_work_t* req)
{
    vector_tile_parse_baton_t *closure = static_cast<vector_tile_parse_baton_t *>(req->data);
    try
    {
        closure->d->parse_proto();
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterParse(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_parse_baton_t *closure = static_cast<vector_tile_parse_baton_t *>(req->data);
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Add features to this tile from a GeoJSON string
 *
 * @memberof mapnik.VectorTile
 * @name addGeoJSON
 * @instance
 * @param {string} geojson as a string
 * @param {string} name of the layer to be added
 */
NAN_METHOD(VectorTile::addGeoJSON)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsString()) {
        Nan::ThrowError("first argument must be a GeoJSON string");
        return;
    }
    if (info.Length() < 2 || !info[1]->IsString()) {
        Nan::ThrowError("second argument must be a layer name (string)");
        return;
    }
    std::string geojson_string = TOSTR(info[0]);
    std::string geojson_name = TOSTR(info[1]);

    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    double area_threshold = 0.1;
    double simplify_distance = 0.0;
    unsigned path_multiplier = 16;
    int buffer_size = 8;
    bool strictly_simple = false;

    if (info.Length() > 2) {
        // options object
        if (!info[2]->IsObject()) {
            Nan::ThrowError("optional third argument must be an options object");
            return;
        }

        options = info[2]->ToObject();

        if (options->Has(Nan::New("area_threshold").ToLocalChecked())) {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("area_threshold").ToLocalChecked());
            if (!param_val->IsNumber()) {
                Nan::ThrowError("option 'area_threshold' must be a number");
                return;
            }
            area_threshold = param_val->IntegerValue();
        }

        if (options->Has(Nan::New("strictly_simple").ToLocalChecked())) {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("strictly_simple").ToLocalChecked());
            if (!param_val->IsBoolean()) {
                Nan::ThrowError("option 'strictly_simple' must be a boolean");
                return;
            }
            strictly_simple = param_val->BooleanValue();
        }

        if (options->Has(Nan::New("path_multiplier").ToLocalChecked())) {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("path_multiplier").ToLocalChecked());
            if (!param_val->IsNumber()) {
                Nan::ThrowError("option 'path_multiplier' must be an unsigned integer");
                return;
            }
            path_multiplier = param_val->NumberValue();
        }

        if (options->Has(Nan::New("simplify_distance").ToLocalChecked())) {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("simplify_distance").ToLocalChecked());
            if (!param_val->IsNumber()) {
                Nan::ThrowTypeError("option 'simplify_distance' must be an floating point number");
                return;
            }
            simplify_distance = param_val->NumberValue();
        }
        
        if (options->Has(Nan::New("buffer_size").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("buffer_size").ToLocalChecked());
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }
            buffer_size = bind_opt->IntegerValue();
        }
    }

    try
    {
        typedef mapnik::vector_tile_impl::backend_pbf backend_type;
        typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
        vector_tile::Tile tiledata;
        backend_type backend(tiledata,path_multiplier);
        mapnik::Map map(d->width_,d->height_,"+init=epsg:3857");
        mapnik::vector_tile_impl::spherical_mercator merc(d->width_);
        double minx,miny,maxx,maxy;
        merc.xyz(d->x_,d->y_,d->z_,minx,miny,maxx,maxy);
        map.zoom_to_box(mapnik::box2d<double>(minx,miny,maxx,maxy));
        mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
        m_req.set_buffer_size(buffer_size);
        mapnik::parameters p;
        p["type"]="geojson";
        p["inline"]=geojson_string;
        mapnik::layer lyr(geojson_name,"+init=epsg:4326");
        lyr.set_datasource(mapnik::datasource_cache::instance().create(p));
        map.add_layer(lyr);
        renderer_type ren(backend,
                          map,
                          m_req,
                          1,
                          0,
                          0,
                          area_threshold,
                          strictly_simple);
        ren.set_simplify_distance(simplify_distance);
        ren.apply();
        detail::add_tile(d->buffer_,tiledata);
        info.GetReturnValue().Set(Nan::True());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
}

NAN_METHOD(VectorTile::addImage)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.This());
    if (info.Length() < 1 || !info[0]->IsObject()) {
        Nan::ThrowError("first argument must be a Buffer representing encoded image data");
        return;
    }
    if (info.Length() < 2 || !info[1]->IsString()) {
        Nan::ThrowError("second argument must be a layer name (string)");
        return;
    }
    std::string layer_name = TOSTR(info[1]);
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowError("first argument must be a Buffer representing encoded image data");
        return;
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        Nan::ThrowError("cannot accept empty buffer as image");
        return;
    }
    // how to ensure buffer width/height?
    vector_tile::Tile tiledata;
    vector_tile::Tile_Layer * new_layer = tiledata.add_layers();
    new_layer->set_name(layer_name);
    new_layer->set_version(1);
    new_layer->set_extent(256 * 16);
    // no need
    // current_feature_->set_id(feature.id());
    vector_tile::Tile_Feature * new_feature = new_layer->add_features();
    new_feature->set_raster(std::string(node::Buffer::Data(obj),buffer_size));
    // report that we have data
    detail::add_tile(d->buffer_,tiledata);
    return;
}

/**
 * Add raw data to this tile as a Buffer
 *
 * @memberof mapnik.VectorTile
 * @name addData
 * @instance
 * @param {Buffer} raw data
 * @param {string} name of the layer to be added
 */
NAN_METHOD(VectorTile::addData)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowError("first argument must be a buffer object");
        return;
    }
    v8::Local<v8::Object> obj = info[0].As<v8::Object>();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return;
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        Nan::ThrowError("cannot accept empty buffer as protobuf");
        return;
    }
    d->buffer_.append(node::Buffer::Data(obj),buffer_size);
    return;
}

NAN_METHOD(VectorTile::setDataSync)
{
    info.GetReturnValue().Set(_setDataSync(info));
}

v8::Local<v8::Value> VectorTile::_setDataSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject()) {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return scope.Escape(Nan::Undefined());
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        Nan::ThrowError("cannot accept empty buffer as protobuf");
        return scope.Escape(Nan::Undefined());
    }
    const char * data = node::Buffer::Data(obj);
    if (mapnik::vector_tile_impl::is_gzip_compressed(data,buffer_size) ||
        mapnik::vector_tile_impl::is_zlib_compressed(data,buffer_size))
    {
        try
        {
            mapnik::vector_tile_impl::zlib_decompress(data, buffer_size, d->buffer_);
        }
        catch (std::exception const& ex)
        {
            Nan::ThrowTypeError( (std::string("failed decoding compressed data ") + ex.what()).c_str() );
            return scope.Escape(Nan::Undefined());
        }
    }
    else
    {
        d->buffer_ = std::string(node::Buffer::Data(obj),buffer_size);
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    const char *data;
    size_t dataLength;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    Nan::Persistent<v8::Object> buffer;
} vector_tile_setdata_baton_t;


/**
 * Replace the data in this vector tile with new raw data
 *
 * @memberof mapnik.VectorTile
 * @name setData
 * @instance
 * @param {Buffer} raw data
 * @param {string} name of the layer to be added
 */
NAN_METHOD(VectorTile::setData)
{
    if (info.Length() == 1) {
        info.GetReturnValue().Set(_setDataSync(info));
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length() - 1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    if (info.Length() < 1 || !info[0]->IsObject()) {
        Nan::ThrowTypeError("first argument must be a buffer object");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("first arg must be a buffer object");
        return;
    }

    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    vector_tile_setdata_baton_t *closure = new vector_tile_setdata_baton_t();
    closure->request.data = closure;
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

    try
    {
        if (mapnik::vector_tile_impl::is_gzip_compressed(closure->data,closure->dataLength) ||
            mapnik::vector_tile_impl::is_zlib_compressed(closure->data,closure->dataLength))
        {
            mapnik::vector_tile_impl::zlib_decompress(closure->data,closure->dataLength, closure->d->buffer_);
        }
        else
        {
            closure->d->buffer_ = std::string(closure->data,closure->dataLength);
        }
    }
    catch (std::exception const& ex)
    {
        // This code is pointless as there is no known way for a buffer to create an invalid string here
        // if there was this would be a bug in node.
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = ex.what();
        // LCOV_EXCL_END
    }
}

void VectorTile::EIO_AfterSetData(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_setdata_baton_t *closure = static_cast<vector_tile_setdata_baton_t *>(req->data);
    if (closure->error) {
        // See note about exception in EIO_SetData
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }

    closure->d->Unref();
    closure->cb.Reset();
    closure->buffer.Reset();
    delete closure;
}

/**
 * Get the data in this vector tile as a buffer
 *
 * @memberof mapnik.VectorTile
 * @name getDataSync
 * @instance
 * @returns {Buffer} raw data
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

        options = info[0]->ToObject();

        if (options->Has(Nan::New<v8::String>("compression").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("compression").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'compression' must be a string, either 'gzip', or 'none' (default)");
                return scope.Escape(Nan::Undefined());
            }
            compress = std::string("gzip") == (TOSTR(param_val->ToString()));
        }

        if (options->Has(Nan::New<v8::String>("level").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("level").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return scope.Escape(Nan::Undefined());
            }
            level = param_val->IntegerValue();
            if (level < 0 || level > 9)
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return scope.Escape(Nan::Undefined());
            }
        }
        if (options->Has(Nan::New<v8::String>("strategy").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("strategy").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return scope.Escape(Nan::Undefined());
            }
            else if (std::string("FILTERED") == TOSTR(param_val->ToString()))
            {
                strategy = Z_FILTERED;
            }
            else if (std::string("HUFFMAN_ONLY") == TOSTR(param_val->ToString()))
            {
                strategy = Z_HUFFMAN_ONLY;
            }
            else if (std::string("RLE") == TOSTR(param_val->ToString()))
            {
                strategy = Z_RLE;
            }
            else if (std::string("FIXED") == TOSTR(param_val->ToString()))
            {
                strategy = Z_FIXED;
            }
            else if (std::string("DEFAULT") == TOSTR(param_val->ToString()))
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
        std::size_t raw_size = d->buffer_.size();
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
                // LCOV_EXCL_END
            }
            if (!compress)
            {
                return scope.Escape(Nan::CopyBuffer((char*)d->buffer_.data(),raw_size).ToLocalChecked());
            }
            else
            {
                std::string compressed;
                mapnik::vector_tile_impl::zlib_compress(d->buffer_, compressed, true, level, strategy);
                return scope.Escape(Nan::CopyBuffer((char*)compressed.data(),compressed.size()).ToLocalChecked());
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
        // LCOV_EXCL_END
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    bool error;
    std::string data;
    bool compress;
    int level;
    int strategy;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} vector_tile_get_data_baton_t;


NAN_METHOD(VectorTile::getData)
{
    if (info.Length() == 0 || !info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_getDataSync(info));
        return;
    }

    v8::Local<v8::Value> callback = info[info.Length()-1];
    bool compress = false;
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

        options = info[0]->ToObject();

        if (options->Has(Nan::New("compression").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("compression").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'compression' must be a string, either 'gzip', or 'none' (default)");
                return;
            }
            compress = std::string("gzip") == (TOSTR(param_val->ToString()));
        }

        if (options->Has(Nan::New("level").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("level").ToLocalChecked());
            if (!param_val->IsNumber())
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return;
            }
            level = param_val->IntegerValue();
            if (level < 0 || level > 9)
            {
                Nan::ThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return;
            }
        }
        if (options->Has(Nan::New("strategy").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("strategy").ToLocalChecked());
            if (!param_val->IsString())
            {
                Nan::ThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return;
            }
            else if (std::string("FILTERED") == TOSTR(param_val->ToString()))
            {
                strategy = Z_FILTERED;
            }
            else if (std::string("HUFFMAN_ONLY") == TOSTR(param_val->ToString()))
            {
                strategy = Z_HUFFMAN_ONLY;
            }
            else if (std::string("RLE") == TOSTR(param_val->ToString()))
            {
                strategy = Z_RLE;
            }
            else if (std::string("FIXED") == TOSTR(param_val->ToString()))
            {
                strategy = Z_FIXED;
            }
            else if (std::string("DEFAULT") == TOSTR(param_val->ToString()))
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
            mapnik::vector_tile_impl::zlib_compress(closure->d->buffer_, closure->data, true, closure->level, closure->strategy);
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
        // LCOV_EXCL_END
    }
}

void VectorTile::after_get_data(uv_work_t* req)
{
    Nan::HandleScope scope;
    vector_tile_get_data_baton_t *closure = static_cast<vector_tile_get_data_baton_t *>(req->data);
    if (closure->error) 
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else if (!closure->data.empty())
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::CopyBuffer((char*)closure->data.data(),closure->data.size()).ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    else
    {
        std::size_t raw_size = closure->d->buffer_.size();
        if (raw_size <= 0)
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::NewBuffer(0).ToLocalChecked() };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
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
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
            // LCOV_EXCL_END
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::CopyBuffer((char*)closure->d->buffer_.data(),raw_size).ToLocalChecked() };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }

    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

using surface_type = mapnik::util::variant<Image *
  , CairoSurface *
#if defined(GRID_RENDERER)
  , Grid *
#endif
  >;

struct deref_visitor
{
    template <typename SurfaceType>
    void operator() (SurfaceType * surface)
    {
        if (surface != nullptr)
        {
            surface->_unref();
        }
    }
};

struct vector_tile_render_baton_t {
    uv_work_t request;
    Map* m;
    VectorTile * d;
    surface_type surface;
    mapnik::attributes variables;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    std::string result;
    std::size_t layer_idx;
    int z;
    int x;
    int y;
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
        surface(nullptr),
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
    ~vector_tile_render_baton_t() {
        mapnik::util::apply_visitor(deref_visitor(),surface);
    }
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
 * Render this vector tile to a surface, like a {@link mapnik.Image}
 *
 * @memberof mapnik.VectorTile
 * @name getData
 * @instance
 * @param {mapnik.Map} map object
 * @param {mapnik.Image} renderable surface
 * @param {Function} callback
 */
NAN_METHOD(VectorTile::render)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    if (info.Length() < 1 || !info[0]->IsObject()) {
        Nan::ThrowTypeError("mapnik.Map expected as first arg");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Map::constructor)->HasInstance(obj)) {
        Nan::ThrowTypeError("mapnik.Map expected as first arg");
        return;
    }

    Map *m = Nan::ObjectWrap::Unwrap<Map>(obj);
    if (info.Length() < 2 || !info[1]->IsObject()) {
        Nan::ThrowTypeError("a renderable mapnik object is expected as second arg");
        return;
    }
    v8::Local<v8::Object> im_obj = info[1]->ToObject();

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
        if (!info[2]->IsObject())
        {
            Nan::ThrowTypeError("optional third argument must be an options object");
            return;
        }
        options = info[2]->ToObject();
        if (options->Has(Nan::New("z").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("z").ToLocalChecked());
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'z' must be a number");
                return;
            }
            closure->z = bind_opt->IntegerValue();
            closure->zxy_override = true;
        }
        if (options->Has(Nan::New("x").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("x").ToLocalChecked());
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'x' must be a number");
                return;
            }
            closure->x = bind_opt->IntegerValue();
            closure->zxy_override = true;
        }
        if (options->Has(Nan::New("y").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("y").ToLocalChecked());
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'y' must be a number");
                return;
            }
            closure->y = bind_opt->IntegerValue();
            closure->zxy_override = true;
        }
        if (options->Has(Nan::New("buffer_size").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("buffer_size").ToLocalChecked());
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }
            closure->buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("scale").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("scale").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale' must be a number");
                return;
            }
            closure->scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(Nan::New("scale_denominator").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("scale_denominator").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'scale_denominator' must be a number");
                return;
            }
            closure->scale_denominator = bind_opt->NumberValue();
        }
        if (options->Has(Nan::New("variables").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("variables").ToLocalChecked());
            if (!bind_opt->IsObject())
            {
                Nan::ThrowTypeError("optional arg 'variables' must be an object");
                return;
            }
            object_to_container(closure->variables,bind_opt->ToObject());
        }
    }

    closure->layer_idx = 0;
    if (Nan::New(Image::constructor)->HasInstance(im_obj))
    {
        Image *im = Nan::ObjectWrap::Unwrap<Image>(im_obj);
        im->_ref();
        closure->width = im->get()->width();
        closure->height = im->get()->height();
        closure->surface = im;
    }
    else if (Nan::New(CairoSurface::constructor)->HasInstance(im_obj))
    {
        CairoSurface *c = Nan::ObjectWrap::Unwrap<CairoSurface>(im_obj);
        c->_ref();
        closure->width = c->width();
        closure->height = c->height();
        closure->surface = c;
        if (options->Has(Nan::New("renderer").ToLocalChecked()))
        {
            v8::Local<v8::Value> renderer = options->Get(Nan::New("renderer").ToLocalChecked());
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
        g->_ref();
        closure->width = g->get()->width();
        closure->height = g->get()->height();
        closure->surface = g;

        std::size_t layer_idx = 0;

        // grid requires special options for now
        if (!options->Has(Nan::New("layer").ToLocalChecked()))
        {
            Nan::ThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
            return;
        } else {
            std::vector<mapnik::layer> const& layers = m->get()->layers();

            v8::Local<v8::Value> layer_id = options->Get(Nan::New("layer").ToLocalChecked());

            if (layer_id->IsString()) {
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
            } else if (layer_id->IsNumber()) {
                layer_idx = layer_id->IntegerValue();
                std::size_t layer_num = layers.size();

                if (layer_idx >= layer_num) {
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
        if (options->Has(Nan::New("fields").ToLocalChecked())) {

            v8::Local<v8::Value> param_val = options->Get(Nan::New("fields").ToLocalChecked());
            if (!param_val->IsArray())
            {
                Nan::ThrowTypeError("option 'fields' must be an array of strings");
                return;
            }
            v8::Local<v8::Array> a = v8::Local<v8::Array>::Cast(param_val);
            unsigned int i = 0;
            unsigned int num_fields = a->Length();
            while (i < num_fields) {
                v8::Local<v8::Value> name = a->Get(i);
                if (name->IsString()){
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
    m->_ref();
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
    // get pbf layers into structure ready to query and render
    using layer_list_type = std::vector<protozero::pbf_reader>;
    std::map<std::string,layer_list_type> pbf_layers;
    protozero::pbf_reader item(closure->d->buffer_.data(),closure->d->buffer_.size());
    while (item.next(3)) {
        protozero::pbf_reader layer_msg = item.get_message();
        // make a copy to ensure that the `get_string()` does not mutate the internal
        // pointers of the `layer_og` stored in the map
        // good thing copies are cheap
        protozero::pbf_reader layer_og(layer_msg);
        std::string layer_name;
        while (layer_msg.next(1)) {
            layer_name = layer_msg.get_string();
        }
        if (!layer_name.empty())
        {
            // we accept dupes currently, even though this
            // is of dubious value (tested by `should render by underzooming or mosaicing` in node-mapnik)
            auto itr = pbf_layers.find(layer_name);
            if (itr == pbf_layers.end())
            {
                pbf_layers.emplace(layer_name,layer_list_type{std::move(layer_og)});
            }
            else
            {
                itr->second.push_back(std::move(layer_og));
            }
        }
    }
    // loop over layers in map and match by name
    // with layers in the vector tile
    for (auto const& lyr : layers)
    {
        if (lyr.visible(scale_denom))
        {
            auto itr = pbf_layers.find(lyr.name());
            if (itr != pbf_layers.end())
            {
                for (auto const& pb : itr->second)
                {
                    mapnik::layer lyr_copy(lyr);
                    lyr_copy.set_srs(map_srs);
                    protozero::pbf_reader layer_og(pb);
                    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                                        layer_og,
                                                        closure->d->x_,
                                                        closure->d->y_,
                                                        closure->d->z_,
                                                        closure->d->width()
                                                        );
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
}

void VectorTile::EIO_RenderTile(uv_work_t* req)
{
    vector_tile_render_baton_t *closure = static_cast<vector_tile_render_baton_t *>(req->data);

    try {
        mapnik::Map const& map_in = *closure->m->get();
        mapnik::vector_tile_impl::spherical_mercator merc(closure->d->width_);
        double minx,miny,maxx,maxy;
        if (closure->zxy_override) {
            merc.xyz(closure->x,closure->y,closure->z,minx,miny,maxx,maxy);
        } else {
            merc.xyz(closure->d->x_,closure->d->y_,closure->d->z_,minx,miny,maxx,maxy);
        }
        mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
        mapnik::request m_req(closure->width,closure->height,map_extent);
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
                if (detail::pbf_get_layer(closure->d->buffer_,lyr.name(),layer_msg))
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
                                                        closure->d->x_,
                                                        closure->d->y_,
                                                        closure->d->z_,
                                                        closure->d->width_
                                                        );
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
    vector_tile_render_baton_t *closure = static_cast<vector_tile_render_baton_t *>(req->data);
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        if (closure->surface.is<Image *>())
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), mapnik::util::get<Image *>(closure->surface)->handle() };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
#if defined(GRID_RENDERER)
        else if (closure->surface.is<Grid *>())
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), mapnik::util::get<Grid *>(closure->surface)->handle() };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
#endif
        else if (closure->surface.is<CairoSurface *>())
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), mapnik::util::get<CairoSurface *>(closure->surface)->handle() };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }

    closure->m->_unref();
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}
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

typedef struct {
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
 * @memberof mapnik.VectorTile
 * @name clear
 * @instance
 * @param {Function} callback
 */
NAN_METHOD(VectorTile::clear)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    if (info.Length() == 0) {
        info.GetReturnValue().Set(_clearSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction()) {
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
        // LCOV_EXCL_END
    }
}

void VectorTile::EIO_AfterClear(uv_work_t* req)
{
    Nan::HandleScope scope;
    clear_vector_tile_baton_t *closure = static_cast<clear_vector_tile_baton_t *>(req->data);
    if (closure->error)
    {
        // No reason this should ever throw an exception, not currently testable.
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Test whether this tile is solid - whether it has features
 *
 * @memberof mapnik.VectorTile
 * @name isSolidSync
 * @instance
 * @returns {boolean} whether the tile is solid
 */
NAN_METHOD(VectorTile::isSolidSync)
{
    info.GetReturnValue().Set(_isSolidSync(info));
}

v8::Local<v8::Value> VectorTile::_isSolidSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    try
    {
        std::string key;
        bool is_solid = mapnik::vector_tile_impl::is_solid_extent(d->buffer_, key);
        if (is_solid)
        {
            return scope.Escape(Nan::New<v8::String>(key).ToLocalChecked());
        }
        else
        {
            return scope.Escape(Nan::False());
        }
    }
    catch (std::exception const& ex)
    {
        // There is a chance of this throwing an error, however, only in the situation such that there
        // is an illegal command within the vector tile. 
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_END
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    std::string key;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
    bool result;
} is_solid_vector_tile_baton_t;

/**
 * Test whether this tile is solid - whether it has features
 *
 * @memberof mapnik.VectorTile
 * @name isSolid
 * @instance
 * @param {Function} callback
 */
NAN_METHOD(VectorTile::isSolid)
{
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());

    if (info.Length() == 0) {
        info.GetReturnValue().Set(_isSolidSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    is_solid_vector_tile_baton_t *closure = new is_solid_vector_tile_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->result = true;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    d->Ref();
    return;
}

void VectorTile::EIO_IsSolid(uv_work_t* req)
{
    is_solid_vector_tile_baton_t *closure = static_cast<is_solid_vector_tile_baton_t *>(req->data);
    try
    {
        closure->result = mapnik::vector_tile_impl::is_solid_extent(closure->d->buffer_,closure->key);
    }
    catch (std::exception const& ex)
    {
        // There is a chance of this throwing an error, however, only in the situation such that there
        // is an illegal command within the vector tile. 
        // LCOV_EXCL_START
        closure->error = true;
        closure->error_name = ex.what();
        // LCOV_EXCL_END
    }
}

void VectorTile::EIO_AfterIsSolid(uv_work_t* req)
{
    Nan::HandleScope scope;
    is_solid_vector_tile_baton_t *closure = static_cast<is_solid_vector_tile_baton_t *>(req->data);
    if (closure->error) 
    {
        // There is a chance of this throwing an error, however, only in the situation such that there
        // is an illegal command within the vector tile. 
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else
    {
        v8::Local<v8::Value> argv[3] = { Nan::Null(),
                                 Nan::New<v8::Boolean>(closure->result),
                                 Nan::New<v8::String>(closure->key).ToLocalChecked()
        };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 3, argv);
    }
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

#if BOOST_VERSION >= 105800

struct not_simple_feature
{
    not_simple_feature(std::string const& layer_, 
                       std::int64_t feature_id_)
        : layer(layer_),
          feature_id(feature_id_) {} 
    std::string const layer;
    std::int64_t const feature_id;
};

struct not_valid_feature
{
    not_valid_feature(std::string const& message_,
                      std::string const& layer_, 
                      std::int64_t feature_id_)
        : message(message_),
          layer(layer_),
          feature_id(feature_id_) {} 
    std::string const message;
    std::string const layer;
    std::int64_t const feature_id;
};

void layer_not_simple(vector_tile::Tile_Layer const& layer,
               unsigned x,
               unsigned y,
               unsigned z,
               unsigned width,
               std::vector<not_simple_feature> & errors)
{
    mapnik::vector_tile_impl::tile_datasource ds(layer,
                                                 x,
                                                 y,
                                                 z,
                                                 width);
    mapnik::query q(ds.envelope());
    mapnik::layer_descriptor ld = ds.get_descriptor();
    for (auto const& item : ld.get_descriptors())
    {
        q.add_property_name(item.get_name());
    }
    mapnik::featureset_ptr fs = ds.features(q);
    if (fs)
    {
        mapnik::feature_ptr feature;
        while ((feature = fs->next()))
        {
            if (!mapnik::geometry::is_simple(feature->get_geometry()))
            {
                errors.emplace_back(layer.name(), feature->id());
            }
        }
    }
}

void layer_not_valid(vector_tile::Tile_Layer const& layer,
               unsigned x,
               unsigned y,
               unsigned z,
               unsigned width,
               std::vector<not_valid_feature> & errors)
{
    mapnik::vector_tile_impl::tile_datasource ds(layer,
                                                 x,
                                                 y,
                                                 z,
                                                 width);
    mapnik::query q(ds.envelope());
    mapnik::layer_descriptor ld = ds.get_descriptor();
    for (auto const& item : ld.get_descriptors())
    {
        q.add_property_name(item.get_name());
    }
    mapnik::featureset_ptr fs = ds.features(q);
    if (fs)
    {
        mapnik::feature_ptr feature;
        while ((feature = fs->next()))
        {
            std::string message;
            if (!mapnik::geometry::is_valid(feature->get_geometry(), message))
            {
                errors.emplace_back(message,
                                    layer.name(),
                                    feature->id());
            }
        }
    }
}

void vector_tile_not_simple(VectorTile * v,
                            std::vector<not_simple_feature> & errors)
{
    vector_tile::Tile tiledata = detail::get_tile(v->buffer_);
    unsigned layer_num = tiledata.layers_size();
    for (unsigned i=0;i<layer_num;++i)
    {
        vector_tile::Tile_Layer const& layer = tiledata.layers(i);
        layer_not_simple(layer,
                         v->x_,
                         v->y_,
                         v->z_,
                         v->width(),
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
        v8::Local<v8::Object> obj = Nan::New<v8::Object>();
        obj->Set(layer_key, Nan::New<v8::String>(error.layer).ToLocalChecked());
        obj->Set(feature_id_key, Nan::New<v8::Number>(error.feature_id));
        array->Set(idx++, obj);
    }
    return scope.Escape(array);
}

void vector_tile_not_valid(VectorTile * v,
                           std::vector<not_valid_feature> & errors)
{
    vector_tile::Tile tiledata = detail::get_tile(v->buffer_);
    unsigned layer_num = tiledata.layers_size();
    for (unsigned i=0;i<layer_num;++i)
    {
        vector_tile::Tile_Layer const& layer = tiledata.layers(i);
        layer_not_valid(layer,
                        v->x_,
                        v->y_,
                        v->z_,
                        v->width(),
                        errors);
    }
}

v8::Local<v8::Array> make_not_valid_array(std::vector<not_valid_feature> & errors) 
{
    Nan::EscapableHandleScope scope;
    v8::Local<v8::Array> array = Nan::New<v8::Array>(errors.size());
    v8::Local<v8::String> layer_key = Nan::New<v8::String>("layer").ToLocalChecked();
    v8::Local<v8::String> feature_id_key = Nan::New<v8::String>("featureId").ToLocalChecked();
    v8::Local<v8::String> message_key = Nan::New<v8::String>("message").ToLocalChecked();
    std::uint32_t idx = 0;
    for (auto const& error : errors)
    {
        v8::Local<v8::Object> obj = Nan::New<v8::Object>();
        obj->Set(layer_key, Nan::New<v8::String>(error.layer).ToLocalChecked());
        obj->Set(message_key, Nan::New<v8::String>(error.message).ToLocalChecked());
        obj->Set(feature_id_key, Nan::New<v8::Number>(error.feature_id));
        array->Set(idx++, obj);
    }
    return scope.Escape(array);
}


struct not_simple_baton {
    uv_work_t request;
    VectorTile* v;
    bool error;
    std::vector<not_simple_feature> result;
    std::string err_msg;
    Nan::Persistent<v8::Function> cb;
};

struct not_valid_baton {
    uv_work_t request;
    VectorTile* v;
    bool error;
    std::vector<not_valid_feature> result;
    std::string err_msg;
    Nan::Persistent<v8::Function> cb;
};

/**
 * Count the number of geometries that are not OGC simple
 *
 * @memberof mapnik.VectorTile
 * @name reportGeometrySimplicitySync
 * @instance
 * @returns {number} number of features that are not simple 
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
        Nan::ThrowError(ex.what());
    }
    return scope.Escape(Nan::Undefined());
}

/**
 * Count the number of geometries that are not OGC valid
 *
 * @memberof mapnik.VectorTile
 * @name reportGeometryValiditySync
 * @instance
 * @returns {number} number of features that are not valid
 */
NAN_METHOD(VectorTile::reportGeometryValiditySync)
{
    info.GetReturnValue().Set(_reportGeometryValiditySync(info));
}

v8::Local<v8::Value> VectorTile::_reportGeometryValiditySync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    try
    {
        std::vector<not_valid_feature> errors;
        vector_tile_not_valid(d, errors);
        return scope.Escape(make_not_valid_array(errors));
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
    }
    return scope.Escape(Nan::Undefined());
}

/**
 * Count the number of non OGC simple geometries
 *
 * @memberof mapnik.VectorTile
 * @name reportGeometrySimplicity
 * @instance
 * @param {Function} callback
 */
NAN_METHOD(VectorTile::reportGeometrySimplicity)
{
    if (info.Length() == 0) {
        info.GetReturnValue().Set(_reportGeometrySimplicitySync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction()) {
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
        closure->error = true;
        closure->err_msg = ex.what();
    }
}

void VectorTile::EIO_AfterReportGeometrySimplicity(uv_work_t* req)
{
    Nan::HandleScope scope;
    not_simple_baton *closure = static_cast<not_simple_baton *>(req->data);
    if (closure->error) 
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->err_msg.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Array> array = make_not_simple_array(closure->result);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), array };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->v->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Count the number of non OGC valid geometries
 *
 * @memberof mapnik.VectorTile
 * @name reportGeometryValidity
 * @instance
 * @param {Function} callback
 */
NAN_METHOD(VectorTile::reportGeometryValidity)
{
    if (info.Length() == 0) {
        info.GetReturnValue().Set(_reportGeometryValiditySync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    not_valid_baton *closure = new not_valid_baton();
    closure->request.data = closure;
    closure->v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    closure->error = false;
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
        vector_tile_not_valid(closure->v, closure->result);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->err_msg = ex.what();
    }
}

void VectorTile::EIO_AfterReportGeometryValidity(uv_work_t* req)
{
    Nan::HandleScope scope;
    not_valid_baton *closure = static_cast<not_valid_baton *>(req->data);
    if (closure->error) 
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->err_msg.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
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
