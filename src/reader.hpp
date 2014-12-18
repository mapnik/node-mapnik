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
    ImageReader(const char * _source, size_t len)
      : reader_(nullptr),
        im_(0,0)
      {
        try {
            reader_ = MAPNIK_UNIQUE_PTR<mapnik::image_reader>(mapnik::get_image_reader(_source,len));
            if (!reader_.get()) {
                err_message = "Unknown image format";
            } else {
                im_ = mapnik::image_data_32(reader_->width(),reader_->height());
            }
        } catch (std::exception const& ex) {
            err_message = ex.what();
        }
    }
    inline std::string const& message() const
    {
        return err_message;
    }

    inline unsigned width() const
    {
        return (reader_.get() ? reader_->width() : 0);
    }

    inline unsigned height() const
    {
        return (reader_.get() ? reader_->height() : 0);
    }

    inline const unsigned int* getData() const
    {
        return im_.getData();
    }

    inline bool has_alpha() const
    {
        return (reader_.get() ? reader_->has_alpha() : false);
    }

    bool decode() {
        try {
            if (reader_ && reader_.get())
            {
                reader_->read(0,0,im_);
                return true;
            } else {
                err_message = "Could not decode image";
            }
        } catch (std::exception const& ex) {
            err_message = ex.what();
        }
        return false;
    }
    ~ImageReader() {}
    std::string err_message;
    std::vector<std::string> warnings;
private:
    MAPNIK_UNIQUE_PTR<mapnik::image_reader> reader_;
    mapnik::image_data_32 im_;
};

#endif
