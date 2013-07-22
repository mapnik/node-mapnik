
// node
#include <node_object_wrap.h>


// mapnik
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/version.hpp>
#include <mapnik/graphics.hpp>
#include <mapnik/request.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
//#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
//#include <mapnik/grid/grid_renderer.hpp>  // for grid_renderer
#include <mapnik/box2d.hpp>
#include <mapnik/scale_denominator.hpp>
#include <mapnik/image_compositing.hpp>


// stl
#include <vector>

#include "mapnik_map.hpp"
#include "mapnik_vector_tile.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_datasource.hpp"
//#include "vector_tile_util.hpp"
#include "vector_tile.pb.h"
#include "mapnik_image.hpp"
#include "utils.hpp"
#include "render.hpp"

#include <boost/make_shared.hpp>

namespace node_mapnik {
/*

var tile = new mapnik.Tile(z,x,y);
tile.render();

var opts = { scale_factor:2, buffer_size:256 };
var sources = [
  vtile,
  {'name':image},
  vtile
];

mapnik.render(z,x,y,map,sources,image,opts,function(err,image) {
    ren = agg_renderer;
    for layer_name in map:
        source = sources.find(layer_name);
        if (vector):
          ren.apply_to_layer(layer,map.styles())
        else if (raster)
          ren.apply_to_image()
});

*/

struct render_baton_t {
    uv_work_t request;
    int z;
    int x;
    int y;
    bool error;
    std::string result;
    Map* m;
    VectorTile* d;
    Image * im;
    std::vector<VectorTile*> vtiles;
    std::map<std::string,Image*> images;
    //CairoSurface * c;
    //Grid * g;
    std::size_t layer_idx;
    double scale_factor;
    int buffer_size;
    double scale_denominator;
    Persistent<Function> cb;
    bool use_cairo;
    unsigned tile_size;
    render_baton_t() :
        request(),
        z(0),
        x(0),
        y(0),
        error(false),
        result(),
        m(NULL),
        d(NULL),
        im(NULL),
        //c(NULL),
        //g(NULL),
        layer_idx(0),
        scale_factor(1.0),
        buffer_size(0),
        scale_denominator(0.0),
        use_cairo(true),
        tile_size(256) {}
};

void AsyncRender(uv_work_t* req)
{
    render_baton_t *closure = static_cast<render_baton_t *>(req->data);

    try {
        mapnik::Map const& map_in = *closure->m->get();
        mapnik::vector::spherical_mercator merc(closure->tile_size);
        double minx,miny,maxx,maxy;
        merc.xyz(closure->x,closure->y,closure->z,minx,miny,maxx,maxy);
        mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
        mapnik::request m_req(closure->tile_size,closure->tile_size,map_extent);
        m_req.set_buffer_size(closure->buffer_size);
        mapnik::projection map_proj(map_in.srs(),true);
        double scale_denom = closure->scale_denominator;
        if (scale_denom <= 0.0)
        {
            scale_denom = mapnik::scale_denominator(m_req.scale(),map_proj.is_geographic());
        }
        scale_denom *= closure->scale_factor;
        std::vector<mapnik::layer> const& layers = map_in.layers();
        mapnik::image_32 & image_buffer = *closure->im->get();
        mapnik::agg_renderer<mapnik::image_32> ren(map_in,m_req,image_buffer,closure->scale_factor);
        ren.start_map_processing(map_in);
        unsigned layers_size = layers.size();
        for (unsigned i=0; i < layers_size; ++i)
        {
            mapnik::layer const& lyr = layers[i];
            if (lyr.visible(scale_denom))
            {
                unsigned layer_hit = 0;
                // search for matching name in images
                typedef std::map<std::string, Image *>::const_iterator iterator_type;
                iterator_type itr =  closure->images.find(lyr.name());
                if (itr != closure->images.end())
                {
                    // TODO - comp_op, filters, opacity from style
                    ++layer_hit;
                    mapnik::composite(image_buffer.data(),itr->second->get()->data(), mapnik::src_over, 1.0f, 0, 0, false);
                }
                else
                {
                    for (unsigned v=0; v < closure->vtiles.size(); ++v)
                    {
                        int tile_layer_idx = -1;
                        VectorTile * vtile = closure->vtiles[v];
                        mapnik::vector::tile const& tiledata = vtile->get_tile();
                        for (int t=0; t < tiledata.layers_size(); ++t)
                        {
                            mapnik::vector::tile_layer const& layer = tiledata.layers(t);
                            if (lyr.name() == layer.name())
                            {
                                tile_layer_idx = t;
                                break;
                            }
                        }
                        if (tile_layer_idx > -1)
                        {
                            ++layer_hit;
                            mapnik::vector::tile_layer const& layer = tiledata.layers(tile_layer_idx);
                            mapnik::layer lyr_copy(lyr);
                            boost::shared_ptr<mapnik::vector::tile_datasource> ds = boost::make_shared<
                                                            mapnik::vector::tile_datasource>(
                                                                layer,
                                                                vtile->x_,
                                                                vtile->y_,
                                                                vtile->z_,
                                                                closure->tile_size
                                                                );
                            // TODO - really should be respecting/using buffered layer extent
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
                if (layer_hit <= 0)
                {
                    std::clog << "notice: no sources matched layer " << lyr.name() << "\n";
                }
                if (layer_hit > 1)
                {
                    std::clog << "notice: multiple sources matched layer " << lyr.name() << "\n";
                }
            }
        }
        ren.end_map_processing(map_in);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->result = ex.what();
    }
}

void AfterRender(uv_work_t* req)
{
    HandleScope scope;

    render_baton_t *closure = static_cast<render_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->result.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        if (closure->im)
        {
            Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->im->handle_) };
            closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        }
        /*
        else if (closure->g)
        {
            Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->g->handle_) };
            closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        }
        else if (closure->c)
        {
            Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(closure->c->handle_) };
            closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        }*/
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->m->_unref();
    if (closure->im) closure->im->_unref();
    //if (closure->g) closure->g->_unref();
    //closure->d->Unref();
    closure->cb.Dispose();
    delete closure;
}

Handle<Value> render(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() < 7) {
        return ThrowException(String::New("at least 7 args expected: z, x, y, map, image, sources, [opts], callback"));
    }
    
    if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsNumber()) {
        return ThrowException(String::New("z,x,y args must be numbers"));
    }

