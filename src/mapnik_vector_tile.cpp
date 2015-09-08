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

Persistent<FunctionTemplate> VectorTile::constructor;

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
void VectorTile::Initialize(Handle<Object> target) {
    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(VectorTile::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("VectorTile"));
    NODE_SET_PROTOTYPE_METHOD(lcons, "render", render);
    NODE_SET_PROTOTYPE_METHOD(lcons, "setData", setData);
    NODE_SET_PROTOTYPE_METHOD(lcons, "setDataSync", setDataSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "getData", getData);
    NODE_SET_PROTOTYPE_METHOD(lcons, "getDataSync", getDataSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "parse", parse);
    NODE_SET_PROTOTYPE_METHOD(lcons, "parseSync", parseSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "addData", addData);
    NODE_SET_PROTOTYPE_METHOD(lcons, "composite", composite);
    NODE_SET_PROTOTYPE_METHOD(lcons, "compositeSync", compositeSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "query", query);
    NODE_SET_PROTOTYPE_METHOD(lcons, "queryMany", queryMany);
    NODE_SET_PROTOTYPE_METHOD(lcons, "names", names);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toJSON", toJSON);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toGeoJSON", toGeoJSON);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toGeoJSONSync", toGeoJSONSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "addGeoJSON", addGeoJSON);
    NODE_SET_PROTOTYPE_METHOD(lcons, "addImage", addImage);

    // common to mapnik.Image
    NODE_SET_PROTOTYPE_METHOD(lcons, "width", width);
    NODE_SET_PROTOTYPE_METHOD(lcons, "height", height);
    NODE_SET_PROTOTYPE_METHOD(lcons, "painted", painted);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clearSync", clearSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "empty", empty);
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolid", isSolid);
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolidSync", isSolidSync);
    target->Set(NanNew("VectorTile"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

VectorTile::VectorTile(int z, int x, int y, unsigned w, unsigned h) :
    node::ObjectWrap(),
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
    NanScope();
    if (!args.IsConstructCall()) {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args.Length() >= 3)
    {
        if (!args[0]->IsNumber() ||
            !args[1]->IsNumber() ||
            !args[2]->IsNumber())
        {
            NanThrowTypeError("required args (z, x, and y) must be a integers");
            NanReturnUndefined();
        }
        unsigned width = 256;
        unsigned height = 256;
        Local<Object> options = NanNew<Object>();
        if (args.Length() > 3) {
            if (!args[3]->IsObject())
            {
                NanThrowTypeError("optional fourth argument must be an options object");
                NanReturnUndefined();
            }
            options = args[3]->ToObject();
            if (options->Has(NanNew("width"))) {
                Local<Value> opt = options->Get(NanNew("width"));
                if (!opt->IsNumber())
                {
                    NanThrowTypeError("optional arg 'width' must be a number");
                    NanReturnUndefined();
                }
                width = opt->IntegerValue();
            }
            if (options->Has(NanNew("height"))) {
                Local<Value> opt = options->Get(NanNew("height"));
                if (!opt->IsNumber())
                {
                    NanThrowTypeError("optional arg 'height' must be a number");
                    NanReturnUndefined();
                }
                height = opt->IntegerValue();
            }
        }

        VectorTile* d = new VectorTile(args[0]->IntegerValue(),
                                   args[1]->IntegerValue(),
                                   args[2]->IntegerValue(),
                                   width,height);

        d->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else
    {
        NanThrowError("please provide a z, x, y");
        NanReturnUndefined();
    }
    NanReturnUndefined();
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
                                  area_threshold);
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
 * @param {Array<mapnik.VectorTile>} vector tiles
 */
NAN_METHOD(VectorTile::compositeSync)
{
    NanScope();
    NanReturnValue(_compositeSync(args));

}

Local<Value> VectorTile::_compositeSync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    if (args.Length() < 1 || !args[0]->IsArray()) {
        NanThrowTypeError("must provide an array of VectorTile objects and an optional options object");
        return NanEscapeScope(NanUndefined());
    }
    Local<Array> vtiles = args[0].As<Array>();
    unsigned num_tiles = vtiles->Length();
    if (num_tiles < 1) {
        NanThrowTypeError("must provide an array with at least one VectorTile object and an optional options object");
        return NanEscapeScope(NanUndefined());
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
    double scale_denominator = 0.0;

    if (args.Length() > 1) {
        // options object
        if (!args[1]->IsObject())
        {
            NanThrowTypeError("optional second argument must be an options object");
            return NanEscapeScope(NanUndefined());
        }
        Local<Object> options = args[1]->ToObject();
        if (options->Has(NanNew("path_multiplier"))) {

            Local<Value> param_val = options->Get(NanNew("path_multiplier"));
            if (!param_val->IsNumber())
            {
                NanThrowTypeError("option 'path_multiplier' must be an unsigned integer");
                return NanEscapeScope(NanUndefined());
            }
            path_multiplier = param_val->NumberValue();
        }
        if (options->Has(NanNew("area_threshold")))
        {
            Local<Value> area_thres = options->Get(NanNew("area_threshold"));
            if (!area_thres->IsNumber())
            {
                NanThrowTypeError("area_threshold value must be a number");
                return NanEscapeScope(NanUndefined());
            }
            area_threshold = area_thres->NumberValue();
        }
        if (options->Has(NanNew("buffer_size"))) {
            Local<Value> bind_opt = options->Get(NanNew("buffer_size"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'buffer_size' must be a number");
                return NanEscapeScope(NanUndefined());
            }
            buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(NanNew("scale"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'scale' must be a number");
                return NanEscapeScope(NanUndefined());
            }
            scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("scale_denominator")))
        {
            Local<Value> bind_opt = options->Get(NanNew("scale_denominator"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'scale_denominator' must be a number");
                return NanEscapeScope(NanUndefined());
            }
            scale_denominator = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("offset_x"))) {
            Local<Value> bind_opt = options->Get(NanNew("offset_x"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'offset_x' must be a number");
                return NanEscapeScope(NanUndefined());
            }
            offset_x = bind_opt->IntegerValue();
        }
        if (options->Has(NanNew("offset_y"))) {
            Local<Value> bind_opt = options->Get(NanNew("offset_y"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'offset_y' must be a number");
                return NanEscapeScope(NanUndefined());
            }
            offset_y = bind_opt->IntegerValue();
        }
    }
    VectorTile* target_vt = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    std::vector<VectorTile*> vtiles_vec;
    vtiles_vec.reserve(num_tiles);
    for (unsigned j=0;j < num_tiles;++j) {
        Local<Value> val = vtiles->Get(j);
        if (!val->IsObject()) {
            NanThrowTypeError("must provide an array of VectorTile objects");
            return NanEscapeScope(NanUndefined());
        }
        Local<Object> tile_obj = val->ToObject();
        if (tile_obj->IsNull() || tile_obj->IsUndefined() || !NanNew(VectorTile::constructor)->HasInstance(tile_obj)) {
            NanThrowTypeError("must provide an array of VectorTile objects");
            return NanEscapeScope(NanUndefined());
        }
        vtiles_vec.push_back(node::ObjectWrap::Unwrap<VectorTile>(tile_obj));
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
                   scale_denominator);
    }
    catch (std::exception const& ex)
    {
        NanThrowTypeError(ex.what());
        return NanEscapeScope(NanUndefined());
    }

    return NanEscapeScope(NanUndefined());
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
    std::string error_name;
    Persistent<Function> cb;
} vector_tile_composite_baton_t;

NAN_METHOD(VectorTile::composite)
{
    NanScope();
    if ((args.Length() < 2) || !args[args.Length()-1]->IsFunction()) {
        NanReturnValue(_compositeSync(args));
    }
    if (!args[0]->IsArray()) {
        NanThrowTypeError("must provide an array of VectorTile objects and an optional options object");
        NanReturnUndefined();
    }
    Local<Array> vtiles = args[0].As<Array>();
    unsigned num_tiles = vtiles->Length();
    if (num_tiles < 1) {
        NanThrowTypeError("must provide an array with at least one VectorTile object and an optional options object");
        NanReturnUndefined();
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
    double scale_denominator = 0.0;
    // not options yet, likely should never be....
    mapnik::box2d<double> max_extent(-20037508.34,-20037508.34,20037508.34,20037508.34);
    std::string merc_srs("+init=epsg:3857");

    if (args.Length() > 2) {
        // options object
        if (!args[1]->IsObject())
        {
            NanThrowTypeError("optional second argument must be an options object");
            NanReturnUndefined();
        }
        Local<Object> options = args[1]->ToObject();
        if (options->Has(NanNew("path_multiplier"))) {

            Local<Value> param_val = options->Get(NanNew("path_multiplier"));
            if (!param_val->IsNumber())
            {
                NanThrowTypeError("option 'path_multiplier' must be an unsigned integer");
                NanReturnUndefined();
            }
            path_multiplier = param_val->NumberValue();
        }
        if (options->Has(NanNew("area_threshold")))
        {
            Local<Value> area_thres = options->Get(NanNew("area_threshold"));
            if (!area_thres->IsNumber())
            {
                NanThrowTypeError("area_threshold value must be a number");
                NanReturnUndefined();
            }
            area_threshold = area_thres->NumberValue();
        }
        if (options->Has(NanNew("buffer_size"))) {
            Local<Value> bind_opt = options->Get(NanNew("buffer_size"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'buffer_size' must be a number");
                NanReturnUndefined();
            }
            buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(NanNew("scale"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'scale' must be a number");
                NanReturnUndefined();
            }
            scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("scale_denominator")))
        {
            Local<Value> bind_opt = options->Get(NanNew("scale_denominator"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'scale_denominator' must be a number");
                NanReturnUndefined();
            }
            scale_denominator = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("offset_x"))) {
            Local<Value> bind_opt = options->Get(NanNew("offset_x"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'offset_x' must be a number");
                NanReturnUndefined();
            }
            offset_x = bind_opt->IntegerValue();
        }
        if (options->Has(NanNew("offset_y"))) {
            Local<Value> bind_opt = options->Get(NanNew("offset_y"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'offset_y' must be a number");
                NanReturnUndefined();
            }
            offset_y = bind_opt->IntegerValue();
        }
    }

    Local<Value> callback = args[args.Length()-1];
    vector_tile_composite_baton_t *closure = new vector_tile_composite_baton_t();
    closure->request.data = closure;
    closure->offset_x = offset_x;
    closure->offset_y = offset_y;
    closure->area_threshold = area_threshold;
    closure->path_multiplier = path_multiplier;
    closure->buffer_size = buffer_size;
    closure->scale_factor = scale_factor;
    closure->scale_denominator = scale_denominator;
    closure->d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    closure->error = false;
    closure->vtiles.reserve(num_tiles);
    for (unsigned j=0;j < num_tiles;++j) {
        Local<Value> val = vtiles->Get(j);
        if (!val->IsObject())
        {
            delete closure;
            NanThrowTypeError("must provide an array of VectorTile objects");
            NanReturnUndefined();
        }
        Local<Object> tile_obj = val->ToObject();
        if (tile_obj->IsNull() || tile_obj->IsUndefined() || !NanNew(VectorTile::constructor)->HasInstance(tile_obj))
        {
            delete closure;
            NanThrowTypeError("must provide an array of VectorTile objects");
            NanReturnUndefined();
        }
        VectorTile* vt = node::ObjectWrap::Unwrap<VectorTile>(tile_obj);
        vt->Ref();
        closure->vtiles.push_back(vt);
    }
    closure->d->Ref();
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Composite, (uv_after_work_cb)EIO_AfterComposite);
    NanReturnUndefined();
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
    NanScope();

    vector_tile_composite_baton_t *closure = static_cast<vector_tile_composite_baton_t *>(req->data);

    if (closure->error)
    {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->d) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }
    for (VectorTile* vt : closure->vtiles)
    {
        vt->Unref();
    }
    closure->d->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}


/**
 * Get the names of all of the layers in this vector tile
 *
 * @memberof mapnik.VectorTile
 * @name names
 * @instance
 * @param {Array<string>} layer names
 */
NAN_METHOD(VectorTile::names)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    int raw_size = d->buffer_.size();
    if (raw_size > 0)
    {
        try
        {
            std::vector<std::string> names = detail::lazy_names(d->buffer_);
            Local<Array> arr = NanNew<Array>(names.size());
            unsigned idx = 0;
            for (std::string const& name : names)
            {
                arr->Set(idx++,NanNew(name.c_str()));
            }
            NanReturnValue(arr);
        }
        catch (std::exception const& ex)
        {
            NanThrowError(ex.what());
            NanReturnUndefined();
        }
    }
    NanReturnValue(NanNew<Array>(0));
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
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    int raw_size = d->buffer_.size();
    if (raw_size > 0)
    {
        try
        {
            NanReturnValue(NanNew<Boolean>(detail::lazy_empty(d->buffer_)));
        }
        catch (std::exception const& ex)
        {
            NanThrowError(ex.what());
            NanReturnUndefined();
        }
    }
    NanReturnValue(NanNew<Boolean>(true));
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
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    NanReturnValue(NanNew<Integer>(d->width()));
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
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    NanReturnValue(NanNew<Integer>(d->height()));
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
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    NanReturnValue(NanNew(d->painted()));
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
    Persistent<Function> cb;
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
    NanScope();
    if (args.Length() < 2 || !args[0]->IsNumber() || !args[1]->IsNumber())
    {
        NanThrowError("expects lon,lat args");
        NanReturnUndefined();
    }
    double tolerance = 0.0; // meters
    std::string layer_name("");
    if (args.Length() > 2)
    {
        Local<Object> options = NanNew<Object>();
        if (!args[2]->IsObject())
        {
            NanThrowTypeError("optional third argument must be an options object");
            NanReturnUndefined();
        }
        options = args[2]->ToObject();
        if (options->Has(NanNew("tolerance")))
        {
            Local<Value> tol = options->Get(NanNew("tolerance"));
            if (!tol->IsNumber())
            {
                NanThrowTypeError("tolerance value must be a number");
                NanReturnUndefined();
            }
            tolerance = tol->NumberValue();
        }
        if (options->Has(NanNew("layer")))
        {
            Local<Value> layer_id = options->Get(NanNew("layer"));
            if (!layer_id->IsString())
            {
                NanThrowTypeError("layer value must be a string");
                NanReturnUndefined();
            }
            layer_name = TOSTR(layer_id);
        }
    }

    double lon = args[0]->NumberValue();
    double lat = args[1]->NumberValue();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());

    // If last argument is not a function go with sync call.
    if (!args[args.Length()-1]->IsFunction()) {
        try  {
            std::vector<query_result> result = _query(d, lon, lat, tolerance, layer_name);
            Local<Array> arr = _queryResultToV8(result);
            NanReturnValue(arr);
        }
        catch (std::exception const& ex)
        {
            NanThrowError(ex.what());
            NanReturnUndefined();
        }
    } else {
        Local<Value> callback = args[args.Length()-1];
        vector_tile_query_baton_t *closure = new vector_tile_query_baton_t();
        closure->request.data = closure;
        closure->lon = lon;
        closure->lat = lat;
        closure->tolerance = tolerance;
        closure->layer_name = layer_name;
        closure->d = d;
        closure->error = false;
        NanAssignPersistent(closure->cb, callback.As<Function>());
        uv_queue_work(uv_default_loop(), &closure->request, EIO_Query, (uv_after_work_cb)EIO_AfterQuery);
        d->Ref();
        NanReturnUndefined();
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
    NanScope();
    vector_tile_query_baton_t *closure = static_cast<vector_tile_query_baton_t *>(req->data);
    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        std::vector<query_result> result = closure->result;
        Local<Array> arr = _queryResultToV8(result);
        Local<Value> argv[2] = { NanNull(), arr };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->d->Unref();
    NanDisposePersistent(closure->cb);
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

Local<Array> VectorTile::_queryResultToV8(std::vector<query_result> const& result)
{
    Local<Array> arr = NanNew<Array>();
    std::size_t i = 0;
    for (auto const& item : result) {
        Handle<Value> feat = Feature::NewInstance(item.feature);
        Local<Object> feat_obj = feat->ToObject();
        feat_obj->Set(NanNew("layer"),NanNew(item.layer.c_str()));
        feat_obj->Set(NanNew("distance"),NanNew<Number>(item.distance));
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
    Persistent<Function> cb;
} vector_tile_queryMany_baton_t;

NAN_METHOD(VectorTile::queryMany)
{

    NanScope();
    if (args.Length() < 2 || !args[0]->IsArray())
    {
        NanThrowError("expects lon,lat args + object with layer property referring to a layer name");
        NanReturnUndefined();
    }

    double tolerance = 0.0; // meters
    std::string layer_name("");
    std::vector<std::string> fields;
    std::vector<query_lonlat> query;

    // Convert v8 queryArray to a std vector
    Local<Array> queryArray = Local<Array>::Cast(args[0]);
    query.reserve(queryArray->Length());
    for (uint32_t p = 0; p < queryArray->Length(); ++p)
    {
        Local<Value> item = queryArray->Get(p);
        if (!item->IsArray())
        {
            NanThrowError("non-array item encountered");
            NanReturnUndefined();
        }
        Local<Array> pair = Local<Array>::Cast(item);
        Local<Value> lon = pair->Get(0);
        Local<Value> lat = pair->Get(1);
        if (!lon->IsNumber() || !lat->IsNumber())
        {
            NanThrowError("lng lat must be numbers");
            NanReturnUndefined();
        }
        query_lonlat lonlat;
        lonlat.lon = lon->NumberValue();
        lonlat.lat = lat->NumberValue();
        query.push_back(std::move(lonlat));
    }

    // Convert v8 options object to std params
    if (args.Length() > 1)
    {
        Local<Object> options = NanNew<Object>();
        if (!args[1]->IsObject())
        {
            NanThrowTypeError("optional second argument must be an options object");
            NanReturnUndefined();
        }
        options = args[1]->ToObject();
        if (options->Has(NanNew("tolerance")))
        {
            Local<Value> tol = options->Get(NanNew("tolerance"));
            if (!tol->IsNumber())
            {
                NanThrowTypeError("tolerance value must be a number");
                NanReturnUndefined();
            }
            tolerance = tol->NumberValue();
        }
        if (options->Has(NanNew("layer")))
        {
            Local<Value> layer_id = options->Get(NanNew("layer"));
            if (!layer_id->IsString())
            {
                NanThrowTypeError("layer value must be a string");
                NanReturnUndefined();
            }
            layer_name = TOSTR(layer_id);
        }
        if (options->Has(NanNew("fields"))) {
            Local<Value> param_val = options->Get(NanNew("fields"));
            if (!param_val->IsArray()) {
                NanThrowTypeError("option 'fields' must be an array of strings");
                NanReturnUndefined();
            }
            Local<Array> a = Local<Array>::Cast(param_val);
            unsigned int i = 0;
            unsigned int num_fields = a->Length();
            fields.reserve(num_fields);
            while (i < num_fields) {
                Local<Value> name = a->Get(i);
                if (name->IsString()){
                    fields.emplace_back(TOSTR(name));
                }
                ++i;
            }
        }
    }

    if (layer_name.empty())
    {
        NanThrowTypeError("options.layer is required");
        NanReturnUndefined();
    }

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());

    // If last argument is not a function go with sync call.
    if (!args[args.Length()-1]->IsFunction()) {
        try {
            queryMany_result result;
            _queryMany(result, d, query, tolerance, layer_name, fields);
            Local<Object> result_obj = _queryManyResultToV8(result);
            NanReturnValue(result_obj);
        }
        catch (std::exception const& ex)
        {
            NanThrowError(ex.what());
            NanReturnUndefined();
        }
    } else {
        Local<Value> callback = args[args.Length()-1];
        vector_tile_queryMany_baton_t *closure = new vector_tile_queryMany_baton_t();
        closure->d = d;
        closure->query = query;
        closure->tolerance = tolerance;
        closure->layer_name = layer_name;
        closure->fields = fields;
        closure->error = false;
        closure->request.data = closure;
        NanAssignPersistent(closure->cb, callback.As<Function>());
        uv_queue_work(uv_default_loop(), &closure->request, EIO_QueryMany, (uv_after_work_cb)EIO_AfterQueryMany);
        d->Ref();
        NanReturnUndefined();
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

Local<Object> VectorTile::_queryManyResultToV8(queryMany_result const& result) {
    Local<Object> results = NanNew<Object>();
    Local<Array> features = NanNew<Array>(result.features.size());
    Local<Array> hits = NanNew<Array>(result.hits.size());
    results->Set(NanNew("hits"), hits);
    results->Set(NanNew("features"), features);

    // result.features => features
    for (auto const& item : result.features) {
        Handle<Value> feat = Feature::NewInstance(item.second.feature);
        Local<Object> feat_obj = feat->ToObject();
        feat_obj->Set(NanNew("layer"),NanNew(item.second.layer.c_str()));
        features->Set(item.first, feat_obj);
    }

    // result.hits => hits
    for (auto const& hit : result.hits) {
        Local<Array> point_hits = NanNew<Array>(hit.second.size());
        std::size_t i = 0;
        for (auto const& h : hit.second) {
            Local<Object> hit_obj = NanNew<Object>();
            hit_obj->Set(NanNew("distance"), NanNew<Number>(h.distance));
            hit_obj->Set(NanNew("feature_id"), NanNew<Number>(h.feature_id));
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
    NanScope();
    vector_tile_queryMany_baton_t *closure = static_cast<vector_tile_queryMany_baton_t *>(req->data);
    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        queryMany_result result = closure->result;
        Local<Object> obj = _queryManyResultToV8(result);
        Local<Value> argv[2] = { NanNull(), obj };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->d->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
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
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    try {
        vector_tile::Tile tiledata = detail::get_tile(d->buffer_);
        Local<Array> arr = NanNew<Array>(tiledata.layers_size());
        for (int i=0; i < tiledata.layers_size(); ++i)
        {
            vector_tile::Tile_Layer const& layer = tiledata.layers(i);
            Local<Object> layer_obj = NanNew<Object>();
            layer_obj->Set(NanNew("name"), NanNew(layer.name().c_str()));
            layer_obj->Set(NanNew("extent"), NanNew<Integer>(layer.extent()));
            layer_obj->Set(NanNew("version"), NanNew<Integer>(layer.version()));

            Local<Array> f_arr = NanNew<Array>(layer.features_size());
            for (int j=0; j < layer.features_size(); ++j)
            {
                Local<Object> feature_obj = NanNew<Object>();
                vector_tile::Tile_Feature const& f = layer.features(j);
                if (f.has_id())
                {
                    feature_obj->Set(NanNew("id"),NanNew<Number>(f.id()));
                }
                if (f.has_raster())
                {
                    std::string const& raster = f.raster();
                    feature_obj->Set(NanNew("raster"),NanNewBufferHandle((char*)raster.data(),raster.size()));
                }
                feature_obj->Set(NanNew("type"),NanNew<Integer>(f.type()));
                Local<Array> g_arr = NanNew<Array>();
                for (int k = 0; k < f.geometry_size();++k)
                {
                    g_arr->Set(k,NanNew<Number>(f.geometry(k)));
                }
                feature_obj->Set(NanNew("geometry"),g_arr);
                Local<Object> att_obj = NanNew<Object>();
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
                            att_obj->Set(NanNew(name.c_str()), NanNew(value.string_value().c_str()));
                        }
                        else if (value.has_int_value())
                        {
                            att_obj->Set(NanNew(name.c_str()), NanNew<Number>(value.int_value()));
                        }
                        else if (value.has_double_value())
                        {
                            att_obj->Set(NanNew(name.c_str()), NanNew<Number>(value.double_value()));
                        }
                        // The following lines are not currently supported by mapnik-vector-tiles
                        // therefore these lines are not currently testable.
                        /* LCOV_EXCL_START */
                        else if (value.has_float_value())
                        {
                            att_obj->Set(NanNew(name.c_str()), NanNew<Number>(value.float_value()));
                        }
                        else if (value.has_bool_value())
                        {
                            att_obj->Set(NanNew(name.c_str()), NanNew<Boolean>(value.bool_value()));
                        }
                        else if (value.has_sint_value())
                        {
                            att_obj->Set(NanNew(name.c_str()), NanNew<Number>(value.sint_value()));
                        }
                        else if (value.has_uint_value())
                        {
                            att_obj->Set(NanNew(name.c_str()), NanNew<Number>(value.uint_value()));
                        }
                        else
                        {
                            att_obj->Set(NanNew(name.c_str()), NanUndefined());
                        }
                        /* LCOV_EXCL_END */
                    }
                    feature_obj->Set(NanNew("properties"),att_obj);
                }

                f_arr->Set(j,feature_obj);
            }
            layer_obj->Set(NanNew("features"), f_arr);
            arr->Set(i, layer_obj);
        }
        NanReturnValue(arr);
    } catch (std::exception const& ex) {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
}

static bool layer_to_geojson(vector_tile::Tile_Layer const& layer,
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
    NanScope();
    NanReturnValue(_toGeoJSONSync(args));
}

void handle_to_geojson_args(Local<Value> const& layer_id,
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
        // This should never be reached as args should have been caught earlier
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

Local<Value> VectorTile::_toGeoJSONSync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    if (args.Length() < 1) {
        NanThrowError("first argument must be either a layer name (string) or layer index (integer)");
        return NanEscapeScope(NanUndefined());
    }
    Local<Value> layer_id = args[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
        NanThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        return NanEscapeScope(NanUndefined());
    }

    VectorTile* v = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
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
            NanThrowTypeError(error_msg.c_str());
            return NanEscapeScope(NanUndefined());
        }
        write_geojson_to_string(result,all_array,all_flattened,layer_idx,v);
    }
    catch (std::exception const& ex)
    {
        // There are currently no known ways to trigger this exception in testing. If it was
        // triggered this would likely be a bug in either mapnik or mapnik-vector-tile.
        // LCOV_EXCL_START
        NanThrowError(ex.what());
        return NanEscapeScope(NanUndefined());
        // LCOV_EXCL_END
    }
    return NanEscapeScope(NanNew(result));
}

struct to_geojson_baton {
    uv_work_t request;
    VectorTile* v;
    bool error;
    std::string result;
    int layer_idx;
    bool all_array;
    bool all_flattened;
    Persistent<Function> cb;
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
    NanScope();
    if ((args.Length() < 1) || !args[args.Length()-1]->IsFunction()) {
        NanReturnValue(_toGeoJSONSync(args));
    }
    to_geojson_baton *closure = new to_geojson_baton();
    closure->request.data = closure;
    closure->v = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    closure->error = false;
    closure->layer_idx = -1;
    closure->all_array = false;
    closure->all_flattened = false;

    std::string error_msg;

    Local<Value> layer_id = args[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
        delete closure;
        NanThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        NanReturnUndefined();
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
            NanThrowTypeError(error_msg.c_str());
            NanReturnUndefined();
        }
    }
    catch (std::exception const& ex)
    {
        delete closure;
        NanThrowTypeError(ex.what());
        NanReturnUndefined();
    }
    Local<Value> callback = args[args.Length()-1];
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, to_geojson, (uv_after_work_cb)after_to_geojson);
    closure->v->Ref();
    NanReturnUndefined();
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
    NanScope();
    to_geojson_baton *closure = static_cast<to_geojson_baton *>(req->data);
    if (closure->error)
    {
        // Because there are no known ways to trigger the exception path in to_geojson
        // there is no easy way to test this path currently
        // LCOV_EXCL_START
        Local<Value> argv[1] = { NanError(closure->result.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else
    {
        Local<Value> argv[2] = { NanNull(), NanNew(closure->result) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }
    closure->v->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}


NAN_METHOD(VectorTile::parseSync)
{
    NanScope();
    NanReturnValue(_parseSync(args));
}

Local<Value> VectorTile::_parseSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    try
    {
        d->parse_proto();
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        return NanEscapeScope(NanUndefined());
    }
    return NanEscapeScope(NanUndefined());
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} vector_tile_parse_baton_t;

NAN_METHOD(VectorTile::parse)
{
    NanScope();
    if (args.Length() == 0) {
        NanReturnValue(_parseSync(args));
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    vector_tile_parse_baton_t *closure = new vector_tile_parse_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Parse, (uv_after_work_cb)EIO_AfterParse);
    d->Ref();
    NanReturnUndefined();
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
    NanScope();
    vector_tile_parse_baton_t *closure = static_cast<vector_tile_parse_baton_t *>(req->data);
    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        Local<Value> argv[1] = { NanNull() };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }

    closure->d->Unref();
    NanDisposePersistent(closure->cb);
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
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    if (args.Length() < 1 || !args[0]->IsString()) {
        NanThrowError("first argument must be a GeoJSON string");
        NanReturnUndefined();
    }
    if (args.Length() < 2 || !args[1]->IsString()) {
        NanThrowError("second argument must be a layer name (string)");
        NanReturnUndefined();
    }
    std::string geojson_string = TOSTR(args[0]);
    std::string geojson_name = TOSTR(args[1]);

    Local<Object> options = NanNew<Object>();
    double area_threshold = 0.1;
    double simplify_distance = 0.0;
    unsigned path_multiplier = 16;
    int buffer_size = 8;

    if (args.Length() > 2) {
        // options object
        if (!args[2]->IsObject()) {
            NanThrowError("optional third argument must be an options object");
            NanReturnUndefined();
        }

        options = args[2]->ToObject();

        if (options->Has(NanNew("area_threshold"))) {
            Local<Value> param_val = options->Get(NanNew("area_threshold"));
            if (!param_val->IsNumber()) {
                NanThrowError("option 'area_threshold' must be a number");
                NanReturnUndefined();
            }
            area_threshold = param_val->IntegerValue();
        }

        if (options->Has(NanNew("path_multiplier"))) {
            Local<Value> param_val = options->Get(NanNew("path_multiplier"));
            if (!param_val->IsNumber()) {
                NanThrowError("option 'path_multiplier' must be an unsigned integer");
                NanReturnUndefined();
            }
            path_multiplier = param_val->NumberValue();
        }

        if (options->Has(NanNew("simplify_distance"))) {
            Local<Value> param_val = options->Get(NanNew("simplify_distance"));
            if (!param_val->IsNumber()) {
                NanThrowTypeError("option 'simplify_distance' must be an floating point number");
                NanReturnUndefined();
            }
            simplify_distance = param_val->NumberValue();
        }
        
        if (options->Has(NanNew("buffer_size"))) {
            Local<Value> bind_opt = options->Get(NanNew("buffer_size"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'buffer_size' must be a number");
                NanReturnUndefined();
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
                          area_threshold);
        ren.set_simplify_distance(simplify_distance);
        ren.apply();
        detail::add_tile(d->buffer_,tiledata);
        NanReturnValue(NanTrue());
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
}

NAN_METHOD(VectorTile::addImage)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    if (args.Length() < 1 || !args[0]->IsObject()) {
        NanThrowError("first argument must be a Buffer representing encoded image data");
        NanReturnUndefined();
    }
    if (args.Length() < 2 || !args[1]->IsString()) {
        NanThrowError("second argument must be a layer name (string)");
        NanReturnUndefined();
    }
    std::string layer_name = TOSTR(args[1]);
    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        NanThrowError("first argument must be a Buffer representing encoded image data");
        NanReturnUndefined();
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        NanThrowError("cannot accept empty buffer as image");
        NanReturnUndefined();
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
    NanReturnUndefined();
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
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    if (args.Length() < 1 || !args[0]->IsObject())
    {
        NanThrowError("first argument must be a buffer object");
        NanReturnUndefined();
    }
    Local<Object> obj = args[0].As<Object>();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        NanThrowTypeError("first arg must be a buffer object");
        NanReturnUndefined();
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        NanThrowError("cannot accept empty buffer as protobuf");
        NanReturnUndefined();
    }
    d->buffer_.append(node::Buffer::Data(obj),buffer_size);
    NanReturnUndefined();
}

NAN_METHOD(VectorTile::setDataSync)
{
    NanScope();
    NanReturnValue(_setDataSync(args));
}

Local<Value> VectorTile::_setDataSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    if (args.Length() < 1 || !args[0]->IsObject()) {
        NanThrowTypeError("first argument must be a buffer object");
        return NanEscapeScope(NanUndefined());
    }
    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        NanThrowTypeError("first arg must be a buffer object");
        return NanEscapeScope(NanUndefined());
    }
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        NanThrowError("cannot accept empty buffer as protobuf");
        return NanEscapeScope(NanUndefined());
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
            NanThrowError((std::string("failed decoding compressed data ") + ex.what()).c_str() );
            return NanEscapeScope(NanUndefined());
        }
    }
    else
    {
        d->buffer_ = std::string(node::Buffer::Data(obj),buffer_size);
    }
    return NanEscapeScope(NanUndefined());
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    const char *data;
    size_t dataLength;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    Persistent<Object> buffer;
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
    NanScope();

    if (args.Length() == 1) {
        NanReturnValue(_setDataSync(args));
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!args[args.Length() - 1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    if (args.Length() < 1 || !args[0]->IsObject()) {
        NanThrowTypeError("first argument must be a buffer object");
        NanReturnUndefined();
    }
    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        NanThrowTypeError("first arg must be a buffer object");
        NanReturnUndefined();
    }

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());

    vector_tile_setdata_baton_t *closure = new vector_tile_setdata_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    NanAssignPersistent(closure->buffer, obj.As<Object>());
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_SetData, (uv_after_work_cb)EIO_AfterSetData);
    d->Ref();
    NanReturnUndefined();
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
    NanScope();

    vector_tile_setdata_baton_t *closure = static_cast<vector_tile_setdata_baton_t *>(req->data);

    if (closure->error) {
        // See note about exception in EIO_SetData
        // LCOV_EXCL_START
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else
    {
        Local<Value> argv[1] = { NanNull() };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }

    closure->d->Unref();
    NanDisposePersistent(closure->cb);
    NanDisposePersistent(closure->buffer);
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
    NanScope();
    NanReturnValue(_getDataSync(args));
}

Local<Value> VectorTile::_getDataSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());

    bool compress = false;
    int level = Z_DEFAULT_COMPRESSION;
    int strategy = Z_DEFAULT_STRATEGY;

    Local<Object> options = NanNew<Object>();

    if (args.Length() > 0)
    {
        if (!args[0]->IsObject())
        {
            NanThrowTypeError("first arg must be a options object");
            return NanEscapeScope(NanUndefined());
        }

        options = args[0]->ToObject();

        if (options->Has(NanNew("compression")))
        {
            Local<Value> param_val = options->Get(NanNew("compression"));
            if (!param_val->IsString())
            {
                NanThrowTypeError("option 'compression' must be a string, either 'gzip', or 'none' (default)");
                return NanEscapeScope(NanUndefined());
            }
            compress = std::string("gzip") == (TOSTR(param_val->ToString()));
        }

        if (options->Has(NanNew("level")))
        {
            Local<Value> param_val = options->Get(NanNew("level"));
            if (!param_val->IsNumber())
            {
                NanThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return NanEscapeScope(NanUndefined());
            }
            level = param_val->IntegerValue();
            if (level < 0 || level > 9)
            {
                NanThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                return NanEscapeScope(NanUndefined());
            }
        }
        if (options->Has(NanNew("strategy")))
        {
            Local<Value> param_val = options->Get(NanNew("strategy"));
            if (!param_val->IsString())
            {
                NanThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return NanEscapeScope(NanUndefined());
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
                NanThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                return NanEscapeScope(NanUndefined());
            }
        }
    }

    try
    {
        std::size_t raw_size = d->buffer_.size();
        if (raw_size <= 0)
        {
            return NanEscapeScope(NanNewBufferHandle(0));
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
                throw std::runtime_error(s.str());
                // LCOV_EXCL_END
            }
            if (!compress)
            {
                return NanEscapeScope(NanNewBufferHandle((char*)d->buffer_.data(),raw_size));
            }
            else
            {
                std::string compressed;
                mapnik::vector_tile_impl::zlib_compress(d->buffer_, compressed, true, level, strategy);
                return NanEscapeScope(NanNewBufferHandle((char*)compressed.data(),compressed.size()));
            }
        }
    } 
    catch (std::exception const& ex) 
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        NanThrowError(ex.what());
        return NanEscapeScope(NanUndefined());
        // LCOV_EXCL_END
    }
    return NanEscapeScope(NanUndefined());
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
    Persistent<Function> cb;
} vector_tile_get_data_baton_t;


