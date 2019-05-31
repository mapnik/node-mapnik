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

Nan::Persistent<v8::FunctionTemplate> GridView::constructor;

void GridView::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(GridView::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("GridView").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "encodeSync", encodeSync);
    Nan::SetPrototypeMethod(lcons, "encode", encode);
    Nan::SetPrototypeMethod(lcons, "fields", fields);
    Nan::SetPrototypeMethod(lcons, "width", width);
    Nan::SetPrototypeMethod(lcons, "height", height);
    Nan::SetPrototypeMethod(lcons, "isSolid", isSolid);
    Nan::SetPrototypeMethod(lcons, "isSolidSync", isSolidSync);
    Nan::SetPrototypeMethod(lcons, "getPixel", getPixel);

    Nan::Set(target, Nan::New("GridView").ToLocalChecked(),Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}


GridView::GridView(Grid * JSGrid) :
    Nan::ObjectWrap(),
    this_(),
    JSGrid_(JSGrid) {
        JSGrid_->Ref();
    }

GridView::~GridView()
{
    JSGrid_->Unref();
}

NAN_METHOD(GridView::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = info[0].As<v8::External>();
        void* ptr = ext->Value();
        GridView* g =  static_cast<GridView*>(ptr);
        g->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    } else {
        Nan::ThrowError("Cannot create this object from Javascript");
        return;
    }

    return;
}

v8::Local<v8::Value> GridView::NewInstance(Grid * JSGrid,
                            unsigned x,
                            unsigned y,
                            unsigned w,
                            unsigned h
    )
{
    Nan::EscapableHandleScope scope;
    GridView* gv = new GridView(JSGrid);
    gv->this_ = std::make_shared<mapnik::grid_view>(JSGrid->get()->get_view(x,y,w,h));
    v8::Local<v8::Value> ext = Nan::New<v8::External>(gv);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new GridView instance");
    return scope.Escape(maybe_local.ToLocalChecked());
}

NAN_METHOD(GridView::width)
{
    GridView* g = Nan::ObjectWrap::Unwrap<GridView>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Integer>(g->get()->width()));
}

NAN_METHOD(GridView::height)
{
    GridView* g = Nan::ObjectWrap::Unwrap<GridView>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Integer>(g->get()->height()));
}

NAN_METHOD(GridView::fields)
{
    GridView* g = Nan::ObjectWrap::Unwrap<GridView>(info.Holder());
    std::set<std::string> const& a = g->get()->get_fields();
    std::set<std::string>::const_iterator itr = a.begin();
    std::set<std::string>::const_iterator end = a.end();
    v8::Local<v8::Array> l = Nan::New<v8::Array>(a.size());
    int idx = 0;
    for (; itr != end; ++itr)
    {
        std::string name = *itr;
        Nan::Set(l, idx, Nan::New<v8::String>(name).ToLocalChecked());
        ++idx;
    }
    info.GetReturnValue().Set(l);
}

typedef struct {
    uv_work_t request;
    GridView* g;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
    bool result;
    mapnik::grid_view::value_type pixel;
} is_solid_grid_view_baton_t;


NAN_METHOD(GridView::isSolid)
{
    GridView* g = Nan::ObjectWrap::Unwrap<GridView>(info.Holder());

    if (info.Length() == 0) {
        info.GetReturnValue().Set(_isSolidSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    is_solid_grid_view_baton_t *closure = new is_solid_grid_view_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->result = true;
    closure->pixel = 0;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
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
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    is_solid_grid_view_baton_t *closure = static_cast<is_solid_grid_view_baton_t *>(req->data);
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            v8::Local<v8::Value> argv[3] = { Nan::Null(),
                                     Nan::New(closure->result),
                                     Nan::New<v8::Number>(closure->pixel),
            };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 3, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(),
                                     Nan::New(closure->result)
            };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->g->Unref();
    closure->cb.Reset();
    delete closure;
}

NAN_METHOD(GridView::isSolidSync)
{
    info.GetReturnValue().Set(_isSolidSync(info));
}

v8::Local<v8::Value> GridView::_isSolidSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    GridView* g = Nan::ObjectWrap::Unwrap<GridView>(info.Holder());
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
                    return scope.Escape(Nan::False());
                }
            }
        }
    }
    return scope.Escape(Nan::True());
}

NAN_METHOD(GridView::getPixel)
{
    unsigned x = 0;
    unsigned y = 0;

    if (info.Length() >= 2) {
        if (!info[0]->IsNumber())
        {
            Nan::ThrowTypeError("first arg, 'x' must be an integer");
            return;
        }
        if (!info[1]->IsNumber())
        {
            Nan::ThrowTypeError("second arg, 'y' must be an integer");
            return;
        }
        x = Nan::To<int>(info[0]).FromJust();
        y = Nan::To<int>(info[1]).FromJust();
    } else {
        Nan::ThrowTypeError("must supply x,y to query pixel color");
        return;
    }

    GridView* g = Nan::ObjectWrap::Unwrap<GridView>(info.Holder());
    grid_view_ptr view = g->get();
    if (x < view->width() && y < view->height())
    {
        mapnik::grid_view::value_type pixel = view->get_row(y)[x];
        info.GetReturnValue().Set(Nan::New<v8::Number>(pixel));
    }
    return;
}

