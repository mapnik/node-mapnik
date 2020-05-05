#if defined(GRID_RENDERER)

// mapnik
#include <mapnik/version.hpp>

#include "mapnik_grid.hpp"
#include "mapnik_grid_view.hpp"
#include "js_grid_utils.hpp"
#include "utils.hpp"

// std
#include <exception>

Napi::FunctionReference Grid::constructor;

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
void Grid::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, Grid::New);

    lcons->SetClassName(Napi::String::New(env, "Grid"));

    // methods
    InstanceMethod("encodeSync", &encodeSync),
    InstanceMethod("encode", &encode),
    InstanceMethod("addField", &addField),
    InstanceMethod("fields", &fields),
    InstanceMethod("view", &view),
    InstanceMethod("width", &width),
    InstanceMethod("height", &height),
    InstanceMethod("painted", &painted),
    InstanceMethod("clear", &clear),
    InstanceMethod("clearSync", &clearSync),
    // properties
    ATTR(lcons, "key", get_key, set_key);

    (target).Set(Napi::String::New(env, "Grid"), Napi::GetFunction(lcons));
    NODE_MAPNIK_DEFINE_64_BIT_CONSTANT(Napi::GetFunction(lcons), "base_mask", mapnik::grid::base_mask);

    constructor.Reset(lcons);
}

Grid::Grid(unsigned int width, unsigned int height, std::string const& key) : Napi::ObjectWrap<Grid>(),
    this_(std::make_shared<mapnik::grid>(width,height,key))
{
}

Grid::~Grid()
{
}

