// mapnik
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_util.hpp>        // for save_to_stream

#include "mapnik_image_encode.hpp"
#include "mapnik_color.hpp"

#include "utils.hpp"
#include "callback_streambuf.hpp"

struct chunked_encode_image_baton_t
{
    encode_image_baton_t image_baton;

    uv_async_t async;
    uv_mutex_t mutex;

    using char_type = char;
    using buffer_type = std::vector<char_type>;
    using buffer_list_type = std::vector<buffer_type>;
    buffer_list_type buffers;

    const std::size_t buffer_size;

    chunked_encode_image_baton_t(std::size_t buffer_size_)
        : buffer_size(buffer_size_)
    {
        // The reinterpret_cast is for backward compatibility
        // https://github.com/libuv/libuv/commit/db2a9072bce129630214904be5e2eedeaafc9835
        if (uv_async_init(uv_default_loop(), &async,
            reinterpret_cast<uv_async_cb>(yield_chunk)))
        {
            throw std::runtime_error("Cannot create async handler");
        }

        if (uv_mutex_init(&mutex))
        {
            uv_close(reinterpret_cast<uv_handle_t*>(&async), NULL);
            throw std::runtime_error("Cannot create mutex");
        }

        async.data = this;
    }

    ~chunked_encode_image_baton_t()
    {
        uv_mutex_destroy(&mutex);
    }

    template<class Char, class Size>
    bool operator()(const Char* buffer, Size size)
    {
        uv_mutex_lock(&mutex);
        buffers.emplace_back(buffer, buffer + size);
        uv_mutex_unlock(&mutex);

        return uv_async_send(&async) == 0;
    }

    static void yield_chunk(uv_async_t* handle)
    {
        using closure_type = chunked_encode_image_baton_t;
        closure_type & closure = *reinterpret_cast<closure_type*>(handle->data);

        if (closure.image_baton.error)
        {
            uv_close(reinterpret_cast<uv_handle_t*>(handle), async_close_cb);
            return;
        }

        buffer_list_type local_buffers;

        uv_mutex_lock(&closure.mutex);
        closure.buffers.swap(local_buffers);
        uv_mutex_unlock(&closure.mutex);

        Nan::HandleScope scope;
        bool done = false;

        for (auto const & buffer : local_buffers)
        {
            v8::Local<v8::Value> argv[2] = {
                Nan::Null(), Nan::CopyBuffer(buffer.data(),
                buffer.size()).ToLocalChecked() };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                              Nan::New(closure.image_baton.cb), 2, argv);
            done = buffer.empty();
        }

        if (done)
        {
            uv_close(reinterpret_cast<uv_handle_t*>(handle), async_close_cb);
        }
    }

    static void async_close_cb(uv_handle_t* handle)
    {
        using closure_type = chunked_encode_image_baton_t;
        closure_type & closure = *reinterpret_cast<closure_type*>(handle->data);

        if (closure.image_baton.error)
        {
            Nan::HandleScope scope;
            v8::Local<v8::Value> argv[1] = {
                Nan::Error(closure.image_baton.error_name.c_str()) };
            Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                              Nan::New(closure.image_baton.cb), 1, argv);
        }

        closure.image_baton.im->_unref();
        closure.image_baton.cb.Reset();
        delete &closure;
    }
};

void Image::EIO_EncodeChunked(uv_work_t* work)
{
    using closure_type = chunked_encode_image_baton_t;
    closure_type & closure = *reinterpret_cast<closure_type*>(work->data);
    try
    {
        callback_streambuf<closure_type&> streambuf(closure, closure.buffer_size);
        std::ostream stream(&streambuf);

        if (closure.image_baton.palette)
        {
            mapnik::save_to_stream(*closure.image_baton.im->this_,
                                   stream,
                                   closure.image_baton.format,
                                   *closure.image_baton.palette);
        }
        else
        {
            mapnik::save_to_stream(*closure.image_baton.im->this_,
                                   stream,
                                   closure.image_baton.format);
        }

        stream.flush();
    }
    catch (std::exception const& ex)
    {
        closure.image_baton.error = true;
        closure.image_baton.error_name = ex.what();
    }

    // Signalize end of stream
    closure(static_cast<closure_type::char_type*>(NULL), 0);
}

NAN_METHOD(Image::encodeChunked)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());

    std::string format = "png";
    palette_ptr palette;

    if (info.Length() != 4)
    {
        Nan::ThrowTypeError("Function requires four arguments");
        return;
    }

    // accept custom format
    if (!info[0]->IsString())
    {
        Nan::ThrowTypeError("first arg, 'format' must be a string");
        return;
    }
    format = TOSTR(info[0]);

    // options hash
    if (!info[1]->IsObject())
    {
        Nan::ThrowTypeError("second arg must be an options object");
        return;
    }

    v8::Local<v8::Object> options = info[1].As<v8::Object>();

    if (options->Has(Nan::New("palette").ToLocalChecked()))
    {
        v8::Local<v8::Value> format_opt = options->Get(Nan::New("palette").ToLocalChecked());
        if (!format_opt->IsObject())
        {
            Nan::ThrowTypeError("'palette' must be an object");
            return;
        }

        v8::Local<v8::Object> obj = format_opt.As<v8::Object>();
        if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Palette::constructor)->HasInstance(obj))
        {
            Nan::ThrowTypeError("mapnik.Palette expected as second arg");
            return;
        }

        palette = Nan::ObjectWrap::Unwrap<Palette>(obj)->palette();
    }

    int buffer_size;
    if (!info[2]->IsNumber() || (buffer_size = info[2]->IntegerValue()) < 1)
    {
        Nan::ThrowTypeError("third arg must be a positive integer");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    chunked_encode_image_baton_t *closure = new chunked_encode_image_baton_t(buffer_size);
    closure->image_baton.request.data = closure;
    closure->image_baton.im = im;
    closure->image_baton.format = format;
    closure->image_baton.palette = palette;
    closure->image_baton.error = false;
    closure->image_baton.cb.Reset(callback.As<v8::Function>());

    uv_queue_work(uv_default_loop(), &closure->image_baton.request, EIO_EncodeChunked, NULL);
    im->Ref();
}
