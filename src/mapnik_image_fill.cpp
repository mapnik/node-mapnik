// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"
#include "mapnik_color.hpp"

#include "utils.hpp"

// std
#include <exception>

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
NAN_METHOD(Image::fillSync)
{
    info.GetReturnValue().Set(_fillSync(info));
}

v8::Local<v8::Value> Image::_fillSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    if (info.Length() < 1 ) {
        Nan::ThrowTypeError("expects one argument: Color object or a number");
        return scope.Escape(Nan::Undefined());
    }
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    try
    {
        if (info[0]->IsUint32())
        {
            std::uint32_t val = info[0]->Uint32Value();
            mapnik::fill<std::uint32_t>(*im->this_,val);
        }
        else if (info[0]->IsInt32())
        {
            std::int32_t val = info[0]->Int32Value();
            mapnik::fill<std::int32_t>(*im->this_,val);
        }
        else if (info[0]->IsNumber())
        {
            double val = info[0]->NumberValue();
            mapnik::fill<double>(*im->this_,val);
        }
        else if (info[0]->IsObject())
        {
            v8::Local<v8::Object> obj = info[0]->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Color::constructor)->HasInstance(obj))
            {
                Nan::ThrowTypeError("A numeric or color value is expected");
            }
            else
            {
                Color * color = Nan::ObjectWrap::Unwrap<Color>(obj);
                mapnik::fill(*im->this_,*(color->get()));
            }
        }
        else
        {
            Nan::ThrowTypeError("A numeric or color value is expected");
        }
    }
    catch(std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
    }
    return scope.Escape(Nan::Undefined());
}

enum fill_type : std::uint8_t
{
    FILL_COLOR = 0,
    FILL_UINT32,
    FILL_INT32,
    FILL_DOUBLE
};

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
    Nan::Persistent<v8::Function> cb;
} fill_image_baton_t;

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
NAN_METHOD(Image::fill)
{
    if (info.Length() <= 1) {
        info.GetReturnValue().Set(_fillSync(info));
        return;
    }

    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    fill_image_baton_t *closure = new fill_image_baton_t();
    if (info[0]->IsUint32())
    {
        closure->val_u32 = info[0]->Uint32Value();
        closure->type = FILL_UINT32;
    }
    else if (info[0]->IsInt32())
    {
        closure->val_32 = info[0]->Int32Value();
        closure->type = FILL_INT32;
    }
    else if (info[0]->IsNumber())
    {
        closure->val_double = info[0]->NumberValue();
        closure->type = FILL_DOUBLE;
    }
    else if (info[0]->IsObject())
    {
        v8::Local<v8::Object> obj = info[0]->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Color::constructor)->HasInstance(obj))
        {
            delete closure;
            Nan::ThrowTypeError("A numeric or color value is expected");
            return;
        }
        else
        {
            Color * color = Nan::ObjectWrap::Unwrap<Color>(obj);
            closure->c = *(color->get());
        }
    }
    else
    {
        delete closure;
        Nan::ThrowTypeError("A numeric or color value is expected");
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        delete closure;
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }
    else
    {
        closure->request.data = closure;
        closure->im = im;
        closure->error = false;
        closure->cb.Reset(callback.As<v8::Function>());
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
    Nan::HandleScope scope;
    fill_image_baton_t *closure = static_cast<fill_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}