Napi::Value Grid::New(Napi::CallbackInfo const& info)
{
    if (!info.IsConstructCall())
    {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() >= 2)
    {
        if (!info[0].IsNumber() || !info[1].IsNumber())
        {
            Napi::TypeError::New(env, "Grid 'width' and 'height' must be a integers").ThrowAsJavaScriptException();
            return env.Null();
        }

        // defaults
        std::string key("__id__");

        if (info.Length() >= 3) {

            if (!info[2].IsObject())
            {
                Napi::TypeError::New(env, "optional third arg must be an options object").ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Object options = info[2].As<Napi::Object>();

            if ((options).Has(Napi::String::New(env, "key")).FromMaybe(false)) {
                Napi::Value bind_opt = (options).Get(Napi::String::New(env, "key"));
                if (!bind_opt.IsString())
                {
                    Napi::TypeError::New(env, "optional arg 'key' must be an string").ThrowAsJavaScriptException();
                    return env.Null();
                }

                key = TOSTR(bind_opt);
            }
        }

        Grid* g = new Grid(info[0].As<Napi::Number>().Int32Value(), info[1].As<Napi::Number>().Int32Value(), key);
        g->Wrap(info.This());
        return info.This();
        return;
    }
    else
    {
        Napi::Error::New(env, "please provide Grid width and height").ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value Grid::clearSync(Napi::CallbackInfo const& info)
{
    return _clearSync(info);
}

Napi::Value Grid::_clearSync(Napi::CallbackInfo const& info)
{
    Napi::EscapableHandleScope scope(env);
    Grid* g = info.Holder().Unwrap<Grid>();
    g->get()->clear();
    return scope.Escape(env.Undefined());
}

typedef struct {
    uv_work_t request;
    Grid* g;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
} clear_grid_baton_t;

Napi::Value Grid::clear(Napi::CallbackInfo const& info)
{
    Grid* g = info.Holder().Unwrap<Grid>();

    if (info.Length() == 0) {
        return _clearSync(info);
        return;
    }
    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }
    clear_grid_baton_t *closure = new clear_grid_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    clear_grid_baton_t *closure = static_cast<clear_grid_baton_t *>(req->data);
    if (closure->error)
    {
        // There seems to be no possible way for the exception to be thrown in the previous
        // process and therefore not possible to have an error here so removing it from code
        // coverage
        /* LCOV_EXCL_START */
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    }
    else
    {
        Napi::Value argv[2] = { env.Null(), closure->g->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }
    closure->g->Unref();
    closure->cb.Reset();
    delete closure;
}

Napi::Value Grid::painted(Napi::CallbackInfo const& info)
{
    Grid* g = info.Holder().Unwrap<Grid>();
    return Napi::New(env, g->get()->painted());
}

/**
 * Get this grid's width
 * @memberof Grid
 * @instance
 * @name width
 * @returns {number} width
 */
Napi::Value Grid::width(Napi::CallbackInfo const& info)
{
    Grid* g = info.Holder().Unwrap<Grid>();
    return Napi::Number::New(env, (unsigned)g->get()->width());
}

/**
 * Get this grid's height
 * @memberof Grid
 * @instance
 * @name height
 * @returns {number} height
 */
Napi::Value Grid::height(Napi::CallbackInfo const& info)
{
    Grid* g = info.Holder().Unwrap<Grid>();
    return Napi::Number::New(env, static_cast<unsigned>(g->get()->height()));
}

Napi::Value Grid::get_key(Napi::CallbackInfo const& info)
{
    Grid* g = info.Holder().Unwrap<Grid>();
    return Napi::String::New(env, g->get()->get_key());
}

void Grid::set_key(Napi::CallbackInfo const& info, const Napi::Value& value)
{
    Grid* g = info.Holder().Unwrap<Grid>();
    if (!value.IsString())
    {
        Napi::TypeError::New(env, "key must be an string").ThrowAsJavaScriptException();
        return env.Null();
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
Napi::Value Grid::addField(Napi::CallbackInfo const& info)
{
    Grid* g = info.Holder().Unwrap<Grid>();

    if (info.Length() == 1) {
        if (!info[0].IsString())
        {
            Napi::TypeError::New(env, "argument must be a string").ThrowAsJavaScriptException();
            return env.Null();
        }
        g->get()->add_field(TOSTR(info[0]));
        return;
    }
    else
    {
        Napi::TypeError::New(env, "one parameter, a string is required").ThrowAsJavaScriptException();
        return env.Null();
    }
}

/**
 * Get all of this grid's fields
 * @memberof Grid
 * @instance
 * @name addField
 * @returns {v8::Array<string>} fields
 */
Napi::Value Grid::fields(Napi::CallbackInfo const& info)
{
    Grid* g = info.Holder().Unwrap<Grid>();
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
Napi::Value Grid::view(Napi::CallbackInfo const& info)
{
    if ( (info.Length() != 4) || (!info[0].IsNumber() && !info[1].IsNumber() && !info[2].IsNumber() && !info[3].IsNumber() ))
    {
        Napi::TypeError::New(env, "requires 4 integer arguments: x, y, width, height").ThrowAsJavaScriptException();
        return env.Null();
    }

    unsigned x = info[0].As<Napi::Number>().Int32Value();
    unsigned y = info[1].As<Napi::Number>().Int32Value();
    unsigned w = info[2].As<Napi::Number>().Int32Value();
    unsigned h = info[3].As<Napi::Number>().Int32Value();

    Grid* g = info.Holder().Unwrap<Grid>();
    return GridView::NewInstance(g,x,y,w,h);
}

/**
 * Get a constrained view of this field given x, y, width, height parameters.
 * @memberof Grid
 * @instance
 * @name encodeSync
 * @param {Object} [options={ resolution: 4, features: false }]
 * @returns {Object} an encoded field with `grid`, `keys`, and `data` members.
 */
Napi::Value Grid::encodeSync(Napi::CallbackInfo const& info)
{
    Grid* g = info.Holder().Unwrap<Grid>();

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
        std::vector<mapnik::grid::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid>(*g->get(),lines,key_order,resolution);

        // convert key order to proper javascript array
        Napi::Array keys_a = Napi::Array::New(env, key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it < key_order.end(); ++it, ++i)
        {
            (keys_a).Set(i, Napi::String::New(env, *it));
        }

        mapnik::grid const& grid_type = *g->get();

        // gather feature data
        Napi::Object feature_data = Napi::Object::New(env);
        if (add_features) {
            node_mapnik::write_features<mapnik::grid>(*g->get(),
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
    Grid* g;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
    std::vector<node_mapnik::grid_line_type> lines;
    unsigned int resolution;
    bool add_features;
    std::vector<mapnik::grid::lookup_type> key_order;
} encode_grid_baton_t;

Napi::Value Grid::encode(Napi::CallbackInfo const& info)
{
    Grid* g = info.Holder().Unwrap<Grid>();

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
    Napi::Function callback = info[info.Length(.As<Napi::Function>()-1]);

    encode_grid_baton_t *closure = new encode_grid_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->error = false;
    closure->resolution = resolution;
    closure->add_features = add_features;
    closure->cb.Reset(callback.As<Napi::Function>());
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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    encode_grid_baton_t *closure = static_cast<encode_grid_baton_t *>(req->data);


    if (closure->error)
    {
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
        for (it = closure->key_order.begin(), i = 0; it < closure->key_order.end(); ++it, ++i)
        {
            (keys_a).Set(i, Napi::String::New(env, *it));
        }

        mapnik::grid const& grid_type = *closure->g->get();
        // gather feature data
        Napi::Object feature_data = Napi::Object::New(env);
        if (closure->add_features) {
            node_mapnik::write_features<mapnik::grid>(grid_type,
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
