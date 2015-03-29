#include "utils.hpp"
#include "mapnik_map.hpp"
#include "mapnik_image.hpp"
#include "mapnik_grid.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_cairo_surface.hpp"
#ifdef SVG_RENDERER
#include <mapnik/svg/output/svg_renderer.hpp>
#endif

#include "mapnik_vector_tile.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_datasource.hpp"
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
#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp>  // for grid_renderer
#include <mapnik/box2d.hpp>
#include <mapnik/scale_denominator.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include "proj_transform_adapter.hpp"
#include <mapnik/json/geometry_generator_grammar.hpp>
#include <mapnik/json/properties_generator_grammar.hpp>
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
#include "pbf.hpp"

// addGeoJSON
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"
#include <mapnik/datasource_cache.hpp>

#include <google/protobuf/io/coded_stream.h>

template <typename PathType>
double path_to_point_distance(PathType & path, double x, double y)
{
    double x0 = 0;
    double y0 = 0;
    double distance = -1;
    path.rewind(0);
    mapnik::geometry_type::types geom_type = static_cast<mapnik::geometry_type::types>(path.type());
    switch(geom_type)
    {
    case mapnik::geometry_type::types::Point:
    {
        unsigned command;
        bool first = true;
        while (mapnik::SEG_END != (command = path.vertex(&x0, &y0)))
        {
            if (command == mapnik::SEG_CLOSE) continue;
            if (first)
            {
                distance = mapnik::distance(x, y, x0, y0);
                first = false;
                continue;
            }
            double dist = mapnik::distance(x, y, x0, y0);
            if (dist < distance) distance = dist;
        }
        return distance;
        break;
    }
    case mapnik::geometry_type::types::Polygon:
    {
        double x1 = 0;
        double y1 = 0;
        bool inside = false;
        unsigned command = path.vertex(&x0, &y0);
        if (command == mapnik::SEG_END) return distance;
        while (mapnik::SEG_END != (command = path.vertex(&x1, &y1)))
        {
            if (command == mapnik::SEG_CLOSE) continue;
            if (command == mapnik::SEG_MOVETO)
            {
                x0 = x1;
                y0 = y1;
                continue;
            }
            if ((((y1 <= y) && (y < y0)) ||
                 ((y0 <= y) && (y < y1))) &&
                (x < (x0 - x1) * (y - y1)/ (y0 - y1) + x1))
            {
                inside=!inside;
            }
            x0 = x1;
            y0 = y1;
        }
        return inside ? 0 : -1;
        break;
    }
    case mapnik::geometry_type::types::LineString:
    {
        double x1 = 0;
        double y1 = 0;
        bool first = true;
        unsigned command = path.vertex(&x0, &y0);
        if (command == mapnik::SEG_END) return distance;
        while (mapnik::SEG_END != (command = path.vertex(&x1, &y1)))
        {
            if (command == mapnik::SEG_CLOSE) continue;
            if (command == mapnik::SEG_MOVETO)
            {
                x0 = x1;
                y0 = y1;
                continue;
            }
            if (first)
            {
                distance = mapnik::point_to_segment_distance(x,y,x0,y0,x1,y1);
                first = false;
            }
            else
            {
                double dist = mapnik::point_to_segment_distance(x,y,x0,y0,x1,y1);
                if (dist >= 0 && dist < distance) distance = dist;
            }
            x0 = x1;
            y0 = y1;
        }
        // Don't return just fall through to distance below 
        // return distance;
    }
    default:
        return distance;
        break;
    }
    return distance;
}

Persistent<FunctionTemplate> VectorTile::constructor;

