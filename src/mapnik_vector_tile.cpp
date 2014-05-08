#include "node.h"                       // for NODE_SET_PROTOTYPE_METHOD, etc
#include <node_buffer.h>
#include <node_version.h>

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
#include <mapnik/cairo_renderer.hpp>
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

// fromGeoJSON
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

Persistent<FunctionTemplate> VectorTile::constructor;

void VectorTile::Initialize(Handle<Object> target) {
    HandleScope scope;
    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(VectorTile::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("VectorTile"));
    NODE_SET_PROTOTYPE_METHOD(constructor, "render", render);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setData", setData);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setDataSync", setDataSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "getData", getData);
    NODE_SET_PROTOTYPE_METHOD(constructor, "parse", parse);
    NODE_SET_PROTOTYPE_METHOD(constructor, "parseSync", parseSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "addData", addData);
    NODE_SET_PROTOTYPE_METHOD(constructor, "composite", composite);
    NODE_SET_PROTOTYPE_METHOD(constructor, "query", query);
    NODE_SET_PROTOTYPE_METHOD(constructor, "names", names);
    NODE_SET_PROTOTYPE_METHOD(constructor, "toJSON", toJSON);
    NODE_SET_PROTOTYPE_METHOD(constructor, "toGeoJSON", toGeoJSON);
    NODE_SET_PROTOTYPE_METHOD(constructor, "fromGeoJSON", fromGeoJSON);
#ifdef PROTOBUF_FULL
    NODE_SET_PROTOTYPE_METHOD(constructor, "toString", toString);
#endif

    // common to mapnik.Image
    NODE_SET_PROTOTYPE_METHOD(constructor, "width", width);
    NODE_SET_PROTOTYPE_METHOD(constructor, "height", height);
    NODE_SET_PROTOTYPE_METHOD(constructor, "painted", painted);
    NODE_SET_PROTOTYPE_METHOD(constructor, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(constructor, "clearSync", clear);
    NODE_SET_PROTOTYPE_METHOD(constructor, "isSolid", isSolid);
    NODE_SET_PROTOTYPE_METHOD(constructor, "isSolidSync", isSolidSync);
    target->Set(String::NewSymbol("VectorTile"),constructor->GetFunction());
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

Handle<Value> VectorTile::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args.Length() >= 3)
    {
        if (!args[0]->IsNumber() ||
            !args[1]->IsNumber() ||
            !args[2]->IsNumber())
        {
            return ThrowException(Exception::Error(
                                      String::New("required args (z, x, and y) must be a integers")));
        }
        unsigned width = 256;
        unsigned height = 256;
        Local<Object> options = Object::New();
        if (args.Length() > 3) {
            if (!args[3]->IsObject())
            {
                return ThrowException(Exception::TypeError(
                                          String::New("optional fourth argument must be an options object")));
            }
            options = args[3]->ToObject();
            if (options->Has(String::New("width"))) {
                Local<Value> opt = options->Get(String::New("width"));
                if (!opt->IsNumber())
                {
                    return ThrowException(Exception::TypeError(
                                              String::New("optional arg 'width' must be a number")));
                }
                width = opt->IntegerValue();
            }
            if (options->Has(String::New("height"))) {
                Local<Value> opt = options->Get(String::New("height"));
                if (!opt->IsNumber())
                {
                    return ThrowException(Exception::TypeError(
                                              String::New("optional arg 'height' must be a number")));
                }
                height = opt->IntegerValue();
            }
        }

        VectorTile* d = new VectorTile(args[0]->IntegerValue(),
                                   args[1]->IntegerValue(),
                                   args[2]->IntegerValue(),
                                   width,height);

        d->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
                                  String::New("please provide a z, x, y")));
    }
    return Undefined();
}

std::vector<std::string> VectorTile::lazy_names()
{
    std::vector<std::string> names;
    pbf::message item(buffer_.data(),buffer_.size());
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
        }
        else
        {
            throw std::runtime_error("could not merge buffer as protobuf");
        }
        break;
    }
    }
}

