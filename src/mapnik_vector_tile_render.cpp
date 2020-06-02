#include "mapnik_vector_tile.hpp"
#include "mapnik_image.hpp"
#include "mapnik_cairo_surface.hpp"
#include "mapnik_grid.hpp"
#include "mapnik_map.hpp"
// mapnik
#include <mapnik/request.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/attribute.hpp>         // for attributes
#include <mapnik/image.hpp>             // for image_rgba8
#include <mapnik/image_any.hpp>

#include "object_to_container.hpp"

/*
struct vector_tile_render_baton_t
{
    uv_work_t request;
    Map* m;
    VectorTile * d;
    surface_type surface;
    mapnik::attributes variables;
    std::string error_name;
    Napi::FunctionReference cb;
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
*/
/*
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
*/


namespace {
struct dummy_surface {};

using surface_type = mapnik::util::variant
    <dummy_surface,
     Image *,
     CairoSurface *
#if defined(GRID_RENDERER)
     ,Grid *
#endif
     >;

/*
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
*/
/*
template <typename Renderer>
void process_layers(Renderer & ren,
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
*/
}

/*
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    vector_tile_render_baton_t *closure = static_cast<vector_tile_render_baton_t *>(req->data);
    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        if (closure->surface.is<Image *>())
        {
            Napi::Value argv[2] = { env.Undefined(), mapnik::util::get<Image *>(closure->surface)->handle() };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
#if defined(GRID_RENDERER)
        else if (closure->surface.is<Grid *>())
        {
            Napi::Value argv[2] = { env.Undefined(), mapnik::util::get<Grid *>(closure->surface)->handle() };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
#endif
        else if (closure->surface.is<CairoSurface *>())
        {
            Napi::Value argv[2] = { env.Undefined(), mapnik::util::get<CairoSurface *>(closure->surface)->handle() };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }

    mapnik::util::apply_visitor(deref_visitor(), closure->surface);
    closure->m->Unref();
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}
*/

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

