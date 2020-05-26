#include "mapnik_map.hpp"
#include "mapnik_image.hpp"
#include "object_to_container.hpp"
// mapnik
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/agg_renderer.hpp>      // for agg_renderer
#include <mapnik/geometry/box2d.hpp>    // for box2d
#include <mapnik/color.hpp>             // for color
#include <mapnik/attribute.hpp>         // for attributes
#include <mapnik/util/variant.hpp>      // for save_to_file, guess_type, etc
#include <mapnik/image.hpp>             // for image_rgba8
#include <mapnik/image_any.hpp>
#include <mapnik/image_util.hpp>        // for save_to_file, guess_type, etc
#if defined(HAVE_CAIRO)
#include <mapnik/cairo_io.hpp>
#endif
#if defined(GRID_RENDERER)
#include "mapnik_grid.hpp"
#include <mapnik/grid/grid.hpp>         // for hit_grid, grid
#include <mapnik/grid/grid_renderer.hpp>// for grid_renderer
#endif

/*
#if defined(GRID_RENDERER)
struct grid_baton_t {
    uv_work_t request;
    Map *m;
    Grid *g;
    std::size_t layer_idx;
    int buffer_size; // TODO - no effect until mapnik::request is used
    double scale_factor;
    double scale_denominator;
    mapnik::attributes variables;
    unsigned offset_x;
    unsigned offset_y;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
    grid_baton_t() :
      layer_idx(-1),
      buffer_size(0),
      scale_factor(1.0),
      scale_denominator(0.0),
      variables(),
      offset_x(0),
      offset_y(0),
      error(false),
      error_name() {}
};
#endif

struct vector_tile_baton_t {
    uv_work_t request;
    Map *m;
    VectorTile *d;
    double area_threshold;
    double scale_factor;
    double scale_denominator;
    mapnik::attributes variables;
    unsigned offset_x;
    unsigned offset_y;
    std::string image_format;
    mapnik::scaling_method_e scaling_method;
    double simplify_distance;
    bool error;
    bool strictly_simple;
    bool multi_polygon_union;
    mapnik::vector_tile_impl::polygon_fill_type fill_type;
    bool process_all_rings;
    std::launch threading_mode;
    std::string error_name;
    Napi::FunctionReference cb;
    vector_tile_baton_t() :
        area_threshold(0.1),
        scale_factor(1.0),
        scale_denominator(0.0),
        variables(),
        offset_x(0),
        offset_y(0),
        image_format("webp"),
        scaling_method(mapnik::SCALING_BILINEAR),
        simplify_distance(0.0),
        error(false),
        strictly_simple(true),
        multi_polygon_union(false),
        fill_type(mapnik::vector_tile_impl::positive_fill),
        process_all_rings(false),
        threading_mode(std::launch::deferred) {}
};
*/
/*
void Map::EIO_RenderVectorTile(uv_work_t* req)
{
    vector_tile_baton_t *closure = static_cast<vector_tile_baton_t *>(req->data);
    try
    {
        mapnik::Map const& map = *closure->m->get();

        mapnik::vector_tile_impl::processor ren(map, closure->variables);
        ren.set_simplify_distance(closure->simplify_distance);
        ren.set_multi_polygon_union(closure->multi_polygon_union);
        ren.set_fill_type(closure->fill_type);
        ren.set_process_all_rings(closure->process_all_rings);
        ren.set_scale_factor(closure->scale_factor);
        ren.set_strictly_simple(closure->strictly_simple);
        ren.set_image_format(closure->image_format);
        ren.set_scaling_method(closure->scaling_method);
        ren.set_area_threshold(closure->area_threshold);
        ren.set_threading_mode(closure->threading_mode);

        ren.update_tile(*closure->d->get_tile(),
                        closure->scale_denominator,
                        closure->offset_x,
                        closure->offset_y);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterRenderVectorTile(uv_work_t* req)
{
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    vector_tile_baton_t *closure = static_cast<vector_tile_baton_t *>(req->data);
    closure->m->release();

    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Napi::Value argv[2] = { env.Null(), closure->d->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->d->Unref();
    closure->cb.Reset();
    delete closure;
}

#if defined(GRID_RENDERER)
void Map::EIO_RenderGrid(uv_work_t* req)
{

    grid_baton_t *closure = static_cast<grid_baton_t *>(req->data);

    std::vector<mapnik::layer> const& layers = closure->m->map_->layers();

    try
    {
        // copy property names
        std::set<std::string> attributes = closure->g->get()->get_fields();

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

        mapnik::grid_renderer<mapnik::grid> ren(*closure->m->map_,
                                                *closure->g->get(),
                                                closure->scale_factor,
                                                closure->offset_x,
                                                closure->offset_y);
        mapnik::layer const& layer = layers[closure->layer_idx];
        ren.apply(layer,attributes,closure->scale_denominator);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Map::EIO_AfterRenderGrid(uv_work_t* req)
{
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);

    grid_baton_t *closure = static_cast<grid_baton_t *>(req->data);
    closure->m->release();

    if (closure->error) {
        // TODO - add more attributes
        // https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/Error
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    } else {
        Napi::Value argv[2] = { env.Null(), closure->g->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }

    closure->m->Unref();
    closure->g->Unref();
    closure->cb.Reset();
    delete closure;
}
#endif
*/