Handle<Value> VectorTile::composite(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() < 1 || !args[0]->IsArray()) {
        return ThrowException(Exception::TypeError(
                                  String::New("must provide an array of VectorTile objects and an optional options object")));
    }
    Local<Array> vtiles = Local<Array>::Cast(args[0]);
    unsigned num_tiles = vtiles->Length();
    if (num_tiles < 1) {
        return ThrowException(Exception::TypeError(
                                  String::New("must provide an array with at least one VectorTile object and an optional options object")));
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

    Local<Object> options = Object::New();

    if (args.Length() > 1) {
        // options object
        if (!args[1]->IsObject())
        {
            return ThrowException(Exception::TypeError(
                                      String::New("optional second argument must be an options object")));
        }
        options = args[1]->ToObject();
        if (options->Has(String::New("path_multiplier"))) {

            Local<Value> param_val = options->Get(String::New("path_multiplier"));
            if (!param_val->IsNumber())
            {
                return ThrowException(Exception::TypeError(
                                          String::New("option 'path_multiplier' must be an unsigned integer")));
            }
            path_multiplier = param_val->NumberValue();
        }
        if (options->Has(String::NewSymbol("tolerance")))
        {
            Local<Value> tol = options->Get(String::New("tolerance"));
            if (!tol->IsNumber())
            {
                return ThrowException(Exception::TypeError(String::New("tolerance value must be a number")));
            }
            tolerance = tol->NumberValue();
        }
        if (options->Has(String::New("buffer_size"))) {
            Local<Value> bind_opt = options->Get(String::New("buffer_size"));
            if (!bind_opt->IsNumber())
            {
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'buffer_size' must be a number")));
            }
            buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(String::New("scale"))) {
            Local<Value> bind_opt = options->Get(String::New("scale"));
            if (!bind_opt->IsNumber())
            {
                return ThrowException(Exception::TypeError(
                                        String::New("optional arg 'scale' must be a number")));
            }
            scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(String::NewSymbol("scale_denominator")))
        {
            Local<Value> bind_opt = options->Get(String::New("scale_denominator"));
            if (!bind_opt->IsNumber())
            {
                return ThrowException(Exception::TypeError(
                                        String::New("optional arg 'scale_denominator' must be a number")));
            }
            scale_denominator = bind_opt->NumberValue();
        }
        if (options->Has(String::New("offset_x"))) {
            Local<Value> bind_opt = options->Get(String::New("offset_x"));
            if (!bind_opt->IsNumber())
            {
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'offset_x' must be a number")));
            }
            offset_x = bind_opt->IntegerValue();
        }
        if (options->Has(String::New("offset_y"))) {
            Local<Value> bind_opt = options->Get(String::New("offset_y"));
            if (!bind_opt->IsNumber())
            {
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'offset_y' must be a number")));
            }
            offset_y = bind_opt->IntegerValue();
        }
    }

    VectorTile* target_vt = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    for (unsigned i=0;i < num_tiles;++i) {
        Local<Value> val = vtiles->Get(i);
        if (!val->IsObject()) {
            return ThrowException(Exception::TypeError(
                                      String::New("must provide an array of VectorTile objects")));
        }
        Local<Object> tile_obj = val->ToObject();
        if (tile_obj->IsNull() || tile_obj->IsUndefined() || !VectorTile::constructor->HasInstance(tile_obj)) {
            return ThrowException(Exception::TypeError(
                                      String::New("must provide an array of VectorTile objects")));
        }
        VectorTile* vt = node::ObjectWrap::Unwrap<VectorTile>(tile_obj);
        // TODO - handle name clashes
        if (target_vt->z_ == vt->z_ &&
            target_vt->x_ == vt->x_ &&
            target_vt->y_ == vt->y_)
        {
            target_vt->buffer_.append(vt->buffer_.data(),vt->buffer_.size());
            target_vt->status_ = VectorTile::LAZY_MERGE;
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
            else // tile is not pre-parsed so parse into new object to avoid mutating input
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
                return ThrowException(Exception::Error(
                          String::New("could not serialize new data for vt")));
            }
            target_vt->buffer_.append(new_message.data(),new_message.size());
            target_vt->status_ = VectorTile::LAZY_MERGE;
        }
    }
    return scope.Close(Undefined());
}

