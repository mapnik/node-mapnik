// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc
#include <mapnik/image_copy.hpp>

#include "mapnik_image.hpp"
#include "utils.hpp"

// std
#include <exception>

typedef struct {
    uv_work_t request;
    Image* im1;
    image_ptr im2;
    mapnik::image_dtype type;
    double offset;
    double scaling;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
} copy_image_baton_t;

/**
 * Copy an image into a new image by creating a clone
 * @name copy
 * @instance
 * @memberof Image
 * @param {number} type - image type to clone into, can be any mapnik.imageType number
 * @param {Object} [options={}]
 * @param {number} [options.scaling] - scale the image
 * @param {number} [options.offset] - offset this image
 * @param {Function} callback
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray16});
 * var img2 = img.copy(mapnik.imageType.gray8, function(err, img2) {
 *   if (err) throw err;
 *   // custom code with `img2` converted into gray8 type
 * });
 */
NAN_METHOD(Image::copy)
{
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_copySync(info));
        return;
    }

    Image* im1 = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = im1->this_->get_dtype();
    v8::Local<v8::Object> options = Nan::New<v8::Object>();

    if (info.Length() >= 2)
    {
        if (info[0]->IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(info[0]->IntegerValue());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Nan::ThrowTypeError("Image 'type' must be a valid image type");
                return;
            }
        }
        else if (info[0]->IsObject())
        {
            options = info[0]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Unknown parameters passed");
            return;
        }
    }
    if (info.Length() >= 3)
    {
        if (info[1]->IsObject())
        {
            options = info[1]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as second argument");
            return;
        }
    }

    if (options->Has(Nan::New("scaling").ToLocalChecked()))
    {
        v8::Local<v8::Value> scaling_val = options->Get(Nan::New("scaling").ToLocalChecked());
        if (scaling_val->IsNumber())
        {
            scaling = scaling_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("scaling argument must be a number");
            return;
        }
    }

    if (options->Has(Nan::New("offset").ToLocalChecked()))
    {
        v8::Local<v8::Value> offset_val = options->Get(Nan::New("offset").ToLocalChecked());
        if (offset_val->IsNumber())
        {
            offset = offset_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("offset argument must be a number");
            return;
        }
    }

    if (!scaling_or_offset_set && type == im1->this_->get_dtype())
    {
        scaling = im1->this_->get_scaling();
        offset = im1->this_->get_offset();
    }

    copy_image_baton_t *closure = new copy_image_baton_t();
    closure->request.data = closure;
    closure->im1 = im1;
    closure->offset = offset;
    closure->scaling = scaling;
    closure->type = type;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Copy, (uv_after_work_cb)EIO_AfterCopy);
    closure->im1->Ref();
    return;
}

void Image::EIO_Copy(uv_work_t* req)
{
    copy_image_baton_t *closure = static_cast<copy_image_baton_t *>(req->data);
    try
    {
        closure->im2 = std::make_shared<mapnik::image_any>(
                               mapnik::image_copy(*(closure->im1->this_),
                                                            closure->type,
                                                            closure->offset,
                                                            closure->scaling)
                               );
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterCopy(uv_work_t* req)
{
    Nan::HandleScope scope;
    copy_image_baton_t *closure = static_cast<copy_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else if (!closure->im2)
    {
        // Not quite sure if this is even required or ever can be reached, but leaving it
        // and simply removing it from coverage tests.
        /* LCOV_EXCL_START */
        v8::Local<v8::Value> argv[1] = { Nan::Error("could not render to image") };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    }
    else
    {
        Image* im = new Image(closure->im2);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im1->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Copy an image into a new image by creating a clone
 * @name copySync
 * @instance
 * @memberof Image
 * @param {number} type - image type to clone into, can be any mapnik.imageType number
 * @param {Object} [options={}]
 * @param {number} [options.scaling] - scale the image
 * @param {number} [options.offset] - offset this image
 * @returns {mapnik.Image} copy
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray16});
 * var img2 = img.copy(mapnik.imageType.gray8);
 * // custom code with `img2` as a gray8 type
 */
NAN_METHOD(Image::copySync)
{
    info.GetReturnValue().Set(_copySync(info));
}

v8::Local<v8::Value> Image::_copySync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    double offset = 0.0;
    bool scaling_or_offset_set = false;
    double scaling = 1.0;
    mapnik::image_dtype type = im->this_->get_dtype();
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() >= 1)
    {
        if (info[0]->IsNumber())
        {
            type = static_cast<mapnik::image_dtype>(info[0]->IntegerValue());
            if (type >= mapnik::image_dtype::IMAGE_DTYPE_MAX)
            {
                Nan::ThrowTypeError("Image 'type' must be a valid image type");
                return scope.Escape(Nan::Undefined());
            }
        }
        else if (info[0]->IsObject())
        {
            options = info[0]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Unknown parameters passed");
            return scope.Escape(Nan::Undefined());
        }
    }
    if (info.Length() >= 2)
    {
        if (info[1]->IsObject())
        {
            options = info[1]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as second argument");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (options->Has(Nan::New("scaling").ToLocalChecked()))
    {
        v8::Local<v8::Value> scaling_val = options->Get(Nan::New("scaling").ToLocalChecked());
        if (scaling_val->IsNumber())
        {
            scaling = scaling_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("scaling argument must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (options->Has(Nan::New("offset").ToLocalChecked()))
    {
        v8::Local<v8::Value> offset_val = options->Get(Nan::New("offset").ToLocalChecked());
        if (offset_val->IsNumber())
        {
            offset = offset_val->NumberValue();
            scaling_or_offset_set = true;
        }
        else
        {
            Nan::ThrowTypeError("offset argument must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (!scaling_or_offset_set && type == im->this_->get_dtype())
    {
        scaling = im->this_->get_scaling();
        offset = im->this_->get_offset();
    }

    try
    {
        image_ptr imagep = std::make_shared<mapnik::image_any>(
                                                       mapnik::image_copy(*(im->this_),
                                                                            type,
                                                                            offset,
                                                                            scaling)
                                                    );
        Image* new_im = new Image(imagep);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(new_im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        return scope.Escape(maybe_local.ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}
