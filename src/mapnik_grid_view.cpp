#if defined(GRID_RENDERER)

// mapnik
#include <mapnik/grid/grid.hpp>         // for hit_grid<>::lookup_type, etc
#include <mapnik/grid/grid_view.hpp>    // for grid_view, hit_grid_view, etc

#include "mapnik_grid_view.hpp"
#include "mapnik_grid.hpp"
#include "js_grid_utils.hpp"
#include "utils.hpp"

// std
#include <exception>

Napi::FunctionReference GridView::constructor;

void GridView::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, GridView::New);

    lcons->SetClassName(Napi::String::New(env, "GridView"));

    InstanceMethod("encodeSync", &encodeSync),
    InstanceMethod("encode", &encode),
    InstanceMethod("fields", &fields),
    InstanceMethod("width", &width),
    InstanceMethod("height", &height),
    InstanceMethod("isSolid", &isSolid),
    InstanceMethod("isSolidSync", &isSolidSync),
    InstanceMethod("getPixel", &getPixel),

    (target).Set(Napi::String::New(env, "GridView"),Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}


GridView::GridView(Grid * JSGrid) : Napi::ObjectWrap<GridView>(),
    this_(),
    JSGrid_(JSGrid) {
        JSGrid_->Ref();
    }

GridView::~GridView()
{
    JSGrid_->Unref();
}

Napi::Value GridView::New(const Napi::CallbackInfo& info)
{
    if (!info.IsConstructCall())
    {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info[0].IsExternal())
    {
        Napi::External ext = info[0].As<Napi::External>();
        void* ptr = ext->Value();
        GridView* g =  static_cast<GridView*>(ptr);
        g->Wrap(info.This());
        return info.This();
        return;
    } else {
        Napi::Error::New(env, "Cannot create this object from Javascript").ThrowAsJavaScriptException();
        return env.Null();
    }

    return;
}

Napi::Value GridView::NewInstance(Grid * JSGrid,
                            unsigned x,
                            unsigned y,
                            unsigned w,
                            unsigned h
    )
{
    Napi::EscapableHandleScope scope(env);
    GridView* gv = new GridView(JSGrid);
    gv->this_ = std::make_shared<mapnik::grid_view>(JSGrid->get()->get_view(x,y,w,h));
    Napi::Value ext = Napi::External::New(env, gv);
    Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
    if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new GridView instance").ThrowAsJavaScriptException();

    return scope.Escape(maybe_local);
}

Napi::Value GridView::width(const Napi::CallbackInfo& info)
{
    GridView* g = info.Holder().Unwrap<GridView>();
    return Napi::Number::New(env, g->get()->width());
}

Napi::Value GridView::height(const Napi::CallbackInfo& info)
{
    GridView* g = info.Holder().Unwrap<GridView>();
    return Napi::Number::New(env, g->get()->height());
}

Napi::Value GridView::fields(const Napi::CallbackInfo& info)
{
    GridView* g = info.Holder().Unwrap<GridView>();
    std::set<std::string> const& a = g->get()->get_fields();
    std::set<std::string>::const_iterator itr = a.begin();
    std::set<std::string>::const_iterator end = a.end();
    Napi::Array l = Napi::Array::New(env, a.size());
    int idx = 0;
    for (; itr != end; ++itr)
    {
        std::string name = *itr;
        (l).Set(idx, Napi::String::New(env, name));
        ++idx;
    }
    return l;
}

typedef struct {
    uv_work_t request;
    GridView* g;
    Napi::FunctionReference cb;
    bool error;
    std::string error_name;
    bool result;
    mapnik::grid_view::value_type pixel;
} is_solid_grid_view_baton_t;


Napi::Value GridView::isSolid(const Napi::CallbackInfo& info)
{
    GridView* g = info.Holder().Unwrap<GridView>();

    if (info.Length() == 0) {
        return _isSolidSync(info);
        return;
    }
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    is_solid_grid_view_baton_t *closure = new is_solid_grid_view_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->result = true;
    closure->pixel = 0;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    g->Ref();
    return;
}
void GridView::EIO_IsSolid(uv_work_t* req)
{
    is_solid_grid_view_baton_t *closure = static_cast<is_solid_grid_view_baton_t *>(req->data);
    grid_view_ptr view = closure->g->get();
    if (view->width() > 0 && view->height() > 0)
    {
        mapnik::grid_view::value_type first_pixel = view->get_row(0)[0];
        closure->pixel = first_pixel;
        for (unsigned y = 0; y < view->height(); ++y)
        {
            mapnik::grid_view::value_type const * row = view->get_row(y);
            for (unsigned x = 0; x < view->width(); ++x)
            {
                if (first_pixel != row[x])
                {
                    closure->result = false;
                    return;
                }
            }
        }
    }
    else
    {
        closure->error = true;
        closure->error_name = "image does not have valid dimensions";
    }
}