#ifdef PROTOBUF_FULL
Handle<Value> VectorTile::toString(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    mapnik::vector::tile const& tiledata = d->get_tile();
    return scope.Close(String::New(tiledata.DebugString().c_str()));
}
#endif

Handle<Value> VectorTile::names(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    int raw_size = d->buffer_.size();
    if (d->byte_size_ <= raw_size)
    {
        std::vector<std::string> names = d->lazy_names();
        Local<Array> arr = Array::New(names.size());
        unsigned idx = 0;
        BOOST_FOREACH ( std::string const& name, names )
        {
            arr->Set(idx++,String::New(name.c_str()));
        }
        return scope.Close(arr);
    } else {
        mapnik::vector::tile const& tiledata = d->get_tile();
        Local<Array> arr = Array::New(tiledata.layers_size());
        for (int i=0; i < tiledata.layers_size(); ++i)
        {
            mapnik::vector::tile_layer const& layer = tiledata.layers(i);
            arr->Set(i, String::New(layer.name().c_str()));
        }
        return scope.Close(arr);
    }
    return scope.Close(Undefined());
}

Handle<Value> VectorTile::width(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    return scope.Close(Integer::New(d->width()));
}

Handle<Value> VectorTile::height(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    return scope.Close(Integer::New(d->height()));
}

Handle<Value> VectorTile::painted(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    return scope.Close(Boolean::New(d->painted()));
}

Handle<Value> VectorTile::query(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() < 2 || !args[0]->IsNumber() || !args[1]->IsNumber())
    {
        return ThrowException(Exception::Error(
                                  String::New("expects lon,lat args")));
    }
    double tolerance = 0.0; // meters
    std::string layer_name("");
    if (args.Length() > 2)
    {
        Local<Object> options = Object::New();
        if (!args[2]->IsObject())
        {
            return ThrowException(Exception::TypeError(String::New("optional third argument must be an options object")));
        }
        options = args[2]->ToObject();
        if (options->Has(String::NewSymbol("tolerance")))
        {
            Local<Value> tol = options->Get(String::New("tolerance"));
            if (!tol->IsNumber())
            {
                return ThrowException(Exception::TypeError(String::New("tolerance value must be a number")));
            }
            tolerance = tol->NumberValue();
        }
        if (options->Has(String::NewSymbol("layer")))
        {
            Local<Value> layer_id = options->Get(String::New("layer"));
            if (!layer_id->IsString())
            {
                return ThrowException(Exception::TypeError(String::New("layer value must be a string")));
            }
            layer_name = TOSTR(layer_id);
        }
    }

    double lon = args[0]->NumberValue();
    double lat = args[1]->NumberValue();
    Local<Array> arr = Array::New();
    try  {
        mapnik::projection wgs84("+init=epsg:4326");
        mapnik::projection merc("+init=epsg:3857");
        mapnik::proj_transform tr(wgs84,merc);
        double x = lon;
        double y = lat;
        double z = 0;
        if (!tr.forward(x,y,z))
        {
            return ThrowException(Exception::Error(
                                      String::New("could not reproject lon/lat to mercator")));
        }
        VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
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
                                feat_obj->Set(String::New("layer"),String::New(layer.name().c_str()));
                                feat_obj->Set(String::New("distance"),Number::New(distance));
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
                            feat_obj->Set(String::New("layer"),String::New(layer.name().c_str()));
                            feat_obj->Set(String::New("distance"),Number::New(distance));
                            arr->Set(idx++,feat);
                        }
                    }
                }
            }
        }
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    return scope.Close(arr);
}

