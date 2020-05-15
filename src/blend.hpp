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

struct BImage {
    BImage() :
        data(nullptr),
        dataLength(0),
        x(0),
        y(0),
        width(0),
        height(0),
        tint(),
        im_ptr(nullptr),
        im_raw_ptr(nullptr),
        im_obj(nullptr) {}
    //Napi::Persistent<v8::Object> buffer;
    Napi::Reference<Napi::Buffer<char>> buffer;
    char const* data;
    size_t dataLength;
    int x;
    int y;
    int width, height;
    Tinter tint;
    std::unique_ptr<mapnik::image_rgba8> im_ptr;
    mapnik::image_rgba8 * im_raw_ptr;
    Image * im_obj;
};

typedef std::shared_ptr<BImage> ImagePtr;
typedef std::vector<ImagePtr> Images;

enum BlendFormat {
    BLEND_FORMAT_PNG,
    BLEND_FORMAT_JPEG,
    BLEND_FORMAT_WEBP
};

enum AlphaMode {
    BLEND_MODE_OCTREE,
    BLEND_MODE_HEXTREE
};

Napi::Value rgb2hsl(Napi::CallbackInfo const& info);
Napi::Value hsl2rgb(Napi::CallbackInfo const& info);
Napi::Value blend(Napi::CallbackInfo const& info);

/*
struct BlendBaton {
    uv_work_t request;
    Napi::FunctionReference callback;
    Images images;

    std::string message;

    int quality;
    BlendFormat format;
    bool reencode;
    int width;
    int height;
    palette_ptr palette;
    unsigned int matte;
    int compression;
    AlphaMode mode;
    std::unique_ptr<std::string> output_data;

    BlendBaton() :
        quality(0),
        format(BLEND_FORMAT_PNG),
        reencode(false),
        width(0),
        height(0),
        matte(0),
        compression(-1),
        mode(BLEND_MODE_HEXTREE),
        output_data()
    {
        this->request.data = this;
    }

    ~BlendBaton()
    {
        for (auto const& image : images) if (image) image->buffer.Reset();
        callback.Reset();
    }
};
*/
}
