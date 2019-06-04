
#if defined(GRID_RENDERER)

// mapnik
#include <mapnik/version.hpp>

#include "mapnik_grid.hpp"
#include "mapnik_grid_view.hpp"
#include "js_grid_utils.hpp"
#include "utils.hpp"

// std
#include <exception>

Nan::Persistent<v8::FunctionTemplate> Grid::constructor;

/**
 * **`mapnik.Grid`**
 * 
 * Generator for [UTFGrid](https://www.mapbox.com/guides/an-open-platform)
 * representations of data.
 *
 * @class Grid
 * @param {number} width
 * @param {number} height
 * @param {Object} [options={}] optional argument, which can have a 'key' property
 * @property {string} key
 */
void Grid::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Grid::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Grid").ToLocalChecked());

    // methods
    Nan::SetPrototypeMethod(lcons, "encodeSync", encodeSync);
    Nan::SetPrototypeMethod(lcons, "encode", encode);
    Nan::SetPrototypeMethod(lcons, "addField", addField);
    Nan::SetPrototypeMethod(lcons, "fields", fields);
    Nan::SetPrototypeMethod(lcons, "view", view);
    Nan::SetPrototypeMethod(lcons, "width", width);
    Nan::SetPrototypeMethod(lcons, "height", height);
    Nan::SetPrototypeMethod(lcons, "painted", painted);
    Nan::SetPrototypeMethod(lcons, "clear", clear);
    Nan::SetPrototypeMethod(lcons, "clearSync", clearSync);
    // properties
    ATTR(lcons, "key", get_key, set_key);

    Nan::Set(target, Nan::New("Grid").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
    NODE_MAPNIK_DEFINE_64_BIT_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(), "base_mask", mapnik::grid::base_mask);

    constructor.Reset(lcons);
}

Grid::Grid(unsigned int width, unsigned int height, std::string const& key) :
    Nan::ObjectWrap(),
    this_(std::make_shared<mapnik::grid>(width,height,key))
{
}

Grid::~Grid()
{
}

NAN_METHOD(Grid::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info.Length() >= 2)
    {
        if (!info[0]->IsNumber() || !info[1]->IsNumber())
        {
            Nan::ThrowTypeError("Grid 'width' and 'height' must be a integers");
            return;
        }

        // defaults
        std::string key("__id__");

        if (info.Length() >= 3) {

            if (!info[2]->IsObject())
            {
                Nan::ThrowTypeError("optional third arg must be an options object");
                return;
            }
            v8::Local<v8::Object> options = info[2].As<v8::Object>();

            if (Nan::Has(options, Nan::New("key").ToLocalChecked()).FromMaybe(false)) {
                v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("key").ToLocalChecked()).ToLocalChecked();
                if (!bind_opt->IsString())
                {
                    Nan::ThrowTypeError("optional arg 'key' must be an string");
                    return;
                }

                key = TOSTR(bind_opt);
            }
        }

        Grid* g = new Grid(Nan::To<int>(info[0]).FromJust(), Nan::To<int>(info[1]).FromJust(), key);
        g->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    else
    {
        Nan::ThrowError("please provide Grid width and height");
        return;
    }
}

NAN_METHOD(Grid::clearSync)
{
    info.GetReturnValue().Set(_clearSync(info));
}

v8::Local<v8::Value> Grid::_clearSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());
    g->get()->clear();
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    Grid* g;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} clear_grid_baton_t;

NAN_METHOD(Grid::clear)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());

    if (info.Length() == 0) {
        info.GetReturnValue().Set(_clearSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }
    clear_grid_baton_t *closure = new clear_grid_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Clear, (uv_after_work_cb)EIO_AfterClear);
    g->Ref();
    return;
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
        /* LCOV_EXCL_STOP */
    }
}

void Grid::EIO_AfterClear(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    clear_grid_baton_t *closure = static_cast<clear_grid_baton_t *>(req->data);
    if (closure->error)
    {
        // There seems to be no possible way for the exception to be thrown in the previous 
        // process and therefore not possible to have an error here so removing it from code
        // coverage
        /* LCOV_EXCL_START */
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->g->handle() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->g->Unref();
    closure->cb.Reset();
    delete closure;
}

NAN_METHOD(Grid::painted)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());
    info.GetReturnValue().Set(Nan::New(g->get()->painted()));
}

/**
 * Get this grid's width
 * @memberof Grid
 * @instance
 * @name width
 * @returns {number} width
 */
NAN_METHOD(Grid::width)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Integer>((unsigned)g->get()->width()));
}