Handle<Value> VectorTile::toJSON(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    mapnik::vector::tile const& tiledata = d->get_tile();
    Local<Array> arr = Array::New(tiledata.layers_size());
    for (int i=0; i < tiledata.layers_size(); ++i)
    {
        mapnik::vector::tile_layer const& layer = tiledata.layers(i);
        Local<Object> layer_obj = Object::New();
        layer_obj->Set(String::NewSymbol("name"), String::New(layer.name().c_str()));
        layer_obj->Set(String::NewSymbol("extent"), Integer::New(layer.extent()));
        layer_obj->Set(String::NewSymbol("version"), Integer::New(layer.version()));

        Local<Array> f_arr = Array::New(layer.features_size());
        for (int j=0; j < layer.features_size(); ++j)
        {
            Local<Object> feature_obj = Object::New();
            mapnik::vector::tile_feature const& f = layer.features(j);
            feature_obj->Set(String::NewSymbol("id"),Number::New(f.id()));
            feature_obj->Set(String::NewSymbol("type"),Integer::New(f.type()));
            Local<Array> g_arr = Array::New();
            for (int k = 0; k < f.geometry_size();++k)
            {
                g_arr->Set(k,Number::New(f.geometry(k)));
            }
            feature_obj->Set(String::NewSymbol("geometry"),g_arr);
            Local<Object> att_obj = Object::New();
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
                        att_obj->Set(String::NewSymbol(name.c_str()), String::New(value.string_value().c_str()));
                    }
                    else if (value.has_int_value())
                    {
                        att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.int_value()));
                    }
                    else if (value.has_double_value())
                    {
                        att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.double_value()));
                    }
                    else if (value.has_float_value())
                    {
                        att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.float_value()));
                    }
                    else if (value.has_bool_value())
                    {
                        att_obj->Set(String::NewSymbol(name.c_str()), Boolean::New(value.bool_value()));
                    }
                    else if (value.has_sint_value())
                    {
                        att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.sint_value()));
                    }
                    else if (value.has_uint_value())
                    {
                        att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.uint_value()));
                    }
                    else
                    {
                        att_obj->Set(String::NewSymbol(name.c_str()), Undefined());
                    }
                }
                feature_obj->Set(String::NewSymbol("properties"),att_obj);
            }

            f_arr->Set(j,feature_obj);
        }
        layer_obj->Set(String::NewSymbol("features"), f_arr);
        arr->Set(i, layer_obj);
    }
    return scope.Close(arr);
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
        Local<Object> feature_obj = Object::New();
        feature_obj->Set(String::NewSymbol("type"),String::New("Feature"));
        Local<Object> geometry = Object::New();
        mapnik::vector::tile_feature const& f = layer.features(j);
        unsigned int g_type = f.type();
        Local<String> js_type = String::New("Unknown");
        switch (g_type)
        {
        case MAPNIK_POINT:
        {
            js_type = String::New("Point");
            break;
        }
        case MAPNIK_LINESTRING:
        {
            js_type = String::New("LineString");
            break;
        }
        case MAPNIK_POLYGON:
        {
            js_type = String::New("Polygon");
            break;
        }
        default:
        {
            break;
        }
        }
        geometry->Set(String::NewSymbol("type"),js_type);
        Local<Array> g_arr = Array::New();
        if (g_type == MAPNIK_POLYGON)
        {
            Local<Array> enclosing_array = Array::New(1);
            enclosing_array->Set(0,g_arr);
            geometry->Set(String::NewSymbol("coordinates"),enclosing_array);
        }
        else
        {
            geometry->Set(String::NewSymbol("coordinates"),g_arr);
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
                            g_arr->Set(0,Number::New(x2));
                            g_arr->Set(1,Number::New(y2));
                        }
                        else
                        {
                            Local<Array> v_arr = Array::New(2);
                            v_arr->Set(0,Number::New(x2));
                            v_arr->Set(1,Number::New(y2));
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
        feature_obj->Set(String::NewSymbol("geometry"),geometry);
        Local<Object> att_obj = Object::New();
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
                    att_obj->Set(String::NewSymbol(name.c_str()), String::New(value.string_value().c_str()));
                }
                else if (value.has_int_value())
                {
                    att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.int_value()));
                }
                else if (value.has_double_value())
                {
                    att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.double_value()));
                }
                else if (value.has_float_value())
                {
                    att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.float_value()));
                }
                else if (value.has_bool_value())
                {
                    att_obj->Set(String::NewSymbol(name.c_str()), Boolean::New(value.bool_value()));
                }
                else if (value.has_sint_value())
                {
                    att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.sint_value()));
                }
                else if (value.has_uint_value())
                {
                    att_obj->Set(String::NewSymbol(name.c_str()), Number::New(value.uint_value()));
                }
                else
                {
                    att_obj->Set(String::NewSymbol(name.c_str()), Undefined());
                }
            }
        }
        feature_obj->Set(String::NewSymbol("properties"),att_obj);
        f_arr->Set(j+idx0,feature_obj);
    }
}

