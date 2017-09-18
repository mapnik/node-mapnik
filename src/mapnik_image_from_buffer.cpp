// mapnik
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any

#include "mapnik_image.hpp"
#include "utils.hpp"

// std
#include <exception>

/**
 * Create an image of the existing buffer.
 *
 * Note: the buffer must live as long as the image.
 * It is recommended that you do not use this method. Be warned!
 *
 * @name fromBufferSync
 * @param {number} width
 * @param {number} height
 * @param {Buffer} buffer
 * @returns {mapnik.Image} image object
 * @static
 * @memberof Image
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * var buffer = img.data(); // returns data as buffer
 * var img2 = mapnik.Image.fromBufferSync(img.width(), img.height(), buffer);
 */
NAN_METHOD(Image::fromBufferSync)
{
    info.GetReturnValue().Set(_fromBufferSync(info));
}

v8::Local<v8::Value> Image::_fromBufferSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;

    if (info.Length() < 3 || !info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsObject()) {
        Nan::ThrowTypeError("must provide a width, height, and buffer argument");
        return scope.Escape(Nan::Undefined());
    }

    unsigned width = info[0]->IntegerValue();
    unsigned height = info[1]->IntegerValue();

    if (width <= 0 || height <= 0)
    {
        Nan::ThrowTypeError("width and height must be greater then zero");
        return scope.Escape(Nan::Undefined());
    }

    v8::Local<v8::Object> obj = info[2]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        Nan::ThrowTypeError("third argument is invalid, must be a Buffer");
        return scope.Escape(Nan::Undefined());
    }

    // TODO - support other image types?
    auto im_size = mapnik::image_rgba8::pixel_size * width * height;
    if (im_size != node::Buffer::Length(obj)) {
        Nan::ThrowTypeError("invalid image size");
        return scope.Escape(Nan::Undefined());
    }

    bool premultiplied = false;
    bool painted = false;

    if (info.Length() >= 4)
    {
        if (info[3]->IsObject())
        {
            v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[3]);
            if (options->Has(Nan::New("type").ToLocalChecked()))
            {
                Nan::ThrowTypeError("'type' option not supported (only rgba images currently viable)");
                return scope.Escape(Nan::Undefined());
            }

            if (options->Has(Nan::New("premultiplied").ToLocalChecked()))
            {
                v8::Local<v8::Value> pre_val = options->Get(Nan::New("premultiplied").ToLocalChecked());
                if (!pre_val.IsEmpty() && pre_val->IsBoolean())
                {
                    premultiplied = pre_val->BooleanValue();
                }
                else
                {
                    Nan::ThrowTypeError("premultiplied option must be a boolean");
                    return scope.Escape(Nan::Undefined());
                }
            }

            if (options->Has(Nan::New("painted").ToLocalChecked()))
            {
                v8::Local<v8::Value> painted_val = options->Get(Nan::New("painted").ToLocalChecked());
                if (!painted_val.IsEmpty() && painted_val->IsBoolean())
                {
                    painted = painted_val->BooleanValue();
                }
                else
                {
                    Nan::ThrowTypeError("painted option must be a boolean");
                    return scope.Escape(Nan::Undefined());
                }
            }
        }
        else
        {
            Nan::ThrowTypeError("Options parameter must be an object");
            return scope.Escape(Nan::Undefined());
        }
    }

    try
    {
        mapnik::image_rgba8 im_wrapper(width, height, reinterpret_cast<unsigned char*>(node::Buffer::Data(obj)), premultiplied, painted);
        image_ptr imagep = std::make_shared<mapnik::image_any>(im_wrapper);
        Image* im = new Image(imagep);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        v8::Local<v8::Object> image_obj = maybe_local.ToLocalChecked()->ToObject();
        image_obj->Set(Nan::New("_buffer").ToLocalChecked(),obj);
        return scope.Escape(maybe_local.ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        // There is no known way for this exception to be reached currently.
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
        // LCOV_EXCL_STOP
    }
}

