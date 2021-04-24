#if defined(GRID_RENDERER)

// mapnik
#include <mapnik/version.hpp>

#include "mapnik_grid.hpp"
#include "mapnik_grid_view.hpp"
#include "js_grid_utils.hpp"
#include "utils.hpp"

// std
#include <exception>

namespace detail {

// AsyncWorker

struct AsyncGridClear : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncGridClear(grid_ptr const& grid, Napi::Function const& callback)
        : Base(callback),
          grid_(grid)
    {
    }

    void Execute() override
    {
        grid_->clear();
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        if (grid_)
        {
            Napi::Value arg = Napi::External<grid_ptr>::New(env, &grid_);
            Napi::Object obj = Grid::constructor.New({arg});
            return {env.Null(), napi_value(obj)};
        }
        return Base::GetResult(env);
    }
    grid_ptr grid_;
};

struct AsyncGridEncode : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    // ctor
    AsyncGridEncode(grid_ptr grid, unsigned resolution, bool add_features, Napi::Function const& callback)
        : Base(callback),
          grid_(grid),
          resolution_(resolution),
          add_features_(add_features)
    {
    }
    void Execute() override
    {
        try
        {
            node_mapnik::grid2utf<mapnik::grid>(*grid_,
                                                lines_,
                                                key_order_,
                                                resolution_);
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        // convert key order to proper javascript array
        Napi::Array keys_a = Napi::Array::New(env, key_order_.size());
        std::vector<std::string>::iterator it;
        std::size_t i;
        for (it = key_order_.begin(), i = 0; it < key_order_.end(); ++it, ++i)
        {
            keys_a.Set(i, *it);
        }

        mapnik::grid const& grid_type = *grid_;
        // gather feature data
        Napi::Object feature_data = Napi::Object::New(env);
        if (add_features_)
        {
            node_mapnik::write_features<mapnik::grid>(env, grid_type,
                                                      feature_data,
                                                      key_order_);
        }

        // Create the return hash.
        Napi::Object json = Napi::Object::New(env);
        Napi::Array grid_array = Napi::Array::New(env, lines_.size());
        for (std::size_t j = 0; j < lines_.size(); ++j)
        {
            node_mapnik::grid_line_type const& line = lines_[j];
            grid_array.Set(j, Napi::String::New(env, (char*)line.get()));
        }
        json.Set("grid", grid_array);
        json.Set("keys", keys_a);
        json.Set("data", feature_data);
        return {env.Null(), json};
    }

  private:
    grid_ptr grid_;
    std::vector<node_mapnik::grid_line_type> lines_;
    unsigned int resolution_;
    bool add_features_;
    std::vector<mapnik::grid::lookup_type> key_order_;
};

} // namespace detail

Napi::FunctionReference Grid::constructor;

Napi::Object Grid::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Grid", {
            InstanceAccessor<&Grid::key, &Grid::key>("key", prop_attr),
            InstanceMethod<&Grid::encodeSync>("encodeSync", prop_attr),
            InstanceMethod<&Grid::encode>("encode", prop_attr),
            InstanceMethod<&Grid::clearSync>("clearSync", prop_attr),
            InstanceMethod<&Grid::clear>("clear", prop_attr),
            InstanceMethod<&Grid::painted>("painted", prop_attr),
            InstanceMethod<&Grid::fields>("fields", prop_attr),
            InstanceMethod<&Grid::addField>("addField", prop_attr),
            InstanceMethod<&Grid::view>("view", prop_attr),
            InstanceMethod<&Grid::width>("width", prop_attr),
            InstanceMethod<&Grid::height>("height", prop_attr),
            StaticValue("base_mask", Napi::Number::New(env, mapnik::grid::base_mask))
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Grid", func);
    return exports;
}

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

Grid::Grid(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Grid>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 1 && info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<grid_ptr>>();
        if (ext) grid_ = *ext.Data();
        return;
    }
    if (info.Length() >= 2)
    {
        if (!info[0].IsNumber() || !info[1].IsNumber())
        {
            Napi::TypeError::New(env, "Grid 'width' and 'height' must be integers").ThrowAsJavaScriptException();
            return;
        }

        // defaults
        std::string key("__id__");

        if (info.Length() >= 3)
        {

            if (!info[2].IsObject())
            {
                Napi::TypeError::New(env, "optional third arg must be an options object").ThrowAsJavaScriptException();
                return;
            }
            Napi::Object options = info[2].As<Napi::Object>();

            if (options.Has("key"))
            {
                Napi::Value bind_opt = options.Get("key");
                if (!bind_opt.IsString())
                {
                    Napi::TypeError::New(env, "optional arg 'key' must be an string").ThrowAsJavaScriptException();
                    return;
                }

                key = bind_opt.As<Napi::String>();
            }
        }
        grid_ = std::make_shared<mapnik::grid>(info[0].As<Napi::Number>().Int32Value(),
                                               info[1].As<Napi::Number>().Int32Value(),
                                               key);
    }
    else
    {
        Napi::Error::New(env, "please provide Grid width and height").ThrowAsJavaScriptException();
    }
}

Napi::Value Grid::clearSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    grid_->clear();
    return env.Undefined();
}

Napi::Value Grid::clear(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return clearSync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function

    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto* worker = new detail::AsyncGridClear{grid_, callback_val.As<Napi::Function>()};
    worker->Queue();
    return env.Undefined();
}

