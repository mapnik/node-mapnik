#ifndef NODE_BLEND_SRC_BLEND_H
#define NODE_BLEND_SRC_BLEND_H

#include <v8.h>
#include <node.h>
#include <nan.h>

// stl
#include <iostream>
#include <sstream>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include "mapnik_palette.hpp"
#include "reader.hpp"
#include "tint.hpp"

namespace node_mapnik {

typedef v8::Persistent<v8::Object> PersistentObject;

struct BImage {
    BImage() :
        data(NULL),
        dataLength(0),
        x(0),
        y(0),
        width(0),
        height(0),
        tint() {}
    PersistentObject buffer;
    unsigned char *data;
    size_t dataLength;
    int x, y;
    int width, height;
    Tinter tint;
    std::auto_ptr<ImageReader> reader;
};

typedef boost::shared_ptr<BImage> ImagePtr;
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

NAN_METHOD(rgb2hsl2);
NAN_METHOD(hsl2rgb2);
NAN_METHOD(Blend);
static void Work_Blend(uv_work_t* req);
static void Work_AfterBlend(uv_work_t* req);

struct BlendBaton {
    uv_work_t request;
    v8::Persistent<v8::Function> callback;
    Images images;

    std::string message;
    std::vector<std::string> warnings;

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

#endif
