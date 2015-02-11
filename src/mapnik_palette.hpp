#ifndef __NODE_MAPNIK_PALETTE_H__
#define __NODE_MAPNIK_PALETTE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/palette.hpp>
#include <memory>

using namespace v8;

typedef std::shared_ptr<mapnik::rgba_palette> palette_ptr;

class Palette: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;

    explicit Palette(std::string const& palette, mapnik::rgba_palette::palette_type type);
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);

    static NAN_METHOD(ToString);
    static NAN_METHOD(ToBuffer);

    inline palette_ptr palette() { return palette_; }
private:
    ~Palette();
    palette_ptr palette_;
};

#endif
