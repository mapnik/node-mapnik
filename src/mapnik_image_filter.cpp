// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any

#include <mapnik/image_filter_types.hpp>
#include <mapnik/image_filter.hpp>      // filter_visitor

#include "mapnik_image.hpp"

#include "utils.hpp"

// std
#include <exception>

/**
 * Apply a filter to this image. This changes all pixel values. (synchronous)
 *
 * @name filterSync
 * @instance
 * @memberof Image
 * @param {string} filter - can be `blur`, `emboss`, `sharpen`,
 * `sobel`, or `gray`.
 * @example
 * var img = new mapnik.Image(5, 5);
 * img.filter('blur');
 * // your custom code with `img` having blur applied
 */
NAN_METHOD(Image::filterSync)
{
    info.GetReturnValue().Set(_filterSync(info));
}

v8::Local<v8::Value> Image::_filterSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    if (info.Length() < 1) {
        Nan::ThrowTypeError("expects one argument: string filter argument");
        return scope.Escape(Nan::Undefined());
    }
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (!info[0]->IsString())
    {
        Nan::ThrowTypeError("A string is expected for filter argument");
        return scope.Escape(Nan::Undefined());
    }
    std::string filter = TOSTR(info[0]);
    try
    {
        mapnik::filter::filter_image(*im->this_,filter);
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
    std::string filter;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} filter_image_baton_t;

/**
 * Apply a filter to this image. Changes all pixel values.
 *
 * @name filter
 * @instance
 * @memberof Image
 * @param {string} filter - can be `blur`, `emboss`, `sharpen`,
 * `sobel`, or `gray`.
 * @param {Function} callback - `function(err, img)`
 * @example
 * var img = new mapnik.Image(5, 5);
 * img.filter('sobel', function(err, img) {
 *   if (err) throw err;
 *   // your custom `img` with sobel filter
 *   // https://en.wikipedia.org/wiki/Sobel_operator
 * });
 */
NAN_METHOD(Image::filter)
{
    if (info.Length() <= 1) {
        info.GetReturnValue().Set(_filterSync(info));
        return;
    }

    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (!info[0]->IsString())
    {
        Nan::ThrowTypeError("A string is expected for filter argument");
        return;
    }
    filter_image_baton_t *closure = new filter_image_baton_t();
    closure->filter = TOSTR(info[0]);

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    closure->request.data = closure;
    closure->im = im;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Filter, (uv_after_work_cb)EIO_AfterFilter);
    im->Ref();
    return;
}

void Image::EIO_Filter(uv_work_t* req)
{
    filter_image_baton_t *closure = static_cast<filter_image_baton_t *>(req->data);
    try
    {
        mapnik::filter::filter_image(*closure->im->this_,closure->filter);
    }
    catch(std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterFilter(uv_work_t* req)
{
    Nan::HandleScope scope;
    filter_image_baton_t *closure = static_cast<filter_image_baton_t *>(req->data);
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

