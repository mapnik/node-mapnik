#ifndef __NODE_MAPNIK_GRID_VIEW_H__
#define __NODE_MAPNIK_GRID_VIEW_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/grid/grid_view.hpp>
#include <memory>

class Grid;
using namespace v8;

typedef std::shared_ptr<mapnik::grid_view> grid_view_ptr;

class GridView: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static Handle<Value> NewInstance(Grid * JSGrid,
                             unsigned x,unsigned y, unsigned w, unsigned h);

    static NAN_METHOD(encodeSync);
    static NAN_METHOD(encode);
    static NAN_METHOD(fields);
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
