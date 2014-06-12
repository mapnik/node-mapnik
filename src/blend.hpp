#ifndef NODE_BLEND_SRC_BLEND_H
#define NODE_BLEND_SRC_BLEND_H

#include <v8.h>
#include <node.h>
#include <node_version.h>
#include <node_buffer.h>

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

#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION <= 4
    #define WORKER_BEGIN(name)                  int name(eio_req *req)
    #define WORKER_END()                        return 0;
    #define QUEUE_WORK(baton, worker, after)    eio_custom((worker), EIO_PRI_DEFAULT, (after), (baton));
#else
    #define WORKER_BEGIN(name)                  void name(uv_work_t *req)
    #define WORKER_END()                        return;
    #define QUEUE_WORK(baton, worker, after)    uv_queue_work(uv_default_loop(), &(baton)->request, (worker), (after));
#endif

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

#define TRY_CATCH_CALL(context, callback, argc, argv)                          \
{   v8::TryCatch try_catch;                                                    \
    (callback)->Call((context), (argc), (argv));                               \
    if (try_catch.HasCaught()) {                                               \
        node::FatalException(try_catch);                                       \
    }                                                                          }

#define TYPE_EXCEPTION(message)                                                \
    ThrowException(Exception::TypeError(String::New(message)))

v8::Handle<v8::Value> rgb2hsl2(const v8::Arguments& args);
v8::Handle<v8::Value> hsl2rgb2(const v8::Arguments& args);
v8::Handle<v8::Value> Blend(const v8::Arguments& args);
WORKER_BEGIN(Work_Blend);
WORKER_BEGIN(Work_AfterBlend);

struct BlendBaton {
#if NODE_MINOR_VERSION >= 5 || NODE_MAJOR_VERSION > 0
    uv_work_t request;
#endif
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
#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION <= 4
        ev_ref(EV_DEFAULT_UC);
#else
        this->request.data = this;
#endif
    }

    ~BlendBaton() {
        for (Images::iterator cur = images.begin(); cur != images.end(); cur++) {
            (*cur)->buffer.Dispose();
        }

#if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION <= 4
        ev_unref(EV_DEFAULT_UC);
#endif
        // Note: The result buffer is freed by the node Buffer's free callback

        callback.Dispose();
    }
};

}

#endif
