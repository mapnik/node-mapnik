// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image_view.hpp>        // for image_view, etc
#include <mapnik/image_view_any.hpp>    // for image_view_any, etc
#include <mapnik/image_util.hpp>

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_color.hpp"
#include "mapnik_palette.hpp"
#include "utils.hpp"

// std
#include <exception>

Napi::FunctionReference ImageView::constructor;

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
void ImageView::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, ImageView::New);

    lcons->SetClassName(Napi::String::New(env, "ImageView"));

    InstanceMethod("encodeSync", &encodeSync),
    InstanceMethod("encode", &encode),
    InstanceMethod("save", &save),
    InstanceMethod("width", &width),
    InstanceMethod("height", &height),
    InstanceMethod("isSolid", &isSolid),
    InstanceMethod("isSolidSync", &isSolidSync),
    InstanceMethod("getPixel", &getPixel),

    (target).Set(Napi::String::New(env, "ImageView"),Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}


ImageView::ImageView(Image * JSImage) : Napi::ObjectWrap<ImageView>(),
    this_(),
    JSImage_(JSImage) {
        JSImage_->Ref();
    }

ImageView::~ImageView()
{
    JSImage_->Unref();
}

Napi::Value ImageView::New(const Napi::CallbackInfo& info)
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
        ImageView* im =  static_cast<ImageView*>(ptr);
        im->Wrap(info.This());
        return info.This();
        return;
    } else {
        Napi::Error::New(env, "Cannot create this object from Javascript").ThrowAsJavaScriptException();
        return env.Null();
    }
    return;
}

Napi::Value ImageView::NewInstance(Image * JSImage ,
                             unsigned x,
                             unsigned y,
                             unsigned w,
                             unsigned h
    )
{
    Napi::EscapableHandleScope scope(env);
    ImageView* imv = new ImageView(JSImage);
    imv->this_ = std::make_shared<mapnik::image_view_any>(mapnik::create_view(*(JSImage->get()),x,y,w,h));
    Napi::Value ext = Napi::External::New(env, imv);
    Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
    if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new ImageView instance").ThrowAsJavaScriptException();

    return scope.Escape(maybe_local);
}

typedef struct {
    uv_work_t request;
    ImageView* im;
    Napi::FunctionReference cb;
    bool error;
    std::string error_name;
    bool result;
} is_solid_image_view_baton_t;

Napi::Value ImageView::isSolid(const Napi::CallbackInfo& info)
{
    ImageView* im = info.Holder().Unwrap<ImageView>();

    if (info.Length() == 0) {
        return _isSolidSync(info);
        return;
    }
    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    is_solid_image_view_baton_t *closure = new is_solid_image_view_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->result = true;
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    im->Ref();
    return;
}

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

struct visitor_get_pixel_view
{
    visitor_get_pixel_view(int x, int y)
        : x_(x), y_(y) {}

    Napi::Value operator() (mapnik::image_view_null const& data)
    {
        // This should never be reached because the width and height of 0 for a null
        // image will prevent the visitor from being called.
        /* LCOV_EXCL_START */
        Napi::EscapableHandleScope scope(env);
        return scope.Escape(env.Undefined());
        /* LCOV_EXCL_STOP */
    }

    Napi::Value operator() (mapnik::image_view_gray8 const& data)
    {
        Napi::EscapableHandleScope scope(env);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Uint32::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_gray8s const& data)
    {
        Napi::EscapableHandleScope scope(env);
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Napi::Int32::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_gray16 const& data)
    {
        Napi::EscapableHandleScope scope(env);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Uint32::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_gray16s const& data)
    {
        Napi::EscapableHandleScope scope(env);
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Napi::Int32::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_gray32 const& data)
    {
        Napi::EscapableHandleScope scope(env);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Uint32::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_gray32s const& data)
    {
        Napi::EscapableHandleScope scope(env);
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Napi::Int32::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_gray32f const& data)
    {
        Napi::EscapableHandleScope scope(env);
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_gray64 const& data)
    {
        Napi::EscapableHandleScope scope(env);
        std::uint64_t val = mapnik::get_pixel<std::uint64_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_gray64s const& data)
    {
        Napi::EscapableHandleScope scope(env);
        std::int64_t val = mapnik::get_pixel<std::int64_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_gray64f const& data)
    {
        Napi::EscapableHandleScope scope(env);
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env, val));
    }

    Napi::Value operator() (mapnik::image_view_rgba8 const& data)
    {
        Napi::EscapableHandleScope scope(env);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env, val));
    }

  private:
    int x_;
    int y_;

};

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


Napi::Value ImageView::isSolidSync(const Napi::CallbackInfo& info)
{
    return _isSolidSync(info);
}

Napi::Value ImageView::_isSolidSync(const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);
    ImageView* im = info.Holder().Unwrap<ImageView>();
    if (im->this_->width() > 0 && im->this_->height() > 0)
    {
        return scope.Escape(Napi::Boolean::New(env, mapnik::is_solid(*(im->this_))));
    }
    else
    {
        Napi::TypeError::New(env, "image does not have valid dimensions").ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }
}