Handle<Value> VectorTile::toGeoJSON(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() < 1)
        return ThrowException(Exception::Error(
                                  String::New("first argument must be either a layer name (string) or layer index (integer)")));
    Local<Value> layer_id = args[0];
    if (! (layer_id->IsString() || layer_id->IsNumber()) )
        return ThrowException(Exception::TypeError(
                                  String::New("'layer' argument must be either a layer name (string) or layer index (integer)")));

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
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
                return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
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
            return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
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
            return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
        }
    } else {
        return ThrowException(Exception::TypeError(String::New("layer id must be a string or index number")));
    }

    try
    {
        if (all_array)
        {
            Local<Array> layer_arr = Array::New(layer_num);
            for (unsigned i=0;i<layer_num;++i)
            {
                Local<Object> layer_obj = Object::New();
                layer_obj->Set(String::NewSymbol("type"), String::New("FeatureCollection"));
                Local<Array> f_arr = Array::New();
                layer_obj->Set(String::NewSymbol("features"), f_arr);
                mapnik::vector::tile_layer const& layer = tiledata.layers(i);
                layer_obj->Set(String::NewSymbol("name"), String::New(layer.name().c_str()));
                layer_to_geojson(layer,f_arr,d->x_,d->y_,d->z_,d->width_,0);
                layer_arr->Set(i,layer_obj);
            }
            return scope.Close(layer_arr);
        }
        else
        {
            Local<Object> layer_obj = Object::New();
            layer_obj->Set(String::NewSymbol("type"), String::New("FeatureCollection"));
            Local<Array> f_arr = Array::New();
            layer_obj->Set(String::NewSymbol("features"), f_arr);
            if (all_flattened)
            {
                for (unsigned i=0;i<layer_num;++i)
                {
                    mapnik::vector::tile_layer const& layer = tiledata.layers(i);
                    layer_to_geojson(layer,f_arr,d->x_,d->y_,d->z_,d->width_,f_arr->Length());
                }
                return scope.Close(layer_obj);
            }
            else
            {
                mapnik::vector::tile_layer const& layer = tiledata.layers(layer_idx);
                layer_obj->Set(String::NewSymbol("name"), String::New(layer.name().c_str()));
                layer_to_geojson(layer,f_arr,d->x_,d->y_,d->z_,d->width_,0);
                return scope.Close(layer_obj);
            }
        }
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

Handle<Value> VectorTile::parseSync(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    try
    {
        d->parse_proto();
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    return Undefined();
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} vector_tile_parse_baton_t;

Handle<Value> VectorTile::parse(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() == 0) {
        return parseSync(args);
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    vector_tile_parse_baton_t *closure = new vector_tile_parse_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Parse, (uv_after_work_cb)EIO_AfterParse);
    d->Ref();
    return Undefined();
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
    HandleScope scope;
    vector_tile_parse_baton_t *closure = static_cast<vector_tile_parse_baton_t *>(req->data);
    TryCatch try_catch;
    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        Local<Value> argv[1] = { Local<Value>::New(Null()) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }
    closure->d->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> VectorTile::fromGeoJSON(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = ObjectWrap::Unwrap<VectorTile>(args.This());
    if (args.Length() < 1 || !args[0]->IsString())
        return ThrowException(Exception::Error(
                                  String::New("first argument must be a GeoJSON string")));
    if (args.Length() < 2 || !args[1]->IsString())
        return ThrowException(Exception::Error(
                                  String::New("second argument must be a layer name (string)")));
    std::string geojson_string = TOSTR(args[0]);
    std::string geojson_name = TOSTR(args[1]);
    try
    {
        typedef mapnik::vector::backend_pbf backend_type;
        typedef mapnik::vector::processor<backend_type> renderer_type;
        backend_type backend(d->get_tile_nonconst(),16);
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
                          m_req);
        ren.apply();
        d->painted(ren.painted());
        return True();
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

Handle<Value> VectorTile::addData(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    if (args.Length() < 1 || !args[0]->IsObject())
        return ThrowException(Exception::Error(
                                  String::New("first argument must be a buffer object")));
    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
        return ThrowException(Exception::Error(
                                  String::New("first arg must be a buffer object")));
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        return ThrowException(Exception::Error(
                                  String::New("cannot accept empty buffer as protobuf")));
    }
    d->buffer_.append(node::Buffer::Data(obj),buffer_size);
    d->status_ = VectorTile::LAZY_MERGE;
    return Undefined();
}

Handle<Value> VectorTile::setDataSync(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    if (args.Length() < 1 || !args[0]->IsObject())
        return ThrowException(Exception::Error(
                                  String::New("first argument must be a buffer object")));
    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
        return ThrowException(Exception::Error(
                                  String::New("first arg must be a buffer object")));
    std::size_t buffer_size = node::Buffer::Length(obj);
    if (buffer_size <= 0)
    {
        return ThrowException(Exception::Error(
                                  String::New("cannot accept empty buffer as protobuf")));
    }
    d->buffer_ = std::string(node::Buffer::Data(obj),buffer_size);
    d->status_ = VectorTile::LAZY_SET;
    return Undefined();
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

Handle<Value> VectorTile::setData(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() == 1) {
        return setDataSync(args);
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    if (args.Length() < 1 || !args[0]->IsObject())
        return ThrowException(Exception::Error(
                                  String::New("first argument must be a buffer object")));
    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
        return ThrowException(Exception::Error(
                                  String::New("first arg must be a buffer object")));

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());

    vector_tile_setdata_baton_t *closure = new vector_tile_setdata_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->data = node::Buffer::Data(obj);
    closure->dataLength = node::Buffer::Length(obj);
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_SetData, (uv_after_work_cb)EIO_AfterSetData);
    d->Ref();
    return Undefined();
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
    HandleScope scope;

    vector_tile_setdata_baton_t *closure = static_cast<vector_tile_setdata_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        Local<Value> argv[1] = { Local<Value>::New(Null()) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }
    closure->d->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> VectorTile::getData(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    try {
        // shortcut: return raw data and avoid trip through proto object
        // TODO  - safe for null string?
        int raw_size = d->buffer_.size();
        if (d->byte_size_ <= raw_size) {
            return scope.Close(node::Buffer::New((char*)d->buffer_.data(),raw_size)->handle_);
        } else {
            // NOTE: tiledata.ByteSize() must be called
            // after each modification of tiledata otherwise the
            // SerializeWithCachedSizesToArray will throw
            mapnik::vector::tile const& tiledata = d->get_tile();
            node::Buffer *retbuf = node::Buffer::New(d->byte_size_);
            // TODO - consider wrapping in fastbuffer: https://gist.github.com/drewish/2732711
            // http://www.samcday.com.au/blog/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
            google::protobuf::uint8* start = reinterpret_cast<google::protobuf::uint8*>(node::Buffer::Data(retbuf));
            google::protobuf::uint8* end = tiledata.SerializeWithCachedSizesToArray(start);
            if (end - start != d->byte_size_) {
                return ThrowException(Exception::Error(
                                          String::New("serialization failed, possible race condition")));
            }
            return scope.Close(retbuf->handle_);
        }
    } catch (std::exception const& ex) {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    return Undefined();
}

struct vector_tile_render_baton_t {
    uv_work_t request;
    Map* m;
    VectorTile* d;
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
        use_cairo(true) {}
};

Handle<Value> VectorTile::render(const Arguments& args)
{
    HandleScope scope;

    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    if (args.Length() < 1 || !args[0]->IsObject()) {
        return ThrowException(Exception::TypeError(String::New("mapnik.Map expected as first arg")));
    }
    Local<Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !Map::constructor->HasInstance(obj))
        return ThrowException(Exception::TypeError(String::New("mapnik.Map expected as first arg")));

    Map *m = node::ObjectWrap::Unwrap<Map>(obj);
    if (args.Length() < 2 || !args[1]->IsObject()) {
        return ThrowException(Exception::TypeError(String::New("a renderable mapnik object is expected as second arg")));
    }
    Local<Object> im_obj = args[1]->ToObject();
    if (im_obj->IsNull() || im_obj->IsUndefined())
        return ThrowException(Exception::TypeError(String::New("a renderable mapnik object is expected as second arg")));

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
    {
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));
    }

    vector_tile_render_baton_t *closure = new vector_tile_render_baton_t();
    Local<Object> options = Object::New();

    if (args.Length() > 2)
    {
        if (!args[2]->IsObject())
        {
            delete closure;
            return ThrowException(Exception::TypeError(String::New("optional third argument must be an options object")));
        }
        options = args[2]->ToObject();
        if (options->Has(String::NewSymbol("z")))
        {
            closure->zxy_override = true;
            closure->z = options->Get(String::New("z"))->IntegerValue();
        }
        if (options->Has(String::NewSymbol("x")))
        {
            closure->zxy_override = true;
            closure->x = options->Get(String::New("x"))->IntegerValue();
        }
        if (options->Has(String::NewSymbol("y")))
        {
            closure->zxy_override = true;
            closure->y = options->Get(String::New("y"))->IntegerValue();
        }
        if (options->Has(String::New("buffer_size"))) {
            Local<Value> bind_opt = options->Get(String::New("buffer_size"));
            if (!bind_opt->IsNumber()) {
                delete closure;
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'buffer_size' must be a number")));
            }
            closure->buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(String::New("scale"))) {
            Local<Value> bind_opt = options->Get(String::New("scale"));
            if (!bind_opt->IsNumber())
            {
                delete closure;
                return ThrowException(Exception::TypeError(
                                        String::New("optional arg 'scale' must be a number")));
            }
            closure->scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(String::NewSymbol("scale_denominator")))
        {
            Local<Value> bind_opt = options->Get(String::New("scale_denominator"));
            if (!bind_opt->IsNumber())
            {
                delete closure;
                return ThrowException(Exception::TypeError(
                                        String::New("optional arg 'scale_denominator' must be a number")));
            }
            closure->scale_denominator = bind_opt->NumberValue();
        }
    }

    closure->layer_idx = 0;
    if (Image::constructor->HasInstance(im_obj))
    {
        Image *im = node::ObjectWrap::Unwrap<Image>(im_obj);
        closure->im = im;
        closure->width = im->get()->width();
        closure->height = im->get()->height();
        closure->im->_ref();
    }
    else if (CairoSurface::constructor->HasInstance(im_obj))
    {
        CairoSurface *c = node::ObjectWrap::Unwrap<CairoSurface>(im_obj);
        closure->c = c;
        closure->width = c->width();
        closure->height = c->height();
        closure->c->_ref();
        if (options->Has(String::New("renderer")))
        {
            Local<Value> renderer = options->Get(String::New("renderer"));
            if (!renderer->IsString() )
            {
                delete closure;
                return ThrowException(String::New("'renderer' option must be a string of either 'svg' or 'cairo'"));
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
                return ThrowException(String::New("'renderer' option must be a string of either 'svg' or 'cairo'"));
            }
        }
    }
    else if (Grid::constructor->HasInstance(im_obj))
    {
        Grid *g = node::ObjectWrap::Unwrap<Grid>(im_obj);
        closure->g = g;
        closure->width = g->get()->width();
        closure->height = g->get()->height();
        closure->g->_ref();

        std::size_t layer_idx = 0;

        // grid requires special options for now
        if (!options->Has(String::New("layer")))
        {
            delete closure;
            return ThrowException(Exception::TypeError(
                                      String::New("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)")));
        } else {
            std::vector<mapnik::layer> const& layers = m->get()->layers();

            Local<Value> layer_id = options->Get(String::New("layer"));
            if (! (layer_id->IsString() || layer_id->IsNumber()) )
            {
                delete closure;
                return ThrowException(Exception::TypeError(
                                          String::New("'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)")));
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
                    return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
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
                    return ThrowException(Exception::TypeError(String::New(s.str().c_str())));
                }
            } else {
                delete closure;
                return ThrowException(Exception::TypeError(String::New("layer id must be a string or index number")));
            }
        }
        if (options->Has(String::New("fields"))) {

            Local<Value> param_val = options->Get(String::New("fields"));
            if (!param_val->IsArray())
            {
                delete closure;
                return ThrowException(Exception::TypeError(
                                          String::New("option 'fields' must be an array of strings")));
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
        return ThrowException(Exception::TypeError(String::New("renderable mapnik object expected as second arg")));
    }
    closure->request.data = closure;
    closure->d = d;
    closure->m = m;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderTile, (uv_after_work_cb)EIO_AfterRenderTile);
    m->_ref();
    d->Ref();
    return Undefined();
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
                mapnik::cairo_renderer<mapnik::cairo_ptr> ren(map_in,m_req,c_context,closure->scale_factor);
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
                svg_ren ren(map_in, m_req, output_stream_iterator, closure->scale_factor);
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
            mapnik::agg_renderer<mapnik::image_32> ren(map_in,m_req,*closure->im->get(),closure->scale_factor);
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
    HandleScope scope;

    vector_tile_render_baton_t *closure = static_cast<vector_tile_render_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        if (closure->im)
        {
            Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->im->handle_) };
            closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        }
        else if (closure->g)
        {
            Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->g->handle_) };
            closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        }
        else if (closure->c)
        {
            Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->c->handle_) };
            closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        }
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->m->_unref();
    if (closure->im) closure->im->_unref();
    if (closure->g) closure->g->_unref();
    closure->d->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> VectorTile::clearSync(const Arguments& args)
{
    HandleScope scope;
#if MAPNIK_VERSION >= 200200
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    d->clear();
#endif
    return Undefined();
}

