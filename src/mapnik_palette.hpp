#ifndef __NODE_MAPNIK_PALETTE_H__
#define __NODE_MAPNIK_PALETTE_H__

#include <v8.h>
#include <node_object_wrap.h>

// boost
#include <boost/shared_ptr.hpp>

#include <mapnik/palette.hpp>

using namespace v8;

typedef boost::shared_ptr<mapnik::rgba_palette> palette_ptr;

class Palette: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;

    explicit Palette(std::string const& palette, mapnik::rgba_palette::palette_type type);
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    static Handle<Value> ToString(const Arguments& args);
    static Handle<Value> ToBuffer(const Arguments& args);

    inline palette_ptr palette() { return palette_; }
private:
    ~Palette();
    palette_ptr palette_;
};

#endif
