#if defined(GRID_RENDERER)

// mapnik
#include <mapnik/grid/grid.hpp>      // for hit_grid<>::lookup_type, etc
#include <mapnik/grid/grid_view.hpp> // for grid_view, hit_grid_view, etc

#include "mapnik_grid_view.hpp"
#include "mapnik_grid.hpp"
#include "js_grid_utils.hpp"
#include "utils.hpp"

namespace {

struct AsyncIsSolid : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncIsSolid(grid_view_ptr const& grid_view, Napi::Function const& callback)
        : Base(callback),
          grid_view_(grid_view)
    {
    }

    void Execute() override
    {
        if (grid_view_ && grid_view_->width() > 0 && grid_view_->height() > 0)
        {
            mapnik::grid_view::value_type first_pixel = grid_view_->get_row(0)[0];
            pixel_ = first_pixel;
            for (unsigned y = 0; y < grid_view_->height(); ++y)
            {
                mapnik::grid_view::value_type const* row = grid_view_->get_row(y);
                for (unsigned x = 0; x < grid_view_->width(); ++x)
                {
                    if (first_pixel != row[x])
                    {
                        solid_ = false;
                        return;
                    }
                }
            }
        }
        else
        {
            SetError("grid_view does not have valid dimensions");
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        std::vector<napi_value> result = {env.Null(), Napi::Boolean::New(env, solid_)};
        if (solid_) result.push_back(Napi::Number::New(env, pixel_));
        return result;
    }

  private:
    bool solid_ = true;
    mapnik::grid_view::value_type pixel_;
    grid_view_ptr grid_view_;
};

struct AsyncGridViewEncode : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    // ctor
    AsyncGridViewEncode(grid_view_ptr grid_view, unsigned resolution, bool add_features, Napi::Function const& callback)
        : Base(callback),
          grid_view_(grid_view),
          resolution_(resolution),
          add_features_(add_features)
    {
    }
    void Execute() override
    {
        try
        {
            node_mapnik::grid2utf<mapnik::grid_view>(*grid_view_,
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

        mapnik::grid_view const& grid_view_type = *grid_view_;
        // gather feature data
        Napi::Object feature_data = Napi::Object::New(env);
        if (add_features_)
        {
            node_mapnik::write_features<mapnik::grid_view>(env, grid_view_type,
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
    grid_view_ptr grid_view_;
    std::vector<node_mapnik::grid_line_type> lines_;
    unsigned int resolution_;
    bool add_features_;
    std::vector<mapnik::grid::lookup_type> key_order_;
};

} // namespace

Napi::FunctionReference GridView::constructor;

Napi::Object GridView::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "GridView", {
            InstanceMethod<&GridView::encodeSync>("encodeSync", prop_attr),
            InstanceMethod<&GridView::encode>("encode", prop_attr),
            InstanceMethod<&GridView::fields>("fields", prop_attr),
            InstanceMethod<&GridView::width>("width", prop_attr),
            InstanceMethod<&GridView::height>("height", prop_attr),
            InstanceMethod<&GridView::isSolid>("isSolid", prop_attr),
            InstanceMethod<&GridView::isSolidSync>("isSolidSync", prop_attr),
            InstanceMethod<&GridView::getPixel>("getPixel", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("GridView", func);
    return exports;
}

GridView::GridView(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<GridView>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 5 && info[0].IsExternal() && info[1].IsNumber() && info[2].IsNumber() && info[3].IsNumber() && info[4].IsNumber())
    {
        std::size_t x = info[1].As<Napi::Number>().Int64Value();
        std::size_t y = info[2].As<Napi::Number>().Int64Value();
        std::size_t w = info[3].As<Napi::Number>().Int64Value();
        std::size_t h = info[4].As<Napi::Number>().Int64Value();

        auto ext = info[0].As<Napi::External<grid_ptr>>();
        if (ext)
        {
            grid_ = *ext.Data();
            grid_view_ = std::make_shared<mapnik::grid_view>(grid_->get_view(x, y, w, h));
            return;
        }
    }
    Napi::Error::New(env, "Cannot create this object from Javascript").ThrowAsJavaScriptException();
}

Napi::Value GridView::width(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), grid_view_->width());
}

Napi::Value GridView::height(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), grid_view_->height());
}

Napi::Value GridView::fields(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::set<std::string> const& a = grid_view_->get_fields();
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

Napi::Value GridView::isSolid(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return isSolidSync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!callback.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto* worker = new AsyncIsSolid(grid_view_, callback.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

Napi::Value GridView::isSolidSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if (grid_view_->width() > 0 && grid_view_->height() > 0)
    {
        mapnik::grid_view::value_type first_pixel = grid_view_->get_row(0)[0];
        for (std::size_t y = 0; y < grid_view_->height(); ++y)
        {
            mapnik::grid_view::value_type const* row = grid_view_->get_row(y);
            for (std::size_t x = 0; x < grid_view_->width(); ++x)
            {
                if (first_pixel != row[x])
                {
                    return scope.Escape(Napi::Boolean::New(env, false));
                }
            }
        }
    }
    return scope.Escape(Napi::Boolean::New(env, true));
}

Napi::Value GridView::getPixel(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    int x = 0;
    int y = 0;

    if (info.Length() >= 2)
    {
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
    }
    else
    {
        Napi::TypeError::New(env, "must supply x,y to query pixel color").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (x >= 0 && x < static_cast<int>(grid_view_->width()) &&
        y >= 0 && y < static_cast<int>(grid_view_->height()))
    {
        mapnik::grid_view::value_type pixel = grid_view_->get_row(y)[x];
        return scope.Escape(Napi::Number::New(env, pixel));
    }
    return env.Undefined();
}

Napi::Value GridView::encodeSync(Napi::CallbackInfo const& info)
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
        std::vector<mapnik::grid_view::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid_view>(*grid_view_, lines, key_order, resolution);

        // convert key order to proper javascript array
        Napi::Array keys_a = Napi::Array::New(env, key_order.size());
        std::vector<std::string>::iterator it;
        std::size_t i;
        for (it = key_order.begin(), i = 0; it != key_order.end(); ++it, ++i)
        {
            keys_a.Set(i, Napi::String::New(env, *it));
        }

        mapnik::grid_view const& grid_type = *grid_view_;

        // gather feature data
        Napi::Object feature_data = Napi::Object::New(env);
        if (add_features)
        {
            node_mapnik::write_features<mapnik::grid_view>(env, grid_type,
                                                           feature_data,
                                                           key_order);
        }

        // Create the return hash.
        Napi::Object json = Napi::Object::New(env);
        Napi::Array grid_array = Napi::Array::New(env);
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

/*
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
*/

Napi::Value GridView::encode(Napi::CallbackInfo const& info)
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

    auto* worker = new AsyncGridViewEncode{grid_view_, resolution, add_features, callback};
    worker->Queue();
    return env.Undefined();
}

#endif
