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

Nan::Persistent<v8::FunctionTemplate> ImageView::constructor;

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
void ImageView::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(ImageView::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("ImageView").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "encodeSync", encodeSync);
    Nan::SetPrototypeMethod(lcons, "encode", encode);
    Nan::SetPrototypeMethod(lcons, "save", save);
    Nan::SetPrototypeMethod(lcons, "width", width);
    Nan::SetPrototypeMethod(lcons, "height", height);
    Nan::SetPrototypeMethod(lcons, "isSolid", isSolid);
    Nan::SetPrototypeMethod(lcons, "isSolidSync", isSolidSync);
    Nan::SetPrototypeMethod(lcons, "getPixel", getPixel);

    Nan::Set(target, Nan::New("ImageView").ToLocalChecked(),Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}


ImageView::ImageView(Image * JSImage) :
    Nan::ObjectWrap(),
    this_(),
    JSImage_(JSImage) {
        JSImage_->Ref();
    }

ImageView::~ImageView()
{
    JSImage_->Unref();
}

NAN_METHOD(ImageView::New)
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
        ImageView* im =  static_cast<ImageView*>(ptr);
        im->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    } else {
        Nan::ThrowError("Cannot create this object from Javascript");
        return;
    }
    return;
}

v8::Local<v8::Value> ImageView::NewInstance(Image * JSImage ,
                             unsigned x,
                             unsigned y,
                             unsigned w,
                             unsigned h
    )
{
    Nan::EscapableHandleScope scope;
    ImageView* imv = new ImageView(JSImage);
    imv->this_ = std::make_shared<mapnik::image_view_any>(mapnik::create_view(*(JSImage->get()),x,y,w,h));
    v8::Local<v8::Value> ext = Nan::New<v8::External>(imv);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new ImageView instance");
    return scope.Escape(maybe_local.ToLocalChecked());
}

typedef struct {
    uv_work_t request;
    ImageView* im;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
    bool result;
} is_solid_image_view_baton_t;

NAN_METHOD(ImageView::isSolid)
{
    ImageView* im = Nan::ObjectWrap::Unwrap<ImageView>(info.Holder());

    if (info.Length() == 0) {
        info.GetReturnValue().Set(_isSolidSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    is_solid_image_view_baton_t *closure = new is_solid_image_view_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->result = true;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
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

    v8::Local<v8::Value> operator() (mapnik::image_view_null const& data)
    {
        // This should never be reached because the width and height of 0 for a null
        // image will prevent the visitor from being called.
        /* LCOV_EXCL_START */
        Nan::EscapableHandleScope scope;
        return scope.Escape(Nan::Undefined());
        /* LCOV_EXCL_STOP */
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray8 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Uint32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray8s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Int32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray16 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Uint32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray16s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Int32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray32 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Uint32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray32s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Int32>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray32f const& data)
    {
        Nan::EscapableHandleScope scope;
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray64 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint64_t val = mapnik::get_pixel<std::uint64_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray64s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int64_t val = mapnik::get_pixel<std::int64_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_gray64f const& data)
    {
        Nan::EscapableHandleScope scope;
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    v8::Local<v8::Value> operator() (mapnik::image_view_rgba8 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

  private:
    int x_;
    int y_;

};

void ImageView::EIO_AfterIsSolid(uv_work_t* req)
{
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);
    is_solid_image_view_baton_t *closure = static_cast<is_solid_image_view_baton_t *>(req->data);
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
                                     mapnik::util::apply_visitor(visitor_get_pixel_view(0,0),*(closure->im->this_)),
            };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 3, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::New(closure->result) };
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}


NAN_METHOD(ImageView::isSolidSync)
{
    info.GetReturnValue().Set(_isSolidSync(info));
}

v8::Local<v8::Value> ImageView::_isSolidSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    ImageView* im = Nan::ObjectWrap::Unwrap<ImageView>(info.Holder());
    if (im->this_->width() > 0 && im->this_->height() > 0)
    {
        return scope.Escape(Nan::New<v8::Boolean>(mapnik::is_solid(*(im->this_))));
    }
    else
    {
        Nan::ThrowTypeError("image does not have valid dimensions");
        return scope.Escape(Nan::Undefined());
    }
}


