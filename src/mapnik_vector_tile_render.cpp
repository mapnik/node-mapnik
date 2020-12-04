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
#include <mapnik/attribute.hpp> // for attributes
#include <mapnik/image.hpp>     // for image_rgba8
#include <mapnik/image_any.hpp>
#include <mapnik/scale_denominator.hpp>
#include <mapnik/agg_renderer.hpp> // for agg_renderer
#if defined(HAVE_CAIRO)
#include <cairo.h>
#include <mapnik/cairo/cairo_renderer.hpp>
#if defined(CAIRO_HAS_SVG_SURFACE)
#include <cairo-svg.h>
#endif // CAIRO_HAS_SVG_SURFACE
#endif
#if defined(GRID_RENDERER)
#include "mapnik_grid.hpp"
#include <mapnik/grid/grid.hpp>          // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp> // for grid_renderer
#endif
#if defined(SVG_RENDERER)
#include <mapnik/svg/output/svg_renderer.hpp>
#endif
// mapnik-vector-tile
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_load_tile.hpp"
#include "object_to_container.hpp"

namespace {

struct dummy_surface
{
};

using surface_type = mapnik::util::variant<dummy_surface,
                                           Image*,
                                           CairoSurface*
#if defined(GRID_RENDERER)
                                           ,
                                           Grid*
#endif
                                           >;

struct ref_visitor
{
    void operator()(dummy_surface) {} // no-op
    template <typename SurfaceType>
    void operator()(SurfaceType* surface)
    {
        if (surface != nullptr)
        {
            surface->Ref();
        }
    }
};

struct deref_visitor
{
    void operator()(dummy_surface) {} // no-op
    template <typename SurfaceType>
    void operator()(SurfaceType* surface)
    {
        if (surface != nullptr)
        {
            surface->Unref();
        }
    }
};

template <typename Renderer>
void process_layers(Renderer& ren,
                    mapnik::request const& m_req,
                    mapnik::projection const& map_proj,
                    std::vector<mapnik::layer> const& layers,
                    double scale_denom,
                    std::string const& map_srs,
                    mapnik::vector_tile_impl::merc_tile_ptr const& tile)
{
    for (auto const& lyr : layers)
    {
        if (lyr.visible(scale_denom))
        {
            protozero::pbf_reader layer_msg;
            if (tile->layer_reader(lyr.name(), layer_msg))
            {
                mapnik::layer lyr_copy(lyr);
                lyr_copy.set_srs(map_srs);
                std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                    layer_msg,
                    tile->x(),
                    tile->y(),
                    tile->z());
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

struct AsyncRenderTile : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncRenderTile(Map* map_obj,
                    mapnik::vector_tile_impl::merc_tile_ptr const& tile,
                    surface_type const& surface,
                    mapnik::attributes const& variables,
                    std::size_t layer_idx,
                    std::int64_t z,
                    std::int64_t x,
                    std::int64_t y,
                    unsigned width,
                    unsigned height,
                    int buffer_size,
                    double scale_factor,
                    double scale_denominator,
                    bool use_cairo,
                    bool zxy_override,
                    Napi::Function const& callback)
        : Base(callback),
          map_obj_(map_obj),
          tile_(tile),
          surface_(surface),
          variables_(variables),
          layer_idx_(layer_idx),
          z_(z),
          x_(x),
          y_(y),
          width_(width),
          height_(height),
          buffer_size_(buffer_size),
          scale_factor_(scale_factor),
          scale_denominator_(scale_denominator),
          use_cairo_(use_cairo),
          zxy_override_(zxy_override) {}

    ~AsyncRenderTile() {}

    void Execute() override
    {
        try
        {
            map_ptr map = map_obj_->impl();
            mapnik::box2d<double> map_extent;
            if (zxy_override_)
            {
                map_extent = mapnik::vector_tile_impl::tile_mercator_bbox(x_, y_, z_);
            }
            else
            {
                map_extent = mapnik::vector_tile_impl::tile_mercator_bbox(tile_->x(),
                                                                          tile_->y(),
                                                                          tile_->z());
            }
            mapnik::request m_req(width_, height_, map_extent);
            m_req.set_buffer_size(buffer_size_);
            mapnik::projection map_proj(map->srs(), true);
            double scale_denom = scale_denominator_;
            if (scale_denom <= 0.0)
            {
                scale_denom = mapnik::scale_denominator(m_req.scale(), map_proj.is_geographic());
            }
            scale_denom *= scale_factor_;
            std::vector<mapnik::layer> const& layers = map->layers();
#if defined(GRID_RENDERER)
            // render grid for layer
            if (surface_.is<Grid*>())
            {
                Grid* g = mapnik::util::get<Grid*>(surface_);
                grid_ptr grid = g->impl();

                mapnik::grid_renderer<mapnik::grid> ren(*map,
                                                        m_req,
                                                        variables_,
                                                        *grid,
                                                        scale_factor_);
                ren.start_map_processing(*map);
                mapnik::layer const& lyr = layers[layer_idx_];
                if (lyr.visible(scale_denom))
                {
                    protozero::pbf_reader layer_msg;
                    if (tile_->layer_reader(lyr.name(), layer_msg))
                    {
                        // copy field names
                        std::set<std::string> attributes = grid->get_fields();
                        // todo - make this a static constant
                        std::string known_id_key = "__id__";
                        if (attributes.find(known_id_key) != attributes.end())
                        {
                            attributes.erase(known_id_key);
                        }
                        std::string join_field = grid->get_key();
                        if (known_id_key != join_field &&
                            attributes.find(join_field) == attributes.end())
                        {
                            attributes.insert(join_field);
                        }

                        mapnik::layer lyr_copy(lyr);
                        lyr_copy.set_srs(map->srs());
                        std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                            mapnik::vector_tile_impl::tile_datasource_pbf>(
                            layer_msg,
                            tile_->x(),
                            tile_->y(),
                            tile_->z());
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
                    ren.end_map_processing(*map);
                }
            }
            else
#endif
                if (surface_.is<CairoSurface*>())
            {
                CairoSurface* c = mapnik::util::get<CairoSurface*>(surface_);
                if (use_cairo_)
                {
#if defined(HAVE_CAIRO)
                    mapnik::cairo_surface_ptr surface;
                    // TODO - support any surface type
                    surface = mapnik::cairo_surface_ptr(cairo_svg_surface_create_for_stream(
                                                            (cairo_write_func_t)c->write_callback,
                                                            (void*)(&c->stream()),
                                                            static_cast<double>(c->width()),
                                                            static_cast<double>(c->height())),
                                                        mapnik::cairo_surface_closer());
                    mapnik::cairo_ptr c_context = mapnik::create_context(surface);
                    mapnik::cairo_renderer<mapnik::cairo_ptr> ren(*map, m_req,
                                                                  variables_,
                                                                  c_context, scale_factor_);
                    ren.start_map_processing(*map);
                    process_layers(ren, m_req, map_proj, layers, scale_denom, map->srs(), tile_);
                    ren.end_map_processing(*map);
#else
                    SetError("no support for rendering svg with cairo backend");
#endif
                }
                else
                {
#if defined(SVG_RENDERER)
                    typedef mapnik::svg_renderer<std::ostream_iterator<char>> svg_ren;
                    std::ostream_iterator<char> output_stream_iterator(c->stream());
                    svg_ren ren(*map, m_req,
                                variables_,
                                output_stream_iterator, scale_factor_);
                    ren.start_map_processing(*map);
                    process_layers(ren, m_req, map_proj, layers, scale_denom, map->srs(), tile_);
                    ren.end_map_processing(*map);
#else
                    SetError("no support for rendering svg with native svg backend (-DSVG_RENDERER)");
#endif
                }
            }
            // render all layers with agg
            else if (surface_.is<Image*>())
            {
                Image* js_image = mapnik::util::get<Image*>(surface_);
                mapnik::image_any& im = *js_image->impl();
                if (im.is<mapnik::image_rgba8>())
                {
                    mapnik::image_rgba8& im_data = mapnik::util::get<mapnik::image_rgba8>(im);
                    mapnik::agg_renderer<mapnik::image_rgba8> ren(*map, m_req,
                                                                  variables_,
                                                                  im_data, scale_factor_);
                    ren.start_map_processing(*map);
                    process_layers(ren, m_req, map_proj, layers, scale_denom, map->srs(), tile_);
                    ren.end_map_processing(*map);
                }
                else
                {
                    SetError("This image type is not currently supported for rendering.");
                }
            }
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

    void OnWorkComplete(Napi::Env env, napi_status status) override
    {
        if (map_obj_)
        {
            map_obj_->release();
            map_obj_->Unref();
        }
        mapnik::util::apply_visitor(deref_visitor(), surface_);
        Base::OnWorkComplete(env, status);
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        if (surface_.is<Image*>())
        {
            image_ptr image = mapnik::util::get<Image*>(surface_)->impl();
            Napi::Value arg = Napi::External<image_ptr>::New(env, &image);
            Napi::Object obj = Image::constructor.New({arg});
            return {env.Undefined(), napi_value(obj)};
        }
#if defined(GRID_RENDERER)
        else if (surface_.is<Grid*>())
        {
            grid_ptr grid = mapnik::util::get<Grid*>(surface_)->impl();
            Napi::Value arg = Napi::External<grid_ptr>::New(env, &grid);
            Napi::Object obj = Grid::constructor.New({arg});
            return {env.Undefined(), napi_value(obj)};
        }
#endif
        else if (surface_.is<CairoSurface*>())
        {
            CairoSurface* c = mapnik::util::get<CairoSurface*>(surface_);
            c->flush();
            Napi::Value width = Napi::Number::New(env, c->width());
            Napi::Value height = Napi::Number::New(env, c->height());
            Napi::Value format = Napi::String::New(env, c->format());
            Napi::Object obj = CairoSurface::constructor.New({format, width, height});
            CairoSurface* new_c = Napi::ObjectWrap<CairoSurface>::Unwrap(obj);
            new_c->set_data(c->data());
            return {env.Undefined(), napi_value(obj)};
        }
        return Base::GetResult(env);
    }

  private:
    Map* map_obj_;
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    surface_type surface_;
    mapnik::attributes variables_;
    std::size_t layer_idx_;
    std::int64_t z_;
    std::int64_t x_;
    std::int64_t y_;
    unsigned width_;
    unsigned height_;
    int buffer_size_;
    double scale_factor_;
    double scale_denominator_;
    bool use_cairo_;
    bool zxy_override_;
};
} // namespace

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

    Map* m = Napi::ObjectWrap<Map>::Unwrap(obj);
    if (info.Length() < 2 || !info[1].IsObject())
    {
        Napi::TypeError::New(env, "a renderable mapnik object is expected as second arg").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Object im_obj = info[1].As<Napi::Object>();
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length() - 1].IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object options = Napi::Object::New(env);
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::int64_t z = 0;
    bool zxy_override = false;
    int buffer_size = 0;
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
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
            z = bind_opt.As<Napi::Number>().Int64Value();
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
            x = bind_opt.As<Napi::Number>().Int64Value();
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
            y = bind_opt.As<Napi::Number>().Int64Value();
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
            object_to_container(variables, bind_opt.As<Napi::Object>());
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
        CairoSurface* c = Napi::ObjectWrap<CairoSurface>::Unwrap(im_obj);
        width = c->width();
        height = c->height();
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
        Grid* g = Napi::ObjectWrap<Grid>::Unwrap(im_obj);
        width = g->impl()->width();
        height = g->impl()->height();
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
                        //layer_idx = idx; error: Value stored to 'layer_idx' is never read [clang-analyzer-deadcode.DeadStores,-warnings-as-errors]
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
    mapnik::util::apply_visitor(ref_visitor(), surface);
    m->Ref();
    auto* worker = new AsyncRenderTile{m,
                                       tile_,
                                       surface,
                                       variables,
                                       layer_idx,
                                       z, x, y,
                                       width, height,
                                       buffer_size,
                                       scale_factor,
                                       scale_denominator,
                                       use_cairo,
                                       zxy_override,
                                       callback.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}