Napi::Value ImageView::getPixel(const Napi::CallbackInfo& info)
{
    int x = 0;
    int y = 0;
    bool get_color = false;

    if (info.Length() >= 3) {

        if (!info[2].IsObject()) {
            Napi::TypeError::New(env, "optional third argument must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[2].ToObject(Napi::GetCurrentContext());

        if ((options).Has(Napi::String::New(env, "get_color")).FromMaybe(false)) {
            Napi::Value bind_opt = (options).Get(Napi::String::New(env, "get_color"));
            if (!bind_opt->IsBoolean()) {
                Napi::TypeError::New(env, "optional arg 'color' must be a boolean").ThrowAsJavaScriptException();
                return env.Null();
            }
            get_color = bind_opt.As<Napi::Boolean>().Value();
        }

    }

    if (info.Length() >= 2) {
        if (!info[0].IsNumber()) {
            Napi::TypeError::New(env, "first arg, 'x' must be an integer").ThrowAsJavaScriptException();
            return env.Null();
        }
        if (!info[1].IsNumber()) {
            Napi::TypeError::New(env, "second arg, 'y' must be an integer").ThrowAsJavaScriptException();
            return env.Null();
        }
        x = info[0].As<Napi::Number>().Int32Value();
        y = info[1].As<Napi::Number>().Int32Value();
    } else {
        Napi::TypeError::New(env, "must supply x,y to query pixel color").ThrowAsJavaScriptException();
        return env.Null();
    }

    ImageView* im = info.Holder().Unwrap<ImageView>();
    if (x >= 0 && x < static_cast<int>(im->this_->width())
        && y >=0 && y < static_cast<int>(im->this_->height()))
    {
        if (get_color)
        {
            mapnik::color val = mapnik::get_pixel<mapnik::color>(*im->this_, x, y);
            return Color::NewInstance(val);
        } else {
            visitor_get_pixel_view visitor(x, y);
            return mapnik::util::apply_visitor(visitor, *im->this_);
        }
    }
    return;
}


Napi::Value ImageView::width(const Napi::CallbackInfo& info)
{
    ImageView* im = info.Holder().Unwrap<ImageView>();
    return Napi::Int32::New(env, static_cast<std::int32_t>(im->this_->width()));
}

Napi::Value ImageView::height(const Napi::CallbackInfo& info)
{
    ImageView* im = info.Holder().Unwrap<ImageView>();
    return Napi::Int32::New(env, static_cast<std::int32_t>(im->this_->height()));
}


Napi::Value ImageView::encodeSync(const Napi::CallbackInfo& info)
{
    ImageView* im = info.Holder().Unwrap<ImageView>();

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() >= 1) {
        if (!info[0].IsString()) {
            Napi::TypeError::New(env, "first arg, 'format' must be a string").ThrowAsJavaScriptException();
            return env.Null();
        }
        format = TOSTR(info[0]);
    }

    // options hash
    if (info.Length() >= 2) {
        if (!info[1].IsObject()) {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        if ((options).Has(Napi::String::New(env, "palette")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "palette"));
            if (!format_opt.IsObject()) {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Null();
            }

            Napi::Object obj = format_opt.As<Napi::Object>();
            if (obj->IsNull() || obj->IsUndefined() || !Napi::New(env, Palette::constructor)->HasInstance(obj)) {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Null();
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
        return env.Null();
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


Napi::Value ImageView::encode(const Napi::CallbackInfo& info)
{
    ImageView* im = info.Holder().Unwrap<ImageView>();

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() > 1){
        if (!info[0].IsString()) {
            Napi::TypeError::New(env, "first arg, 'format' must be a string").ThrowAsJavaScriptException();
            return env.Null();
        }
        format = TOSTR(info[0]);
    }

    // options hash
    if (info.Length() >= 2) {
        if (!info[1].IsObject()) {
            Napi::TypeError::New(env, "optional second arg must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Object options = info[1].As<Napi::Object>();

        if ((options).Has(Napi::String::New(env, "palette")).FromMaybe(false))
        {
            Napi::Value format_opt = (options).Get(Napi::String::New(env, "palette"));
            if (!format_opt.IsObject()) {
                Napi::TypeError::New(env, "'palette' must be an object").ThrowAsJavaScriptException();
                return env.Null();
            }

            Napi::Object obj = format_opt.As<Napi::Object>();
            if (obj->IsNull() || obj->IsUndefined() || !Napi::New(env, Palette::constructor)->HasInstance(obj)) {
                Napi::TypeError::New(env, "mapnik.Palette expected as second arg").ThrowAsJavaScriptException();
                return env.Null();
            }

            palette = obj)->palette(.Unwrap<Palette>();
        }
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
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


Napi::Value ImageView::save(const Napi::CallbackInfo& info)
{
    if (info.Length() == 0 || !info[0].IsString()){
        Napi::TypeError::New(env, "filename required").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string filename = TOSTR(info[0]);

    std::string format("");

    if (info.Length() >= 2) {
        if (!info[1].IsString()) {
            Napi::TypeError::New(env, "both 'filename' and 'format' arguments must be strings").ThrowAsJavaScriptException();
            return env.Null();
        }

        format = mapnik::guess_type(TOSTR(info[1]));
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
            return env.Null();
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
