#ifndef __NODE_MAPNIK_GRID_VIEW_H__
#define __NODE_MAPNIK_GRID_VIEW_H__

#include <nan.h>
#include <mapnik/grid/grid_view.hpp>
#include "mapnik3x_compatibility.hpp"
#include MAPNIK_SHARED_INCLUDE

class Grid;
using namespace v8;

typedef MAPNIK_SHARED_PTR<mapnik::grid_view> grid_view_ptr;

class GridView: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static Handle<Value> New(Grid * JSGrid,
                             unsigned x,unsigned y, unsigned w, unsigned h);

    static NAN_METHOD(encodeSync);
    static NAN_METHOD(encode);
    static void EIO_Encode(uv_work_t* req);
    static void EIO_AfterEncode(uv_work_t* req);

    static NAN_METHOD(width);
    static NAN_METHOD(height);
    static NAN_METHOD(isSolid);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static NAN_METHOD(isSolidSync);
    static NAN_METHOD(getPixel);

    GridView(Grid * JSGrid);
    inline grid_view_ptr get() { return this_; }

private:
    ~GridView();
    grid_view_ptr this_;
    Grid * JSGrid_;
};

#endif
