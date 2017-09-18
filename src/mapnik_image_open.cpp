// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_reader.hpp>      // for get_image_reader, etc
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"
#include "utils.hpp"

// std
#include <exception>

/**
 * Load in a pre-existing image as an image object
 * @name openSync
 * @memberof Image
 * @instance
 * @param {string} path - path to the image you want to load
 * @returns {mapnik.Image} new image object based on existing image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.jpg');
 */
NAN_METHOD(Image::openSync)
{
    info.GetReturnValue().Set(_openSync(info));
}

v8::Local<v8::Value> Image::_openSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;

    if (info.Length() < 1) {
        Nan::ThrowError("must provide a string argument");
        return scope.Escape(Nan::Undefined());
    }

    if (!info[0]->IsString()) {
        Nan::ThrowTypeError("Argument must be a string");
        return scope.Escape(Nan::Undefined());
    }

    try
    {
        std::string filename = TOSTR(info[0]);
        boost::optional<std::string> type = mapnik::type_from_filename(filename);
        if (type)
        {
            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(filename,*type));
            if (reader.get())
            {
                image_ptr imagep = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(), reader->height()));
                if (!reader->has_alpha())
                {
                    mapnik::set_premultiplied_alpha(*imagep, true);
                }
                Image* im = new Image(imagep);
                v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
                Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
                if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
                return scope.Escape(maybe_local.ToLocalChecked());
            }
        }
        Nan::ThrowTypeError(("Unsupported image format:" + filename).c_str());
        return scope.Escape(Nan::Undefined());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}

typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string filename;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} image_file_ptr_baton_t;

/**
 * Load in a pre-existing image as an image object
 * @name open
 * @memberof Image
 * @static
 * @param {string} path - path to the image you want to load
 * @param {Function} callback -
 * @example
 * mapnik.Image.open('./path/to/image.jpg', function(err, img) {
 *   if (err) throw err;
 *   // img is now an Image object
 * });
 */
NAN_METHOD(Image::open)
{
    if (info.Length() == 1) {
        info.GetReturnValue().Set(_openSync(info));
        return;
    }

    if (info.Length() < 2) {
        Nan::ThrowError("must provide a string argument");
        return;
    }

    if (!info[0]->IsString()) {
        Nan::ThrowTypeError("Argument must be a string");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    image_file_ptr_baton_t *closure = new image_file_ptr_baton_t();
    closure->request.data = closure;
    closure->filename = TOSTR(info[0]);
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Open, (uv_after_work_cb)EIO_AfterOpen);
    return;
}

void Image::EIO_Open(uv_work_t* req)
{
    image_file_ptr_baton_t *closure = static_cast<image_file_ptr_baton_t *>(req->data);

    try
    {
        boost::optional<std::string> type = mapnik::type_from_filename(closure->filename);
        if (!type)
        {
            closure->error = true;
            closure->error_name = "Unsupported image format: " + closure->filename;
        }
        else
        {
            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(closure->filename,*type));
            if (reader.get())
            {
                closure->im = std::make_shared<mapnik::image_any>(reader->read(0,0,reader->width(),reader->height()));
                if (!reader->has_alpha())
                {
                    mapnik::set_premultiplied_alpha(*(closure->im), true);
                }
            }
            else
            {
                // The only way this is ever reached is if the reader factory in
                // mapnik was not providing an image type it should. This should never
                // be occuring so marking this out from coverage
                /* LCOV_EXCL_START */
                closure->error = true;
                closure->error_name = "Failed to load: " + closure->filename;
                /* LCOV_EXCL_STOP */
            }
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterOpen(uv_work_t* req)
{
    Nan::HandleScope scope;
    image_file_ptr_baton_t *closure = static_cast<image_file_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->cb.Reset();
    delete closure;
}
