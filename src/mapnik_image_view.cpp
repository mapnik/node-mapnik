// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image_view.hpp>        // for image_view, etc
#include <mapnik/image_view_any.hpp>    // for image_view_any, etc
#include <mapnik/image_util.hpp>

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_color.hpp"
#include "mapnik_palette.hpp"
//#include "utils.hpp"
#include "pixel_utils.hpp"
// std
//#include <exception>


namespace {

struct AsyncIsSolid : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncIsSolid(image_view_ptr const& image_view, Napi::Function const& callback)
        : Base(callback),
          image_view_(image_view)
    {}

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
}

Napi::FunctionReference ImageView::constructor;

Napi::Object ImageView::Initialize(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "ImageView", {
            InstanceMethod<&ImageView::width>("width"),
            InstanceMethod<&ImageView::height>("height"),
            //InstanceMethod<&ImageView::encodeSync>("encodeSync"),
            //InstanceMethod<&ImageView::encode>("encode"),
            //InstanceMethod<&ImageView::save>("save"),
            InstanceMethod<&ImageView::isSolidSync>("isSolidSync"),
            InstanceMethod<&ImageView::isSolid>("isSolid"),
            InstanceMethod<&ImageView::getPixel>("getPixel")
        });
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
    :Napi::ObjectWrap<ImageView>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 5 && info[0].IsExternal()
        && info[1].IsNumber() && info[2].IsNumber()
        && info[3].IsNumber() && info[4].IsNumber())
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
            return;
        }
    }
    Napi::Error::New(env, "Cannot create this object from Javascript").ThrowAsJavaScriptException();
}

/*
typedef struct {
    uv_work_t request;
    ImageView* im;
    Napi::FunctionReference cb;
    bool error;
    std::string error_name;
    bool result;
} is_solid_image_view_baton_t;
*/


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

    auto * worker = new AsyncIsSolid(image_view_, callback_val.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}
/*
void ImageView::EIO_IsSolid(uv_work_t* req)
{
    is_solid_image_view_baton_t *closure = static_cast<is_solid_image_view_baton_t *>(req->data);
    if (closure->im->this_->width() > 0 && closure->im->this_->height() > 0)
    {
        closure->result = mapnik::is_solid(*(closure->im->this_));
    }
    else
    {
        closure->error = true;
        closure->error_name = "image does not have valid dimensions";
    }
}

void ImageView::EIO_AfterIsSolid(uv_work_t* req)
{
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    is_solid_image_view_baton_t *closure = static_cast<is_solid_image_view_baton_t *>(req->data);
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
                                     mapnik::util::apply_visitor(visitor_get_pixel_view(0,0),*(closure->im->this_)),
            };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 3, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), Napi::New(env, closure->result) };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

*/

