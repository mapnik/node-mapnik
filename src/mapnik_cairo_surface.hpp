#pragma once

#include <napi.h>
// stl
#include <sstream>
// cairo
#if defined(HAVE_CAIRO)
#include <cairo.h>
#else
#define cairo_status_t int
#endif

class CairoSurface : public Napi::ObjectWrap<CairoSurface>
{
  public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit CairoSurface(Napi::CallbackInfo const& info);
    // methods
    Napi::Value getData(Napi::CallbackInfo const& info);
    Napi::Value width(Napi::CallbackInfo const& info);
    Napi::Value height(Napi::CallbackInfo const& info);
    static cairo_status_t write_callback(void* stream,
                                         const unsigned char* data,
                                         unsigned int length)
    {
#if defined(HAVE_CAIRO)
        if (!stream)
        {
            // Since the closure here is the passing of the std::stringstream "i_stream" of this
            // class it is unlikely that this could ever be reached unless something was very wrong
            // and went out of scope, aka it was improperly programmed. Therefore, it is not possible
            // to reach this point with standard testing.
            // LCOV_EXCL_START
            return CAIRO_STATUS_WRITE_ERROR;
            // LCOV_EXCL_STOP
        }
        std::stringstream* fin = reinterpret_cast<std::stringstream*>(stream);
        *fin << std::string((const char*)data, (size_t)length);
        return CAIRO_STATUS_SUCCESS;
#else
        return CAIRO_STATUS_WRITE_ERROR;
#endif
    }

    inline unsigned width() const { return width_; }
    inline unsigned height() const { return height_; }
    inline std::string const& format() const { return format_; }
    inline std::stringstream& stream() { return stream_; }
    inline void flush() { data_ = stream_.str(); }
    inline void set_data(std::string const& data) { data_ = data; }
    inline std::string const& data() const { return data_; }
    static Napi::FunctionReference constructor;

  private:
    unsigned width_;
    unsigned height_;
    std::string format_;
    std::stringstream stream_;
    std::string data_;
};
