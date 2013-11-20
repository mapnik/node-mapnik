#ifndef __NODE_MAPNIK_VECTOR_TILE_H__
#define __NODE_MAPNIK_VECTOR_TILE_H__

#include <v8.h>
#include <node_object_wrap.h>
#include "uv.h"
#include "vector_tile.pb.h"

using namespace v8;

class VectorTile: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(Arguments const&args);
    static Handle<Value> getData(Arguments const& args);
    static Handle<Value> render(Arguments const& args);
    static Handle<Value> toJSON(Arguments const& args);
    static Handle<Value> query(Arguments const& args);
    static Handle<Value> names(Arguments const& args);    
    static Handle<Value> toGeoJSON(Arguments const& args);
#ifdef PROTOBUF_FULL
    static Handle<Value> toString(Arguments const& args);
#endif
    static void EIO_RenderTile(uv_work_t* req);
    static void EIO_AfterRenderTile(uv_work_t* req);
    static Handle<Value> setData(Arguments const& args);
    static void EIO_SetData(uv_work_t* req);
    static void EIO_AfterSetData(uv_work_t* req);
    static Handle<Value> setDataSync(Arguments const& args);
    // methods common to mapnik.Image
    static Handle<Value> width(Arguments const& args);
    static Handle<Value> height(Arguments const& args);
    static Handle<Value> painted(Arguments const& args);
    static Handle<Value> clearSync(const Arguments& args);
    static Handle<Value> clear(const Arguments& args);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);
    static Handle<Value> isSolid(Arguments const& args);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static Handle<Value> isSolidSync(Arguments const& args);

    VectorTile(int z, int x, int y, unsigned w=256, unsigned h=256);

    void clear() {
        painted_ = false;
        tiledata_.Clear();
    }
    mapnik::vector::tile & get_tile_nonconst() {
        return tiledata_;
    }
    mapnik::vector::tile const& get_tile() const {
        return tiledata_;
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

private:
    ~VectorTile();
    mapnik::vector::tile tiledata_;
    unsigned width_;
    unsigned height_;
    bool painted_;
public:
    int estimated_size_;
};

#endif // __NODE_MAPNIK_VECTOR_TILE_H__
