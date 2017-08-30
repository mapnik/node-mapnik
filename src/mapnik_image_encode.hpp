#ifndef __NODE_MAPNIK_IMAGE_ENCODE_H__
#define __NODE_MAPNIK_IMAGE_ENCODE_H__

#include "mapnik_image.hpp"
#include "mapnik_palette.hpp"

typedef struct {
    uv_work_t request;
    Image* im;
    std::string format;
    palette_ptr palette;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
    std::string result;
} encode_image_baton_t;

#endif
