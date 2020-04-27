#include "mapnik_image.hpp"
#include "mapnik_color.hpp"
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc


namespace {

// AsyncWorker

}


/**
 * Fill this image with a given color. Changes all pixel values. (synchronous)
 *
 * @name fillSync
 * @instance
 * @memberof Image
 * @param {mapnik.Color|number} color
 * @example
 * var img = new mapnik.Image(5,5);
 * // blue pixels
 * img.fillSync(new mapnik.Color('blue'));
 * var colors = img.getPixel(0,0, {get_color: true});
 * // blue value is filled
 * console.log(colors.b); // 255
 */

Napi::Value Image::fillSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    if (info.Length() < 1 )
    {
        Napi::TypeError::New(env, "expects one argument: Color object or a number").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    try
    {
        if (info[0].IsNumber())
        {
            std::uint32_t val = info[0].As<Napi::Number>().Int32Value();
            mapnik::fill<std::uint32_t>(*image_,val);
        }
        else if (info[0].IsObject())
        {
            Napi::Object obj = info[0].As<Napi::Object>();
            if (obj.IsNull() || obj.IsUndefined() || !obj.InstanceOf(Color::constructor.Value()))
            {
                Napi::TypeError::New(env, "A numeric or color value is expected").ThrowAsJavaScriptException();
            }
            else
            {
                Color * color = Napi::ObjectWrap<Color>::Unwrap(obj);
                mapnik::fill(*image_, color->color_);
            }
        }
        else
        {
            Napi::TypeError::New(env, "A numeric or color value is expected").ThrowAsJavaScriptException();

        }
    }
    catch(std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();

    }
    return env.Undefined();
}

enum fill_type : std::uint8_t
{
    FILL_COLOR = 0,
    FILL_UINT32,
    FILL_INT32,
    FILL_DOUBLE
};

/*
typedef struct {
    uv_work_t request;
    Image* im;
    fill_type type;
    mapnik::color c;
    std::uint32_t val_u32;
    std::int32_t val_32;
    double val_double;
    //std::string format;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
} fill_image_baton_t;
*/

/**
 * Fill this image with a given color. Changes all pixel values.
 *
 * @name fill
 * @instance
 * @memberof Image
 * @param {mapnik.Color|number} color
 * @param {Function} callback - `function(err, img)`
 * @example
 * var img = new mapnik.Image(5,5);
 * img.fill(new mapnik.Color('blue'), function(err, img) {
 *   if (err) throw err;
 *   var colors = img.getPixel(0,0, {get_color: true});
 *   pixel is colored blue
 *   console.log(color.b); // 255
 * });
 *
 * // or fill with rgb string
 * img.fill('rgba(255,255,255,0)', function(err, img) { ... });
 */

Napi::Value Image::fill(Napi::CallbackInfo const& info)
{
    return fillSync(info);
}
/*
    if (info.Length() <= 1) {
        return _fillSync(info);
        return;
    }

    Image* im = info.Holder().Unwrap<Image>();
    fill_image_baton_t *closure = new fill_image_baton_t();
    if (info[0].IsUint32())
    {
        closure->val_u32 = Napi::To<std::uint32_t>(info[0]);
        closure->type = FILL_UINT32;
    }
    else if (info[0].IsNumber())
    {
        closure->val_32 = info[0].As<Napi::Number>().Int32Value();
        closure->type = FILL_INT32;
    }
    else if (info[0].IsNumber())
    {
        closure->val_double = info[0].As<Napi::Number>().DoubleValue();
        closure->type = FILL_DOUBLE;
    }
    else if (info[0].IsObject())
    {
        Napi::Object obj = info[0].ToObject(Napi::GetCurrentContext());
        if (obj->IsNull() || obj->IsUndefined() || !Napi::New(env, Color::constructor)->HasInstance(obj))
        {
            delete closure;
            Napi::TypeError::New(env, "A numeric or color value is expected").ThrowAsJavaScriptException();
            return env.Null();
        }
        else
        {
            Color * color = obj.Unwrap<Color>();
            closure->c = *(color->get());
        }
    }
    else
    {
        delete closure;
        Napi::TypeError::New(env, "A numeric or color value is expected").ThrowAsJavaScriptException();
        return env.Null();
    }
    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        delete closure;
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }
    else
    {
        closure->request.data = closure;
        closure->im = im;
        closure->error = false;
        closure->cb.Reset(callback.As<Napi::Function>());
        uv_queue_work(uv_default_loop(), &closure->request, EIO_Fill, (uv_after_work_cb)EIO_AfterFill);
        im->Ref();
    }
    return;
}

void Image::EIO_Fill(uv_work_t* req)
{
    fill_image_baton_t *closure = static_cast<fill_image_baton_t *>(req->data);
    try
    {
        switch (closure->type)
        {
            case FILL_UINT32:
                mapnik::fill(*closure->im->this_, closure->val_u32);
                break;
            case FILL_INT32:
                mapnik::fill(*closure->im->this_, closure->val_32);
                break;
            default:
            case FILL_DOUBLE:
                mapnik::fill(*closure->im->this_, closure->val_double);
                break;
            case FILL_COLOR:
                mapnik::fill(*closure->im->this_,closure->c);
                break;
        }
    }
    catch(std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterFill(uv_work_t* req)
{
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    fill_image_baton_t *closure = static_cast<fill_image_baton_t *>(req->data);
    if (closure->error)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Napi::Value argv[2] = { env.Null(), closure->im->handle() };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}
  */
