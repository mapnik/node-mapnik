#pragma once
#include <napi.h>

// stl
#include <string>
#include <vector>
#include <memory>
#include "mapnik_palette.hpp"
#include "mapnik_image.hpp"
#include "tint.hpp"

namespace node_mapnik {

struct BImage
{
    BImage() : data(nullptr),
               dataLength(0),
               x(0),
               y(0),
               width(0),
               height(0),
               tint(),
               im_ptr(nullptr),
               im_raw_ptr(nullptr),
               im_obj(nullptr) {}

    Napi::Reference<Napi::Buffer<char>> buffer;
    char const* data;
    size_t dataLength;
    int x;
    int y;
    int width, height;
    Tinter tint;
    std::unique_ptr<mapnik::image_rgba8> im_ptr;
    mapnik::image_rgba8* im_raw_ptr;
    image_ptr im_obj;
};

typedef std::shared_ptr<BImage> ImagePtr;
typedef std::vector<ImagePtr> Images;

enum BlendFormat
{
    BLEND_FORMAT_PNG,
    BLEND_FORMAT_JPEG,
    BLEND_FORMAT_WEBP
};

enum AlphaMode
{
    BLEND_MODE_OCTREE,
    BLEND_MODE_HEXTREE
};

Napi::Value rgb2hsl(Napi::CallbackInfo const& info);
Napi::Value hsl2rgb(Napi::CallbackInfo const& info);
Napi::Value blend(Napi::CallbackInfo const& info);

} // namespace node_mapnik