void GridView::EIO_AfterIsSolid(uv_work_t* req)
{
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    is_solid_grid_view_baton_t *closure = static_cast<is_solid_grid_view_baton_t *>(req->data);
    if (closure->error) {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            Napi::Value argv[3] = { env.Null(),
                                     Napi::New(env, closure->result),
                                     Napi::Number::New(env, closure->pixel),
            };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 3, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(),
                                     Napi::New(env, closure->result)
            };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }
    closure->g->Unref();
    closure->cb.Reset();
    delete closure;
}

Napi::Value GridView::isSolidSync(const Napi::CallbackInfo& info)
{
    return _isSolidSync(info);
}

Napi::Value GridView::_isSolidSync(const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);
    GridView* g = info.Holder().Unwrap<GridView>();
    grid_view_ptr view = g->get();
    if (view->width() > 0 && view->height() > 0)
    {
        mapnik::grid_view::value_type first_pixel = view->get_row(0)[0];
        for (unsigned y = 0; y < view->height(); ++y)
        {
            mapnik::grid_view::value_type const * row = view->get_row(y);
            for (unsigned x = 0; x < view->width(); ++x)
            {
                if (first_pixel != row[x])
                {
                    return scope.Escape(env.False());
                }
            }
        }
    }
    return scope.Escape(env.True());
}

Napi::Value GridView::getPixel(const Napi::CallbackInfo& info)
{
    unsigned x = 0;
    unsigned y = 0;

    if (info.Length() >= 2) {
        if (!info[0].IsNumber())
        {
            Napi::TypeError::New(env, "first arg, 'x' must be an integer").ThrowAsJavaScriptException();
            return env.Null();
        }
        if (!info[1].IsNumber())
        {
            Napi::TypeError::New(env, "second arg, 'y' must be an integer").ThrowAsJavaScriptException();
            return env.Null();
        }
        x = info[0].As<Napi::Number>().Int32Value();
        y = info[1].As<Napi::Number>().Int32Value();
    } else {
        Napi::TypeError::New(env, "must supply x,y to query pixel color").ThrowAsJavaScriptException();
        return env.Null();
    }

    GridView* g = info.Holder().Unwrap<GridView>();
    grid_view_ptr view = g->get();
    if (x < view->width() && y < view->height())
    {
        mapnik::grid_view::value_type pixel = view->get_row(y)[x];
        return Napi::Number::New(env, pixel);
    }
    return;
}

Napi::Value GridView::encodeSync(const Napi::CallbackInfo& info)
{
    GridView* g = info.Holder().Unwrap<GridView>();

    // defaults
    unsigned int resolution = 4;
    bool add_features = true;

    // options hash
    if (info.Length() >= 1) {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "optional arg must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[0].As<Napi::Object>();

        if ((options).Has(Napi::String::New(env, "resolution")).FromMaybe(false))
        {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "resolution"));
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'resolution' must be an Integer").ThrowAsJavaScriptException();
                return env.Null();
            }

            resolution = bind_opt.As<Napi::Number>().Int32Value();

            if (resolution == 0)
            {
                Napi::TypeError::New(env, "'resolution' can not be zero").ThrowAsJavaScriptException();
                return env.Null();
            }
        }

        if ((options).Has(Napi::String::New(env, "features")).FromMaybe(false))
        {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "features"));
            if (!bind_opt->IsBoolean())
            {
                Napi::TypeError::New(env, "'features' must be an Boolean").ThrowAsJavaScriptException();
                return env.Null();
            }

            add_features = bind_opt.As<Napi::Boolean>().Value();
        }
    }

    try {

        std::vector<node_mapnik::grid_line_type> lines;
        std::vector<mapnik::grid_view::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid_view>(*g->get(),lines,key_order,resolution);

        // convert key order to proper javascript array
        Napi::Array keys_a = Napi::Array::New(env, key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it != key_order.end(); ++it, ++i)
        {
            (keys_a).Set(i, Napi::String::New(env, *it));
        }

        mapnik::grid_view const& grid_type = *g->get();

        // gather feature data
        Napi::Object feature_data = Napi::Object::New(env);
        if (add_features) {
            node_mapnik::write_features<mapnik::grid_view>(grid_type,
                                                           feature_data,
                                                           key_order);
        }

        // Create the return hash.
        Napi::Object json = Napi::Object::New(env);
        Napi::Array grid_array = Napi::Array::New(env);
        unsigned array_size = std::ceil(grid_type.width()/static_cast<float>(resolution));
        for (unsigned j=0;j<lines.size();++j)
        {
            node_mapnik::grid_line_type const & line = lines[j];
            (grid_array).Set(j, Napi::String::New(env, line.get(),array_size));
        }
        (json).Set(Napi::String::New(env, "grid"), grid_array);
        (json).Set(Napi::String::New(env, "keys"), keys_a);
        (json).Set(Napi::String::New(env, "data"), feature_data);
        return json;

    }
    catch (std::exception const& ex)
    {
        // There is no known exception throws in the processing above
        // so simply removing the following from coverage
        /* LCOV_EXCL_START */
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
        /* LCOV_EXCL_STOP */
    }

}

