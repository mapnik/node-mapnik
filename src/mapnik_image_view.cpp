// mapnik
#include <mapnik/color.hpp>          // for color
#include <mapnik/image_view.hpp>     // for image_view, etc
#include <mapnik/image_view_any.hpp> // for image_view_any, etc
#include <mapnik/image_util.hpp>

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_color.hpp"
#include "mapnik_palette.hpp"
#include "pixel_utils.hpp"

namespace {

struct AsyncIsSolid : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncIsSolid(image_view_ptr const& image_view, Napi::Function const& callback)
        : Base(callback),
          image_view_(image_view)
    {
    }

    void Execute() override
    {
        if (image_view_ && image_view_->width() > 0 && image_view_->height() > 0)
        {
            solid_ = mapnik::is_solid(*image_view_);
        }
        else
        {
            SetError("image_view does not have valid dimensions");
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        std::vector<napi_value> result = {env.Null(), Napi::Boolean::New(env, solid_)};
        if (solid_) result.push_back(mapnik::util::apply_visitor(detail::visitor_get_pixel<mapnik::image_view_any>(env, 0, 0), *image_view_));
        return result;
    }

  private:
    bool solid_;
    image_view_ptr image_view_;
};
} // namespace

Napi::FunctionReference ImageView::constructor;

Napi::Object ImageView::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "ImageView", {
            InstanceMethod<&ImageView::width>("width", prop_attr),
            InstanceMethod<&ImageView::height>("height", prop_attr),
            InstanceMethod<&ImageView::encodeSync>("encodeSync", prop_attr),
            InstanceMethod<&ImageView::encode>("encode", prop_attr),
            InstanceMethod<&ImageView::saveSync>("saveSync", prop_attr),
            InstanceMethod<&ImageView::isSolidSync>("isSolidSync", prop_attr),
            InstanceMethod<&ImageView::isSolid>("isSolid", prop_attr),
            InstanceMethod<&ImageView::getPixel>("getPixel", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("ImageView", func);
    return exports;
}

/**
 * This is usually not initialized directly: you'll use the `mapnik.Image#view`
 * method to instantiate an instance.
 *
 * @name mapnik.ImageView
 * @class
 * @param {number} left
 * @param {number} top
 * @param {number} width
 * @param {number} height
 * @throws {TypeError} if any argument is missing or not numeric
 * @example
 * var im = new mapnik.Image(256, 256);
 * var view = im.view(0, 0, 256, 256);
 */

ImageView::ImageView(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<ImageView>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() >= 5 && info[0].IsExternal() && info[1].IsNumber() && info[2].IsNumber() && info[3].IsNumber() && info[4].IsNumber())
    {
        std::size_t x = info[1].As<Napi::Number>().Int64Value();
        std::size_t y = info[2].As<Napi::Number>().Int64Value();
        std::size_t w = info[3].As<Napi::Number>().Int64Value();
        std::size_t h = info[4].As<Napi::Number>().Int64Value();
        auto ext = info[0].As<Napi::External<image_ptr>>();
        if (ext)
        {
            image_ = *ext.Data();
            image_view_ = std::make_shared<mapnik::image_view_any>(mapnik::create_view(*image_, x, y, w, h));
            if (info.Length() == 6 && info[5].IsBuffer())
            {
                buf_ref_ = Napi::Persistent(info[5].As<Napi::Buffer<unsigned char>>());
            }
            return;
        }
    }
    Napi::Error::New(env, "Cannot create this object from Javascript").ThrowAsJavaScriptException();
}

