#ifndef NODE_BLEND_SRC_READER_H
#define NODE_BLEND_SRC_READER_H

#include "mapnik3x_compatibility.hpp"
#include MAPNIK_SHARED_INCLUDE
#include <mapnik/version.hpp>
#include <mapnik/image_reader.hpp>
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

class ImageReader {
public:
    ImageReader(unsigned char* _source, size_t len) :
      width(0),
      height(0),
      alpha(true),
      surface(NULL),
      source(_source),
      length(len),
      pos(0) {
        try {
            reader_ = MAPNIK_UNIQUE_PTR<mapnik::image_reader>(mapnik::get_image_reader((const char*)source,len));
            if (reader_.get())
            {
#if MAPNIK_VERSION >= 200300
                alpha = reader_->has_alpha();
#endif
                width = reader_->width();
                height = reader_->height();
                surface = (unsigned int*)malloc(width * height * 4);
            } else {
                message = "Unknown image format";            
            }
        } catch (std::exception const& ex) {
            message = ex.what();
        }
    }

    bool decode() {
        try {
            if (reader_.get())
            {
#if MAPNIK_VERSION >= 200300
                mapnik::image_data_32 im(reader_->width(),reader_->height(),surface);
                reader_->read(0,0,im);
                return true;
#else
                message = "Could not decode image (>= Mapnik 2.3.x required)";
#endif
            } else {
                message = "Could not decode image";
            }
        } catch (std::exception const& ex) {
            message = ex.what();            
        }
        return false;
    }
    ~ImageReader() {
        if (surface) {
            free(surface);
            surface = NULL;
        }
    }
    unsigned width;
    unsigned height;
    bool alpha;
    unsigned int *surface;

    std::string message;
    std::vector<std::string> warnings;
    MAPNIK_UNIQUE_PTR<mapnik::image_reader> reader_;
protected:
    unsigned char* source;
    size_t length;
    size_t pos;
};

#endif
