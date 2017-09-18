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
 * Encode this image and save it to disk as a file.
 *
 * @name saveSync
 * @param {string} filename
 * @param {string} [format=png]
 * @instance
 * @memberof Image
 * @example
 * img.saveSync('foo.png');
 */
NAN_METHOD(Image::saveSync)
{
    info.GetReturnValue().Set(_saveSync(info));
}

v8::Local<v8::Value> Image::_saveSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    if (info.Length() == 0 || !info[0]->IsString()){
        Nan::ThrowTypeError("filename required to save file");
        return scope.Escape(Nan::Undefined());
    }

    std::string filename = TOSTR(info[0]);
    std::string format("");

    if (info.Length() >= 2) {
        if (!info[1]->IsString()) {
            Nan::ThrowTypeError("both 'filename' and 'format' arguments must be strings");
            return scope.Escape(Nan::Undefined());
        }
        format = TOSTR(info[1]);
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Nan::ThrowError(s.str().c_str());
            return scope.Escape(Nan::Undefined());
        }
    }

    try
    {
        mapnik::save_to_file(*(im->this_),filename, format);
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
    }
    return scope.Escape(Nan::Undefined());
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string format;
    std::string filename;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} save_image_baton_t;

/**
 * Encode this image and save it to disk as a file.
 *
 * @name save
 * @param {string} filename
 * @param {string} [format=png]
 * @param {Function} callback
 * @instance
 * @memberof Image
 * @example
 * img.save('image.png', 'png', function(err) {
 *   if (err) throw err;
 *   // your custom code
 * });
 */
NAN_METHOD(Image::save)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    if (info.Length() == 0 || !info[0]->IsString()){
        Nan::ThrowTypeError("filename required to save file");
        return;
    }

    if (!info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_saveSync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];

    std::string filename = TOSTR(info[0]);
    std::string format("");

    if (info.Length() >= 3) {
        if (!info[1]->IsString()) {
            Nan::ThrowTypeError("both 'filename' and 'format' arguments must be strings");
            return;
        }
        format = TOSTR(info[1]);
    }
    else
    {
        format = mapnik::guess_type(filename);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            Nan::ThrowError(s.str().c_str());
            return;
        }
    }

    save_image_baton_t *closure = new save_image_baton_t();
    closure->request.data = closure;
    closure->format = format;
    closure->filename = filename;
    closure->im = im;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Save, (uv_after_work_cb)EIO_AfterSave);
    im->Ref();
    return;
}

void Image::EIO_Save(uv_work_t* req)
{
    save_image_baton_t *closure = static_cast<save_image_baton_t *>(req->data);
    try
    {
        mapnik::save_to_file(*(closure->im->this_),
                             closure->filename,
                             closure->format);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterSave(uv_work_t* req)
{
    Nan::HandleScope scope;
    save_image_baton_t *closure = static_cast<save_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[1] = { Nan::Null() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

