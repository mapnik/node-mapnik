// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"
#include "utils.hpp"

// std
#include <exception>

typedef struct {
    uv_work_t request;
    Image* im;
    Nan::Persistent<v8::Function> cb;
} image_op_baton_t;

/**
 * Determine whether the given image is premultiplied.
 * https://en.wikipedia.org/wiki/Alpha_compositing
 *
 * @name premultiplied
 * @memberof Image
 * @instance
 * @returns {boolean} premultiplied `true` if the image is premultiplied
 * @example
 * var img = new mapnik.Image(5,5);
 * console.log(img.premultiplied()); // false
 * img.premultiplySync()
 * console.log(img.premultiplied()); // true
 */
NAN_METHOD(Image::premultiplied)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    bool premultiplied = im->this_->get_premultiplied();
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(premultiplied));
}

/**
 * Premultiply the pixels in this image.
 *
 * @name premultiplySync
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image(5,5);
 * img.premultiplySync();
 * console.log(img.premultiplied()); // true
 */
NAN_METHOD(Image::premultiplySync)
{
    info.GetReturnValue().Set(_premultiplySync(info));
}

v8::Local<v8::Value> Image::_premultiplySync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    mapnik::premultiply_alpha(*im->this_);
    return scope.Escape(Nan::Undefined());
}

/**
 * Premultiply the pixels in this image, asynchronously
 *
 * @name premultiply
 * @memberof Image
 * @instance
 * @param {Function} callback
 * @example
 * var img = new mapnik.Image(5,5);
 * img.premultiply(function(err, img) {
 *   if (err) throw err;
 *   // your custom code with premultiplied img
 * })
 */
NAN_METHOD(Image::premultiply)
{
    if (info.Length() == 0) {
        info.GetReturnValue().Set(_premultiplySync(info));
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Premultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    return;
}

void Image::EIO_Premultiply(uv_work_t* req)
{
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    mapnik::premultiply_alpha(*closure->im->this_);
}

void Image::EIO_AfterMultiply(uv_work_t* req)
{
    Nan::HandleScope scope;
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im->handle() };
    Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Demultiply the pixels in this image. The opposite of
 * premultiplying.
 *
 * @name demultiplySync
 * @instance
 * @memberof Image
 */
NAN_METHOD(Image::demultiplySync)
{
    Nan::HandleScope scope;
    info.GetReturnValue().Set(_demultiplySync(info));
}

v8::Local<v8::Value> Image::_demultiplySync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    mapnik::demultiply_alpha(*im->this_);
    return scope.Escape(Nan::Undefined());
}

/**
 * Demultiply the pixels in this image, asynchronously. The opposite of
 * premultiplying
 *
 * @name demultiply
 * @param {Function} callback
 * @instance
 * @memberof Image
 */
NAN_METHOD(Image::demultiply)
{
    if (info.Length() == 0) {
        info.GetReturnValue().Set(_demultiplySync(info));
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    image_op_baton_t *closure = new image_op_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Demultiply, (uv_after_work_cb)EIO_AfterMultiply);
    im->Ref();
    return;
}

void Image::EIO_Demultiply(uv_work_t* req)
{
    image_op_baton_t *closure = static_cast<image_op_baton_t *>(req->data);
    mapnik::demultiply_alpha(*closure->im->this_);
}