NAN_METHOD(GridView::encodeSync)
{
    GridView* g = Nan::ObjectWrap::Unwrap<GridView>(info.Holder());

    // defaults
    unsigned int resolution = 4;
    bool add_features = true;

    // options hash
    if (info.Length() >= 1) {
        if (!info[0]->IsObject())
        {
            Nan::ThrowTypeError("optional arg must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[0].As<v8::Object>();

        if (Nan::Has(options, Nan::New("resolution").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("resolution").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("'resolution' must be an Integer");
                return;
            }

            resolution = Nan::To<int>(bind_opt).FromJust();

            if (resolution == 0)
            {
                Nan::ThrowTypeError("'resolution' can not be zero");
                return;
            }
        }

        if (Nan::Has(options, Nan::New("features").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("features").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsBoolean())
            {
                Nan::ThrowTypeError("'features' must be an Boolean");
                return;
            }

            add_features = Nan::To<bool>(bind_opt).FromJust();
        }
    }

    try {

        std::vector<node_mapnik::grid_line_type> lines;
        std::vector<mapnik::grid_view::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid_view>(*g->get(),lines,key_order,resolution);

        // convert key order to proper javascript array
        v8::Local<v8::Array> keys_a = Nan::New<v8::Array>(key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it != key_order.end(); ++it, ++i)
        {
            Nan::Set(keys_a, i, Nan::New<v8::String>(*it).ToLocalChecked());
        }

        mapnik::grid_view const& grid_type = *g->get();

        // gather feature data
        v8::Local<v8::Object> feature_data = Nan::New<v8::Object>();
        if (add_features) {
            node_mapnik::write_features<mapnik::grid_view>(grid_type,
                                                           feature_data,
                                                           key_order);
        }

        // Create the return hash.
        v8::Local<v8::Object> json = Nan::New<v8::Object>();
        v8::Local<v8::Array> grid_array = Nan::New<v8::Array>();
        unsigned array_size = std::ceil(grid_type.width()/static_cast<float>(resolution));
        for (unsigned j=0;j<lines.size();++j)
        {
            node_mapnik::grid_line_type const & line = lines[j];
            Nan::Set(grid_array, j, Nan::New<v8::String>(line.get(),array_size).ToLocalChecked());
        }
        Nan::Set(json, Nan::New("grid").ToLocalChecked(), grid_array);
        Nan::Set(json, Nan::New("keys").ToLocalChecked(), keys_a);
        Nan::Set(json, Nan::New("data").ToLocalChecked(), feature_data);
        info.GetReturnValue().Set(json);

    }
    catch (std::exception const& ex)
    {
        // There is no known exception throws in the processing above
        // so simply removing the following from coverage
        /* LCOV_EXCL_START */
        Nan::ThrowError(ex.what());
        return;
        /* LCOV_EXCL_STOP */
    }

}

typedef struct {
    uv_work_t request;
    GridView* g;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    std::vector<node_mapnik::grid_line_type> lines;
    unsigned int resolution;
    bool add_features;
    std::vector<mapnik::grid::lookup_type> key_order;
} encode_grid_view_baton_t;


NAN_METHOD(GridView::encode)
{
    GridView* g = Nan::ObjectWrap::Unwrap<GridView>(info.Holder());

    // defaults
    unsigned int resolution = 4;
    bool add_features = true;

    // options hash
    if (info.Length() >= 1) {
        if (!info[0]->IsObject())
        {
            Nan::ThrowTypeError("optional arg must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[0].As<v8::Object>();

        if (Nan::Has(options, Nan::New("resolution").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("resolution").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsNumber())
            {
                Nan::ThrowTypeError("'resolution' must be an Integer");
                return;
            }

            resolution = Nan::To<int>(bind_opt).FromJust();

            if (resolution == 0)
            {
                Nan::ThrowTypeError("'resolution' can not be zero");
                return;
            }
        }

        if (Nan::Has(options, Nan::New("features").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("features").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsBoolean())
            {
                Nan::ThrowTypeError("'features' must be an Boolean");
                return;
            }

            add_features = Nan::To<bool>(bind_opt).FromJust();
        }
    }

    // ensure callback is a function
    if (!info[info.Length()-1]->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }
    v8::Local<v8::Function> callback = info[info.Length() - 1].As<v8::Function>();

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
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);

    encode_grid_view_baton_t *closure = static_cast<encode_grid_view_baton_t *>(req->data);

    if (closure->error) {
        // There is no known ways to throw errors in the processing prior
        // so simply removing the following from coverage
        /* LCOV_EXCL_START */
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    }
    else
    {
        // convert key order to proper javascript array
        v8::Local<v8::Array> keys_a = Nan::New<v8::Array>(closure->key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = closure->key_order.begin(), i = 0; it != closure->key_order.end(); ++it, ++i)
        {
            Nan::Set(keys_a, i, Nan::New<v8::String>(*it).ToLocalChecked());
        }

        mapnik::grid_view const& grid_type = *(closure->g->get());

        // gather feature data
        v8::Local<v8::Object> feature_data = Nan::New<v8::Object>();
        if (closure->add_features) {
            node_mapnik::write_features<mapnik::grid_view>(grid_type,
                                                           feature_data,
                                                           closure->key_order);
        }
        // Create the return hash.
        v8::Local<v8::Object> json = Nan::New<v8::Object>();
        v8::Local<v8::Array> grid_array = Nan::New<v8::Array>(closure->lines.size());
        unsigned array_size = std::ceil(grid_type.width()/static_cast<float>(closure->resolution));
        for (unsigned j=0;j<closure->lines.size();++j)
        {
            node_mapnik::grid_line_type const & line = closure->lines[j];
            Nan::Set(grid_array, j, Nan::New<v8::String>(line.get(),array_size).ToLocalChecked());
        }
        Nan::Set(json, Nan::New("grid").ToLocalChecked(), grid_array);
        Nan::Set(json, Nan::New("keys").ToLocalChecked(), keys_a);
        Nan::Set(json, Nan::New("data").ToLocalChecked(), feature_data);

        v8::Local<v8::Value> argv[2] = { Nan::Null(), json };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->g->Unref();
    closure->cb.Reset();
    delete closure;
}

#endif