Napi::Value ImageView::isSolid(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return isSolidSync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto* worker = new AsyncIsSolid(image_view_, callback_val.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

Napi::Value ImageView::isSolidSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (image_view_->width() > 0 && image_view_->height() > 0)
    {
        return scope.Escape(Napi::Boolean::New(env, mapnik::is_solid(*(image_view_))));
    }

    Napi::TypeError::New(env, "image does not have valid dimensions").ThrowAsJavaScriptException();
    return env.Undefined();
}

Napi::Value ImageView::getPixel(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    int x = 0;
    int y = 0;
    bool get_color = false;
    if (info.Length() >= 3)
    {
        if (!info[2].IsObject())
        {
            Napi::TypeError::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[2].As<Napi::Object>();
        if (options.Has("get_color"))
        {
            Napi::Value bind_opt = options.Get("get_color");
            if (!bind_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "optional arg 'color' must be a boolean").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            get_color = bind_opt.As<Napi::Boolean>();
        }
    }

    if (info.Length() >= 2)
    {
        if (!info[0].IsNumber())
        {
            Napi::TypeError::New(env, "first arg, 'x' must be an integer").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        if (!info[1].IsNumber())
        {
            Napi::TypeError::New(env, "second arg, 'y' must be an integer").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        x = info[0].As<Napi::Number>().Int32Value();
        y = info[1].As<Napi::Number>().Int32Value();
    }
    else
    {
        Napi::Error::New(env, "must supply x,y to query pixel color").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (x >= 0 && x < static_cast<int>(image_view_->width()) && y >= 0 && y < static_cast<int>(image_view_->height()))
    {
        if (get_color)
        {
            Napi::EscapableHandleScope scope(env);
            mapnik::color col = mapnik::get_pixel<mapnik::color>(*image_view_, x, y);
            Napi::Value arg = Napi::External<mapnik::color>::New(env, &col);
            Napi::Object obj = Color::constructor.New({arg});
            return scope.Escape(obj);
        }
        else
        {
            detail::visitor_get_pixel<mapnik::image_view_any> visitor{env, x, y};
            return mapnik::util::apply_visitor(visitor, *image_view_);
        }
    }
    return env.Undefined();
}

Napi::Value ImageView::width(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), image_view_->width());
}

Napi::Value ImageView::height(Napi::CallbackInfo const& info)
{
    return Napi::Number::New(info.Env(), image_view_->height());
}

void ImageView::encode_common_args_(Napi::CallbackInfo const& info, std::string& format, palette_ptr& palette)
{
    Napi::Env env = info.Env();
    // accept custom format
    if (info.Length() >= 1)
    {
        if (!info[0].IsString())
        {
            Napi::TypeError::New(env, "first arg, 'format' must be a string").ThrowAsJavaScriptException();
            return;
        }
        format = info[0].As<Napi::String>();
    }
    // options
    if (info.Length() >= 2)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return;
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("palette"))
        {
            Napi::Value palette_opt = options.Get("palette");
            if (!palette_opt.IsObject())
            {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return;
            }

            Napi::Object obj = palette_opt.As<Napi::Object>();

            if (!obj.InstanceOf(Palette::constructor.Value()))
            {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return;
            }
            palette = Napi::ObjectWrap<Palette>::Unwrap(obj)->palette_;
        }
    }
}

namespace {

struct AsyncEncode : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    // ctor
    AsyncEncode(ImageView* obj, image_view_ptr image_view, palette_ptr palette, std::string const& format, Napi::Function const& callback)
        : Base(callback),
          obj_(obj),
          image_view_(image_view),
          palette_(palette),
          format_(format)
    {
    }
    ~AsyncEncode() {}
    void OnWorkComplete(Napi::Env env, napi_status status) override
    {
        if (obj_ && !obj_->IsEmpty())
        {
            obj_->Unref();
        }
        Base::OnWorkComplete(env, status);
    }
    void Execute() override
    {
        try
        {
            if (palette_)
                result_ = std::make_unique<std::string>(save_to_string(*image_view_, format_, *palette_));
            else
                result_ = std::make_unique<std::string>(save_to_string(*image_view_, format_));
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        if (result_)
        {
            std::string& str = *result_;
            auto buffer = Napi::Buffer<char>::New(
                env,
                str.empty() ? nullptr : &str[0],
                str.size(),
                [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                    if (str_ptr != nullptr)
                    {
                        Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                    }
                    delete str_ptr;
                },
                result_.release());
            Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(str.size()));
            return {env.Null(), buffer};
        }
        return Base::GetResult(env);
    }

  private:
    ImageView* obj_;
    image_view_ptr image_view_;
    palette_ptr palette_;
    std::string format_;
    std::unique_ptr<std::string> result_;
};

} // namespace

Napi::Value ImageView::encodeSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::string format{"png"};
    palette_ptr palette;
    encode_common_args_(info, format, palette);
    try
    {
        std::unique_ptr<std::string> result;
        if (palette)
            result = std::make_unique<std::string>(save_to_string(*image_, format, *palette));
        else
            result = std::make_unique<std::string>(save_to_string(*image_, format));
        std::string& str = *result;
        auto buffer = Napi::Buffer<char>::New(
            env,
            str.empty() ? nullptr : &str[0],
            str.size(),
            [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                if (str_ptr != nullptr)
                {
                    Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                }
                delete str_ptr;
            },
            result.release());
        Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(str.size()));
        return scope.Escape(buffer);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value ImageView::encode(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    std::string format = "png";
    palette_ptr palette;

    encode_common_args_(info, format, palette);
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Function callback = callback_val.As<Napi::Function>();
    this->Ref();
    auto* worker = new AsyncEncode{this, image_view_, palette, format, callback};
    worker->Queue();
    return env.Undefined();
}

void ImageView::saveSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 0 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "filename required to save file").ThrowAsJavaScriptException();
        return;
    }

    std::string filename = info[0].As<Napi::String>();
    std::string format("");

    if (info.Length() >= 2)
    {
        if (!info[1].IsString())
        {
            Napi::TypeError::New(env, "both 'filename' and 'format' arguments must be strings").ThrowAsJavaScriptException();
            return;
        }
        format = info[1].As<Napi::String>();
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>")
        {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return;
        }
    }
    try
    {
        mapnik::save_to_file(*image_view_, filename, format);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
}
