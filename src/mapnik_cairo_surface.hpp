#ifndef __NODE_MAPNIK_CAIRO_H__
#define __NODE_MAPNIK_CAIRO_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <string>
#include <sstream>

// cairo
#if defined(HAVE_CAIRO)
#include <cairo.h>
#else
#define cairo_status_t int
#endif

using namespace v8;

class CairoSurface: public node::ObjectWrap {
public:
    typedef std::stringstream i_stream;
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static NAN_METHOD(getData);
    static NAN_METHOD(width);
    static NAN_METHOD(height);
    void _ref() { Ref(); }
    void _unref() { Unref(); }
    CairoSurface(std::string const& format, unsigned int width, unsigned int height);
    static cairo_status_t write_callback(void *closure,
                                         const unsigned char *data,
                                         unsigned int length)
    {
#if defined(HAVE_CAIRO)
        if (!closure)
        {
            return CAIRO_STATUS_WRITE_ERROR;
        }
        i_stream* fin = reinterpret_cast<i_stream*>(closure);
        *fin << std::string((const char*)data,(size_t)length);
        return CAIRO_STATUS_SUCCESS;
#else
        return 11; // CAIRO_STATUS_WRITE_ERROR
#endif
    }
    unsigned width() { return width_; }
    unsigned height() { return height_; }
    mutable i_stream ss_;
private:
    unsigned width_;
    unsigned height_;
    std::string format_;
    ~CairoSurface();
};

#endif
