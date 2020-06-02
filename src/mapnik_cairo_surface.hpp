#pragma once

#include <napi.h>

//#include <string>
#include <sstream>
// cairo
#if defined(HAVE_CAIRO)
#include <cairo.h>
#else
#define cairo_status_t int
#endif


class CairoSurface : public Napi::ObjectWrap<CairoSurface>
{
    friend class VectorTile;
public:
    //typedef std::stringstream i_stream;
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports);
    // ctor
    explicit CairoSurface(Napi::CallbackInfo const& info);
    // methods
    Napi::Value getData(Napi::CallbackInfo const& info);
    Napi::Value width(Napi::CallbackInfo const& info);
    Napi::Value height(Napi::CallbackInfo const& info);

    //using Napi::ObjectWrap::Ref;
    //using Napi::ObjectWrap::Unref;

    //CairoSurface(std::string const& format, unsigned int width, unsigned int height);
    /*
    static cairo_status_t write_callback(void *closure,
                                         const unsigned char *data,
                                         unsigned int length)
    {
#if defined(HAVE_CAIRO)
        if (!closure)
        {
            // Since the closure here is the passing of the std::stringstream "i_stream" of this
            // class it is unlikely that this could ever be reached unless something was very wrong
            // and went out of scope, aka it was improperly programmed. Therefore, it is not possible
            // to reach this point with standard testing.
            // LCOV_EXCL_START
            return CAIRO_STATUS_WRITE_ERROR;
            // LCOV_EXCL_STOP
        }
        i_stream* fin = reinterpret_cast<i_stream*>(closure);
        *fin << std::string((const char*)data,(size_t)length);
        return CAIRO_STATUS_SUCCESS;
#else
        return 11; // CAIRO_STATUS_WRITE_ERROR
#endif
    }
*/
    //unsigned width() { return width_; }
    //unsigned height() { return height_; }
    //mutable i_stream ss_;
private:
    static Napi::FunctionReference constructor;
    unsigned width_;
    unsigned height_;
    std::string format_;
    std::stringstream stream_;
};
