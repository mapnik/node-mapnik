// mapnik
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_reader.hpp>      // for get_image_reader, etc
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc
//
#include "mapnik_image.hpp"

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

Napi::Value Image::openSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if (info.Length() < 1)
    {
        Napi::Error::New(env, "must provide a string argument").ThrowAsJavaScriptException();
        return scope.Escape(env.Undefined());
    }
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "Argument must be a string").ThrowAsJavaScriptException();
        return scope.Escape(env.Undefined());
    }
    try
    {
        std::string filename = info[0].As<Napi::String>();
        boost::optional<std::string> type = mapnik::type_from_filename(filename);
        if (type)
        {
            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(filename,*type));
            if (reader.get())
            {
                image_ptr imagep = std::make_shared<mapnik::image_any>(reader->read(0, 0, reader->width(), reader->height()));
                if (!reader->has_alpha())
                {
                    mapnik::set_premultiplied_alpha(*imagep, true);
                }
                Napi::Value arg = Napi::External<image_ptr>::New(env, &imagep);
                Napi::Object obj = constructor.New({arg});
                return scope.Escape(napi_value(obj)).ToObject();
            }
        }
        Napi::TypeError::New(env, ("Unsupported image format:" + filename)).ThrowAsJavaScriptException();

        return scope.Escape(env.Undefined());
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return scope.Escape(env.Undefined());
    }
}
/*
typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string error_name;
    Napi::Persistent<v8::Object> buffer;
    Napi::FunctionReference cb;
    bool premultiply;
    std::uint32_t max_size;
    const char *data;
    size_t dataLength;
    bool error;
} image_mem_ptr_baton_t;

typedef struct {
    uv_work_t request;
    image_ptr im;
    std::string filename;
    bool error;
    std::string error_name;
    Napi::FunctionReference cb;
} image_file_ptr_baton_t;
  */
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
  /*
Napi::Value Image::open(const Napi::CallbackInfo& info)
{
    if (info.Length() == 1) {
        return _openSync(info);
        return;
    }

    if (info.Length() < 2) {
        Napi::Error::New(env, "must provide a string argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Argument must be a string").ThrowAsJavaScriptException();
        return env.Null();
    }

    // ensure callback is a function
    Napi::Value callback = info[info.Length()-1];
    if (!info[info.Length()-1]->IsFunction()) {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Null();
    }

    image_file_ptr_baton_t *closure = new image_file_ptr_baton_t();
    closure->request.data = closure;
    closure->filename = TOSTR(info[0]);
    closure->error = false;
    closure->cb.Reset(callback.As<Napi::Function>());
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

                closure->error = true;
                closure->error_name = "Failed to load: " + closure->filename;

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
    Napi::HandleScope scope(env);
    Napi::AsyncResource async_resource(__func__);
    image_file_ptr_baton_t *closure = static_cast<image_file_ptr_baton_t *>(req->data);
    if (closure->error || !closure->im)
    {
        Napi::Value argv[1] = { Napi::Error::New(env, closure->error_name.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im);
        Napi::Value ext = Napi::External::New(env, im);
        Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
        if (maybe_local.IsEmpty())
        {
            Napi::Value argv[1] = { Napi::Error::New(env, "Could not create new Image instance") };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        }
        else
        {
            Napi::Value argv[2] = { env.Null(), maybe_local };
            async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
        }
    }
    closure->cb.Reset();
    delete closure;
}
*/
