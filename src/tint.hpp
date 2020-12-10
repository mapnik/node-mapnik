#pragma once

#include <cmath>
#include <string>

static inline void rgb_to_hsl(std::uint32_t red,
                              std::uint32_t green,
                              std::uint32_t blue,
                              double& h,
                              double& s,
                              double& l)
{
    double r = red / 255.0;
    double g = green / 255.0;
    double b = blue / 255.0;
    double max = std::max(r, std::max(g, b));
    double min = std::min(r, std::min(g, b));
    double delta = max - min;
    double gamma = max + min;
    h = 0.0, s = 0.0, l = gamma / 2.0;
    if (delta > 0.0)
    {
        s = l > 0.5 ? delta / (2.0 - gamma) : delta / gamma;
        if (r >= b && r > g) h = (g - b) / delta + (g < b ? 6.0 : 0.0);
        if (g >= r && g > b) h = (b - r) / delta + 2.0;
        if (b >= g && b > r) h = (r - g) / delta + 4.0;
        h /= 6.0;
    }
}

static inline double hueToRGB(double m1, double m2, double h)
{
    // poor mans fmod
    if (h < 0) h += 1;
    if (h > 1) h -= 1;
    if (h * 6 < 1) return m1 + (m2 - m1) * h * 6;
    if (h * 2 < 1) return m2;
    if (h * 3 < 2) return m1 + (m2 - m1) * (0.66666 - h) * 6;
    return m1;
}

static inline void hsl_to_rgb(double h,
                              double s,
                              double l,
                              std::uint32_t& r,
                              std::uint32_t& g,
                              std::uint32_t& b)
{
    if (!s)
    {
        r = g = b = static_cast<std::uint32_t>(std::floor((l * 255.0) + .5));
    }
    else
    {
        double m2 = (l <= 0.5) ? l * (s + 1) : l + s - l * s;
        double m1 = l * 2.0 - m2;
        r = static_cast<std::uint32_t>(std::floor(hueToRGB(m1, m2, h + 0.33333) * 255.0) + .5);
        g = static_cast<std::uint32_t>(std::floor(hueToRGB(m1, m2, h) * 255.0) + .5);
        b = static_cast<std::uint32_t>(std::floor(hueToRGB(m1, m2, h - 0.33333) * 255.0) + .5);
    }
}

struct Tinter
{
    double h0;
    double h1;
    double s0;
    double s1;
    double l0;
    double l1;
    double a0;
    double a1;

    Tinter() : h0(0),
               h1(1),
               s0(0),
               s1(1),
               l0(0),
               l1(1),
               a0(0),
               a1(1) {}

    bool is_identity() const
    {
        return (h0 == 0 &&
                h1 == 1 &&
                s0 == 0 &&
                s1 == 1 &&
                l0 == 0 &&
                l1 == 1);
    }

    bool is_alpha_identity() const
    {
        return (a0 == 0 &&
                a1 == 1);
    }
};