Napi::Value ImageView::isSolidSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (image_view_->width() > 0 && image_view_->height() > 0)
    {
        return scope.Escape(Napi::Boolean::New(env, mapnik::is_solid(*(image_view_))));
    }

    Napi::TypeError::New(env, "image does not have valid dimensions").ThrowAsJavaScriptException();
    return scope.Escape(env.Undefined());
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

    if (x >= 0 && x < static_cast<int>(image_view_->width())
        && y >= 0 && y < static_cast<int>(image_view_->height()))
    {
        if (get_color)
        {
            Napi::EscapableHandleScope scope(env);
            mapnik::color col = mapnik::get_pixel<mapnik::color>(*image_view_, x, y);
            Napi::Value arg = Napi::External<mapnik::color>::New(env, &col);
            Napi::Object obj = Color::constructor.New({arg});
            return scope.Escape(napi_value(obj)).ToObject();
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

/*
Napi::Value ImageView::encodeSync(Napi::CallbackInfo const& info)
{
    ImageView* im = info.Holder().Unwrap<ImageView>();

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() >= 1) {
        if (!info[0].IsString()) {
            Napi::TypeError::New(env, "first arg, 'format' must be a string").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        format = TOSTR(info[0]);
    }

    // options hash
    if (info.Length() >= 2) {
        if (!info[1].IsObject()) {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        if ((options).Has(Napi::String::New(env, "palette")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "palette"));
            if (!format_opt.IsObject()) {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            Napi::Object obj = format_opt.As<Napi::Object>();
            if (!Napi::New(env, Palette::constructor)->HasInstance(obj)) {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            palette = obj)->palette(.Unwrap<Palette>();
        }
    }

    try {
        std::string s;
        if (palette.get())
        {
            s = save_to_string(*(im->this_), format, *palette);
        }
        else {
            s = save_to_string(*(im->this_), format);
        }

        return Napi::Buffer::Copy(env, (char*)s.data(),s.size());
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}


typedef struct {
    uv_work_t request;
    ImageView* im;
    std::string format;
    palette_ptr palette;
    std::string error_name;
    Napi::FunctionReference cb;
    std::string result;
} encode_image_view_baton_t;


Napi::Value ImageView::encode(Napi::CallbackInfo const& info)
{
    ImageView* im = info.Holder().Unwrap<ImageView>();

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() > 1){
        if (!info[0].IsString()) {
            Napi::TypeError::New(env, "first arg, 'format' must be a string").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        format = TOSTR(info[0]);
    }

    // options hash
    if (info.Length() >= 2) {
        if (!info[1].IsObject()) {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        if ((options).Has(Napi::String::New(env, "palette")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "palette"));
            if (!format_opt.IsObject()) {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            Napi::Object obj = format_opt.As<Napi::Object>();
            if (!Napi::New(env, Palette::constructor)->HasInstance(obj)) {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            palette = obj)->palette(.Unwrap<Palette>();
        }
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    encode_image_view_baton_t *baton = new encode_image_view_baton_t();
    baton->request.data = baton;
    baton->im = im;
    baton->format = format;
    baton->palette = palette;
    baton->cb.Reset(callback.As<Napi::Function>());
    uv_queue_work(uv_default_loop(), &baton->request, AsyncEncode, (uv_after_work_cb)AfterEncode);
    im->Ref();
    return;
}

void ImageView::AsyncEncode(uv_work_t* req)
{
    encode_image_view_baton_t *baton = static_cast<encode_image_view_baton_t *>(req->data);

    try {
        if (baton->palette.get())
        {
            baton->result = save_to_string(*(baton->im->this_), baton->format, *baton->palette);
        }
        else
        {
            baton->result = save_to_string(*(baton->im->this_), baton->format);
        }
    }
    catch (std::exception const& ex)
    {
        baton->error_name = ex.what();
    }
}

void ImageView::AfterEncode(uv_work_t* req)
{
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);

    encode_image_view_baton_t *baton = static_cast<encode_image_view_baton_t *>(req->data);

    if (!baton->error_name.empty()) {
        Napi::Value argv[1] = { Napi::Error::New(env, baton->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, baton->cb), 1, argv);
    }
    else
    {
        Napi::Value argv[2] = { env.Null(), Napi::Buffer::Copy(env, (char*)baton->result.data(), baton->result.size()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, baton->cb), 2, argv);
    }

    baton->im->Unref();
    baton->cb.Reset();
    delete baton;
}


Napi::Value ImageView::save(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0 || !info[0].IsString()){
        Napi::TypeError::New(env, "filename required").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::string filename = TOSTR(info[0]);

    std::string format("");

    if (info.Length() >= 2) {
        if (!info[1].IsString()) {
            Napi::TypeError::New(env, "both 'filename' and 'format' arguments must be strings").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        format = mapnik::guess_type(TOSTR(info[1]));
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    ImageView* im = info.Holder().Unwrap<ImageView>();
    try
    {
        save_to_file(*im->this_,filename);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

    }
    return;
}
*/
