#include "node.h"                       // for NODE_SET_PROTOTYPE_METHOD, etc
#include <node_buffer.h>
#include <node_version.h>

#include "utils.hpp"
#include "mapnik_map.hpp"
#include "mapnik_image.hpp"
#include "mapnik_grid.hpp"
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

#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/version.hpp>
#include <mapnik/request.hpp>
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

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

#include <set>                          // for set, etc
#include <sstream>                      // for operator<<, basic_ostream, etc
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector


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
    NODE_SET_PROTOTYPE_METHOD(constructor, "names", names);
    NODE_SET_PROTOTYPE_METHOD(constructor, "toJSON", toJSON);
    NODE_SET_PROTOTYPE_METHOD(constructor, "toGeoJSON", toGeoJSON);
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
    tiledata_(),
    width_(w),
    height_(h),
    painted_(false) {}

VectorTile::~VectorTile() { }

Handle<Value> VectorTile::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args.Length() == 3)
    {
        if (!args[0]->IsNumber() ||
            !args[1]->IsNumber() ||
            !args[2]->IsNumber())
            return ThrowException(Exception::Error(
                                      String::New("required args (z, x, and y) must be a integers")));
        VectorTile* d = new VectorTile(args[0]->IntegerValue(),
                                   args[1]->IntegerValue(),
                                   args[2]->IntegerValue()
            );
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
    mapnik::vector::tile const& tiledata = d->get_tile();
    Local<Array> arr = Array::New(tiledata.layers_size());
    for (int i=0; i < tiledata.layers_size(); ++i)
    {
        mapnik::vector::tile_layer const& layer = tiledata.layers(i);
        arr->Set(i, String::New(layer.name().c_str()));
    }
    return scope.Close(arr);
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
        case mapnik::datasource::Point:
        {
            js_type = String::New("Point");
            break;
        }
        case mapnik::datasource::LineString:
        {
            js_type = String::New("LineString");
            break;
        }
        case mapnik::datasource::Polygon:
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
        if (g_type == mapnik::datasource::Polygon)
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
                        if (g_type == mapnik::datasource::Point)
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
                    throw std::runtime_error("Unknown command type");
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
    } catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
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
    mapnik::vector::tile & tiledata = d->get_tile_nonconst();
    unsigned proto_len = node::Buffer::Length(obj);
    if (proto_len == 0)
    {
        return ThrowException(Exception::Error(
                                  String::New("could not parse empty buffer as protobuf")));
    }
    if (tiledata.ParseFromArray(node::Buffer::Data(obj), proto_len))
    {
        d->painted(true);
    }
    else
    {
        return ThrowException(Exception::Error(
                                  String::New("could not parse buffer as protobuf")));
    }
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

    try {
        mapnik::vector::tile & tiledata = closure->d->get_tile_nonconst();
        if (tiledata.ParseFromArray(closure->data, closure->dataLength))
        {
            closure->d->painted(true);
        }
        else
        {
            closure->error = true;
            if (closure->dataLength == 0)
            {
                closure->error_name = "could not parse empty protobuf";
            }
            else
            {
                closure->error_name = "could not parse protobuf";
            }
        }
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
    mapnik::vector::tile const& tiledata = d->get_tile();
    // TODO - cache bytesize?
    int size = tiledata.ByteSize();
    #if NODE_VERSION_AT_LEAST(0, 11, 0)
    Local<Object> retbuf = node::Buffer::New(size);
    // TODO - consider wrapping in fastbuffer: https://gist.github.com/drewish/2732711
    // http://www.samcday.com.au/blog/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
    if (tiledata.SerializeToArray(node::Buffer::Data(retbuf),size))
    {
        return scope.Close(retbuf);
    }
    #else
    node::Buffer *retbuf = node::Buffer::New(size);
    // TODO - consider wrapping in fastbuffer: https://gist.github.com/drewish/2732711
    // http://www.samcday.com.au/blog/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
    if (tiledata.SerializeToArray(node::Buffer::Data(retbuf),size))
    {
        return scope.Close(retbuf->handle_);
    }
    #endif
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
    bool zxy_override;
    bool error;
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
        zxy_override(false),
        error(false),
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
        closure->im->_ref();
    }
    else if (CairoSurface::constructor->HasInstance(im_obj))
    {
        CairoSurface *c = node::ObjectWrap::Unwrap<CairoSurface>(im_obj);
        closure->c = c;
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
                        s << "only '" << layers.size() << "' layers exist in map";
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
                mapnik::layer lyr_copy(lyr);
                boost::shared_ptr<mapnik::vector::tile_datasource> ds = boost::make_shared<
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
        mapnik::request m_req(map_in.width(),map_in.height(),map_extent);
        m_req.set_buffer_size(map_in.buffer_size());
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
                    boost::shared_ptr<mapnik::vector::tile_datasource> ds = boost::make_shared<
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
    std::string key;
    bool is_solid = mapnik::vector::is_solid_extent(d->get_tile(),key);
    if (is_solid) return scope.Close(String::New(key.c_str()));
    else return scope.Close(False());
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
    closure->result = mapnik::vector::is_solid_extent(closure->d->get_tile(),closure->key);
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