namespace detail {
struct agg_renderer_visitor
{
    agg_renderer_visitor(mapnik::Map const& m,
                         mapnik::request const& req,
                         mapnik::attributes const& vars,
                         double scale_factor,
                         unsigned offset_x,
                         unsigned offset_y,
                         double scale_denominator)
        : m_(m),
          req_(req),
          vars_(vars),
          scale_factor_(scale_factor),
          offset_x_(offset_x),
          offset_y_(offset_y),
          scale_denominator_(scale_denominator) {}

    void operator() (mapnik::image_rgba8 & pixmap)
    {
        mapnik::agg_renderer<mapnik::image_rgba8> ren(m_,req_,vars_,pixmap,scale_factor_,offset_x_,offset_y_);
        ren.apply(scale_denominator_);
    }

    template <typename T>
    void operator() (T &)
    {
        throw std::runtime_error("This image type is not currently supported for rendering.");
    }

  private:
    mapnik::Map const& m_;
    mapnik::request const& req_;
    mapnik::attributes const& vars_;
    double scale_factor_;
    unsigned offset_x_;
    unsigned offset_y_;
    double scale_denominator_;
};

struct AsyncRender : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;

    AsyncRender(map_ptr const& map, image_ptr const& image,
                double scale_factor, double scale_denominator,
                int buffer_size, unsigned offset_x, unsigned offset_y,
                mapnik::attributes const& variables,
                Napi::Function const& callback)
        :Base(callback),
         map_(map),
         image_(image),
         scale_factor_(scale_factor),
         scale_denominator_(scale_denominator),
         buffer_size_(buffer_size),
         offset_x_(offset_x),
         offset_y_(offset_y),
         variables_(variables) {}