Napi::Value Grid::painted(Napi::CallbackInfo const& info)
{
    return Napi::Boolean::New(info.Env(), grid_->painted());
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
    return Napi::Number::New(info.Env(), grid_->width());
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
    return Napi::Number::New(info.Env(), grid_->height());
}

Napi::Value Grid::key(Napi::CallbackInfo const& info)
{
    return Napi::String::New(info.Env(), grid_->get_key());
}

void Grid::key(Napi::CallbackInfo const& info, const Napi::Value& value)
{
    Napi::Env env = info.Env();
    if (!value.IsString())
    {
        Napi::TypeError::New(env, "key must be an string").ThrowAsJavaScriptException();
        return;
    }
    grid_->set_key(value.As<Napi::String>());
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
    Napi::Env env = info.Env();
    if (info.Length() == 1)
    {
        if (!info[0].IsString())
        {
            Napi::TypeError::New(env, "argument must be a string").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        grid_->add_field(info[0].As<Napi::String>());
    }
    else
    {
        Napi::TypeError::New(env, "one parameter, a string is required").ThrowAsJavaScriptException();
    }
    return env.Undefined();
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
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::set<std::string> const& a = grid_->get_fields();
    std::set<std::string>::const_iterator itr = a.begin();
    std::set<std::string>::const_iterator end = a.end();
    Napi::Array l = Napi::Array::New(env, a.size());
    std::size_t idx = 0;
    for (; itr != end; ++itr)
    {
        l.Set(idx++, *itr);
    }
    return scope.Escape(l);
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
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if ((info.Length() != 4) || (!info[0].IsNumber() && !info[1].IsNumber() && !info[2].IsNumber() && !info[3].IsNumber()))
    {
        Napi::TypeError::New(env, "requires 4 integer arguments: x, y, width, height").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Number x = info[0].As<Napi::Number>();
    Napi::Number y = info[1].As<Napi::Number>();
    Napi::Number w = info[2].As<Napi::Number>();
    Napi::Number h = info[3].As<Napi::Number>();
    Napi::Value grid_obj = Napi::External<grid_ptr>::New(env, &grid_);
    Napi::Object obj = GridView::constructor.New({grid_obj, x, y, w, h});
    return scope.Escape(obj);
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
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    // defaults
    unsigned int resolution = 4;
    bool add_features = true;

    // options hash
    if (info.Length() >= 1)
    {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "optional arg must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[0].As<Napi::Object>();

        if (options.Has("resolution"))
        {
            Napi::Value bind_opt = options.Get("resolution");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'resolution' must be an Integer").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            resolution = bind_opt.As<Napi::Number>().Int32Value();
            if (resolution == 0)
            {
                Napi::TypeError::New(env, "'resolution' can not be zero").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }

        if (options.Has("features"))
        {
            Napi::Value bind_opt = options.Get("features");
            if (!bind_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "'features' must be an Boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            add_features = bind_opt.As<Napi::Boolean>();
        }
    }

    try
    {

        std::vector<node_mapnik::grid_line_type> lines;
        std::vector<mapnik::grid::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid>(*grid_, lines, key_order, resolution);

        // convert key order to proper javascript array
        Napi::Array keys_a = Napi::Array::New(env, key_order.size());
        std::vector<std::string>::iterator it;
        std::size_t i;
        for (it = key_order.begin(), i = 0; it < key_order.end(); ++it, ++i)
        {
            keys_a.Set(i, Napi::String::New(env, *it));
        }

        // gather feature data
        Napi::Object feature_data = Napi::Object::New(env);
        if (add_features)
        {
            node_mapnik::write_features<mapnik::grid>(env, *grid_,
                                                      feature_data,
                                                      key_order);
        }

        // Create the return hash.
        Napi::Object json = Napi::Object::New(env);
        Napi::Array grid_array = Napi::Array::New(env, lines.size());
        for (std::size_t j = 0; j < lines.size(); ++j)
        {
            node_mapnik::grid_line_type const& line = lines[j];
            grid_array.Set(j, Napi::String::New(env, (char*)line.get()));
        }
        json.Set("grid", grid_array);
        json.Set("keys", keys_a);
        json.Set("data", feature_data);
        return scope.Escape(json);
    }
    catch (std::exception const& ex)
    {
        // There is no known exception throws in the processing above
        // so simply removing the following from coverage
        // LCOV_EXCL_START
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
        // LCOV_EXCL_STOP
    }
}

Napi::Value Grid::encode(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    // defaults
    unsigned int resolution = 4;
    bool add_features = true;

    // options hash
    if (info.Length() >= 1)
    {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "optional arg must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[0].As<Napi::Object>();

        if (options.Has("resolution"))
        {
            Napi::Value bind_opt = options.Get("resolution");
            if (!bind_opt.IsNumber())
            {
                Napi::TypeError::New(env, "'resolution' must be an Integer").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            resolution = bind_opt.As<Napi::Number>().Int32Value();
            if (resolution == 0)
            {
                Napi::TypeError::New(env, "'resolution' can not be zero").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }

        if (options.Has("features"))
        {
            Napi::Value bind_opt = options.Get("features");
            if (!bind_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "'features' must be an Boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            add_features = bind_opt.As<Napi::Boolean>();
        }
    }

    // ensure callback is a function
    if (!info[info.Length() - 1].IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Function callback = info[info.Length() - 1].As<Napi::Function>();

    auto* worker = new detail::AsyncGridEncode{grid_, resolution, add_features, callback};
    worker->Queue();
    return env.Undefined();
}

#endif