Napi::Value VectorTile::render(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "mapnik.Map expected as first arg").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.InstanceOf(Map::constructor.Value()))
    {
        Napi::TypeError::New(env, "mapnik.Map expected as first arg").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Map *m = Napi::ObjectWrap<Map>::Unwrap(obj);
    if (info.Length() < 2 || !info[1].IsObject())
    {
        Napi::TypeError::New(env, "a renderable mapnik object is expected as second arg").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Object im_obj = info[1].As<Napi::Object>();
    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    if (!info[info.Length()-1].IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object options = Napi::Object::New(env);
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    bool zxy_override = false;
    int buffer_size = 0;
    double scale_factor = 1.0;
    double scale_denominator = 1.0;
    mapnik::attributes variables;

    if (info.Length() > 2)
    {
        bool set_x = false;
        bool set_y = false;
        bool set_z = false;
        if (!info[2].IsObject())
        {
            Napi::TypeError::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        options = info[2].As<Napi::Object>();
        if (options.Has("z"))
        {
            Napi::Value bind_opt = options.Get("z");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'z' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            z = bind_opt.As<Napi::Number>().Int32Value();
            set_z = true;
            zxy_override = true;
        }
        if (options.Has("x"))
        {
            Napi::Value bind_opt = options.Get("x");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'x' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            x = bind_opt.As<Napi::Number>().Int32Value();
            set_x = true;
            zxy_override = true;
        }
        if (options.Has("y"))
        {
            Napi::Value bind_opt = options.Get("y");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'y' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            y = bind_opt.As<Napi::Number>().Int32Value();
            set_y = true;
            zxy_override = true;
        }

        if (zxy_override)
        {
            if (!set_z || !set_x || !set_y)
            {
                Napi::TypeError::New(env, "original args 'z', 'x', and 'y' must all be used together").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            if (x < 0 || y < 0 || z < 0)
            {
                Napi::TypeError::New(env, "original args 'z', 'x', and 'y' can not be negative").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::int64_t max_at_zoom = pow(2, z);
            if (x >= max_at_zoom)
            {
                Napi::TypeError::New(env, "required parameter x is out of range of possible values based on z value").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            if (y >= max_at_zoom)
            {
                Napi::TypeError::New(env, "required parameter y is out of range of possible values based on z value").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }

        if (options.Has("buffer_size"))
        {
            Napi::Value bind_opt = options.Get("buffer_size");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            buffer_size = bind_opt.As<Napi::Number>().Int32Value();
        }
        if (options.Has("scale"))
        {
            Napi::Value bind_opt = options.Get("scale");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_factor = bind_opt.As<Napi::Number>().DoubleValue();
        }
        if (options.Has("scale_denominator"))
        {
            Napi::Value bind_opt = options.Get("scale_denominator");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_denominator = bind_opt.As<Napi::Number>().DoubleValue();
        }
        if (options.Has("variables"))
        {
            Napi::Value bind_opt = options.Get("variables");
            if (!bind_opt.IsObject())
            {
                Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            object_to_container(variables,bind_opt.As<Napi::Object>());
        }
    }

    unsigned layer_idx = 0;
    unsigned width = 0;
    unsigned height = 0;
    surface_type surface;
    bool use_cairo = false;
    if (im_obj.InstanceOf(Image::constructor.Value()))
    {
        Image* im = Napi::ObjectWrap<Image>::Unwrap(im_obj);
        width = im->impl()->width();
        height = im->impl()->height();
        surface = im;
    }
    else if (im_obj.InstanceOf(CairoSurface::constructor.Value()))
    {
        CairoSurface *c = Napi::ObjectWrap<CairoSurface>::Unwrap(im_obj);
        width = c->width_;
        height = c->height_;
        surface = c;
        if (options.Has("renderer"))
        {
            Napi::Value renderer = options.Get("renderer");
            if (!renderer.IsString())
            {
                Napi::Error::New(env, "'renderer' option must be a string of either 'svg' or 'cairo'").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            std::string renderer_name = renderer.As<Napi::String>();
            if (renderer_name == "cairo")
            {
                use_cairo = true;
            }
            else if (renderer_name == "svg")
            {
                use_cairo = false;
            }
            else
            {
                Napi::Error::New(env, "'renderer' option must be a string of either 'svg' or 'cairo'").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
    }
#if defined(GRID_RENDERER)
    else if (im_obj.InstanceOf(Grid::constructor.Value()))
    {
        Grid *g = Napi::ObjectWrap<Grid>::Unwrap(im_obj);
        width = g->grid_->width();
        height = g->grid_->height();
        surface = g;

        std::size_t layer_idx = 0;

        // grid requires special options for now
        if (!options.Has("layer"))
        {
            Napi::TypeError::New(env, "'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        else
        {
            std::vector<mapnik::layer> const& layers = m->map_->layers();
            Napi::Value layer_id = options.Get("layer");
            if (layer_id.IsString())
            {
                bool found = false;
                unsigned int idx(0);
                std::string layer_name = layer_id.As<Napi::String>();
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
                    Napi::TypeError::New(env, s.str()).ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
            else if (layer_id.IsNumber())
            {
                layer_idx = layer_id.As<Napi::Number>().Int32Value();
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
                    Napi::TypeError::New(env, s.str()).ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
            else
            {
                Napi::TypeError::New(env, "'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("fields"))
        {
            Napi::Value param_val = options.Get("fields");
            if (!param_val.IsArray())
            {
                Napi::TypeError::New(env, "option 'fields' must be an array of strings").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Array a = param_val.As<Napi::Array>();
            std::size_t i = 0;
            std::size_t num_fields = a.Length();
            while (i < num_fields)
            {
                Napi::Value name = a.Get(i++);
                if (name.IsString())
                {
                    g->impl()->add_field(name.As<Napi::String>());
                }
            }
        }
    }
#endif
    else
    {
        Napi::TypeError::New(env, "renderable mapnik object expected as second arg").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    //closure->request.data = closure;
    //closure->d = d;
    //closure->m = m;
    //closure->error = false;
    //closure->cb.Reset(callback.As<Napi::Function>());
    //uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderTile, (uv_after_work_cb)EIO_AfterRenderTile);
    //mapnik::util::apply_visitor(ref_visitor(), surface);
    //m->Ref();
    //d->Ref();
    //guard.release();
    return env.Undefined();

}