void VectorTile::Initialize(Handle<Object> target) {
    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(VectorTile::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("VectorTile"));
    NODE_SET_PROTOTYPE_METHOD(lcons, "render", render);
    NODE_SET_PROTOTYPE_METHOD(lcons, "setData", setData);
    NODE_SET_PROTOTYPE_METHOD(lcons, "setDataSync", setDataSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "getData", getData);
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
#ifdef PROTOBUF_FULL
    NODE_SET_PROTOTYPE_METHOD(lcons, "toString", toString);
#endif

    // common to mapnik.Image
    NODE_SET_PROTOTYPE_METHOD(lcons, "width", width);
    NODE_SET_PROTOTYPE_METHOD(lcons, "height", height);
    NODE_SET_PROTOTYPE_METHOD(lcons, "painted", painted);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clearSync", clear);
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
    status_(VectorTile::LAZY_DONE),
    tiledata_(),
    width_(w),
    height_(h),
    painted_(false),
    byte_size_(0) {}

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

std::vector<std::string> VectorTile::lazy_names()
{
    std::vector<std::string> names;
    std::size_t bytes = buffer_.size();
    if (bytes > 0)
    {
        pbf::message item(buffer_.data(),bytes);
        while (item.next()) {
            if (item.tag == 3) {
                uint64_t len = item.varint();
                pbf::message layermsg(item.getData(),static_cast<std::size_t>(len));
                while (layermsg.next()) {
                    if (layermsg.tag == 1) {
                        names.emplace_back(layermsg.string());
                    } else {
                        layermsg.skip();
                    }
                }
                item.skipBytes(len);
            } else {
                item.skip();
            }
        }
    }
    return names;
}

void VectorTile::parse_proto()
{
    switch (status_)
    {
    case LAZY_DONE:
    {
        // no-op
        break;
    }
    case LAZY_SET:
    {
        status_ = LAZY_DONE;
        std::size_t bytes = buffer_.size();
        if (bytes == 0)
        {
            throw std::runtime_error("cannot parse 0 length buffer as protobuf");
        }
        if (tiledata_.ParseFromArray(buffer_.data(), bytes))
        {
            painted(true);
            cache_bytesize();
        }
        else
        {
            throw std::runtime_error("could not parse buffer as protobuf");
        }
        break;
    }
    case LAZY_MERGE:
    {
        status_ = LAZY_DONE;
        std::size_t bytes = buffer_.size();
        if (bytes == 0)
        {
            throw std::runtime_error("cannot parse 0 length buffer as protobuf");
        }
        unsigned remaining = bytes - byte_size_;
        const char * data = buffer_.data() + byte_size_;
        google::protobuf::io::CodedInputStream input(
              reinterpret_cast<const google::protobuf::uint8*>(
                  data), remaining);
        if (tiledata_.MergeFromCodedStream(&input))
        {
            painted(true);
            cache_bytesize();
        }
        else
        {
            throw std::runtime_error("could not merge buffer as protobuf");
        }
        break;
    }
    }
}

void _composite(VectorTile* target_vt,
                std::vector<VectorTile*> & vtiles,
                unsigned path_multiplier,
                int buffer_size,
                double scale_factor,
                unsigned offset_x,
                unsigned offset_y,
                unsigned tolerance,
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
            if (bytes > 0 && vt->byte_size() <= bytes) {
                target_vt->buffer_.append(vt->buffer_.data(),vt->buffer_.size());
                target_vt->status_ = VectorTile::LAZY_MERGE;
            }
            else if (vt->byte_size() > 0)
            {
                std::string new_message;
                vector_tile::Tile const& tiledata = vt->get_tile();
                if (!tiledata.SerializeToString(&new_message))
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
                    target_vt->status_ = VectorTile::LAZY_MERGE;
                }
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
            // ensure data is in tile object
            if (vt->status_ == VectorTile::LAZY_DONE) // tile is already parsed, we're good
            {
                vector_tile::Tile const& tiledata = vt->get_tile();
                unsigned num_layers = tiledata.layers_size();
                if (num_layers > 0)
                {
                    for (int i=0; i < tiledata.layers_size(); ++i)
                    {
                        vector_tile::Tile_Layer const& layer = tiledata.layers(i);
                        mapnik::layer lyr(layer.name(),merc_srs);
                        std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds = std::make_shared<
                                                        mapnik::vector_tile_impl::tile_datasource>(
                                                            layer,
                                                            vt->x_,
                                                            vt->y_,
                                                            vt->z_,
                                                            vt->width()
                                                            );
                        ds->set_envelope(m_req.get_buffered_extent());
                        lyr.set_datasource(ds);
                        map.add_layer(lyr);
                    }
                    renderer_type ren(backend,
                                      map,
                                      m_req,
                                      scale_factor,
                                      offset_x,
                                      offset_y,
                                      tolerance);
                    ren.apply(scale_denominator);
                }
            }
            else // tile is not pre-parsed so parse into new object to avoid needing to mutate input
            {
                std::size_t bytes = vt->buffer_.size();
                if (bytes > 1) // throw instead?
                {
                    vector_tile::Tile new_tiledata2;
                    if (new_tiledata2.ParseFromArray(vt->buffer_.data(), bytes))
                    {
                        unsigned num_layers = new_tiledata2.layers_size();
                        if (num_layers > 0)
                        {
                            for (int i=0; i < new_tiledata2.layers_size(); ++i)
                            {
                                vector_tile::Tile_Layer const& layer = new_tiledata2.layers(i);
                                mapnik::layer lyr(layer.name(),merc_srs);
                                std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds = std::make_shared<
                                                                mapnik::vector_tile_impl::tile_datasource>(
                                                                    layer,
                                                                    vt->x_,
                                                                    vt->y_,
                                                                    vt->z_,
                                                                    vt->width()
                                                                    );
                                ds->set_envelope(m_req.get_buffered_extent());
                                lyr.set_datasource(ds);
                                map.add_layer(lyr);
                            }
                            renderer_type ren(backend,
                                              map,
                                              m_req,
                                              scale_factor,
                                              offset_x,
                                              offset_y,
                                              tolerance);
                            ren.apply(scale_denominator);
                        }
                    }
                    else
                    {
                        // throw here?
                    }
                }
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
                target_vt->status_ = VectorTile::LAZY_MERGE;
            }
        }
    }
}

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
    unsigned tolerance = 8;
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
        if (options->Has(NanNew("tolerance")))
        {
            Local<Value> tol = options->Get(NanNew("tolerance"));
            if (!tol->IsNumber())
            {
                NanThrowTypeError("tolerance value must be a number");
                return NanEscapeScope(NanUndefined());
            }
            tolerance = tol->NumberValue();
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
                   tolerance,
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
    unsigned tolerance;
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
    unsigned tolerance = 8;
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
    closure->tolerance = tolerance;
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
                   closure->tolerance,
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

#ifdef PROTOBUF_FULL
NAN_METHOD(VectorTile::toString)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    vector_tile::Tile const& tiledata = d->get_tile();
    NanReturnValue(NanNew(tiledata.DebugString().c_str()));
}
#endif

