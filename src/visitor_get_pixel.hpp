#ifndef __NODE_MAPNIK_VISITOR_GET_PIXEL_H__
#define __NODE_MAPNIK_VISITOR_GET_PIXEL_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/image.hpp>             // for image types

struct visitor_get_pixel
{
    visitor_get_pixel(int x, int y)
        : x_(x), y_(y) {}

    inline v8::Local<v8::Value> operator() (mapnik::image_null const&)
    {
        // This should never be reached because the width and height of 0 for a null
        // image will prevent the visitor from being called.
        /* LCOV_EXCL_START */
        Nan::EscapableHandleScope scope;
        return scope.Escape(Nan::Undefined());
        /* LCOV_EXCL_STOP */

    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray8 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Uint32>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray8s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Int32>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray16 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Uint32>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray16s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Int32>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray32 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Uint32>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray32s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Int32>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray32f const& data)
    {
        Nan::EscapableHandleScope scope;
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray64 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint64_t val = mapnik::get_pixel<std::uint64_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray64s const& data)
    {
        Nan::EscapableHandleScope scope;
        std::int64_t val = mapnik::get_pixel<std::int64_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_gray64f const& data)
    {
        Nan::EscapableHandleScope scope;
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

    inline v8::Local<v8::Value> operator() (mapnik::image_rgba8 const& data)
    {
        Nan::EscapableHandleScope scope;
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Nan::New<v8::Number>(val));
    }

  private:
    int x_;
    int y_;

};

#endif
