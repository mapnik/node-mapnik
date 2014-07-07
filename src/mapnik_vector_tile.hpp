#ifndef __NODE_MAPNIK_VECTOR_TILE_H__
#define __NODE_MAPNIK_VECTOR_TILE_H__

#include <nan.h>
#include "vector_tile.pb.h"
#include <stdexcept>
#include <google/protobuf/io/coded_stream.h>
#include <vector>
#include <string>
#include "mapnik3x_compatibility.hpp"

using namespace v8;

class VectorTile: public node::ObjectWrap {
public:
    enum parsing_status {
        LAZY_DONE = 1,
        LAZY_SET = 2,
        LAZY_MERGE = 3
    };
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static NAN_METHOD(getData);
    static NAN_METHOD(render);
    static NAN_METHOD(toJSON);
    static NAN_METHOD(query);
    static NAN_METHOD(queryMany);
    static NAN_METHOD(names);
    static NAN_METHOD(toGeoJSON);
    static NAN_METHOD(addGeoJSON);
    static NAN_METHOD(addImage);
#ifdef PROTOBUF_FULL
    static NAN_METHOD(toString);
#endif
    static void EIO_RenderTile(uv_work_t* req);
    static void EIO_AfterRenderTile(uv_work_t* req);
    static NAN_METHOD(setData);
    static void EIO_SetData(uv_work_t* req);
    static void EIO_AfterSetData(uv_work_t* req);
    static Local<Value> _setDataSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(setDataSync);
    static NAN_METHOD(parse);
    static void EIO_Parse(uv_work_t* req);
    static void EIO_AfterParse(uv_work_t* req);
    static NAN_METHOD(parseSync);
    static Local<Value> _parseSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(addData);
    static NAN_METHOD(composite);
    // methods common to mapnik.Image
    static NAN_METHOD(width);
    static NAN_METHOD(height);
    static NAN_METHOD(painted);
    static NAN_METHOD(clearSync);
    static Local<Value> _clearSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(clear);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);
    static NAN_METHOD(isSolid);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static NAN_METHOD(isSolidSync);
    static Local<Value> _isSolidSync(_NAN_METHOD_ARGS);

    VectorTile(int z, int x, int y, unsigned w, unsigned h);

    void clear() {
        tiledata_.Clear();
        buffer_.clear();
        painted(false);
        byte_size_ = 0;
    }
    mapnik::vector::tile & get_tile_nonconst() {
        return tiledata_;
    }
    std::vector<std::string> lazy_names();
    void parse_proto();
    mapnik::vector::tile const& get_tile() {
        return tiledata_;
    }
    void cache_bytesize() {
        byte_size_ = tiledata_.ByteSize();
    }
    void painted(bool painted) {
        painted_ = painted;
    }
    bool painted() const {
        return painted_;
    }
    unsigned width() const {
        return width_;
    }
    unsigned height() const {
        return height_;
    }
    void _ref() { Ref(); }
    void _unref() { Unref(); }
    int z_;
    int x_;
    int y_;
    std::string buffer_;
    parsing_status status_;
private:
    ~VectorTile();
    mapnik::vector::tile tiledata_;
    unsigned width_;
    unsigned height_;
    bool painted_;
    int byte_size_;
};

#endif // __NODE_MAPNIK_VECTOR_TILE_H__
