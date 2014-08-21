#include "utils.hpp"
#include "mapnik_map.hpp"
#include "mapnik_image.hpp"
#include "mapnik_grid.hpp"
#include "mapnik_feature.hpp"
#include "mapnik_cairo_surface.hpp"
#ifdef SVG_RENDERER
#include <mapnik/svg/output/svg_renderer.hpp>
#endif

#include "mapnik_datasource.hpp"

#include "mapnik_vector_tile.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_datasource.hpp"
#include "vector_tile_util.hpp"
#include "vector_tile.pb.h"
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"


#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/version.hpp>
#include <mapnik/request.hpp>
#include <mapnik/graphics.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp>  // for grid_renderer
#include <mapnik/box2d.hpp>
#include <mapnik/scale_denominator.hpp>

#ifdef HAVE_CAIRO
#if MAPNIK_VERSION >= 300000
#include <mapnik/cairo/cairo_renderer.hpp>
#else
#include <mapnik/cairo_renderer.hpp>
#endif
#include <cairo.h>
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif // CAIRO_HAS_SVG_SURFACE
#endif

#include MAPNIK_MAKE_SHARED_INCLUDE
#include <boost/foreach.hpp>

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
#include <mapnik/save_map.hpp>

template <typename PathType>
bool _hit_test(PathType & path, double x, double y, double tol, double & distance)
{
    double x0 = 0;
    double y0 = 0;
    path.rewind(0);
    MAPNIK_GEOM_TYPE geom_type = static_cast<MAPNIK_GEOM_TYPE>(path.type());
    switch(geom_type)
    {
    case MAPNIK_POINT:
    {
        unsigned command = path.vertex(&x0, &y0);
        if (command == mapnik::SEG_END) return false;
        distance = mapnik::distance(x, y, x0, y0);
        return distance <= tol;
        break;
    }
    case MAPNIK_POLYGON:
    {
        double x1 = 0;
        double y1 = 0;
        bool inside = false;
        unsigned command = path.vertex(&x0, &y0);
        if (command == mapnik::SEG_END) return false;
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
        return inside;
        break;
    }
    case MAPNIK_LINESTRING:
    {
        double x1 = 0;
        double y1 = 0;
        unsigned command = path.vertex(&x0, &y0);
        if (command == mapnik::SEG_END) return false;
        while (mapnik::SEG_END != (command = path.vertex(&x1, &y1)))
        {
            if (command == mapnik::SEG_CLOSE) continue;
            if (command == mapnik::SEG_MOVETO)
            {
                x0 = x1;
                y0 = y1;
                continue;
            }
            distance = mapnik::point_to_segment_distance(x,y,x0,y0,x1,y1);
            if (distance < tol)
                return true;
            x0 = x1;
            y0 = y1;
        }
        return false;
        break;
    }
    default:
        return false;
        break;
    }
    return false;
}