typedef struct {
    uv_work_t request;
    GridView* g;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
    std::vector<node_mapnik::grid_line_type> lines;
    unsigned int resolution;
    bool add_features;
    std::vector<mapnik::grid::lookup_type> key_order;
} encode_grid_view_baton_t;


Napi::Value GridView::encode(const Napi::CallbackInfo& info)
{
    GridView* g = info.Holder().Unwrap<GridView>();

    // defaults
    unsigned int resolution = 4;
    bool add_features = true;

    // options hash
    if (info.Length() >= 1) {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "optional arg must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[0].As<Napi::Object>();

        if ((options).Has(Napi::String::New(env, "resolution")).FromMaybe(false))
        {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "resolution"));
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'resolution' must be an Integer").ThrowAsJavaScriptException();
                return env.Null();
            }

            resolution = bind_opt.As<Napi::Number>().Int32Value();

            if (resolution == 0)
            {
                Napi::TypeError::New(env, "'resolution' can not be zero").ThrowAsJavaScriptException();
                return env.Null();
            }
        }

        if ((options).Has(Napi::String::New(env, "features")).FromMaybe(false))
        {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "features"));
            if (!bind_opt->IsBoolean())
            {
                Napi::TypeError::New(env, "'features' must be an Boolean").ThrowAsJavaScriptException();
                return env.Null();
            }

            add_features = bind_opt.As<Napi::Boolean>().Value();
        }
    }

    // ensure callback is a function
    if (!info[info.Length()-1]->IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::Function callback = info[info.Length() - 1].As<Napi::Function>();

    encode_grid_view_baton_t *closure = new encode_grid_view_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->error = false;
    closure->resolution = resolution;
    closure->add_features = add_features;
    closure->cb.Reset(callback);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    g->Ref();
    return;
}

void GridView::EIO_Encode(uv_work_t* req)
{
    encode_grid_view_baton_t *closure = static_cast<encode_grid_view_baton_t *>(req->data);

    try
    {
        // TODO - write features and clear here as well?
        node_mapnik::grid2utf<mapnik::grid_view>(*(closure->g->get()),
                                                 closure->lines,
                                                 closure->key_order,
                                                 closure->resolution);
    }
    catch (std::exception const& ex)
    {
        // There is no known exception throws in the processing above
        // so simply removing the following from coverage
        /* LCOV_EXCL_START */
        closure->error = true;
        closure->error_name = ex.what();
        /* LCOV_EXCL_STOP */
    }
}

void GridView::EIO_AfterEncode(uv_work_t* req)
{
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);

    encode_grid_view_baton_t *closure = static_cast<encode_grid_view_baton_t *>(req->data);

    if (closure->error) {
        // There is no known ways to throw errors in the processing prior
        // so simply removing the following from coverage
        /* LCOV_EXCL_START */
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    }
    else
    {
        // convert key order to proper javascript array
        Napi::Array keys_a = Napi::Array::New(env, closure->key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = closure->key_order.begin(), i = 0; it != closure->key_order.end(); ++it, ++i)
        {
            (keys_a).Set(i, Napi::String::New(env, *it));
        }

        mapnik::grid_view const& grid_type = *(closure->g->get());

        // gather feature data
        Napi::Object feature_data = Napi::Object::New(env);
        if (closure->add_features) {
            node_mapnik::write_features<mapnik::grid_view>(grid_type,
                                                           feature_data,
                                                           closure->key_order);
        }
        // Create the return hash.
        Napi::Object json = Napi::Object::New(env);
        Napi::Array grid_array = Napi::Array::New(env, closure->lines.size());
        unsigned array_size = std::ceil(grid_type.width()/static_cast<float>(closure->resolution));
        for (unsigned j=0;j<closure->lines.size();++j)
        {
            node_mapnik::grid_line_type const & line = closure->lines[j];
            (grid_array).Set(j, Napi::String::New(env, line.get(),array_size));
        }
        (json).Set(Napi::String::New(env, "grid"), grid_array);
        (json).Set(Napi::String::New(env, "keys"), keys_a);
        (json).Set(Napi::String::New(env, "data"), feature_data);

        Napi::Value argv[2] = { env.Null(), json };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }

    closure->g->Unref();
    closure->cb.Reset();
    delete closure;
}

#endif
