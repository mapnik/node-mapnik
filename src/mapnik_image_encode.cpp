// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"
#include "mapnik_palette.hpp"

#include "utils.hpp"

// std
#include <exception>

/**
 * Encode this image into a buffer of encoded data (synchronous)
 *
 * @name encodeSync
 * @param {string} [format=png] image format
 * @param {Object} [options]
 * @param {mapnik.Palette} [options.palette] - mapnik.Palette object
 * @returns {Buffer} buffer - encoded image data
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * var buffer = img.encodeSync('png');
 * // write buffer to a new file
 * fs.writeFileSync('myimage.png', buffer);
 */
NAN_METHOD(Image::encodeSync)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() >= 1){
        if (!info[0]->IsString()) {
            Nan::ThrowTypeError("first arg, 'format' must be a string");
            return;
        }
        format = TOSTR(info[0]);
    }


    // options hash
    if (info.Length() >= 2) {
        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1]->ToObject();
        if (options->Has(Nan::New("palette").ToLocalChecked()))
        {
            v8::Local<v8::Value> format_opt = options->Get(Nan::New("palette").ToLocalChecked());
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }
            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    try {
        std::string s;
        if (palette.get())
        {
            s = save_to_string(*(im->this_), format, *palette);
        }
        else {
            s = save_to_string(*(im->this_), format);
        }

        info.GetReturnValue().Set(Nan::CopyBuffer((char*)s.data(), s.size()).ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
}

typedef struct {
    uv_work_t request;
    Image* im;
    std::string format;
    palette_ptr palette;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    std::string result;
} encode_image_baton_t;

/**
 * Encode this image into a buffer of encoded data
 *
 * @name encode
 * @param {string} [format=png] image format
 * @param {Object} [options]
 * @param {mapnik.Palette} [options.palette] - mapnik.Palette object
 * @param {Function} callback - `function(err, encoded)`
 * @returns {Buffer} encoded image data
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * myImage.encode('png', function(err, encoded) {
 *   if (err) throw err;
 *   // write buffer to new file
 *   fs.writeFileSync('myimage.png', encoded);
 * });
 *
 * // encoding an image object with a mapnik.Palette
 * var im = new mapnik.Image(256, 256);
 * var pal = new mapnik.Palette(new Buffer('\xff\x09\x93\xFF\x01\x02\x03\x04','ascii'));
 * im.encode('png', {palette: pal}, function(err, encode) {
 *   if (err) throw err;
 *   // your custom code with `encode` image buffer
 * });
 */
NAN_METHOD(Image::encode)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (info.Length() >= 1){
        if (!info[0]->IsString()) {
            Nan::ThrowTypeError("first arg, 'format' must be a string");
            return;
        }
        format = TOSTR(info[0]);
    }

    // options hash
    if (info.Length() >= 2) {
        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[1].As<v8::Object>();

        if (options->Has(Nan::New("palette").ToLocalChecked()))
        {
            v8::Local<v8::Value> format_opt = options->Get(Nan::New("palette").ToLocalChecked());
            if (!format_opt->IsObject()) {
                Nan::ThrowTypeError("'palette' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = format_opt.As<v8::Object>();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.Palette expected as second arg");
                return;
            }

            palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    encode_image_baton_t *closure = new encode_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->format = format;
    closure->palette = palette;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    im->Ref();

    return;
}

void Image::EIO_Encode(uv_work_t* req)
{
    encode_image_baton_t *closure = static_cast<encode_image_baton_t *>(req->data);

    try {
        if (closure->palette.get())
        {
            closure->result = save_to_string(*(closure->im->this_), closure->format, *closure->palette);
        }
        else
        {
            closure->result = save_to_string(*(closure->im->this_), closure->format);
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterEncode(uv_work_t* req)
{
    Nan::HandleScope scope;

    encode_image_baton_t *closure = static_cast<encode_image_baton_t *>(req->data);

    if (closure->error) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::CopyBuffer((char*)closure->result.data(), closure->result.size()).ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->im->Unref();
    closure->cb.Reset();
    delete closure;
}