template <typename PathType>
double path_to_point_distance(PathType & path, double x, double y)
{
    double x0 = 0;
    double y0 = 0;
    double distance = -1;
    path.rewind(0);
    MAPNIK_GEOM_TYPE geom_type = static_cast<MAPNIK_GEOM_TYPE>(path.type());
    switch(geom_type)
    {
    case MAPNIK_POINT:
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
            double d = mapnik::distance(x, y, x0, y0);
            if (d < distance) distance = d;
        }
        return distance;
        break;
    }
    case MAPNIK_POLYGON:
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
    case MAPNIK_LINESTRING:
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
                double d = mapnik::point_to_segment_distance(x,y,x0,y0,x1,y1);
                if (d >= 0 && d < distance) distance = d;
            }
            x0 = x1;
            y0 = y1;
        }
        return distance;
        break;
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
    NODE_SET_PROTOTYPE_METHOD(lcons, "query", query);
    NODE_SET_PROTOTYPE_METHOD(lcons, "queryMany", queryMany);
    NODE_SET_PROTOTYPE_METHOD(lcons, "names", names);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toJSON", toJSON);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toGeoJSON", toGeoJSON);
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
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolid", isSolid);
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolidSync", isSolidSync);
    target->Set(NanNew("VectorTile"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

VectorTile::VectorTile(int z, int x, int y, unsigned w, unsigned h) :
    ObjectWrap(),
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

VectorTile::~VectorTile() { }

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
                        names.push_back(layermsg.string());
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

NAN_METHOD(VectorTile::composite)
{
    NanScope();
    if (args.Length() < 1 || !args[0]->IsArray()) {
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
    int buffer_size = 256;
    double scale_factor = 1.0;
    unsigned offset_x = 0;
    unsigned offset_y = 0;
    unsigned tolerance = 1;
    double scale_denominator = 0.0;
    // not options yet, likely should never be....
    mapnik::box2d<double> max_extent(-20037508.34,-20037508.34,20037508.34,20037508.34);
    std::string merc_srs("+init=epsg:3857");

    Local<Object> options = NanNew<Object>();

    if (args.Length() > 1) {
        // options object
        if (!args[1]->IsObject())
        {
            NanThrowTypeError("optional second argument must be an options object");
            NanReturnUndefined();
        }
        options = args[1]->ToObject();
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

    VectorTile* target_vt = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    for (unsigned i=0;i < num_tiles;++i) {
        Local<Value> val = vtiles->Get(i);
        if (!val->IsObject()) {
            NanThrowTypeError("must provide an array of VectorTile objects");
            NanReturnUndefined();
        }
        Local<Object> tile_obj = val->ToObject();
        if (tile_obj->IsNull() || tile_obj->IsUndefined() || !NanNew(VectorTile::constructor)->HasInstance(tile_obj)) {
            NanThrowTypeError("must provide an array of VectorTile objects");
            NanReturnUndefined();
        }
        VectorTile* vt = node::ObjectWrap::Unwrap<VectorTile>(tile_obj);
        // TODO - handle name clashes
        if (target_vt->z_ == vt->z_ &&
            target_vt->x_ == vt->x_ &&
            target_vt->y_ == vt->y_)
        {
            int bytes = static_cast<int>(vt->buffer_.size());
            if (bytes > 0 && vt->byte_size_ <= bytes) {
                target_vt->buffer_.append(vt->buffer_.data(),vt->buffer_.size());
                target_vt->status_ = VectorTile::LAZY_MERGE;
            }
            else if (vt->byte_size_ > 0)
            {
                std::string new_message;
                mapnik::vector::tile const& tiledata = vt->get_tile();
                if (!tiledata.SerializeToString(&new_message))
                {
                    NanThrowTypeError("could not serialize new data for vt");
                    NanReturnUndefined();
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
            // set up to render to new vtile
            typedef mapnik::vector::backend_pbf backend_type;
            typedef mapnik::vector::processor<backend_type> renderer_type;
            mapnik::vector::tile new_tiledata;
            backend_type backend(new_tiledata,
                                    path_multiplier);

            // get mercator extent of target tile
            mapnik::vector::spherical_mercator merc(target_vt->width());
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
            if (vt->status_ == LAZY_DONE) // tile is already parsed, we're good
            {
                mapnik::vector::tile const& tiledata = vt->get_tile();
                unsigned num_layers = tiledata.layers_size();
                if (num_layers > 0)
                {
                    for (int i=0; i < tiledata.layers_size(); ++i)
                    {
                        mapnik::vector::tile_layer const& layer = tiledata.layers(i);
                        mapnik::layer lyr(layer.name(),merc_srs);
                        MAPNIK_SHARED_PTR<mapnik::vector::tile_datasource> ds = MAPNIK_MAKE_SHARED<
                                                        mapnik::vector::tile_datasource>(
                                                            layer,
                                                            vt->x_,
                                                            vt->y_,
                                                            vt->z_,
                                                            vt->width()
                                                            );
                        ds->set_envelope(m_req.get_buffered_extent());
                        lyr.set_datasource(ds);
                        map.MAPNIK_ADD_LAYER(lyr);
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
                    mapnik::vector::tile tiledata;
                    if (tiledata.ParseFromArray(vt->buffer_.data(), bytes))
                    {
                        unsigned num_layers = tiledata.layers_size();
                        if (num_layers > 0)
                        {
                            for (int i=0; i < tiledata.layers_size(); ++i)
                            {
                                mapnik::vector::tile_layer const& layer = tiledata.layers(i);
                                mapnik::layer lyr(layer.name(),merc_srs);
                                MAPNIK_SHARED_PTR<mapnik::vector::tile_datasource> ds = MAPNIK_MAKE_SHARED<
                                                                mapnik::vector::tile_datasource>(
                                                                    layer,
                                                                    vt->x_,
                                                                    vt->y_,
                                                                    vt->z_,
                                                                    vt->width()
                                                                    );
                                ds->set_envelope(m_req.get_buffered_extent());
                                lyr.set_datasource(ds);
                                map.MAPNIK_ADD_LAYER(lyr);
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
                NanThrowError("could not serialize new data for vt");
                NanReturnUndefined();
            }
            if (!new_message.empty())
            {
                target_vt->buffer_.append(new_message.data(),new_message.size());
                target_vt->status_ = VectorTile::LAZY_MERGE;
            }
        }
    }
    NanReturnUndefined();
}

#ifdef PROTOBUF_FULL
NAN_METHOD(VectorTile::toString)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    mapnik::vector::tile const& tiledata = d->get_tile();
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
        std::vector<std::string> names = d->lazy_names();
        Local<Array> arr = NanNew<Array>(names.size());
        unsigned idx = 0;
        BOOST_FOREACH ( std::string const& name, names )
        {
            arr->Set(idx++,NanNew(name.c_str()));
        }
        NanReturnValue(arr);
    } else {
        mapnik::vector::tile const& tiledata = d->get_tile();
        Local<Array> arr = NanNew<Array>(tiledata.layers_size());
        for (int i=0; i < tiledata.layers_size(); ++i)
        {
            mapnik::vector::tile_layer const& layer = tiledata.layers(i);
            arr->Set(i, NanNew(layer.name().c_str()));
        }
        NanReturnValue(arr);
    }
    NanReturnUndefined();
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
    Local<Array> arr = NanNew<Array>();
    try  {
        mapnik::projection wgs84("+init=epsg:4326");
        mapnik::projection merc("+init=epsg:3857");
        mapnik::proj_transform tr(wgs84,merc);
        double x = lon;
        double y = lat;
        double z = 0;
        if (!tr.forward(x,y,z))
        {
            NanThrowError("could not reproject lon/lat to mercator");
            NanReturnUndefined();
        }
        VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
        mapnik::vector::tile const& tiledata = d->get_tile();
        mapnik::coord2d pt(x,y);
        unsigned idx = 0;
        if (!layer_name.empty())
        {
                int tile_layer_idx = -1;
                for (int j=0; j < tiledata.layers_size(); ++j)
                {
                    mapnik::vector::tile_layer const& layer = tiledata.layers(j);
                    if (layer_name == layer.name())
                    {
                        tile_layer_idx = j;
                        break;
                    }
                }
                if (tile_layer_idx > -1)
                {
                    mapnik::vector::tile_layer const& layer = tiledata.layers(tile_layer_idx);
                    MAPNIK_SHARED_PTR<mapnik::vector::tile_datasource> ds = MAPNIK_MAKE_SHARED<
                                                mapnik::vector::tile_datasource>(
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
                            bool hit = false;
                            double distance = 0.0;
                            BOOST_FOREACH ( mapnik::geometry_type const& geom, feature->paths() )
                            {
                               if (_hit_test(geom,x,y,tolerance,distance))
                               {
                                   hit = true;
                                   break;
                               }
                            }
                            if (hit)
                            {
                                Handle<Value> feat = Feature::New(feature);
                                Local<Object> feat_obj = feat->ToObject();
                                feat_obj->Set(NanNew("layer"),NanNew(layer.name().c_str()));
                                feat_obj->Set(NanNew("distance"),NanNew<Number>(distance));
                                arr->Set(idx++,feat);
                            }
                        }
                    }
                }
        }
        else
        {
            for (int i=0; i < tiledata.layers_size(); ++i)
            {
                mapnik::vector::tile_layer const& layer = tiledata.layers(i);
                MAPNIK_SHARED_PTR<mapnik::vector::tile_datasource> ds = MAPNIK_MAKE_SHARED<
                                            mapnik::vector::tile_datasource>(
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
                        bool hit = false;
                        double distance = 0.0;
                        BOOST_FOREACH ( mapnik::geometry_type const& geom, feature->paths() )
                        {
                           if (_hit_test(geom,x,y,tolerance,distance))
                           {
                               hit = true;
                               break;
                           }
                        }
                        if (hit)
                        {
                            Handle<Value> feat = Feature::New(feature);
                            Local<Object> feat_obj = feat->ToObject();
                            feat_obj->Set(NanNew("layer"),NanNew(layer.name().c_str()));
                            feat_obj->Set(NanNew("distance"),NanNew<Number>(distance));
                            arr->Set(idx++,feat);
                        }
                    }
                }
            }
        }
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnValue(arr);
}

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
            while (i < num_fields) {
                Local<Value> name = a->Get(i);
                if (name->IsString()){
                    fields.push_back(TOSTR(name));
                }
                i++;
            }
        }
    }

    if (layer_name.empty())
    {
        NanThrowTypeError("options.layer is required");
        NanReturnUndefined();
    }

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    mapnik::vector::tile const& tiledata = d->get_tile();

    int tile_layer_idx = -1;
    for (int j=0; j < tiledata.layers_size(); ++j)
    {
        mapnik::vector::tile_layer const& layer = tiledata.layers(j);
        if (layer_name == layer.name())
        {
            tile_layer_idx = j;
            break;
        }
    }
    if (tile_layer_idx == -1)
    {
        NanThrowError("Could not find layer in vector tile");
        NanReturnUndefined();
    }

    Local<Object> results = NanNew<Object>();
    Local<Array> queryArray = Local<Array>::Cast(args[0]);
    mapnik::box2d<double> bbox;
    std::vector<std::pair<uint32_t, mapnik::coord2d> > points;
    mapnik::projection wgs84("+init=epsg:4326");
    mapnik::projection merc("+init=epsg:3857");
    mapnik::proj_transform tr(wgs84,merc);
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
        double x = lon->NumberValue();
        double y = lat->NumberValue();
        double z = 0;
        if (!tr.forward(x,y,z))
        {
            NanThrowError("could not reproject lon/lat to mercator");
            NanReturnUndefined();
        }
        mapnik::coord2d pt(x,y);
        points.push_back(std::make_pair(p,pt));
        bbox.expand_to_include(pt);

        Local<Array> twoClosest = NanNew<Array>();
        Local<Object> defaultObject1 = NanNew<Object>();
        Local<Object> defaultObject2 = NanNew<Object>();
        defaultObject1->Set(NanNew("distance"), NanNew<Number>(1000000000));
        defaultObject2->Set(NanNew("distance"), NanNew<Number>(1000000000));
        twoClosest->Set(0, defaultObject1);
        twoClosest->Set(1, defaultObject2);
        results->Set(p, twoClosest);
    }

    bbox.pad(tolerance);

    mapnik::vector::tile_layer const& layer = tiledata.layers(tile_layer_idx);
    MAPNIK_SHARED_PTR<mapnik::vector::tile_datasource> ds = MAPNIK_MAKE_SHARED<
                                mapnik::vector::tile_datasource>(
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
        BOOST_FOREACH ( std::string const& name, fields )
        {
            q.add_property_name(name);
        }
    }

    mapnik::featureset_ptr fs = ds->features(q);
    if (fs)
    {
        try {
            mapnik::feature_ptr feature;
            while ((feature = fs->next()))
            {
                typedef std::pair<uint32_t, mapnik::coord2d> mapnikCoord;
                BOOST_FOREACH (mapnikCoord const& pair, points)
                {
                    mapnik::coord2d pt(pair.second);
                    double distance = -1;
                    BOOST_FOREACH (mapnik::geometry_type const& geom, feature->paths())
                    {
                        double d = path_to_point_distance(geom,pt.x,pt.y);
                        if (d >= 0)
                        {
                            if (distance >= 0)
                            {
                                if (d < distance) distance = d;
                            }
                            else
                            {
                                distance = d;
                            }
                        }
                    }
                    if (distance >= 0)
                    {
                        Local<Array> twoClosest = Local<Array>::Cast(results->Get(pair.first));
                        Local<Object> closest1 = twoClosest->Get(0)->ToObject();
                        Local<Object> closest2 = twoClosest->Get(1)->ToObject();
                        double distance1 = closest1->Get(NanNew("distance"))->NumberValue();
                        double distance2 = closest2->Get(NanNew("distance"))->NumberValue();

                        if(distance <= distance1) {
                            twoClosest->Set(1, closest1);
                            Local <Object> newValues = NanNew<Object>();
                            newValues->Set(NanNew("distance"), NanNew<Number>(distance));
                            Handle<Value> feat = Feature::New(feature);
                            newValues->Set(NanNew("layer"),NanNew(layer.name().c_str()));
                            newValues->Set(NanNew("feature"),feat->ToObject());
                            twoClosest->Set(0, newValues);
                        } else if (distance <= distance2) {
                            Local <Object> newValues = NanNew<Object>();
                            newValues->Set(NanNew("distance"), NanNew<Number>(distance));
                            Handle<Value> feat = Feature::New(feature);
                            newValues->Set(NanNew("layer"),NanNew(layer.name().c_str()));
                            newValues->Set(NanNew("feature"),feat->ToObject());
                            twoClosest->Set(1, newValues);
                        }
                    }
                }
            }
        }
        catch (std::exception const& ex)
        {
            NanThrowError(ex.what());
            NanReturnUndefined();
        }
    }

    NanReturnValue(results);
}

NAN_METHOD(VectorTile::toJSON)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    mapnik::vector::tile const& tiledata = d->get_tile();
    Local<Array> arr = NanNew<Array>(tiledata.layers_size());
    for (int i=0; i < tiledata.layers_size(); ++i)
    {
        mapnik::vector::tile_layer const& layer = tiledata.layers(i);
        Local<Object> layer_obj = NanNew<Object>();
        layer_obj->Set(NanNew("name"), NanNew(layer.name().c_str()));
        layer_obj->Set(NanNew("extent"), NanNew<Integer>(layer.extent()));
        layer_obj->Set(NanNew("version"), NanNew<Integer>(layer.version()));

        Local<Array> f_arr = NanNew<Array>(layer.features_size());
        for (int j=0; j < layer.features_size(); ++j)
        {
            Local<Object> feature_obj = NanNew<Object>();
            mapnik::vector::tile_feature const& f = layer.features(j);
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
                    mapnik::vector::tile_value const& value = layer.values(key_value);
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

static void layer_to_geojson(mapnik::vector::tile_layer const& layer,
                             Local<Array> f_arr,
                             unsigned x,
                             unsigned y,
                             unsigned z,
                             unsigned width,
                             unsigned idx0)
{
    mapnik::projection wgs84("+init=epsg:4326");
    mapnik::projection merc("+init=epsg:3857");
    mapnik::proj_transform tr(merc,wgs84);
    double zc = 0;
    double resolution = mapnik::EARTH_CIRCUMFERENCE/(1 << z);
    double tile_x_ = -0.5 * mapnik::EARTH_CIRCUMFERENCE + x * resolution;
    double tile_y_ =  0.5 * mapnik::EARTH_CIRCUMFERENCE - y * resolution;
    for (int j=0; j < layer.features_size(); ++j)
    {
        double scale_ = (static_cast<double>(layer.extent()) / width) * static_cast<double>(width)/resolution;
        Local<Object> feature_obj = NanNew<Object>();
        feature_obj->Set(NanNew("type"),NanNew("Feature"));
        Local<Object> geometry = NanNew<Object>();
        mapnik::vector::tile_feature const& f = layer.features(j);
        unsigned int g_type = f.type();
        Local<String> js_type = NanNew("Unknown");
        switch (g_type)
        {
        case MAPNIK_POINT:
        {
            js_type = NanNew("Point");
            break;
        }
        case MAPNIK_LINESTRING:
        {
            js_type = NanNew("LineString");
            break;
        }
        case MAPNIK_POLYGON:
        {
            js_type = NanNew("Polygon");
            break;
        }
        default:
        {
            break;
        }
        }
        geometry->Set(NanNew("type"), js_type);
        Local<Array> g_arr = NanNew<Array>();
        if (g_type == MAPNIK_POLYGON)
        {
            Local<Array> enclosing_array = NanNew<Array>(1);
            enclosing_array->Set(0,g_arr);
            geometry->Set(NanNew("coordinates"),enclosing_array);
        }
        else
        {
            geometry->Set(NanNew("coordinates"),g_arr);
        }
        int cmd = -1;
        const int cmd_bits = 3;
        unsigned length = 0;
        double x1 = tile_x_;
        double y1 = tile_y_;
        unsigned idx = 0;
        for (int k = 0; k < f.geometry_size();)
        {
            if (!length) {
                unsigned cmd_length = f.geometry(k++);
                cmd = cmd_length & ((1 << cmd_bits) - 1);
                length = cmd_length >> cmd_bits;
            }
            if (length > 0) {
                length--;
                if (cmd == mapnik::SEG_MOVETO || cmd == mapnik::SEG_LINETO)
                {
                    int32_t dx = f.geometry(k++);
                    int32_t dy = f.geometry(k++);
                    dx = ((dx >> 1) ^ (-(dx & 1)));
                    dy = ((dy >> 1) ^ (-(dy & 1)));
                    x1 += (static_cast<double>(dx) / scale_);
                    y1 -= (static_cast<double>(dy) / scale_);
                    double x2 = x1;
                    double y2 = y1;
                    if (tr.forward(x2,y2,zc))
                    {
                        if (g_type == MAPNIK_POINT)
                        {
                            g_arr->Set(0,NanNew<Number>(x2));
                            g_arr->Set(1,NanNew<Number>(y2));
                        }
                        else
                        {
                            Local<Array> v_arr = NanNew<Array>(2);
                            v_arr->Set(0,NanNew<Number>(x2));
                            v_arr->Set(1,NanNew<Number>(y2));
                            g_arr->Set(idx++,v_arr);
                        }
                    }
                    else
                    {
                        std::clog << "could not project\n";
                    }
                }
                else if (cmd == (mapnik::SEG_CLOSE & ((1 << cmd_bits) - 1)))
                {
                    if (g_arr->Length() > 0) g_arr->Set(idx++,Local<Array>::Cast(g_arr->Get(0)));
                }
                else
                {
                    std::stringstream msg;
                    msg << "Unknown command type (layer_to_geojson): "
                        << cmd;
                    throw std::runtime_error(msg.str());
                }
            }
        }
        feature_obj->Set(NanNew("geometry"),geometry);
        Local<Object> att_obj = NanNew<Object>();
        for (int m = 0; m < f.tags_size(); m += 2)
        {
            std::size_t key_name = f.tags(m);
            std::size_t key_value = f.tags(m + 1);

            if (key_name < static_cast<std::size_t>(layer.keys_size())
                && key_value < static_cast<std::size_t>(layer.values_size()))
            {
                std::string const& name = layer.keys(key_name);
                mapnik::vector::tile_value const& value = layer.values(key_value);
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
        }
        feature_obj->Set(NanNew("properties"),att_obj);
        f_arr->Set(j+idx0,feature_obj);
    }
}

NAN_METHOD(VectorTile::toGeoJSON)
{
    NanScope();
    if (args.Length() < 1) {
        NanThrowError("first argument must be either a layer name (string) or layer index (integer)");
        NanReturnUndefined();
    }
    Local<Value> layer_id = args[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) ) {
        NanThrowTypeError("'layer' argument must be either a layer name (string) or layer index (integer)");
        NanReturnUndefined();
    }

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    mapnik::vector::tile const& tiledata = d->get_tile();
    std::size_t layer_num = tiledata.layers_size();
    int layer_idx = -1;
    bool all_array = false;
    bool all_flattened = false;

    if (layer_id->IsString()) {
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
                mapnik::vector::tile_layer const& layer = tiledata.layers(i);
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
                NanThrowTypeError(s.str().c_str());
                NanReturnUndefined();
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
            NanThrowTypeError(s.str().c_str());
            NanReturnUndefined();
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
            NanThrowTypeError(s.str().c_str());
            NanReturnUndefined();
        }
    } else {
        NanThrowTypeError("layer id must be a string or index number");
        NanReturnUndefined();
    }

    try
    {
        if (all_array)
        {
            Local<Array> layer_arr = NanNew<Array>(layer_num);
            for (unsigned i=0;i<layer_num;++i)
            {
                Local<Object> layer_obj = NanNew<Object>();
                layer_obj->Set(NanNew("type"), NanNew("FeatureCollection"));
                Local<Array> f_arr = NanNew<Array>();
                layer_obj->Set(NanNew("features"), f_arr);
                mapnik::vector::tile_layer const& layer = tiledata.layers(i);
                layer_obj->Set(NanNew("name"), NanNew(layer.name().c_str()));
                layer_to_geojson(layer,f_arr,d->x_,d->y_,d->z_,d->width_,0);
                layer_arr->Set(i,layer_obj);
            }
            NanReturnValue(layer_arr);
        }
        else
        {
            Local<Object> layer_obj = NanNew<Object>();
            layer_obj->Set(NanNew("type"), NanNew("FeatureCollection"));
            Local<Array> f_arr = NanNew<Array>();
            layer_obj->Set(NanNew("features"), f_arr);
            if (all_flattened)
            {
                for (unsigned i=0;i<layer_num;++i)
                {
                    mapnik::vector::tile_layer const& layer = tiledata.layers(i);
                    layer_to_geojson(layer,f_arr,d->x_,d->y_,d->z_,d->width_,f_arr->Length());
                }
                NanReturnValue(layer_obj);
            }
            else
            {
                mapnik::vector::tile_layer const& layer = tiledata.layers(layer_idx);
                layer_obj->Set(NanNew("name"), NanNew(layer.name().c_str()));
                layer_to_geojson(layer,f_arr,d->x_,d->y_,d->z_,d->width_,0);
                NanReturnValue(layer_obj);
            }
        }
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
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
    VectorTile* d = ObjectWrap::Unwrap<VectorTile>(args.Holder());
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
        typedef mapnik::vector::backend_pbf backend_type;
        typedef mapnik::vector::processor<backend_type> renderer_type;
        backend_type backend(d->get_tile_nonconst(),path_multiplier);
        mapnik::Map map(d->width_,d->height_,"+init=epsg:3857");
        mapnik::vector::spherical_mercator merc(d->width_);
        double minx,miny,maxx,maxy;
        merc.xyz(d->x_,d->y_,d->z_,minx,miny,maxx,maxy);
        map.zoom_to_box(mapnik::box2d<double>(minx,miny,maxx,maxy));
        mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
        m_req.set_buffer_size(8);
        mapnik::parameters p;
        // TODO - use mapnik core GeoJSON parser
        p["type"]="ogr";
        p["file"]=geojson_string;
        p["layer_by_index"]="0";
        mapnik::layer lyr(geojson_name,"+init=epsg:4326");
        lyr.set_datasource(mapnik::datasource_cache::instance().create(p));
        map.MAPNIK_ADD_LAYER(lyr);
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
    VectorTile* d = ObjectWrap::Unwrap<VectorTile>(args.This());
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
    mapnik::vector::tile & tiledata = d->get_tile_nonconst();
    mapnik::vector::tile_layer * new_layer = tiledata.add_layers();
    new_layer->set_name(layer_name);
    new_layer->set_version(1);
    new_layer->set_extent(256 * 16);
    // no need
    // current_feature_->set_id(feature.id());
    mapnik::vector::tile_feature * new_feature = new_layer->add_features();
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
    char *data;
    size_t dataLength;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
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
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
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
    delete closure;
}

NAN_METHOD(VectorTile::getData)
{
    NanScope();
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    try {
        // shortcut: return raw data and avoid trip through proto object
        // TODO  - safe for null string?
        int raw_size = static_cast<int>(d->buffer_.size());
        if (raw_size > 0 && d->byte_size_ <= raw_size) {
            NanReturnValue(NanNewBufferHandle((char*)d->buffer_.data(),raw_size));
        } else {
            if (d->byte_size_ <= 0) {
                NanReturnValue(NanNewBufferHandle(0));
            } else {
                // NOTE: tiledata.ByteSize() must be called
                // after each modification of tiledata otherwise the
                // SerializeWithCachedSizesToArray will throw:
                // Error: CHECK failed: !coded_out.HadError()
                mapnik::vector::tile const& tiledata = d->get_tile();
                Local<Object> retbuf = NanNewBufferHandle(d->byte_size_);
                // TODO - consider wrapping in fastbuffer: https://gist.github.com/drewish/2732711
                // http://www.samcday.com.au/blog/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
                google::protobuf::uint8* start = reinterpret_cast<google::protobuf::uint8*>(node::Buffer::Data(retbuf));
                google::protobuf::uint8* end = tiledata.SerializeWithCachedSizesToArray(start);
                if (end - start != d->byte_size_) {
                    NanThrowError("serialization failed, possible race condition");
                    NanReturnUndefined();
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

struct vector_tile_render_baton_t {
    uv_work_t request;
    Map* m;
    VectorTile * d;
    Image * im;
    CairoSurface * c;
    Grid * g;
    std::size_t layer_idx;
    int z;
    int x;
    int y;
    unsigned width;
    unsigned height;
    bool zxy_override;
    bool error;
    int buffer_size;
    double scale_factor;
    double scale_denominator;
#if MAPNIK_VERSION >= 300000
    mapnik::attributes variables;
#endif
    std::string error_name;
    Persistent<Function> cb;
    std::string result;
    bool use_cairo;
    vector_tile_render_baton_t() :
        request(),
        m(NULL),
        d(NULL),
        im(NULL),
        c(NULL),
        g(NULL),
        layer_idx(0),
        z(0),
        x(0),
        y(0),
        width(0),
        height(0),
        zxy_override(false),
        error(false),
        buffer_size(0),
        scale_factor(1.0),
        scale_denominator(0.0),
#if MAPNIK_VERSION >= 300000
        variables(),
#endif
        use_cairo(true) {}
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
    Local<Object> options = NanNew<Object>();

    if (args.Length() > 2)
    {
        if (!args[2]->IsObject())
        {
            delete closure;
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
                delete closure;
                NanThrowTypeError("optional arg 'buffer_size' must be a number");
                NanReturnUndefined();
            }
            closure->buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(NanNew("scale"))) {
            Local<Value> bind_opt = options->Get(NanNew("scale"));
            if (!bind_opt->IsNumber())
            {
                delete closure;
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
                delete closure;
                NanThrowTypeError("optional arg 'scale_denominator' must be a number");
                NanReturnUndefined();
            }
            closure->scale_denominator = bind_opt->NumberValue();
        }
    }

    closure->layer_idx = 0;
    if (NanNew(Image::constructor)->HasInstance(im_obj))
    {
        Image *im = node::ObjectWrap::Unwrap<Image>(im_obj);
        closure->im = im;
        closure->width = im->get()->width();
        closure->height = im->get()->height();
        closure->im->_ref();
    }
    else if (NanNew(CairoSurface::constructor)->HasInstance(im_obj))
    {
        CairoSurface *c = node::ObjectWrap::Unwrap<CairoSurface>(im_obj);
        closure->c = c;
        closure->width = c->width();
        closure->height = c->height();
        closure->c->_ref();
        if (options->Has(NanNew("renderer")))
        {
            Local<Value> renderer = options->Get(NanNew("renderer"));
            if (!renderer->IsString() )
            {
                delete closure;
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
                delete closure;
                NanThrowError("'renderer' option must be a string of either 'svg' or 'cairo'");
                NanReturnUndefined();
            }
        }
    }
    else if (NanNew(Grid::constructor)->HasInstance(im_obj))
    {
        Grid *g = node::ObjectWrap::Unwrap<Grid>(im_obj);
        closure->g = g;
        closure->width = g->get()->width();
        closure->height = g->get()->height();
        closure->g->_ref();

        std::size_t layer_idx = 0;

        // grid requires special options for now
        if (!options->Has(NanNew("layer")))
        {
            delete closure;
            NanThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
            NanReturnUndefined();
        } else {
            std::vector<mapnik::layer> const& layers = m->get()->layers();

            Local<Value> layer_id = options->Get(NanNew("layer"));
            if (! (layer_id->IsString() || layer_id->IsNumber()) )
            {
                delete closure;
                NanThrowTypeError("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)");
                NanReturnUndefined();
            }

            if (layer_id->IsString()) {
                bool found = false;
                unsigned int idx(0);
                std::string layer_name = TOSTR(layer_id);
                BOOST_FOREACH ( mapnik::layer const& lyr, layers )
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
                    delete closure;
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
                    delete closure;
                    NanThrowTypeError(s.str().c_str());
                    NanReturnUndefined();
                }
            } else {
                delete closure;
                NanThrowTypeError("layer id must be a string or index number");
                NanReturnUndefined();
            }
        }
        if (options->Has(NanNew("fields"))) {

            Local<Value> param_val = options->Get(NanNew("fields"));
            if (!param_val->IsArray())
            {
                delete closure;
                NanThrowTypeError("option 'fields' must be an array of strings");
                NanReturnUndefined();
            }
            Local<Array> a = Local<Array>::Cast(param_val);
            unsigned int i = 0;
            unsigned int num_fields = a->Length();
            while (i < num_fields) {
                Local<Value> name = a->Get(i);
                if (name->IsString()){
                    g->get()->add_property_name(TOSTR(name));
                }
                i++;
            }
        }
        closure->layer_idx = layer_idx;
    }
    else
    {
        delete closure;
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
    NanReturnUndefined();
}

template <typename Renderer> void process_layers(Renderer & ren,
                                            mapnik::request const& m_req,
                                            mapnik::projection const& map_proj,
                                            std::vector<mapnik::layer> const& layers,
                                            double scale_denom,
                                            mapnik::vector::tile const& tiledata,
                                            vector_tile_render_baton_t *closure,
                                            mapnik::box2d<double> const& map_extent)
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
                mapnik::vector::tile_layer const& layer = tiledata.layers(j);
                if (lyr.name() == layer.name())
                {
                    mapnik::layer lyr_copy(lyr);
                    MAPNIK_SHARED_PTR<mapnik::vector::tile_datasource> ds = MAPNIK_MAKE_SHARED<
                                                    mapnik::vector::tile_datasource>(
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
        mapnik::vector::spherical_mercator merc(closure->d->width_);
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
        mapnik::vector::tile const& tiledata = closure->d->get_tile();
        // render grid for layer
        if (closure->g)
        {
            mapnik::grid_renderer<mapnik::grid> ren(map_in,
                                                    m_req,
#if MAPNIK_VERSION >= 300000
                                                    closure->variables,
#endif
                                                    *closure->g->get(),
                                                    closure->scale_factor);
            ren.start_map_processing(map_in);

            mapnik::layer const& lyr = layers[closure->layer_idx];
            if (lyr.visible(scale_denom))
            {
                int tile_layer_idx = -1;
                for (int j=0; j < tiledata.layers_size(); ++j)
                {
                    mapnik::vector::tile_layer const& layer = tiledata.layers(j);
                    if (lyr.name() == layer.name())
                    {
                        tile_layer_idx = j;
                        break;
                    }
                }
                if (tile_layer_idx > -1)
                {
                    mapnik::vector::tile_layer const& layer = tiledata.layers(tile_layer_idx);
                    if (layer.features_size() <= 0)
                    {
                        return;
                    }

                    // copy property names
                    std::set<std::string> attributes = closure->g->get()->property_names();
                    // todo - make this a static constant
                    std::string known_id_key = "__id__";
                    if (attributes.find(known_id_key) != attributes.end())
                    {
                        attributes.erase(known_id_key);
                    }
                    std::string join_field = closure->g->get()->get_key();
                    if (known_id_key != join_field &&
                        attributes.find(join_field) == attributes.end())
                    {
                        attributes.insert(join_field);
                    }

                    mapnik::layer lyr_copy(lyr);
                    MAPNIK_SHARED_PTR<mapnik::vector::tile_datasource> ds = MAPNIK_MAKE_SHARED<
                                                    mapnik::vector::tile_datasource>(
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
        else if (closure->c)
        {
            if (closure->use_cairo)
            {
#if defined(HAVE_CAIRO)
                mapnik::cairo_surface_ptr surface;
                // TODO - support any surface type
                surface = mapnik::cairo_surface_ptr(cairo_svg_surface_create_for_stream(
                                                       (cairo_write_func_t)closure->c->write_callback,
                                                       (void*)(&closure->c->ss_),
                                                       static_cast<double>(closure->c->width()),
                                                       static_cast<double>(closure->c->height())
                                                    ),mapnik::cairo_surface_closer());
                mapnik::cairo_ptr c_context = (mapnik::create_context(surface));
                mapnik::cairo_renderer<mapnik::cairo_ptr> ren(map_in,m_req,
#if MAPNIK_VERSION >= 300000
                                                                closure->variables,
#endif
                                                                c_context,closure->scale_factor);
                ren.start_map_processing(map_in);
                process_layers(ren,m_req,map_proj,layers,scale_denom,tiledata,closure,map_extent);
                ren.end_map_processing(map_in);
#else
                closure->error = true;
                closure->error_name = "no support for rendering svg with cairo backend";
#endif
            }
            else
            {
#if MAPNIK_VERSION >= 200300
  #if defined(SVG_RENDERER)
                typedef mapnik::svg_renderer<std::ostream_iterator<char> > svg_ren;
                std::ostream_iterator<char> output_stream_iterator(closure->c->ss_);
                svg_ren ren(map_in, m_req,
#if MAPNIK_VERSION >= 300000
                            closure->variables,
#endif
                            output_stream_iterator, closure->scale_factor);
                ren.start_map_processing(map_in);
                process_layers(ren,m_req,map_proj,layers,scale_denom,tiledata,closure,map_extent);
                ren.end_map_processing(map_in);
  #else
                closure->error = true;
                closure->error_name = "no support for rendering svg with native svg backend (-DSVG_RENDERER)";
  #endif
#else
                closure->error = true;
                closure->error_name = "no support for rendering svg with native svg backend (-DSVG_RENDERER)";
#endif
            }
        }
        // render all layers with agg
        else
        {
            mapnik::agg_renderer<mapnik::image_32> ren(map_in,m_req,
#if MAPNIK_VERSION >= 300000
                                                    closure->variables,
#endif
                                                    *closure->im->get(),closure->scale_factor);
            ren.start_map_processing(map_in);
            process_layers(ren,m_req,map_proj,layers,scale_denom,tiledata,closure,map_extent);
            ren.end_map_processing(map_in);
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
        if (closure->im)
        {
            Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->im) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
        else if (closure->g)
        {
            Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->g) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
        else if (closure->c)
        {
            Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->c) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
    }

    closure->m->_unref();
    if (closure->im) closure->im->_unref();
    if (closure->g) closure->g->_unref();
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
#if MAPNIK_VERSION >= 200200
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.Holder());
    d->clear();
#endif
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
#if MAPNIK_VERSION >= 200200
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
#endif
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
        bool is_solid = mapnik::vector::is_solid_extent(d->get_tile(), key);
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
        closure->result = mapnik::vector::is_solid_extent(closure->d->get_tile(),closure->key);
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
