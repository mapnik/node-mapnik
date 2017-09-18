// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"
#include "utils.hpp"
#include "visitor_get_pixel.hpp"

// std
#include <exception>

typedef struct {
    uv_work_t request;
    Image* im;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
    bool result;
} is_solid_image_baton_t;

/**
 * Test if an image's pixels are all exactly the same
 * @name isSolid
 * @memberof Image
 * @instance
 * @returns {boolean} `true` means all pixels are exactly the same
 * @example
 * var img = new mapnik.Image(2,2);
 * console.log(img.isSolid()); // true
 *
 * // change a pixel
 * img.setPixel(0,0, new mapnik.Color('green'));
 * console.log(img.isSolid()); // false
 */
NAN_METHOD(Image::isSolid)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

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

    is_solid_image_baton_t *closure = new is_solid_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->result = true;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    im->Ref();
    return;
}

void Image::EIO_IsSolid(uv_work_t* req)
{
    is_solid_image_baton_t *closure = static_cast<is_solid_image_baton_t *>(req->data);
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

void Image::EIO_AfterIsSolid(uv_work_t* req)
{
    Nan::HandleScope scope;
    is_solid_image_baton_t *closure = static_cast<is_solid_image_baton_t *>(req->data);
    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            v8::Local<v8::Value> argv[3] = { Nan::Null(),
                                     Nan::New(closure->result),
                                     mapnik::util::apply_visitor(visitor_get_pixel(0,0),*(closure->im->this_)),
            };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 3, argv);
        }
        else
        {
            v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::New(closure->result) };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
        }
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Determine whether the image is solid - whether it has alpha values of greater
 * than one.
 *
 * @name isSolidSync
 * @returns {boolean} whether the image is solid
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image(256, 256);
 * var view = img.view(0, 0, 256, 256);
 * console.log(view.isSolidSync()); // true
 */
NAN_METHOD(Image::isSolidSync)
{
    info.GetReturnValue().Set(_isSolidSync(info));
}

v8::Local<v8::Value> Image::_isSolidSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (im->this_->width() > 0 && im->this_->height() > 0)
    {
        return scope.Escape(Nan::New<v8::Boolean>(mapnik::is_solid(*(im->this_))));
    }
    Nan::ThrowError("image does not have valid dimensions");
    return scope.Escape(Nan::Undefined());
}