NAN_METHOD(VectorTile::names)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    int raw_size = d->buffer_.size();
    if (raw_size > 0 && d->byte_size_ <= raw_size)
    {   
        try
        {
            std::vector<std::string> names = d->lazy_names();
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
    } else {
        vector_tile::Tile const& tiledata = d->get_tile();
        Local<Array> arr = NanNew<Array>(tiledata.layers_size());
        for (int i=0; i < tiledata.layers_size(); ++i)
        {
            vector_tile::Tile_Layer const& layer = tiledata.layers(i);
            arr->Set(i, NanNew(layer.name().c_str()));
        }
        NanReturnValue(arr);
    }
    NanReturnUndefined();
}

bool VectorTile::lazy_empty()
{
    std::size_t bytes = buffer_.size();
    if (bytes > 0)
    {
        pbf::message item(buffer_.data(),bytes);
        while (item.next()) {
            if (item.tag == 3) {
                uint64_t len = item.varint();
                pbf::message layermsg(item.getData(),static_cast<std::size_t>(len));
                while (layermsg.next()) {
                    if (layermsg.tag == 2) {
                        // we hit a feature, assume we've got data
                        return false;
                    } else {
                        layermsg.skip();
                    }
                }
                item.skipBytes(len);
            } 
            else 
            {
                item.skip();
            }
        }
    }
    return true;
}