    if (!args[3]->IsObject() || !args[4]->IsObject()) {
        return ThrowException(String::New("map and image args (4th and 5th) must be objects"));
    }

    Local<Object> map_obj = args[3]->ToObject();
    if (map_obj->IsNull() || map_obj->IsUndefined() || !Map::constructor->HasInstance(map_obj)) {
        return ThrowException(Exception::TypeError(String::New("mapnik.Map expected as fourth arg")));
    }

    Local<Object> image_obj = args[4]->ToObject();
    if (image_obj->IsNull() || image_obj->IsUndefined() || !Image::constructor->HasInstance(image_obj)) {
        return ThrowException(Exception::TypeError(String::New("mapnik.Image expected as fifth arg")));
    }

    Local<Value> sources_obj = args[5];
    if (!sources_obj->IsArray()) {
        return ThrowException(Exception::TypeError(String::New("sources (6th arg) must be an array")));
    }
    Local<Array> sources_array = Local<Array>::Cast(sources_obj);
    unsigned int sources_len = sources_array->Length();
    if (sources_len <= 0) {
        return ThrowException(Exception::TypeError(String::New("sources array is empty!")));
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
    {
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));
    }

    // TODO - RAII
    render_baton_t *closure = new render_baton_t();

    Local<Object> options = Object::New();
    if (args.Length() > 7)
    {
        if (!args[6]->IsObject())
        {
            delete closure;
            return ThrowException(Exception::TypeError(String::New("optional 7th argument must be an options object")));
        }
        options = args[6]->ToObject();
        if (options->Has(String::New("scale"))) {
            Local<Value> bind_opt = options->Get(String::New("scale"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale' must be a number")));
            closure->scale_factor = bind_opt->NumberValue();
        }
        if (options->Has(String::New("buffer_size"))) {
            Local<Value> bind_opt = options->Get(String::New("buffer_size"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'buffer_size' must be a number")));
            closure->buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(String::New("scale_denominator"))) {
            Local<Value> bind_opt = options->Get(String::New("scale_denominator"));
            if (!bind_opt->IsNumber())
                return ThrowException(Exception::TypeError(
                                          String::New("optional arg 'scale_denominator' must be a number")));
            closure->scale_denominator = bind_opt->NumberValue();
        }
    }
    
    unsigned int i = 0;
    while (i < sources_len) {
        Local<Value> source = sources_array->Get(i);
        if (!source->IsObject()) {
            delete closure;
            return ThrowException(Exception::TypeError(String::New("found source that is not an object")));
        }
        Local<Object> source_obj = source->ToObject();
        if (source_obj->IsNull() || source_obj->IsUndefined()) {
            delete closure;
            return ThrowException(Exception::TypeError(String::New("found source object that is not valid")));
        }
        if (VectorTile::constructor->HasInstance(source_obj)) {
            VectorTile * vtile = node::ObjectWrap::Unwrap<VectorTile>(source_obj);
            vtile->_ref();
            closure->vtiles.push_back(vtile);
        } else {
            if (!source_obj->Has(String::NewSymbol("name"))) {
                delete closure;
                return ThrowException(Exception::TypeError(String::New("found non vector tile source object that does not provide a name property")));
            }
            if (!source_obj->Has(String::NewSymbol("image"))) {
                delete closure;
                return ThrowException(Exception::TypeError(String::New("found non vector tile source object that does not provide a image property")));
            }
            std::string name = TOSTR(source_obj->Get(String::NewSymbol("name")));
            Local<Value> image = source_obj->Get(String::NewSymbol("image"));
            if (!image->IsObject()) {
                delete closure;
                return ThrowException(Exception::TypeError(String::New("found source image that is not an object")));
            }
            Local<Object> source_image_obj = image->ToObject();
            if (source_image_obj->IsNull() || source_image_obj->IsUndefined()) {
                delete closure;
                return ThrowException(Exception::TypeError(String::New("found source image that is not valid")));
            }
            if (Image::constructor->HasInstance(source_image_obj)) {
                Image * named_im = node::ObjectWrap::Unwrap<Image>(source_image_obj);
                named_im->_ref();
                closure->images.insert(std::make_pair(name,named_im));
            } else {
                delete closure;
                return ThrowException(Exception::TypeError(String::New("found unknown source object type")));
            }
        }
        ++i;
    }
    closure->request.data = closure;
    closure->z = args[0]->IntegerValue();
    closure->x = args[1]->IntegerValue();
    closure->y = args[2]->IntegerValue();
    closure->m = node::ObjectWrap::Unwrap<Map>(map_obj);
    closure->im = node::ObjectWrap::Unwrap<Image>(image_obj);
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, AsyncRender, (uv_after_work_cb)AfterRender);
    closure->m->_ref();
    closure->im->_ref();
    return Undefined();
}

}
