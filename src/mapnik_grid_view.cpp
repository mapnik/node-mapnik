// mapnik
#include <mapnik/grid/grid.hpp>         // for hit_grid<>::lookup_type, etc
#include <mapnik/grid/grid_view.hpp>    // for grid_view, hit_grid_view, etc

#include "mapnik_grid_view.hpp"
#include "mapnik_grid.hpp"
#include "js_grid_utils.hpp"
#include "utils.hpp"

// std
#include <exception>

Persistent<FunctionTemplate> GridView::constructor;

void GridView::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(GridView::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("GridView"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "encodeSync", encodeSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "encode", encode);
    NODE_SET_PROTOTYPE_METHOD(lcons, "fields", fields);
    NODE_SET_PROTOTYPE_METHOD(lcons, "width", width);
    NODE_SET_PROTOTYPE_METHOD(lcons, "height", height);
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolid", isSolid);
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolidSync", isSolidSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "getPixel", getPixel);

    target->Set(NanNew("GridView"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}


GridView::GridView(Grid * JSGrid) :
    node::ObjectWrap(),
    this_(),
    JSGrid_(JSGrid) {
        JSGrid_->_ref();
    }

GridView::~GridView()
{
    JSGrid_->_unref();
}

NAN_METHOD(GridView::New)
{
    NanScope();
    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args[0]->IsExternal())
    {
        Local<External> ext = args[0].As<External>();
        void* ptr = ext->Value();
        GridView* g =  static_cast<GridView*>(ptr);
        g->Wrap(args.This());
        NanReturnValue(args.This());
    } else {
        NanThrowError("Cannot create this object from Javascript");
        NanReturnUndefined();
    }

    NanReturnUndefined();
}

Handle<Value> GridView::NewInstance(Grid * JSGrid,
                            unsigned x,
                            unsigned y,
                            unsigned w,
                            unsigned h
    )
{
    NanEscapableScope();
    GridView* gv = new GridView(JSGrid);
    gv->this_ = std::make_shared<mapnik::grid_view>(JSGrid->get()->get_view(x,y,w,h));
    Handle<Value> ext = NanNew<External>(gv);
    Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
    return NanEscapeScope(obj);
}

NAN_METHOD(GridView::width)
{
    NanScope();

    GridView* g = node::ObjectWrap::Unwrap<GridView>(args.Holder());
    NanReturnValue(NanNew<Integer>(g->get()->width()));
}

NAN_METHOD(GridView::height)
{
    NanScope();

    GridView* g = node::ObjectWrap::Unwrap<GridView>(args.Holder());
    NanReturnValue(NanNew<Integer>(g->get()->height()));
}

NAN_METHOD(GridView::fields)
{
    NanScope();

    GridView* g = node::ObjectWrap::Unwrap<GridView>(args.Holder());
    std::set<std::string> const& a = g->get()->get_fields();
    std::set<std::string>::const_iterator itr = a.begin();
    std::set<std::string>::const_iterator end = a.end();
    Local<Array> l = NanNew<Array>(a.size());
    int idx = 0;
    for (; itr != end; ++itr)
    {
        std::string name = *itr;
        l->Set(idx, NanNew(name.c_str()));
        ++idx;
    }
    NanReturnValue(l);
}

typedef struct {
    uv_work_t request;
    GridView* g;
    Persistent<Function> cb;
    bool error;
    std::string error_name;
    bool result;
    mapnik::grid_view::value_type pixel;
} is_solid_grid_view_baton_t;


NAN_METHOD(GridView::isSolid)
{
    NanScope();
    GridView* g = node::ObjectWrap::Unwrap<GridView>(args.Holder());

    if (args.Length() == 0) {
        return isSolidSync(args);
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!args[args.Length()-1]->IsFunction())
    {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    is_solid_grid_view_baton_t *closure = new is_solid_grid_view_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->result = true;
    closure->pixel = 0;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    g->Ref();
    NanReturnUndefined();
}
void GridView::EIO_IsSolid(uv_work_t* req)
{
    is_solid_grid_view_baton_t *closure = static_cast<is_solid_grid_view_baton_t *>(req->data);
    grid_view_ptr view = closure->g->get();
    if (view->width() > 0 && view->height() > 0)
    {
        mapnik::grid_view::value_type first_pixel = view->getRow(0)[0];
        closure->pixel = first_pixel;
        for (unsigned y = 0; y < view->height(); ++y)
        {
            mapnik::grid_view::value_type const * row = view->getRow(y);
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
    NanScope();
    is_solid_grid_view_baton_t *closure = static_cast<is_solid_grid_view_baton_t *>(req->data);
    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            Local<Value> argv[3] = { NanNull(),
                                     NanNew(closure->result),
                                     NanNew<Number>(closure->pixel),
            };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 3, argv);
        }
        else
        {
            Local<Value> argv[2] = { NanNull(),
                                     NanNew(closure->result)
            };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
    }
    closure->g->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}

NAN_METHOD(GridView::isSolidSync)
{
    NanScope();
    GridView* g = node::ObjectWrap::Unwrap<GridView>(args.Holder());
    grid_view_ptr view = g->get();
    if (view->width() > 0 && view->height() > 0)
    {
        mapnik::grid_view::value_type first_pixel = view->getRow(0)[0];
        for (unsigned y = 0; y < view->height(); ++y)
        {
            mapnik::grid_view::value_type const * row = view->getRow(y);
            for (unsigned x = 0; x < view->width(); ++x)
            {
                if (first_pixel != row[x])
                {
                    NanReturnValue(NanFalse());
                }
            }
        }
    }
    NanReturnValue(NanTrue());
}

NAN_METHOD(GridView::getPixel)
{
    NanScope();


    unsigned x = 0;
    unsigned y = 0;

    if (args.Length() >= 2) {
        if (!args[0]->IsNumber())
        {
            NanThrowTypeError("first arg, 'x' must be an integer");
            NanReturnUndefined();
        }
        if (!args[1]->IsNumber())
        {
            NanThrowTypeError("second arg, 'y' must be an integer");
            NanReturnUndefined();
        }
        x = args[0]->IntegerValue();
        y = args[1]->IntegerValue();
    } else {
        NanThrowTypeError("must supply x,y to query pixel color");
        NanReturnUndefined();
    }

    GridView* g = node::ObjectWrap::Unwrap<GridView>(args.Holder());
    grid_view_ptr view = g->get();
    if (x < view->width() && y < view->height())
    {
        mapnik::grid_view::value_type pixel = view->getRow(y)[x];
        NanReturnValue(NanNew<Number>(pixel));
    }
    NanReturnUndefined();
}

NAN_METHOD(GridView::encodeSync)
{
    NanScope();

    GridView* g = node::ObjectWrap::Unwrap<GridView>(args.Holder());

    // defaults
    unsigned int resolution = 4;
    bool add_features = true;

    // options hash
    if (args.Length() >= 1) {
        if (!args[0]->IsObject())
        {
            NanThrowTypeError("optional arg must be an options object");
            NanReturnUndefined();
        }

        Local<Object> options = args[0].As<Object>();

        if (options->Has(NanNew("resolution")))
        {
            Local<Value> bind_opt = options->Get(NanNew("resolution"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("'resolution' must be an Integer");
                NanReturnUndefined();
            }

            resolution = bind_opt->IntegerValue();
            
            if (resolution == 0)
            {
                NanThrowTypeError("'resolution' can not be zero");
                NanReturnUndefined();
            }
        }

        if (options->Has(NanNew("features")))
        {
            Local<Value> bind_opt = options->Get(NanNew("features"));
            if (!bind_opt->IsBoolean())
            {
                NanThrowTypeError("'features' must be an Boolean");
                NanReturnUndefined();
            }

            add_features = bind_opt->BooleanValue();
        }
    }

    try {

        std::vector<node_mapnik::grid_line_type> lines;
        std::vector<mapnik::grid_view::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid_view>(*g->get(),lines,key_order,resolution);

        // convert key order to proper javascript array
        Local<Array> keys_a = NanNew<Array>(key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it != key_order.end(); ++it, ++i)
        {
            keys_a->Set(i, NanNew((*it).c_str()));
        }

        mapnik::grid_view const& grid_type = *g->get();

        // gather feature data
        Local<Object> feature_data = NanNew<Object>();
        if (add_features) {
            node_mapnik::write_features<mapnik::grid_view>(grid_type,
                                                           feature_data,
                                                           key_order);
        }

        // Create the return hash.
        Local<Object> json = NanNew<Object>();
        Local<Array> grid_array = NanNew<Array>();
        unsigned array_size = std::ceil(grid_type.width()/static_cast<float>(resolution));
        for (unsigned j=0;j<lines.size();++j)
        {
            node_mapnik::grid_line_type const & line = lines[j];
            grid_array->Set(j, NanNew<String>(line.get(),array_size));
        }
        json->Set(NanNew("grid"), grid_array);
        json->Set(NanNew("keys"), keys_a);
        json->Set(NanNew("data"), feature_data);
        NanReturnValue(json);

    }
    catch (std::exception const& ex)
    {
        // There is no known exception throws in the processing above
        // so simply removing the following from coverage
        /* LCOV_EXCL_START */
        NanThrowError(ex.what());
        NanReturnUndefined();
        /* LCOV_EXCL_END */
    }

}

typedef struct {
    uv_work_t request;
    GridView* g;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    std::vector<node_mapnik::grid_line_type> lines;
    unsigned int resolution;
    bool add_features;
    std::vector<mapnik::grid::lookup_type> key_order;
} encode_grid_view_baton_t;


NAN_METHOD(GridView::encode)
{
    NanScope();

    GridView* g = node::ObjectWrap::Unwrap<GridView>(args.Holder());

    // defaults
    unsigned int resolution = 4;
    bool add_features = true;

    // options hash
    if (args.Length() >= 1) {
        if (!args[0]->IsObject())
        {
            NanThrowTypeError("optional arg must be an options object");
            NanReturnUndefined();
        }

        Local<Object> options = args[0].As<Object>();

        if (options->Has(NanNew("resolution")))
        {
            Local<Value> bind_opt = options->Get(NanNew("resolution"));
            if (!bind_opt->IsNumber())
            {
                NanThrowTypeError("'resolution' must be an Integer");
                NanReturnUndefined();
            }

            resolution = bind_opt->IntegerValue();

            if (resolution == 0)
            {
                NanThrowTypeError("'resolution' can not be zero");
                NanReturnUndefined();
            }
        }

        if (options->Has(NanNew("features")))
        {
            Local<Value> bind_opt = options->Get(NanNew("features"));
            if (!bind_opt->IsBoolean())
            {
                NanThrowTypeError("'features' must be an Boolean");
                NanReturnUndefined();
            }

            add_features = bind_opt->BooleanValue();
        }
    }

    // ensure callback is a function
    if (!args[args.Length()-1]->IsFunction())
    {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }
    Local<Function> callback = args[args.Length() - 1].As<Function>();

    encode_grid_view_baton_t *closure = new encode_grid_view_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->error = false;
    closure->resolution = resolution;
    closure->add_features = add_features;
    NanAssignPersistent(closure->cb, callback);
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    g->Ref();
    NanReturnUndefined();
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
        /* LCOV_EXCL_END */
    }
}

void GridView::EIO_AfterEncode(uv_work_t* req)
{
    NanScope();

    encode_grid_view_baton_t *closure = static_cast<encode_grid_view_baton_t *>(req->data);

    if (closure->error) {
        // There is no known ways to throw errors in the processing prior
        // so simply removing the following from coverage
        /* LCOV_EXCL_START */
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
        /* LCOV_EXCL_END */
    }
    else
    {
        // convert key order to proper javascript array
        Local<Array> keys_a = NanNew<Array>(closure->key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = closure->key_order.begin(), i = 0; it != closure->key_order.end(); ++it, ++i)
        {
            keys_a->Set(i, NanNew((*it).c_str()));
        }

        mapnik::grid_view const& grid_type = *(closure->g->get());

        // gather feature data
        Local<Object> feature_data = NanNew<Object>();
        if (closure->add_features) {
            node_mapnik::write_features<mapnik::grid_view>(grid_type,
                                                           feature_data,
                                                           closure->key_order);
        }
        // Create the return hash.
        Local<Object> json = NanNew<Object>();
        Local<Array> grid_array = NanNew<Array>(closure->lines.size());
        unsigned array_size = std::ceil(grid_type.width()/static_cast<float>(closure->resolution));
        for (unsigned j=0;j<closure->lines.size();++j)
        {
            node_mapnik::grid_line_type const & line = closure->lines[j];
            grid_array->Set(j, NanNew<String>(line.get(),array_size));
        }
        json->Set(NanNew("grid"), grid_array);
        json->Set(NanNew("keys"), keys_a);
        json->Set(NanNew("data"), feature_data);

        Local<Value> argv[2] = { NanNull(), json };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->g->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}
