#pragma once

#include <mapnik/image.hpp>
#include <mapnik/image_view.hpp>

namespace detail {
namespace traits {

template <typename T>
struct null_image
{
};

template <>
struct null_image<mapnik::image_any>
{
    using type = mapnik::image_null;
};

template <>
struct null_image<mapnik::image_view_any>
{
    using type = mapnik::image_view_null;
};
} // namespace traits

template <typename Image> // image or image_view
struct visitor_get_pixel
{
    using image_null_type = typename traits::null_image<Image>::type;

    visitor_get_pixel(Napi::Env env, int x, int y)
        : env_(env), x_(x), y_(y) {}

    Napi::Value operator()(image_null_type const&) const
    {
        // This should never be reached because the width and height of 0 for a null
        // image will prevent the visitor from being called.
        return env_.Undefined();
    }

    template <typename T>
    Napi::Value operator()(T const& data) const
    {
        using image_type = T;
        using pixel_type = typename image_type::pixel_type;
        Napi::EscapableHandleScope scope(env_);
        pixel_type val = mapnik::get_pixel<pixel_type>(data, x_, y_);
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

    void operator()(mapnik::image_null&) const
    {
        // no-op
    }
    template <typename T>
    void operator()(T& image) const
    {
        mapnik::set_pixel(image, x_, y_, num_.DoubleValue());
    }

  private:
    Napi::Number const& num_;
    int x_;
    int y_;
};
} // namespace detail
