#pragma once


namespace detail {

struct visitor_get_pixel
{
    visitor_get_pixel(Napi::Env env, int x, int y)
        : env_(env), x_(x), y_(y) {}

    Napi::Value operator() (mapnik::image_null const&)
    {
        // This should never be reached because the width and height of 0 for a null
        // image will prevent the visitor from being called.

        Napi::EscapableHandleScope scope(env_);
        return scope.Escape(env_.Undefined());
    }

    Napi::Value operator() (mapnik::image_gray8 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray8s const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray16 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray16s const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray32 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray32s const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray32f const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray64 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint64_t val = mapnik::get_pixel<std::uint64_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray64s const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::int64_t val = mapnik::get_pixel<std::int64_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_gray64f const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

    Napi::Value operator() (mapnik::image_rgba8 const& data)
    {
        Napi::EscapableHandleScope scope(env_);
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return scope.Escape(Napi::Number::New(env_, val));
    }

  private:
    Napi::Env env_;
    int x_;
    int y_;

};

struct visitor_set_pixel
{
    visitor_set_pixel(Napi::Number const& num, int x, int y)
        : num_(num), x_(x), y_(y) {}

    void operator() (mapnik::image_null &) const
    {
        // no-op
    }
    template <typename T>
    void operator() (T & image) const
    {
        mapnik::set_pixel(image, x_, y_, num_.DoubleValue());
    }
  private:
    Napi::Number const& num_;
    int x_;
    int y_;

};
} // ns detail
