#include "utils.hpp"
#include "mapnik_map.hpp"
#include "mapnik_image.hpp"
#include "mapnik_vector_tile.hpp"
#if defined(GRID_RENDERER)
#include "mapnik_grid.hpp"
#endif
#include "mapnik_feature.hpp"
#include "mapnik_cairo_surface.hpp"
#ifdef SVG_RENDERER
#include <mapnik/svg/output/svg_renderer.hpp>
#endif

#include "vector_tile_projection.hpp"
#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_processor.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_load_tile.hpp"
#include "object_to_container.hpp"

// mapnik
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
#include <mapnik/datasource_cache.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/request.hpp>
#include <mapnik/scale_denominator.hpp>
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
#include <sstream>                      // for operator<<, basic_ostream, etc
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

struct dummy_surface {};

using surface_type = mapnik::util::variant
    <dummy_surface,
     Image *,
     CairoSurface *
#if defined(GRID_RENDERER)
     ,Grid *
#endif
     >;

struct deref_visitor
{
    void operator() (dummy_surface) {} // no-op
    template <typename SurfaceType>
    void operator() (SurfaceType * surface)
    {
        if (surface != nullptr)
        {
            surface->_unref();
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

    ~vector_tile_render_baton_t()
    {
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
    v8::Local<v8::Object> obj = info[0]->ToObject();
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
        bool set_x = false;
        bool set_y = false;
        bool set_z = false;
        if (!info[2]->IsObject())
        {
            Nan::ThrowTypeError("optional third argument must be an options object");
            return;
        }
        options = info[2]->ToObject();
        if (options->Has(Nan::New("z").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("z").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'z' must be a number");
                return;
            }
            closure->z = bind_opt->IntegerValue();
            set_z = true;
            closure->zxy_override = true;
        }
        if (options->Has(Nan::New("x").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("x").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'x' must be a number");
                return;
            }
            closure->x = bind_opt->IntegerValue();
            set_x = true;
            closure->zxy_override = true;
        }
        if (options->Has(Nan::New("y").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("y").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'y' must be a number");
                return;
            }
            closure->y = bind_opt->IntegerValue();
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

        if (options->Has(Nan::New("buffer_size").ToLocalChecked()))
        {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("buffer_size").ToLocalChecked());
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("optional arg 'buffer_size' must be a number");
                return;
            }
            closure->buffer_size = bind_opt->IntegerValue();
        }
        if (options->Has(Nan::New("scale").ToLocalChecked()))
        {
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
        }
        else
        {
            std::vector<mapnik::layer> const& layers = m->get()->layers();
            v8::Local<v8::Value> layer_id = options->Get(Nan::New("layer").ToLocalChecked());
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
                layer_idx = layer_id->IntegerValue();
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
        if (options->Has(Nan::New("fields").ToLocalChecked()))
        {
            v8::Local<v8::Value> param_val = options->Get(Nan::New("fields").ToLocalChecked());
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
                v8::Local<v8::Value> name = a->Get(i);
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
        mapnik::vector_tile_impl::spherical_mercator merc(closure->d->tile_size());
        double minx,miny,maxx,maxy;
        if (closure->zxy_override)
        {
            merc.xyz(closure->x,closure->y,closure->z,minx,miny,maxx,maxy);
        }
        else
        {
            merc.xyz(closure->d->get_tile()->x(),
                     closure->d->get_tile()->y(),
                     closure->d->get_tile()->z(),
                     minx,miny,maxx,maxy);
        }
        mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
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
    vector_tile_render_baton_t *closure = static_cast<vector_tile_render_baton_t *>(req->data);
    if (closure->error)
    {
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