NAN_METHOD(VectorTile::empty)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    int raw_size = d->buffer_.size();
    if (raw_size > 0 && d->byte_size_ <= raw_size)
    {
        try
        {
            NanReturnValue(NanNew<Boolean>(d->lazy_empty()));
        }
        catch (std::exception const& ex)
        {
            NanThrowError(ex.what());
            NanReturnUndefined();
        }
    } else {
        vector_tile::Tile const& tiledata = d->get_tile();
        if (tiledata.layers_size() == 0) {
            NanReturnValue(NanNew<Boolean>(true));
        } else {
            for (int i=0; i < tiledata.layers_size(); ++i)
            {
                vector_tile::Tile_Layer const& layer = tiledata.layers(i);
                if (layer.features_size()) {
                    NanReturnValue(NanNew<Boolean>(false));
                    break;
                }
            }
        }
    }
    NanReturnValue(NanNew<Boolean>(true));
}

NAN_METHOD(VectorTile::width)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    NanReturnValue(NanNew<Integer>(d->width()));
}

NAN_METHOD(VectorTile::height)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    NanReturnValue(NanNew<Integer>(d->height()));
}

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
    vector_tile::Tile const& tiledata = d->get_tile();
    mapnik::coord2d pt(x,y);
    if (!layer_name.empty())
    {
        int layer_idx = -1;
        for (int j=0; j < tiledata.layers_size(); ++j)
        {
            vector_tile::Tile_Layer const& layer = tiledata.layers(j);
            if (layer_name == layer.name())
            {
                layer_idx = j;
                break;
            }
        }
        if (layer_idx > -1)
        {
            vector_tile::Tile_Layer const& layer = tiledata.layers(layer_idx);
            std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds = std::make_shared<
                                        mapnik::vector_tile_impl::tile_datasource>(
                                            layer,
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
                    double distance = -1;
                    for (mapnik::geometry_type const& geom : feature->paths())
                    {
                        mapnik::vertex_adapter va(geom);
                        double dist = path_to_point_distance(va,x,y);
                        if (dist >= 0 && (distance < 0 || dist < distance))
                        {
                            distance = dist;
                        }
                    }
                    if (distance >= 0 && distance <= tolerance)
                    {
                        query_result res;
                        res.distance = distance;
                        res.layer = layer.name();
                        res.feature = feature;
                        arr.push_back(std::move(res));
                    }
                }
            }
        }
    }
    else
    {
        for (int i=0; i < tiledata.layers_size(); ++i)
        {
            vector_tile::Tile_Layer const& layer = tiledata.layers(i);
            std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds = std::make_shared<
                                        mapnik::vector_tile_impl::tile_datasource>(
                                            layer,
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
                    double distance = -1;
                    for (mapnik::geometry_type const& geom : feature->paths())
                    {
                        mapnik::vertex_adapter va(geom);
                        double dist = path_to_point_distance(va,x,y);
                        if (dist >= 0 && (distance < 0 || dist < distance))
                        {
                            distance = dist;
                        }
                    }
                    if (distance >= 0 && distance <= tolerance)
                    {
                        query_result res;
                        res.distance = distance;
                        res.layer = layer.name();
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
    for (std::size_t i = 0; i < result.size(); ++i) {
        Handle<Value> feat = Feature::NewInstance(result[i].feature);
        Local<Object> feat_obj = feat->ToObject();
        feat_obj->Set(NanNew("layer"),NanNew(result[i].layer.c_str()));
        feat_obj->Set(NanNew("distance"),NanNew<Number>(result[i].distance));
        arr->Set(i,feat);
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
        try  {
            queryMany_result result = _queryMany(d, query, tolerance, layer_name, fields);
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

queryMany_result VectorTile::_queryMany(VectorTile* d, std::vector<query_lonlat> const& query, double tolerance, std::string const& layer_name, std::vector<std::string> const& fields) {
    vector_tile::Tile const& tiledata = d->get_tile();
    int layer_idx = -1;
    for (int j=0; j < tiledata.layers_size(); ++j)
    {
        vector_tile::Tile_Layer const& layer = tiledata.layers(j);
        if (layer_name == layer.name())
        {
            layer_idx = j;
            break;
        }
    }
    if (layer_idx == -1)
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

    vector_tile::Tile_Layer const& layer = tiledata.layers(layer_idx);
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds = std::make_shared<
                                mapnik::vector_tile_impl::tile_datasource>(
                                    layer,
                                    d->x_,
                                    d->y_,
                                    d->z_,
                                    d->width()
                                    );
    mapnik::query q(bbox);
    if (fields.empty())
    {
        // request all data attributes
        for (int i = 0; i < layer.keys_size(); ++i)
        {
            q.add_property_name(layer.keys(i));
        }
    }
    else
    {
        for ( std::string const& name : fields)
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
                double distance = -1;
                for (mapnik::geometry_type const& geom : feature->paths())
                {
                    mapnik::vertex_adapter va(geom);
                    double dist = path_to_point_distance(va,pt.x,pt.y);
                    if (dist >= 0 && (distance < 0 || dist < distance))
                    {
                        distance = dist;
                    }
                }
                if (distance >= 0 && distance <= tolerance)
                {
                    has_hit = 1;
                    query_result res;
                    res.feature = feature;
                    res.distance = 0;
                    res.layer = layer.name();

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
    typedef std::map<unsigned,std::vector<query_hit> >::iterator hits_it_type;
    for (hits_it_type it = hits.begin(); it != hits.end(); it++) {
        std::sort(it->second.begin(), it->second.end(), _queryManySort);
    }

    queryMany_result result;
    result.hits = hits;
    result.features = features;
    return result;
}

bool VectorTile::_queryManySort(query_hit const& a, query_hit const& b) {
    return a.distance < b.distance;
}

Local<Object> VectorTile::_queryManyResultToV8(queryMany_result const& result) {
    Local<Object> results = NanNew<Object>();
    Local<Array> features = NanNew<Array>();
    Local<Array> hits = NanNew<Array>();
    results->Set(NanNew("hits"), hits);
    results->Set(NanNew("features"), features);

    // result.features => features
    typedef std::map<unsigned,query_result>::const_iterator features_it_type;
    for (features_it_type it = result.features.begin(); it != result.features.end(); it++) {
        Handle<Value> feat = Feature::NewInstance(it->second.feature);
        Local<Object> feat_obj = feat->ToObject();
        feat_obj->Set(NanNew("layer"),NanNew(it->second.layer.c_str()));
        features->Set(it->first, feat_obj);
    }

    // result.hits => hits
    typedef std::map<unsigned,std::vector<query_hit> >::const_iterator results_it_type;
    for (results_it_type it = result.hits.begin(); it != result.hits.end(); it++) {
        Local<Array> point_hits = NanNew<Array>();
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            Local<Object> hit_obj = NanNew<Object>();
            hit_obj->Set(NanNew("distance"), NanNew<Number>(it->second[i].distance));
            hit_obj->Set(NanNew("feature_id"), NanNew<Number>(it->second[i].feature_id));
            point_hits->Set(i, hit_obj);
        }
        hits->Set(it->first, point_hits);
    }

    return results;
}

void VectorTile::EIO_QueryMany(uv_work_t* req)
{
    vector_tile_queryMany_baton_t *closure = static_cast<vector_tile_queryMany_baton_t *>(req->data);
    try
    {
        closure->result = _queryMany(closure->d, closure->query, closure->tolerance, closure->layer_name, closure->fields);
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

NAN_METHOD(VectorTile::toJSON)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    vector_tile::Tile const& tiledata = d->get_tile();
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
                }
                feature_obj->Set(NanNew("properties"),att_obj);
            }

            f_arr->Set(j,feature_obj);
        }
        layer_obj->Set(NanNew("features"), f_arr);
        arr->Set(i, layer_obj);
    }
    NanReturnValue(arr);
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
                                                 width,
                                                 true);
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
        using sink_type = std::back_insert_iterator<std::string>;
        static const mapnik::json::properties_generator_grammar<sink_type, mapnik::feature_impl> prop_grammar;
        static const mapnik::json::multi_geometry_generator_grammar<sink_type,node_mapnik::proj_transform_container> proj_grammar;
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
            result += "{\"type\":\"Feature\",\"geometry\":";
            if (feature->paths().empty())
            {
                result += "null";
            }
            else
            {
                std::string geometry;
                sink_type sink(geometry);
                node_mapnik::proj_transform_container projected_paths;
                for (auto const& geom : feature->paths())
                {
                    projected_paths.push_back(new node_mapnik::proj_transform_path_type(geom,prj_trans));
                }
                if (boost::spirit::karma::generate(sink, proj_grammar, projected_paths))
                {
                    result += geometry;
                }
                else
                {
                    /* LCOV_EXCL_START */
                    throw std::runtime_error("Failed to generate GeoJSON geometry");
                    /* LCOV_EXCL_END */
                }
            }
            result += ",\"properties\":";
            std::string properties;
            sink_type sink(properties);
            if (boost::spirit::karma::generate(sink, prop_grammar, *feature))
            {
                result += properties;
            }
            else
            {
                /* LCOV_EXCL_START */
                throw std::runtime_error("Failed to generate GeoJSON properties");
                /* LCOV_EXCL_END */
            }
            result += "}";
        }
    }
    return !first;
}

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
        error_msg = "layer id must be a string or index number";
    }
}

