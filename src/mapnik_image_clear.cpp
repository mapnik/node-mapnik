// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"
#include "utils.hpp"

// std
#include <exception>

/**
 * Make this image transparent. (synchronous)
 *
 * @name clearSync
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image(5,5);
 * img.fillSync(1);
 * console.log(img.getPixel(0, 0)); // 1
 * img.clearSync();
 * console.log(img.getPixel(0, 0)); // 0
 */
NAN_METHOD(Image::clearSync)
{
    info.GetReturnValue().Set(_clearSync(info));
}

v8::Local<v8::Value> Image::_clearSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    try
    {
        mapnik::fill(*im->this_, 0);
    }
    catch(std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    //std::string format;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} clear_image_baton_t;

/**
 * Make this image transparent, removing all image data from it.
 *
 * @name clear
 * @instance
 * @param {Function} callback
 * @memberof Image
 * @example
 * var img = new mapnik.Image(5,5);
 * img.fillSync(1);
 * console.log(img.getPixel(0, 0)); // 1
 * img.clear(function(err, result) {
 *   console.log(result.getPixel(0,0)); // 0
 * });
 */
NAN_METHOD(Image::clear)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    if (info.Length() == 0) {
        info.GetReturnValue().Set(_clearSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }
    clear_image_baton_t *closure = new clear_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Clear, (uv_after_work_cb)EIO_AfterClear);
    im->Ref();
    return;
}

void Image::EIO_Clear(uv_work_t* req)
{
    clear_image_baton_t *closure = static_cast<clear_image_baton_t *>(req->data);
    try
    {
        mapnik::fill(*closure->im->this_, 0);
    }
    catch(std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterClear(uv_work_t* req)
{
    Nan::HandleScope scope;
    clear_image_baton_t *closure = static_cast<clear_image_baton_t *>(req->data);
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

