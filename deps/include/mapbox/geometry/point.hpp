#pragma once

namespace mapbox { namespace geometry {

template <typename T>
struct point
{
    using coordinate_type = T;
    point()
        : x(), y()
    {}
    point(T x_, T y_)
        : x(x_), y(y_)
    {}
    T x;
    T y;
};

template <typename T>
bool operator==(point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
bool operator!=(point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

}}