void write_geojson_to_string(std::string & result,
                             bool array,
                             bool all,
                             int layer_idx,
                             VectorTile * v)
{
    vector_tile::Tile const& tiledata = v->get_tile();
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
    vector_tile::Tile const& tiledata = v->get_tile();
    int layer_idx = -1;
    bool all_array = false;
    bool all_flattened = false;
    std::string error_msg;
    std::string result;
    try
    {
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
        NanThrowError(ex.what());
        return NanEscapeScope(NanUndefined());
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
    vector_tile::Tile const& tiledata = closure->v->get_tile();

    Local<Value> layer_id = args[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
        delete closure;
        NanThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        NanReturnUndefined();
    }

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
        closure->error = true;
        closure->result = ex.what();
    }
}

void VectorTile::after_to_geojson(uv_work_t* req)
{
    NanScope();
    to_geojson_baton *closure = static_cast<to_geojson_baton *>(req->data);
    if (closure->error)
    {
        Local<Value> argv[1] = { NanError(closure->result.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
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
    unsigned tolerance = 1;
    unsigned path_multiplier = 16;

    if (args.Length() > 2) {
        // options object
        if (!args[2]->IsObject()) {
            NanThrowError("optional third argument must be an options object");
            NanReturnUndefined();
        }

        options = args[2]->ToObject();

        if (options->Has(NanNew("tolerance"))) {
            Local<Value> param_val = options->Get(NanNew("tolerance"));
            if (!param_val->IsNumber()) {
                NanThrowError("option 'tolerance' must be an unsigned integer");
                NanReturnUndefined();
            }
            tolerance = param_val->IntegerValue();
        }

        if (options->Has(NanNew("path_multiplier"))) {
            Local<Value> param_val = options->Get(NanNew("path_multiplier"));
            if (!param_val->IsNumber()) {
                NanThrowError("option 'path_multiplier' must be an unsigned integer");
                NanReturnUndefined();
            }
            path_multiplier = param_val->NumberValue();
        }
    }

    try
    {
        typedef mapnik::vector_tile_impl::backend_pbf backend_type;
        typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
        backend_type backend(d->get_tile_nonconst(),path_multiplier);
        mapnik::Map map(d->width_,d->height_,"+init=epsg:3857");
        mapnik::vector_tile_impl::spherical_mercator merc(d->width_);
        double minx,miny,maxx,maxy;
        merc.xyz(d->x_,d->y_,d->z_,minx,miny,maxx,maxy);
        map.zoom_to_box(mapnik::box2d<double>(minx,miny,maxx,maxy));
        mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
        m_req.set_buffer_size(8);
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
                          tolerance);
        ren.apply();
        d->painted(ren.painted());
        d->cache_bytesize();
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
    vector_tile::Tile & tiledata = d->get_tile_nonconst();
    vector_tile::Tile_Layer * new_layer = tiledata.add_layers();
    new_layer->set_name(layer_name);
    new_layer->set_version(1);
    new_layer->set_extent(256 * 16);
    // no need
    // current_feature_->set_id(feature.id());
    vector_tile::Tile_Feature * new_feature = new_layer->add_features();
    new_feature->set_raster(std::string(node::Buffer::Data(obj),buffer_size));
    // report that we have data
    d->painted(true);
    // cache modified size
    d->cache_bytesize();
    NanReturnUndefined();
}

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
    d->status_ = VectorTile::LAZY_MERGE;
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
    d->buffer_ = std::string(node::Buffer::Data(obj),buffer_size);
    d->status_ = VectorTile::LAZY_SET;
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
        closure->d->buffer_ = std::string(closure->data,closure->dataLength);
        closure->d->status_ = VectorTile::LAZY_SET;
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterSetData(uv_work_t* req)
{
    NanScope();

    vector_tile_setdata_baton_t *closure = static_cast<vector_tile_setdata_baton_t *>(req->data);

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
    NanDisposePersistent(closure->buffer);
    delete closure;
}

NAN_METHOD(VectorTile::getData)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    try {
        // shortcut: return raw data and avoid trip through proto object
        std::size_t raw_size = d->buffer_.size();
        if (raw_size > 0 && (d->byte_size_ < 0 || static_cast<std::size_t>(d->byte_size_) <= raw_size)) {
            if (raw_size >= node::Buffer::kMaxLength) {
                std::ostringstream s;
                s << "Data is too large to convert to a node::Buffer ";
                s << "(" << raw_size << " raw bytes >= node::Buffer::kMaxLength)";
                throw std::runtime_error(s.str());
            }
            NanReturnValue(NanNewBufferHandle((char*)d->buffer_.data(),raw_size));
        } else {
            if (d->byte_size_ <= 0) {
                NanReturnValue(NanNewBufferHandle(0));
            } else {
                // NOTE: tiledata.ByteSize() must be called
                // after each modification of tiledata otherwise the
                // SerializeWithCachedSizesToArray will throw:
                // Error: CHECK failed: !coded_out.HadError()
                if (static_cast<std::size_t>(d->byte_size_) >= node::Buffer::kMaxLength) {
                    std::ostringstream s;
                    s << "Data is too large to convert to a node::Buffer ";
                    s << "(" << d->byte_size_ << " cached bytes >= node::Buffer::kMaxLength)";
                    throw std::runtime_error(s.str());
                }
                Local<Object> retbuf = NanNewBufferHandle(d->byte_size_);
                // TODO - consider wrapping in fastbuffer: https://gist.github.com/drewish/2732711
                // http://www.samcday.com.au/blog/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
                google::protobuf::uint8* start = reinterpret_cast<google::protobuf::uint8*>(node::Buffer::Data(retbuf));
                vector_tile::Tile const& tiledata = d->get_tile();
                google::protobuf::uint8* end = tiledata.SerializeWithCachedSizesToArray(start);
                if (end - start != d->byte_size_) {
                    throw std::runtime_error("serialization failed, possible race condition");
                }
                NanReturnValue(retbuf);
            }
        }
    } catch (std::exception const& ex) {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

using surface_type = mapnik::util::variant<Image *, CairoSurface *, Grid *>;

struct deref_visitor
{
    void operator() (Image * surface)
    {
        surface->_unref();
    }
    void operator() (CairoSurface * surface)
    {
        surface->_unref();
    }
    void operator() (Grid * surface)
    {
        surface->_unref();
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
    if (im_obj->IsNull() || im_obj->IsUndefined()) {
        NanThrowTypeError("a renderable mapnik object is expected as second arg");
        NanReturnUndefined();
    }

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
            closure->zxy_override = true;
            closure->z = options->Get(NanNew("z"))->IntegerValue();
        }
        if (options->Has(NanNew("x")))
        {
            closure->zxy_override = true;
            closure->x = options->Get(NanNew("x"))->IntegerValue();
        }
        if (options->Has(NanNew("y")))
        {
            closure->zxy_override = true;
            closure->y = options->Get(NanNew("y"))->IntegerValue();
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
            if (! (layer_id->IsString() || layer_id->IsNumber()) )
            {
                NanThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
                NanReturnUndefined();
            }

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
                NanThrowTypeError("layer id must be a string or index number");
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
                                            vector_tile::Tile const& tiledata,
                                            vector_tile_render_baton_t *closure)
{
    // loop over layers in map and match by name
    // with layers in the vector tile
    unsigned layers_size = layers.size();
    for (unsigned i=0; i < layers_size; ++i)
    {
        mapnik::layer const& lyr = layers[i];
        if (lyr.visible(scale_denom))
        {
            for (int j=0; j < tiledata.layers_size(); ++j)
            {
                vector_tile::Tile_Layer const& layer = tiledata.layers(j);
                if (lyr.name() == layer.name())
                {
                    mapnik::layer lyr_copy(lyr);
                    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds = std::make_shared<
                                                    mapnik::vector_tile_impl::tile_datasource>(
                                                        layer,
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
        vector_tile::Tile const& tiledata = closure->d->get_tile();
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
                int layer_idx = -1;
                for (int j=0; j < tiledata.layers_size(); ++j)
                {
                    vector_tile::Tile_Layer const& layer = tiledata.layers(j);
                    if (lyr.name() == layer.name())
                    {
                        layer_idx = j;
                        break;
                    }
                }
                if (layer_idx > -1)
                {
                    vector_tile::Tile_Layer const& layer = tiledata.layers(layer_idx);
                    if (layer.features_size() <= 0)
                    {
                        return;
                    }

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
                    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds = std::make_shared<
                                                    mapnik::vector_tile_impl::tile_datasource>(
                                                        layer,
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
        else if (closure->surface.is<CairoSurface *>())
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
                process_layers(ren,m_req,map_proj,layers,scale_denom,tiledata,closure);
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
                process_layers(ren,m_req,map_proj,layers,scale_denom,tiledata,closure);
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
                process_layers(ren,m_req,map_proj,layers,scale_denom,tiledata,closure);
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
        else if (closure->surface.is<Grid *>())
        {
            Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(mapnik::util::get<Grid *>(closure->surface)) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
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
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterClear(uv_work_t* req)
{
    NanScope();
    clear_vector_tile_baton_t *closure = static_cast<clear_vector_tile_baton_t *>(req->data);
    if (closure->error)
    {
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
        bool is_solid = mapnik::vector_tile_impl::is_solid_extent(d->get_tile(), key);
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
        NanThrowError(ex.what());
        return NanEscapeScope(NanUndefined());
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
    try {
        closure->result = mapnik::vector_tile_impl::is_solid_extent(closure->d->get_tile(),closure->key);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void VectorTile::EIO_AfterIsSolid(uv_work_t* req)
{
    NanScope();
    is_solid_vector_tile_baton_t *closure = static_cast<is_solid_vector_tile_baton_t *>(req->data);
    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
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
