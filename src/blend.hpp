#ifndef NODE_MAPNIK_BLEND_HPP
#define NODE_MAPNIK_BLEND_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

// stl
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include "mapnik_palette.hpp"
#include "tint.hpp"

namespace node_mapnik {

struct BImage {
    BImage() :
        data(NULL),
        dataLength(0),
        x(0),
        y(0),
        width(0),
        height(0),
        tint(),
        im_ptr(nullptr) {}
    v8::Persistent<v8::Object> buffer;
    const char * data;
    size_t dataLength;
    int x;
    int y;
    int width, height;
    Tinter tint;
    std::unique_ptr<mapnik::image_rgba8> im_ptr;
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

enum EncoderType {
    BLEND_ENCODER_LIBPNG,
    BLEND_ENCODER_MINIZ
};

NAN_METHOD(rgb2hsl);
NAN_METHOD(hsl2rgb);
NAN_METHOD(Blend);

struct BlendBaton {
    uv_work_t request;
    v8::Persistent<v8::Function> callback;
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
    EncoderType encoder;
    std::ostringstream stream;

    BlendBaton() :
        quality(0),
        format(BLEND_FORMAT_PNG),
        reencode(false),
        width(0),
        height(0),
        matte(0),
        compression(-1),
        mode(BLEND_MODE_HEXTREE),
        encoder(BLEND_ENCODER_LIBPNG),
        stream(std::ios::out | std::ios::binary)
    {
        this->request.data = this;
    }

    ~BlendBaton() {
        for (Images::iterator cur = images.begin(); cur != images.end(); cur++) {
            NanDisposePersistent((*cur)->buffer);
        }
        NanDisposePersistent(callback);
    }
};

}

#endif // NODE_MAPNIK_BLEND_HPP