NAN_METHOD(ImageView::getPixel)
{
    int x = 0;
    int y = 0;
    bool get_color = false;

    if (info.Length() >= 3) {

        if (!info[2]->IsObject()) {
            Nan::ThrowTypeError("optional third argument must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

        if (Nan::Has(options, Nan::New("get_color").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> bind_opt = Nan::Get(options, Nan::New("get_color").ToLocalChecked()).ToLocalChecked();
            if (!bind_opt->IsBoolean()) {
                Nan::ThrowTypeError("optional arg 'color' must be a boolean");
                return;
            }
            get_color = Nan::To<bool>(bind_opt).FromJust();
        }

    }

    if (info.Length() >= 2) {
        if (!info[0]->IsNumber()) {
            Nan::ThrowTypeError("first arg, 'x' must be an integer");
            return;
        }
        if (!info[1]->IsNumber()) {
            Nan::ThrowTypeError("second arg, 'y' must be an integer");
            return;
        }
        x = Nan::To<int>(info[0]).FromJust();
        y = Nan::To<int>(info[1]).FromJust();
    } else {
        Nan::ThrowTypeError("must supply x,y to query pixel color");
        return;
    }

    ImageView* im = Nan::ObjectWrap::Unwrap<ImageView>(info.Holder());
    if (x >= 0 && x < static_cast<int>(im->this_->width())
        && y >=0 && y < static_cast<int>(im->this_->height()))
    {
        if (get_color)
        {
            mapnik::color val = mapnik::get_pixel<mapnik::color>(*im->this_, x, y);
            info.GetReturnValue().Set(Color::NewInstance(val));
        } else {
            visitor_get_pixel_view visitor(x, y);
            info.GetReturnValue().Set(mapnik::util::apply_visitor(visitor, *im->this_));
        }
    }
    return;
}


NAN_METHOD(ImageView::width)
{
    ImageView* im = Nan::ObjectWrap::Unwrap<ImageView>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<std::int32_t>(im->this_->width())));
}

NAN_METHOD(ImageView::height)
{
    ImageView* im = Nan::ObjectWrap::Unwrap<ImageView>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<std::int32_t>(im->this_->height())));
}


NAN_METHOD(ImageView::encodeSync)
{
    ImageView* im = Nan::ObjectWrap::Unwrap<ImageView>(info.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() >= 1) {
        if (!info[0]->IsString()) {
            Nan::ThrowTypeError("first arg, 'format' must be a string");
            return;
        }
        format = TOSTR(info[0]);
    }

    // options hash
    if (info.Length() >= 2) {
        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[1].As<v8::Object>();

        if (Nan::Has(options, Nan::New("palette").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("palette").ToLocalChecked()).ToLocalChecked();
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt.As<v8::Object>();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }

            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
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

        info.GetReturnValue().Set(Nan::CopyBuffer((char*)s.data(),s.size()).ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
}

typedef struct {
    uv_work_t request;
    ImageView* im;
    std::string format;
    palette_ptr palette;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    std::string result;
} encode_image_view_baton_t;


NAN_METHOD(ImageView::encode)
{
    ImageView* im = Nan::ObjectWrap::Unwrap<ImageView>(info.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() > 1){
        if (!info[0]->IsString()) {
            Nan::ThrowTypeError("first arg, 'format' must be a string");
            return;
        }
        format = TOSTR(info[0]);
    }

    // options hash
    if (info.Length() >= 2) {
        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[1].As<v8::Object>();

        if (Nan::Has(options, Nan::New("palette").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> format_opt = Nan::Get(options, Nan::New("palette").ToLocalChecked()).ToLocalChecked();
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt.As<v8::Object>();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }

            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    encode_image_view_baton_t *baton = new encode_image_view_baton_t();
    baton->request.data = baton;
    baton->im = im;
    baton->format = format;
    baton->palette = palette;
    baton->cb.Reset(callback.As<v8::Function>());
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
    Nan::HandleScope scope;
    Nan::AsyncResource async_resource(__func__);

    encode_image_view_baton_t *baton = static_cast<encode_image_view_baton_t *>(req->data);

    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(baton->error_name.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::CopyBuffer((char*)baton->result.data(), baton->result.size()).ToLocalChecked() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->cb), 2, argv);
    }

    baton->im->Unref();
    baton->cb.Reset();
    delete baton;
}


NAN_METHOD(ImageView::save)
{
    if (info.Length() == 0 || !info[0]->IsString()){
        Nan::ThrowTypeError("filename required");
        return;
    }

    std::string filename = TOSTR(info[0]);

    std::string format("");

    if (info.Length() >= 2) {
        if (!info[1]->IsString()) {
            Nan::ThrowTypeError("both 'filename' and 'format' arguments must be strings");
            return;
        }

        format = mapnik::guess_type(TOSTR(info[1]));
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Nan::ThrowError(s.str().c_str());
            return;
        }
    }

    ImageView* im = Nan::ObjectWrap::Unwrap<ImageView>(info.Holder());
    try
    {
        save_to_file(*im->this_,filename);
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
    }
    return;
}
