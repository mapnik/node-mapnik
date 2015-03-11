
// mapnik
#include <mapnik/version.hpp>


#include "mapnik_grid.hpp"
#include "mapnik_grid_view.hpp"
#include "js_grid_utils.hpp"
#include "utils.hpp"

// std
#include <exception>

Persistent<FunctionTemplate> Grid::constructor;

void Grid::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Grid::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Grid"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(lcons, "encodeSync", encodeSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "encode", encode);
    NODE_SET_PROTOTYPE_METHOD(lcons, "addField", addField);
    NODE_SET_PROTOTYPE_METHOD(lcons, "fields", fields);
    NODE_SET_PROTOTYPE_METHOD(lcons, "view", view);
    NODE_SET_PROTOTYPE_METHOD(lcons, "width", width);
    NODE_SET_PROTOTYPE_METHOD(lcons, "height", height);
    NODE_SET_PROTOTYPE_METHOD(lcons, "painted", painted);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(lcons, "clearSync", clearSync);
    // properties
    ATTR(lcons, "key", get_key, set_key);

    target->Set(NanNew("Grid"), lcons->GetFunction());
    NODE_MAPNIK_DEFINE_64_BIT_CONSTANT(lcons->GetFunction(), "base_mask", mapnik::grid::base_mask);

    NanAssignPersistent(constructor, lcons);
}

Grid::Grid(unsigned int width, unsigned int height, std::string const& key) :
    node::ObjectWrap(),
    this_(std::make_shared<mapnik::grid>(width,height,key)),
    estimated_size_(width * height) {
    NanAdjustExternalMemory(estimated_size_);
}

Grid::~Grid()
{
    NanAdjustExternalMemory(-estimated_size_);
}

NAN_METHOD(Grid::New)
{
    NanScope();
    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args.Length() >= 2)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
        {
            NanThrowTypeError("Grid 'width' and 'height' must be a integers");
            NanReturnUndefined();
        }

        // defaults
        std::string key("__id__");

        if (args.Length() >= 3) {

            if (!args[2]->IsObject())
            {
                NanThrowTypeError("optional third arg must be an options object");
                NanReturnUndefined();
            }
            Local<Object> options = args[2].As<Object>();

            if (options->Has(NanNew("key"))) {
                Local<Value> bind_opt = options->Get(NanNew("key"));
                if (!bind_opt->IsString())
                {
                    NanThrowTypeError("optional arg 'key' must be an string");
                    NanReturnUndefined();
                }

                key = TOSTR(bind_opt);
            }
        }

        Grid* g = new Grid(args[0]->IntegerValue(), args[1]->IntegerValue(), key);
        g->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else
    {
        NanThrowError("please provide Grid width and height");
        NanReturnUndefined();
    }
}

NAN_METHOD(Grid::clearSync)
{
    NanScope();
    NanReturnValue(_clearSync(args));
}

Local<Value> Grid::_clearSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();
    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());
    g->get()->clear();
    return NanEscapeScope(NanUndefined());
}

typedef struct {
    uv_work_t request;
    Grid* g;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
} clear_grid_baton_t;

NAN_METHOD(Grid::clear)
{
    NanScope();
    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());

    if (args.Length() == 0) {
        NanReturnValue(_clearSync(args));
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
    {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }
    clear_grid_baton_t *closure = new clear_grid_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Clear, (uv_after_work_cb)EIO_AfterClear);
    g->Ref();
    NanReturnUndefined();
}

void Grid::EIO_Clear(uv_work_t* req)
{
    clear_grid_baton_t *closure = static_cast<clear_grid_baton_t *>(req->data);
    try
    {
        closure->g->get()->clear();
    }
    catch(std::exception const& ex)
    {
        // It is not clear how clear could EVER throw an exception but leaving
        // it in but removing the following lines from coverage test.
        /* LCOV_EXCL_START */
        closure->error = true;
        closure->error_name = ex.what();
        /* LCOV_EXCL_END */
    }
}

void Grid::EIO_AfterClear(uv_work_t* req)
{
    NanScope();
    clear_grid_baton_t *closure = static_cast<clear_grid_baton_t *>(req->data);
    if (closure->error)
    {
        // There seems to be no possible way for the exception to be thrown in the previous 
        // process and therefore not possible to have an error here so removing it from code
        // coverage
        /* LCOV_EXCL_START */
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
        /* LCOV_EXCL_END */
    }
    else
    {
        Local<Value> argv[2] = { NanNull(), NanObjectWrapHandle(closure->g) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }
    closure->g->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}

NAN_METHOD(Grid::painted)
{
    NanScope();

    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());
    NanReturnValue(NanNew(g->get()->painted()));
}