typedef struct {
    uv_work_t request;
    VectorTile* d;
    std::string format;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} clear_vector_tile_baton_t;

Handle<Value> VectorTile::clear(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());

    if (args.Length() == 0) {
        return clearSync(args);
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));
    clear_vector_tile_baton_t *closure = new clear_vector_tile_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Clear, (uv_after_work_cb)EIO_AfterClear);
    d->Ref();
    return Undefined();
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
    HandleScope scope;
    clear_vector_tile_baton_t *closure = static_cast<clear_vector_tile_baton_t *>(req->data);
    TryCatch try_catch;
    if (closure->error)
    {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        Local<Value> argv[2] = { Local<Value>::New(Null()) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    if (try_catch.HasCaught())
    {
        node::FatalException(try_catch);
    }
    closure->d->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> VectorTile::isSolidSync(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());
    try
    {
        std::string key;
        bool is_solid = mapnik::vector::is_solid_extent(d->get_tile(),key);
        if (is_solid)
        {
            return scope.Close(String::New(key.c_str()));
        }
        else
        {
            return scope.Close(False());
        }
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::TypeError(
                                  String::New(ex.what())));
    }
    return scope.Close(Undefined());
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

Handle<Value> VectorTile::isSolid(const Arguments& args)
{
    HandleScope scope;
    VectorTile* d = node::ObjectWrap::Unwrap<VectorTile>(args.This());

    if (args.Length() == 0) {
        return isSolidSync(args);
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    is_solid_vector_tile_baton_t *closure = new is_solid_vector_tile_baton_t();
    closure->request.data = closure;
    closure->d = d;
    closure->result = true;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    d->Ref();
    return Undefined();
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
    HandleScope scope;
    is_solid_vector_tile_baton_t *closure = static_cast<is_solid_vector_tile_baton_t *>(req->data);
    TryCatch try_catch;
    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        Local<Value> argv[3] = { Local<Value>::New(Null()),
                                 Local<Value>::New(Boolean::New(closure->result)),
                                 Local<Value>::New(String::New(closure->key.c_str()))
        };
        closure->cb->Call(Context::GetCurrent()->Global(), 3, argv);
    }
    if (try_catch.HasCaught())
    {
        node::FatalException(try_catch);
    }
    closure->d->Unref();
    closure->cb.Dispose();
    delete closure;
}