    void Execute() override
    {
        try
        {
            mapnik::request request(map_->width(),map_->height(),map_->get_current_extent());
            request.set_buffer_size(buffer_size_);
            agg_renderer_visitor visit(*map_,
                                       request,
                                       variables_,
                                       scale_factor_,
                                       offset_x_,
                                       offset_y_,
                                       scale_denominator_);
            mapnik::util::apply_visitor(visit, *image_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        Napi::Value arg = Napi::External<image_ptr>::New(env, &image_);
        Napi::Object obj = Image::constructor.New({arg});
        return {env.Null(), napi_value(obj)};
    }

private:
    map_ptr map_;
    image_ptr image_;
    double scale_factor_;
    double scale_denominator_;
    int buffer_size_;
    unsigned offset_x_;
    unsigned offset_y_;
    mapnik::attributes variables_;
};


struct AsyncRenderGrid : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;

    AsyncRenderGrid(map_ptr const& map, grid_ptr const& grid,
                    double scale_factor, double scale_denominator,
                    int buffer_size, unsigned offset_x, unsigned offset_y,
                    std::size_t layer_idx,
                    Napi::Function const& callback)
        :Base(callback),
         map_(map),
         grid_(grid),
         scale_factor_(scale_factor),
         scale_denominator_(scale_denominator),
         buffer_size_(buffer_size),
         offset_x_(offset_x),
         offset_y_(offset_y),
         layer_idx_(layer_idx) {}

    void Execute() override
    {
        try
        {
            std::vector<mapnik::layer> const& layers = map_->layers();
            // copy property names
            std::set<std::string> attributes = grid_->get_fields();

            // todo - make this a static constant
            std::string known_id_key = "__id__";
            if (attributes.find(known_id_key) != attributes.end())
            {
                attributes.erase(known_id_key);
            }

            std::string join_field = grid_->get_key();
            if (known_id_key != join_field &&
                attributes.find(join_field) == attributes.end())
            {
                attributes.insert(join_field);
            }

            mapnik::grid_renderer<mapnik::grid> ren(*map_,
                                                    *grid_,
                                                    scale_factor_,
                                                    offset_x_,
                                                    offset_y_);
            mapnik::layer const& layer = layers[layer_idx_];
            ren.apply(layer, attributes, scale_denominator_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        Napi::Value arg = Napi::External<grid_ptr>::New(env, &grid_);
        Napi::Object obj = Grid::constructor.New({arg});
        return {env.Null(), napi_value(obj)};
    }

private:
    map_ptr map_;
    grid_ptr grid_;
    double scale_factor_;
    double scale_denominator_;
    int buffer_size_;
    unsigned offset_x_;
    unsigned offset_y_;
    std::size_t layer_idx_;
    //mapnik::attributes variables_;
};

struct AsyncRenderFile : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;

    AsyncRenderFile(map_ptr const& map, std::string const& output_filename,
                    double scale_factor, double scale_denominator,
                    int buffer_size, palette_ptr const& palette,
                    std::string const& format, bool use_cairo,
                    mapnik::attributes const& variables,
                    Napi::Function const& callback)
        :Base(callback),
         map_(map),
         output_filename_(output_filename),
         scale_factor_(scale_factor),
         scale_denominator_(scale_denominator),
         buffer_size_(buffer_size),
         palette_(palette),
         format_(format),
         use_cairo_(use_cairo),
         variables_(variables) {}

    void Execute() override
    {
        try
        {
            if (use_cairo_)
            {
#if defined(HAVE_CAIRO)
                // https://github.com/mapnik/mapnik/issues/1930
                mapnik::save_to_cairo_file(*map_, output_filename_,
                                           format_, scale_factor_,
                                           scale_denominator_);
#else
                SetError("Cairo renderer is not available");
#endif
            }
            else
            {
                mapnik::image_rgba8 im(map_->width(), map_->height());
                mapnik::request m_req(map_->width(), map_->height(), map_->get_current_extent());
                m_req.set_buffer_size(buffer_size_);
                mapnik::agg_renderer<mapnik::image_rgba8> ren(*map_,
                                                              m_req,
                                                              variables_,
                                                              im,
                                                              scale_factor_);
                ren.apply(scale_denominator_);
                if (palette_.get())
                {
                    mapnik::save_to_file(im, output_filename_, *palette_);
                }
                else
                {
                    mapnik::save_to_file(im, output_filename_);
                }
            }
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        return Base::GetResult(env);
    }
private:
    map_ptr map_;
    std::string output_filename_;
    double scale_factor_;
    double scale_denominator_;
    int buffer_size_;
    palette_ptr palette_;
    std::string format_;
    bool use_cairo_;
    mapnik::attributes variables_;
};

}

/**
 * Renders a mapnik object (image tile, grid, vector tile) by passing in a renderable mapnik object.
 *
 * @instance
 * @name render
 * @memberof Map
 * @param {mapnik.Image} renderable mapnik object
 * @param {Object} [options={}]
 * @param {Number} [options.buffer_size=0] size of the buffer on the image
 * @param {Number} [options.scale=1.0] scale the image
 * @param {Number} [options.scale_denominator=0.0]
 * @param {Number} [options.offset_x=0] pixel offset along the x-axis
 * @param {Number} [options.offset_y=0] pixel offset along the y-axis
 * @param {String} [options.image_scaling] must be a valid scaling method (used when rendering a vector tile)
 * @param {String} [options.image_format] must be a string and valid image format (used when rendering a vector tile)
 * @param {Number} [options.area_threshold] used to discard small polygons by setting a minimum size (used when rendering a vector tile)
 * @param {Boolean} [options.strictly_simple=] ensure all geometry is valid according to
 * OGC Simple definition (used when rendering a vector tile)
 * @param {Boolean} [options.multi_polygon_union] union all multipolygons (used when rendering a vector tile)
 * @param {String} [options.fill_type] the fill type used in determining what are holes and what are outer rings. See the
 * [Clipper documentation](http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/PolyFillType.htm)
 * to learn more about fill types. (used when rendering a vector tile)
 * @param {String} [options.threading_mode] (used when rendering a vector tile)
 * @param {Number} [options.simplify_distance] Simplification works to generalize
 * geometries before encoding into vector tiles.simplification distance The
 * `simplify_distance` value works in integer space over a 4096 pixel grid and uses
 * the [Douglas-Peucker algorithm](https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm).
 * (used when rendering a vector tile)
 * @param {Object} [options.variables] Mapnik 3.x ONLY: A javascript object
 * containing key value pairs that should be passed into Mapnik as variables
 * for rendering and for datasource queries. For example if you passed
 * `vtile.render(map,image,{ variables : {zoom:1} },cb)` then the `@zoom`
 * variable would be usable in Mapnik symbolizers like `line-width:"@zoom"`
 * and as a token in Mapnik postgis sql sub-selects like
 * `(select * from table where some_field > @zoom)` as tmp (used when rendering a vector tile)
 * @param {Boolean} [options.process_all_rings] if `true`, don't assume winding order and ring order of
 * polygons are correct according to the [`2.0` Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec)
 * (used when rendering a vector tile)
 * @returns {mapnik.Map} rendered image tile
 *
 * @example
 * // render data to an image object
 * var map = new mapnik.Map(256, 256);
 * map.loadSync('./path/to/stylesheet.xml');
 * var image = new mapnik.Image(map.width, map.height);
 * map.render(image, {}, function(err, image) {
 *     if (err) throw err;
 *     console.log(image) // => mapnik image object with data from xml
 * });
 *
 * @example
 * // render data to a vector tile object
 * var map = new mapnik.Map(256, 256);
 * map.loadSync('./path/to/stylesheet.xml');
 * var vtile = new mapnik.VectorTile(9,112,195);
 * map.render(vtile, {}, function(err, vtile) {
 *     if (err) throw err;
 *     console.log(vtile); // => vector tile object with data from xml
 * });
 */

Napi::Value Map::render(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    // ensure at least 2 args
    if (info.Length() < 2)
    {
        Napi::TypeError::New(env, "requires at least two arguments, a renderable mapnik object, and a callback")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // ensure renderable object
    if (!info[0].IsObject())
    {
        Napi::TypeError::New(env, "requires a renderable mapnik object to be passed as first argument")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // ensure function callback
    if (!info[info.Length()-1].IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    try
    {
        // parse options
        // defaults
        int buffer_size = 0;
        double scale_factor = 1.0;
        double scale_denominator = 0.0;
        unsigned offset_x = 0;
        unsigned offset_y = 0;

        Napi::Object options = Napi::Object::New(env);
        if (info.Length() > 2)
        {
            // options object
            if (!info[1].IsObject())
            {
                Napi::TypeError::New(env, "optional second argument must be an options object").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            options = info[1].As<Napi::Object>();

            if (options.Has("buffer_size"))
            {
                Napi::Value buffer_size_val = options.Get("buffer_size");
                if (!buffer_size_val.IsNumber())
                {
                    Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
                buffer_size = buffer_size_val.As<Napi::Number>().Int32Value();
            }

            if (options.Has("scale"))
            {
                Napi::Value scale_val = options.Get("scale");
                if (!scale_val.IsNumber())
                {
                    Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
                scale_factor = scale_val.As<Napi::Number>().DoubleValue();
            }

            if (options.Has("scale_denominator"))
            {
                Napi::Value scale_denominator_val = options.Get("scale_denominator");
                if (!scale_denominator_val.IsNumber())
                {
                    Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
                scale_denominator = scale_denominator_val.As<Napi::Number>().DoubleValue();
            }

            if (options.Has("offset_x"))
            {
                Napi::Value offset_x_val = options.Get("offset_x");
                if (!offset_x_val.IsNumber())
                {
                    Napi::TypeError::New(env, "optional arg 'offset_x' must be a number").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
                offset_x = offset_x_val.As<Napi::Number>().Int32Value();
            }
            if (options.Has("offset_y"))
            {
                Napi::Value offset_y_val = options.Get("offset_y");
                if (!offset_y_val.IsNumber())
                {
                    Napi::TypeError::New(env, "optional arg 'offset_y' must be a number").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
                offset_y = offset_y_val.As<Napi::Number>().Int32Value();
            }
        }

        Napi::Object obj = info[0].As<Napi::Object>();

        if (obj.InstanceOf(Image::constructor.Value()))
        {
            image_ptr image = Napi::ObjectWrap<Image>::Unwrap(obj)->impl();
            mapnik::attributes variables;
            if (options.Has("variables"))
            {
                Napi::Value variables_val = options.Get("variables");
                if (!variables_val.IsObject())
                {
                    //delete closure;
                    Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
                object_to_container(variables, variables_val.As<Napi::Object>());
            }
            if (!acquire())
            {
                Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Function callback = info[info.Length()-1].As<Napi::Function>();
            auto* worker = new detail::AsyncRender(map_,
                                                   image,
                                                   scale_factor,
                                                   scale_denominator,
                                                   offset_x,
                                                   offset_y,
                                                   buffer_size,
                                                   variables,
                                                   callback);
            worker->Queue();
            return env.Undefined();
        }
#if defined(GRID_RENDERER)
        else if (obj.InstanceOf(Grid::constructor.Value()))
        {
            grid_ptr grid = Napi::ObjectWrap<Grid>::Unwrap(obj)->impl();
            std::size_t layer_idx = 0;

            // grid requires special options for now
            if (!options.Has("layer"))
            {
                Napi::TypeError::New(env, "'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            else
            {
                std::vector<mapnik::layer> const& layers = map_->layers();

                Napi::Value layer_id = options.Get("layer");
                if (! (layer_id.IsString() || layer_id.IsNumber()) )
                {
                    Napi::TypeError::New(env, "'layer' option required for grid rendering and must be either a layer name(string) or layer index (integer)").ThrowAsJavaScriptException();
                    return env.Null();
                }

                if (layer_id.IsString())
                {
                    bool found = false;
                    unsigned int idx(0);
                    std::string const & layer_name = layer_id.As<Napi::String>();
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
                else
                { // IS NUMBER
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
                        Napi::TypeError::New(env, s.str().c_str()).ThrowAsJavaScriptException();
                        return env.Null();
                    }
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
                unsigned int i = 0;
                unsigned int num_fields = a.Length();
                while (i < num_fields)
                {
                    Napi::Value name = a.Get(i);
                    if (name.IsString())
                    {
                        grid->add_field(name.As<Napi::String>());
                    }
                    i++;
                }
            }

            //grid_baton_t *closure = new grid_baton_t();

            if (options.Has("variables"))
            {
                Napi::Value bind_opt = options.Get("variables");
                if (!bind_opt.IsObject())
                {
                    Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
                //object_to_container(variables, bind_opt.As<Napi::Object>());
            }

            //closure->request.data = closure;
            //closure->m = m;
            //closure->g = g;
            //closure->layer_idx = layer_idx;
            //closure->buffer_size = buffer_size;
            //closure->scale_factor = scale_factor;
            //closure->scale_denominator = scale_denominator;
            //closure->offset_x = offset_x;
            //closure->offset_y = offset_y;
            //closure->error = false;
            if (!acquire())
            {
                Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Function callback = info[info.Length()-1].As<Napi::Function>();
            auto* worker = new detail::AsyncRenderGrid(map_,
                                                       grid,
                                                       scale_factor,
                                                       scale_denominator,
                                                       offset_x,
                                                       offset_y,
                                                       buffer_size,
                                                       layer_idx,
                                                       callback);
            worker->Queue();
            return env.Undefined();
        }
#endif

/*
        // VT
        else if (Napi::New(env, VectorTile::constructor)->HasInstance(obj))
        {

            vector_tile_baton_t *closure = new vector_tile_baton_t();

            if ((options).Has(Napi::String::New(env, "image_scaling")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "image_scaling"));
                if (!param_val.IsString())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'image_scaling' must be a string").ThrowAsJavaScriptException();
                    return env.Null();
                }
                std::string image_scaling = TOSTR(param_val);
                boost::optional<mapnik::scaling_method_e> method = mapnik::scaling_method_from_string(image_scaling);
                if (!method)
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'image_scaling' must be a string and a valid scaling method (e.g 'bilinear')").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->scaling_method = *method;
            }

            if ((options).Has(Napi::String::New(env, "image_format")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "image_format"));
                if (!param_val.IsString())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'image_format' must be a string").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->image_format = TOSTR(param_val);
            }

            if ((options).Has(Napi::String::New(env, "area_threshold")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "area_threshold"));
                if (!param_val.IsNumber())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'area_threshold' must be a number").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->area_threshold = param_val.As<Napi::Number>().DoubleValue();
                if (closure->area_threshold < 0.0)
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'area_threshold' must not be a negative number").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }

            if ((options).Has(Napi::String::New(env, "strictly_simple")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "strictly_simple"));
                if (!param_val->IsBoolean())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'strictly_simple' must be a boolean").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->strictly_simple = param_val.As<Napi::Boolean>().Value();
            }

            if ((options).Has(Napi::String::New(env, "multi_polygon_union")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "multi_polygon_union"));
                if (!param_val->IsBoolean())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'multi_polygon_union' must be a boolean").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->multi_polygon_union = param_val.As<Napi::Boolean>().Value();
            }

            if ((options).Has(Napi::String::New(env, "fill_type")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "fill_type"));
                if (!param_val.IsNumber())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'fill_type' must be an unsigned integer").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->fill_type = static_cast<mapnik::vector_tile_impl::polygon_fill_type>(param_val.As<Napi::Number>().Int32Value());
                if (closure->fill_type >= mapnik::vector_tile_impl::polygon_fill_type_max)
                {
                    delete closure;
                    Napi::TypeError::New(env, "optional arg 'fill_type' out of possible range").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }

            if ((options).Has(Napi::String::New(env, "threading_mode")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "threading_mode"));
                if (!param_val.IsNumber())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'threading_mode' must be an unsigned integer").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->threading_mode = static_cast<std::launch>(param_val.As<Napi::Number>().Int32Value());
                if (closure->threading_mode != std::launch::async &&
                    closure->threading_mode != std::launch::deferred &&
                    closure->threading_mode != (std::launch::async | std::launch::deferred))
                {
                    delete closure;
                    Napi::TypeError::New(env, "optional arg 'threading_mode' value passed is invalid").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }

            if ((options).Has(Napi::String::New(env, "simplify_distance")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "simplify_distance"));
                if (!param_val.IsNumber())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'simplify_distance' must be an floating point number").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->simplify_distance = param_val.As<Napi::Number>().DoubleValue();
                if (closure->simplify_distance < 0)
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'simplify_distance' can not be negative").ThrowAsJavaScriptException();
                    return env.Null();
                }
            }

            if ((options).Has(Napi::String::New(env, "variables")).FromMaybe(false))
            {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "variables"));
                if (!bind_opt.IsObject())
                {
                    delete closure;
                    Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
                    return env.Null();
                }
                object_to_container(closure->variables,bind_opt->ToObject(Napi::GetCurrentContext()));
            }

            if ((options).Has(Napi::String::New(env, "process_all_rings")).FromMaybe(false))
            {
                Napi::Value param_val = (options).Get(Napi::String::New(env, "process_all_rings"));
                if (!param_val->IsBoolean())
                {
                    delete closure;
                    Napi::TypeError::New(env, "option 'process_all_rings' must be a boolean").ThrowAsJavaScriptException();
                    return env.Null();
                }
                closure->process_all_rings = param_val.As<Napi::Boolean>().Value();
            }

            closure->request.data = closure;
            closure->m = m;
            closure->d = obj.Unwrap<VectorTile>();
            closure->scale_factor = scale_factor;
            closure->scale_denominator = scale_denominator;
            closure->offset_x = offset_x;
            closure->offset_y = offset_y;
            closure->error = false;
            if (!m->acquire())
            {
                delete closure;
                Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
                return env.Null();
            }
            closure->cb.Reset(info[info.Length() - 1].As<Napi::Function>());
            uv_queue_work(uv_default_loop(), &closure->request, EIO_RenderVectorTile, (uv_after_work_cb)EIO_AfterRenderVectorTile);
            closure->d->Ref();
        }
#endif
*/
        else
        {
            Napi::TypeError::New(env, "renderable mapnik object expected").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        return env.Undefined();
    }
    catch (std::exception const& ex)
    {
        // I am not quite sure it is possible to put a test in to cover an exception here
        // LCOV_EXCL_START
        Napi::TypeError::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
        // LCOV_EXCL_STOP
    }
}

// TODO - add support for grids
Napi::Value Map::renderSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    std::string format = "png";
    palette_ptr palette;
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    int buffer_size = 0;

    if (info.Length() >= 1)
    {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "first argument is optional, but if provided must be an object, eg. {format: 'pdf'}").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[0].As<Napi::Object>();
        if (options.Has("format"))
        {
            Napi::Value format_opt = options.Get("format");
            if (!format_opt.IsString())
            {
                Napi::TypeError::New(env, "'format' must be a String").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            format = format_opt.As<Napi::String>();
        }

        if (options.Has("palette"))
        {
            Napi::Value palette_opt = options.Get("palette");
            if (!palette_opt.IsObject())
            {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            Napi::Object obj = palette_opt.As<Napi::Object>();

            if (!obj.InstanceOf(Palette::constructor.Value()))
            {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            palette = Napi::ObjectWrap<Palette>::Unwrap(obj)->palette_;
        }

        if (options.Has("buffer_size"))
        {
            Napi::Value buffer_size_val = options.Get("buffer_size");
            if (!buffer_size_val.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            buffer_size = buffer_size_val.As<Napi::Number>().Int32Value();
        }

        if (options.Has("scale"))
        {
            Napi::Value scale_val = options.Get("scale");
            if (!scale_val.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_factor = scale_val.As<Napi::Number>().DoubleValue();
        }

        if (options.Has("scale_denominator"))
        {
            Napi::Value scale_denominator_val = options.Get("scale_denominator");
            if (!scale_denominator_val.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_denominator = scale_denominator_val.As<Napi::Number>().DoubleValue();
        }
    }

    if (!acquire())
    {
        Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string s;
    try
    {
        mapnik::image_rgba8 im(map_->width(), map_->height());
        mapnik::request m_req(map_->width(),map_->height(),map_->get_current_extent());
        m_req.set_buffer_size(buffer_size);
        mapnik::agg_renderer<mapnik::image_rgba8> ren(*map_,
                                                      m_req,
                                                      mapnik::attributes(),
                                                      im,
                                                      scale_factor);
        ren.apply(scale_denominator);

        if (palette.get())
        {
            s = save_to_string(im, format, *palette);
        }
        else {
            s = save_to_string(im, format);
        }
    }
    catch (std::exception const& ex)
    {
        release();
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
    release();
    return scope.Escape(Napi::Buffer<char>::Copy(env, (char*)s.data(), s.size()));
}


Napi::Value Map::renderFile(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to a file to save").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // defaults
    std::string format = "png";
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    palette_ptr palette;
    int buffer_size = 0;

    Napi::Value callback_val = info[info.Length() - 1];

    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object options = Napi::Object::New(env);

    if (!info[1].IsFunction() && info[1].IsObject())
    {
        options = info[1].As<Napi::Object>();;
        if (options.Has("format"))
        {
            Napi::Value format_opt = options.Get("format");
            if (!format_opt.IsString())
            {
                Napi::TypeError::New(env, "'format' must be a String").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            format = format_opt.As<Napi::String>();
        }

        if (options.Has("palette"))
        {
            Napi::Value palette_opt = options.Get("palette");
            if (!palette_opt.IsObject())
            {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            Napi::Object obj = palette_opt.As<Napi::Object>();

            if (!obj.InstanceOf(Palette::constructor.Value()))
            {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            palette = Napi::ObjectWrap<Palette>::Unwrap(obj)->palette_;
        }

        if (options.Has("buffer_size"))
        {
            Napi::Value buffer_size_val = options.Get("buffer_size");
            if (!buffer_size_val.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            buffer_size = buffer_size_val.As<Napi::Number>().Int32Value();
        }

        if (options.Has("scale"))
        {
            Napi::Value scale_val = options.Get("scale");
            if (!scale_val.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_factor = scale_val.As<Napi::Number>().DoubleValue();
        }

        if (options.Has("scale_denominator"))
        {
            Napi::Value scale_denominator_val = options.Get("scale_denominator");
            if (!scale_denominator_val.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_denominator = scale_denominator_val.As<Napi::Number>().DoubleValue();
        }
    }
    else if (!info[1].IsFunction())
    {
        Napi::TypeError::New(env, "optional argument must be an object").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string output_filename = info[0].As<Napi::String>();

    if (format.empty())
    {
        format = mapnik::guess_type(output_filename);
        if (format == "<unknown>")
        {
            std::ostringstream s("");
            s << "unknown output  extension for: " << output_filename << "\n";
            Napi::Error::New(env, s.str()).ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    mapnik::attributes variables;
    if (options.Has("variables"))
    {
        Napi::Value variables_val = options.Get("variables");
        if (!variables_val.IsObject())
        {
            Napi::TypeError::New(env, "optional arg 'variables' must be an object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        object_to_container(variables, variables_val.As<Napi::Object>());
    }

    bool use_cairo = false;
    if (format == "pdf" || format == "svg" || format == "ps" || format == "ARGB32" || format == "RGB24")
    {
#if defined(HAVE_CAIRO)
        use_cairo = true;
#else
        std::ostringstream s("");
        s << "Cairo backend is not available, cannot write to " << format << "\n";
        Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
        return env.Undefined();
#endif
    }

    if (!acquire())
    {
        Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Function callback = callback_val.As<Napi::Function>();
    auto* worker = new detail::AsyncRenderFile(map_,
                                               output_filename,
                                               scale_factor,
                                               scale_denominator,
                                               buffer_size,
                                               palette,
                                               format,
                                               use_cairo,
                                               variables,
                                               callback);
    worker->Queue();
    return env.Undefined();

}

Napi::Value Map::renderFileSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    if (info.Length() < 1 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "first argument must be a path to a file to save").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() > 2)
    {
        Napi::Error::New(env, "accepts two arguments, a required path to a file, an optional options object, eg. {format: 'pdf'}")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // defaults
    double scale_factor = 1.0;
    double scale_denominator = 0.0;
    int buffer_size = 0;
    std::string format = "png";
    palette_ptr palette;

    if (info.Length() >= 2)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "second argument is optional, but if provided must be an object, eg. {format: 'pdf'}")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("format"))
        {
            Napi::Value format_opt = options.Get("format");
            if (!format_opt.IsString())
            {
                Napi::TypeError::New(env, "'format' must be a String").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            format = format_opt.As<Napi::String>();
        }

        if (options.Has("palette"))
        {
            Napi::Value palette_opt = options.Get("palette");
            if (!palette_opt.IsObject())
            {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            Napi::Object obj = palette_opt.As<Napi::Object>();

            if (!obj.InstanceOf(Palette::constructor.Value()))
            {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            palette = Napi::ObjectWrap<Palette>::Unwrap(obj)->palette_;
        }

        if (options.Has("buffer_size"))
        {
            Napi::Value buffer_size_val = options.Get("buffer_size");
            if (!buffer_size_val.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'buffer_size' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            buffer_size = buffer_size_val.As<Napi::Number>().Int32Value();
        }

        if (options.Has("scale"))
        {
            Napi::Value scale_val = options.Get("scale");
            if (!scale_val.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_factor = scale_val.As<Napi::Number>().DoubleValue();
        }

        if (options.Has("scale_denominator"))
        {
            Napi::Value scale_denominator_val = options.Get("scale_denominator");
            if (!scale_denominator_val.IsNumber())
            {
                Napi::TypeError::New(env, "optional arg 'scale_denominator' must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            scale_denominator = scale_denominator_val.As<Napi::Number>().DoubleValue();
        }
    }

    std::string output_filename = info[0].As<Napi::String>();

    if (format.empty())
    {
        format = mapnik::guess_type(output_filename);
        if (format == "<unknown>")
        {
            std::ostringstream s("");
            s << "unknown output extension for: " << output_filename << "\n";
            Napi::Error::New(env, s.str()).ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    if (!acquire())
    {
        Napi::TypeError::New(env, "render: Map currently in use by another thread. Consider using a map pool.").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    try
    {

        if (format == "pdf" || format == "svg" || format =="ps" || format == "ARGB32" || format == "RGB24")
        {
#if defined(HAVE_CAIRO)
            mapnik::save_to_cairo_file(*map_, output_filename, format, scale_factor, scale_denominator);
#else
            std::ostringstream s("");
            s << "Cairo backend is not available, cannot write to " << format << "\n";
            m->release();
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return env.Undefined();
#endif
        }
        else
        {
            mapnik::image_rgba8 im(map_->width(),map_->height());
            mapnik::request m_req(map_->width(), map_->height(), map_->get_current_extent());
            m_req.set_buffer_size(buffer_size);
            mapnik::agg_renderer<mapnik::image_rgba8> ren(*map_,
                                                          m_req,
                                                          mapnik::attributes(),
                                                          im,
                                                          scale_factor);

            ren.apply(scale_denominator);

            if (palette.get())
            {
                mapnik::save_to_file(im, output_filename, *palette);
            }
            else {
                mapnik::save_to_file(im, output_filename);
            }
        }
    }
    catch (std::exception const& ex)
    {
        release();
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
    release();
    return env.Undefined();
}