NAN_METHOD(Grid::width)
{
    NanScope();

    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());
    NanReturnValue(NanNew<Integer>(g->get()->width()));
}

NAN_METHOD(Grid::height)
{
    NanScope();

    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());
    NanReturnValue(NanNew<Integer>(g->get()->height()));
}

NAN_GETTER(Grid::get_key)
{
    NanScope();
    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());
    NanReturnValue(NanNew(g->get()->get_key().c_str()));
}

NAN_SETTER(Grid::set_key)
{
    NanScope();
    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());
    if (!value->IsString())
    {
        NanThrowTypeError("key must be an string");
        return;
    }
    g->get()->set_key(TOSTR(value));
}

NAN_METHOD(Grid::addField)
{
    NanScope();
    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());

    if (args.Length() == 1) {
        if (!args[0]->IsString())
        {
            NanThrowTypeError("argument must be a string");
            NanReturnUndefined();
        }
        g->get()->add_field(TOSTR(args[0]));
        NanReturnUndefined();
    }
    else
    {
        NanThrowTypeError("one parameter, a string is required");
        NanReturnUndefined();
    }
}

NAN_METHOD(Grid::fields)
{
    NanScope();

    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());
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

NAN_METHOD(Grid::view)
{
    NanScope();

    if ( (args.Length() != 4) || (!args[0]->IsNumber() && !args[1]->IsNumber() && !args[2]->IsNumber() && !args[3]->IsNumber() ))
    {
        NanThrowTypeError("requires 4 integer arguments: x, y, width, height");
        NanReturnUndefined();
    }

    unsigned x = args[0]->IntegerValue();
    unsigned y = args[1]->IntegerValue();
    unsigned w = args[2]->IntegerValue();
    unsigned h = args[3]->IntegerValue();

    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());
    NanReturnValue(GridView::NewInstance(g,x,y,w,h));
}

NAN_METHOD(Grid::encodeSync) 
{
    NanScope();

    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());

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
        std::vector<mapnik::grid::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid>(*g->get(),lines,key_order,resolution);

        // convert key order to proper javascript array
        Local<Array> keys_a = NanNew<Array>(key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it < key_order.end(); ++it, ++i)
        {
            keys_a->Set(i, NanNew((*it).c_str()));
        }

        mapnik::grid const& grid_type = *g->get();

        // gather feature data
        Local<Object> feature_data = NanNew<Object>();
        if (add_features) {
            node_mapnik::write_features<mapnik::grid>(*g->get(),
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
    Grid* g;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    std::vector<node_mapnik::grid_line_type> lines;
    unsigned int resolution;
    bool add_features;
    std::vector<mapnik::grid::lookup_type> key_order;
} encode_grid_baton_t;

NAN_METHOD(Grid::encode) 
{
    NanScope();

    Grid* g = node::ObjectWrap::Unwrap<Grid>(args.Holder());

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
    Local<Function> callback = Local<Function>::Cast(args[args.Length()-1]);

    encode_grid_baton_t *closure = new encode_grid_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->error = false;
    closure->resolution = resolution;
    closure->add_features = add_features;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    // todo - reserve lines size?
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    g->Ref();
    NanReturnUndefined();
}

void Grid::EIO_Encode(uv_work_t* req)
{
    encode_grid_baton_t *closure = static_cast<encode_grid_baton_t *>(req->data);

    try
    {
        node_mapnik::grid2utf<mapnik::grid>(*closure->g->get(),
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

void Grid::EIO_AfterEncode(uv_work_t* req)
{
    NanScope();

    encode_grid_baton_t *closure = static_cast<encode_grid_baton_t *>(req->data);


    if (closure->error) 
    {
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
        for (it = closure->key_order.begin(), i = 0; it < closure->key_order.end(); ++it, ++i)
        {
            keys_a->Set(i, NanNew((*it).c_str()));
        }

        mapnik::grid const& grid_type = *closure->g->get();
        // gather feature data
        Local<Object> feature_data = NanNew<Object>();
        if (closure->add_features) {
            node_mapnik::write_features<mapnik::grid>(grid_type,
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

        Local<Value> argv[2] = { NanNull(), NanNew(json) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->g->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}