NAN_METHOD(VectorTile::getData)
{
    NanScope();

    if (args.Length() == 0 || !args[args.Length()-1]->IsFunction()) {
        NanReturnValue(_getDataSync(args));
    }

    Local<Value> callback = args[args.Length()-1];
    bool compress = false;
    int level = Z_DEFAULT_COMPRESSION;
    int strategy = Z_DEFAULT_STRATEGY;

    Local<Object> options = NanNew<Object>();

    if (args.Length() > 1)
    {
        if (!args[0]->IsObject())
        {
            NanThrowTypeError("first arg must be a options object");
            NanReturnUndefined();
        }

        options = args[0]->ToObject();

        if (options->Has(NanNew("compression")))
        {
            Local<Value> param_val = options->Get(NanNew("compression"));
            if (!param_val->IsString())
            {
                NanThrowTypeError("option 'compression' must be a string, either 'gzip', or 'none' (default)");
                NanReturnUndefined();
            }
            compress = std::string("gzip") == (TOSTR(param_val->ToString()));
        }

        if (options->Has(NanNew("level")))
        {
            Local<Value> param_val = options->Get(NanNew("level"));
            if (!param_val->IsNumber())
            {
                NanThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                NanReturnUndefined();
            }
            level = param_val->IntegerValue();
            if (level < 0 || level > 9)
            {
                NanThrowTypeError("option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive");
                NanReturnUndefined();
            }
        }
        if (options->Has(NanNew("strategy")))
        {
            Local<Value> param_val = options->Get(NanNew("strategy"));
            if (!param_val->IsString())
            {
                NanThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                NanReturnUndefined();
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
                NanThrowTypeError("option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT");
                NanReturnUndefined();
            }
        }
    }

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    vector_tile_get_data_baton_t *closure = new vector_tile_get_data_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->compress = compress;
    closure->level = level;
    closure->strategy = strategy;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, get_data, (uv_after_work_cb)after_get_data);
    d->Ref();
    NanReturnUndefined();
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
    NanScope();
    vector_tile_get_data_baton_t *closure = static_cast<vector_tile_get_data_baton_t *>(req->data);
    if (closure->error) 
    {
        // As all exception throwing paths are not easily testable or no way can be
        // found to test with repeatability this exception path is not included
        // in test coverage.
        // LCOV_EXCL_START
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else if (!closure->data.empty())
    {
        Local<Value> argv[2] = { NanNull(), NanNewBufferHandle((char*)closure->data.data(),closure->data.size()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }
    else
    {
        std::size_t raw_size = closure->d->buffer_.size();
        if (raw_size <= 0)
        {
            Local<Value> argv[2] = { NanNull(), NanNewBufferHandle(0) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
        else if (raw_size >= node::Buffer::kMaxLength)
        {
            // This is a valid test path, but I am excluding it from test coverage due to the
            // requirement of loading a very large object in memory in order to test it.
            // LCOV_EXCL_START
            std::ostringstream s;
            s << "Data is too large to convert to a node::Buffer ";
            s << "(" << raw_size << " raw bytes >= node::Buffer::kMaxLength)";
            Local<Value> argv[1] = { NanError(s.str().c_str()) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
            // LCOV_EXCL_END
        }
        else
        {
            Local<Value> argv[2] = { NanNull(), NanNewBufferHandle((char*)closure->d->buffer_.data(),raw_size) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
    }

    closure->d->Unref();
    NanDisposePersistent(closure->cb);
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
    Persistent<Function> cb;
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
    NanScope();

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    if (args.Length() < 1 || !args[0]->IsObject()) {
        NanThrowTypeError("mapnik.Map expected as first arg");
        NanReturnUndefined();
    }
    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !NanNew(Map::constructor)->HasInstance(obj)) {
        NanThrowTypeError("mapnik.Map expected as first arg");
        NanReturnUndefined();
    }

    Map *m = node::ObjectWrap::Unwrap<Map>(obj);
    if (args.Length() < 2 || !args[1]->IsObject()) {
        NanThrowTypeError("a renderable mapnik object is expected as second arg");
        NanReturnUndefined();
    }
    Local<Object> im_obj = args[1]->ToObject();

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
    {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    vector_tile_render_baton_t *closure = new vector_tile_render_baton_t();
    baton_guard guard(closure);
    Local<Object> options = NanNew<Object>();

    if (args.Length() > 2)
    {
        if (!args[2]->IsObject())
        {
            NanThrowTypeError("optional third argument must be an options object");
            NanReturnUndefined();
        }
        options = args[2]->ToObject();
        if (options->Has(NanNew("z")))
        {
            Local<Value> bind_opt = options->Get(NanNew("z"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'z' must be a number");
                NanReturnUndefined();
            }
            closure->z = bind_opt->IntegerValue();
            closure->zxy_override = true;
        }
        if (options->Has(NanNew("x")))
        {
            Local<Value> bind_opt = options->Get(NanNew("x"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'x' must be a number");
                NanReturnUndefined();
            }
            closure->x = bind_opt->IntegerValue();
            closure->zxy_override = true;
        }
        if (options->Has(NanNew("y")))
        {
            Local<Value> bind_opt = options->Get(NanNew("y"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'y' must be a number");
                NanReturnUndefined();
            }
            closure->y = bind_opt->IntegerValue();
            closure->zxy_override = true;
        }
        if (options->Has(NanNew("buffer_size"))) {
            Local<Value> bind_opt = options->Get(NanNew("buffer_size"));
            if (!bind_opt->IsNumber()) {
                NanThrowTypeError("optional arg 'buffer_size' must be a number");
                NanReturnUndefined();
            }
            closure->buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(NanNew("scale"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'scale' must be a number");
                NanReturnUndefined();
            }
            closure->scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("scale_denominator")))
        {
            Local<Value> bind_opt = options->Get(NanNew("scale_denominator"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("optional arg 'scale_denominator' must be a number");
                NanReturnUndefined();
            }
            closure->scale_denominator = bind_opt->NumberValue();
        }
        if (options->Has(NanNew("variables")))
        {
            Local<Value> bind_opt = options->Get(NanNew("variables"));
            if (!bind_opt->IsObject())
            {
                NanThrowTypeError("optional arg 'variables' must be an object");
                NanReturnUndefined();
            }
            object_to_container(closure->variables,bind_opt->ToObject());
        }
    }

    closure->layer_idx = 0;
    if (NanNew(Image::constructor)->HasInstance(im_obj))
    {
        Image *im = node::ObjectWrap::Unwrap<Image>(im_obj);
        im->_ref();
        closure->width = im->get()->width();
        closure->height = im->get()->height();
        closure->surface = im;
    }
    else if (NanNew(CairoSurface::constructor)->HasInstance(im_obj))
    {
        CairoSurface *c = node::ObjectWrap::Unwrap<CairoSurface>(im_obj);
        c->_ref();
        closure->width = c->width();
        closure->height = c->height();
        closure->surface = c;
        if (options->Has(NanNew("renderer")))
        {
            Local<Value> renderer = options->Get(NanNew("renderer"));
            if (!renderer->IsString() )
            {
                NanThrowError("'renderer' option must be a string of either 'svg' or 'cairo'");
                NanReturnUndefined();
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
                NanThrowError("'renderer' option must be a string of either 'svg' or 'cairo'");
                NanReturnUndefined();
            }
        }
    }
#if defined(GRID_RENDERER)
    else if (NanNew(Grid::constructor)->HasInstance(im_obj))
    {
        Grid *g = node::ObjectWrap::Unwrap<Grid>(im_obj);
        g->_ref();
        closure->width = g->get()->width();
        closure->height = g->get()->height();
        closure->surface = g;

        std::size_t layer_idx = 0;

        // grid requires special options for now
        if (!options->Has(NanNew("layer")))
        {
            NanThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
            NanReturnUndefined();
        } else {
            std::vector<mapnik::layer> const& layers = m->get()->layers();

            Local<Value> layer_id = options->Get(NanNew("layer"));

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
                    NanThrowTypeError(s.str().c_str());
                    NanReturnUndefined();
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
                    NanThrowTypeError(s.str().c_str());
                    NanReturnUndefined();
                }
            }
            else
            {
                NanThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
                NanReturnUndefined();
            }
        }
        if (options->Has(NanNew("fields"))) {

            Local<Value> param_val = options->Get(NanNew("fields"));
            if (!param_val->IsArray())
            {
                NanThrowTypeError("option 'fields' must be an array of strings");
                NanReturnUndefined();
            }
            Local<Array> a = Local<Array>::Cast(param_val);
            unsigned int i = 0;
            unsigned int num_fields = a->Length();
            while (i < num_fields) {
                Local<Value> name = a->Get(i);
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
        NanThrowTypeError("renderable mapnik object expected as second arg");
        NanReturnUndefined();
    }
    closure->request.data = closure;
    closure->d = d;
    closure->m = m;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderTile, (uv_after_work_cb)EIO_AfterRenderTile);
    m->_ref();
    d->Ref();
    guard.release();
    NanReturnUndefined();
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
    NanScope();

    vector_tile_render_baton_t *closure = static_cast<vector_tile_render_baton_t *>(req->data);

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        if (closure->surface.is<Image *>())
        {
            Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(mapnik::util::get<Image *>(closure->surface)) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
#if defined(GRID_RENDERER)
        else if (closure->surface.is<Grid *>())
        {
            Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(mapnik::util::get<Grid *>(closure->surface)) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
#endif
        else if (closure->surface.is<CairoSurface *>())
        {
            Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(mapnik::util::get<CairoSurface *>(closure->surface)) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
    }

    closure->m->_unref();
    closure->d->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}
NAN_METHOD(VectorTile::clearSync)
{
    NanScope();
    NanReturnValue(_clearSync(args));
}

Local<Value> VectorTile::_clearSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    d->clear();
    return NanEscapeScope(NanUndefined());
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    std::string format;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
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
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());

    if (args.Length() == 0) {
        NanReturnValue(_clearSync(args));
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!callback->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }
    clear_vector_tile_baton_t *closure = new clear_vector_tile_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Clear, (uv_after_work_cb)EIO_AfterClear);
    d->Ref();
    NanReturnUndefined();
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
    NanScope();
    clear_vector_tile_baton_t *closure = static_cast<clear_vector_tile_baton_t *>(req->data);
    if (closure->error)
    {
        // No reason this should ever throw an exception, not currently testable.
        // LCOV_EXCL_START
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else
    {
        Local<Value> argv[1] = { NanNull() };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    closure->d->Unref();
    NanDisposePersistent(closure->cb);
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
    NanScope();
    NanReturnValue(_isSolidSync(args));
}

Local<Value> VectorTile::_isSolidSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    try
    {
        std::string key;
        bool is_solid = mapnik::vector_tile_impl::is_solid_extent(d->buffer_, key);
        if (is_solid)
        {
            return NanEscapeScope(NanNew(key.c_str()));
        }
        else
        {
            return NanEscapeScope(NanFalse());
        }
    }
    catch (std::exception const& ex)
    {
        // There is a chance of this throwing an error, however, only in the situation such that there
        // is an illegal command within the vector tile. 
        // LCOV_EXCL_START
        NanThrowError(ex.what());
        return NanEscapeScope(NanUndefined());
        // LCOV_EXCL_END
    }
    return NanEscapeScope(NanUndefined());
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    std::string key;
    Persistent<Function> cb;
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
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());

    if (args.Length() == 0) {
        NanReturnValue(_isSolidSync(args));
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!callback->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    is_solid_vector_tile_baton_t *closure = new is_solid_vector_tile_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->result = true;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    d->Ref();
    NanReturnUndefined();
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
    NanScope();
    is_solid_vector_tile_baton_t *closure = static_cast<is_solid_vector_tile_baton_t *>(req->data);
    if (closure->error) 
    {
        // There is a chance of this throwing an error, however, only in the situation such that there
        // is an illegal command within the vector tile. 
        // LCOV_EXCL_START
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
        // LCOV_EXCL_END
    }
    else
    {
        Local<Value> argv[3] = { NanNull(),
                                 NanNew(closure->result),
                                 NanNew(closure->key.c_str())
        };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 3, argv);
    }
    closure->d->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}