/**
 * Get this grid's height
 * @memberof Grid
 * @instance
 * @name height
 * @returns {number} height
 */
NAN_METHOD(Grid::height)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Integer>(static_cast<unsigned>(g->get()->height())));
}

NAN_GETTER(Grid::get_key)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::String>(g->get()->get_key()).ToLocalChecked());
}

NAN_SETTER(Grid::set_key)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());
    if (!value->IsString())
    {
        Nan::ThrowTypeError("key must be an string");
        return;
    }
    g->get()->set_key(TOSTR(value));
}


/**
 * Add a field to this grid's output
 * @memberof Grid
 * @instance
 * @name addField
 * @param {string} field
 */
NAN_METHOD(Grid::addField)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());

    if (info.Length() == 1) {
        if (!info[0]->IsString())
        {
            Nan::ThrowTypeError("argument must be a string");
            return;
        }
        g->get()->add_field(TOSTR(info[0]));
        return;
    }
    else
    {
        Nan::ThrowTypeError("one parameter, a string is required");
        return;
    }
}

/**
 * Get all of this grid's fields
 * @memberof Grid
 * @instance
 * @name addField
 * @returns {v8::Array<string>} fields
 */
NAN_METHOD(Grid::fields)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());
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

/**
 * Get a constrained view of this field given x, y, width, height parameters.
 * @memberof Grid
 * @instance
 * @name view
 * @param {number} x
 * @param {number} y
 * @param {number} width
 * @param {number} height
 * @returns {mapnik.Grid} a grid constrained to this new view
 */
NAN_METHOD(Grid::view)
{
    if ( (info.Length() != 4) || (!info[0]->IsNumber() && !info[1]->IsNumber() && !info[2]->IsNumber() && !info[3]->IsNumber() ))
    {
        Nan::ThrowTypeError("requires 4 integer arguments: x, y, width, height");
        return;
    }

    unsigned x = Nan::To<int>(info[0]).FromJust();
    unsigned y = Nan::To<int>(info[1]).FromJust();
    unsigned w = Nan::To<int>(info[2]).FromJust();
    unsigned h = Nan::To<int>(info[3]).FromJust();

    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());
    info.GetReturnValue().Set(GridView::NewInstance(g,x,y,w,h));
}

/**
 * Get a constrained view of this field given x, y, width, height parameters.
 * @memberof Grid
 * @instance
 * @name encodeSync
 * @param {Object} [options={ resolution: 4, features: false }]
 * @returns {Object} an encoded field with `grid`, `keys`, and `data` members.
 */
NAN_METHOD(Grid::encodeSync)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());

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
        std::vector<mapnik::grid::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid>(*g->get(),lines,key_order,resolution);

        // convert key order to proper javascript array
        v8::Local<v8::Array> keys_a = Nan::New<v8::Array>(key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it < key_order.end(); ++it, ++i)
        {
            Nan::Set(keys_a, i, Nan::New<v8::String>(*it).ToLocalChecked());
        }

        mapnik::grid const& grid_type = *g->get();

        // gather feature data
        v8::Local<v8::Object> feature_data = Nan::New<v8::Object>();
        if (add_features) {
            node_mapnik::write_features<mapnik::grid>(*g->get(),
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
    Grid* g;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    std::vector<node_mapnik::grid_line_type> lines;
    unsigned int resolution;
    bool add_features;
    std::vector<mapnik::grid::lookup_type> key_order;
} encode_grid_baton_t;

NAN_METHOD(Grid::encode)
{
    Grid* g = Nan::ObjectWrap::Unwrap<Grid>(info.Holder());

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
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(info[info.Length()-1]);

    encode_grid_baton_t *closure = new encode_grid_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->error = false;
    closure->resolution = resolution;
    closure->add_features = add_features;
    closure->cb.Reset(callback.As<v8::Function>());
    // todo - reserve lines size?
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    g->Ref();
    return;
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
        /* LCOV_EXCL_STOP */
    }
}

void Grid::EIO_AfterEncode(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    encode_grid_baton_t *closure = static_cast<encode_grid_baton_t *>(req->data);


    if (closure->error) 
    {
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
        for (it = closure->key_order.begin(), i = 0; it < closure->key_order.end(); ++it, ++i)
        {
            Nan::Set(keys_a, i, Nan::New<v8::String>(*it).ToLocalChecked());
        }

        mapnik::grid const& grid_type = *closure->g->get();
        // gather feature data
        v8::Local<v8::Object> feature_data = Nan::New<v8::Object>();
        if (closure->add_features) {
            node_mapnik::write_features<mapnik::grid>(grid_type,
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
