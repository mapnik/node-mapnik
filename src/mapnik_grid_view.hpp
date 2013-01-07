#ifndef __NODE_MAPNIK_GRID_VIEW_H__
#define __NODE_MAPNIK_GRID_VIEW_H__

#include <v8.h>
#include <uv.h>
#include <node_object_wrap.h>
#include <mapnik/grid/grid_view.hpp>
#include <boost/shared_ptr.hpp>

class Grid;
using namespace v8;

typedef boost::shared_ptr<mapnik::grid_view> grid_view_ptr;

class GridView: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> New(Grid * JSGrid,
                             unsigned x,unsigned y, unsigned w, unsigned h);

    static Handle<Value> encodeSync(const Arguments &args);
    static Handle<Value> encode(const Arguments &args);
    static void EIO_Encode(uv_work_t* req);
    static void EIO_AfterEncode(uv_work_t* req);

    static Handle<Value> width(const Arguments &args);
    static Handle<Value> height(const Arguments &args);
    static Handle<Value> isSolid(const Arguments &args);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static Handle<Value> isSolidSync(const Arguments &args);
    static Handle<Value> getPixel(const Arguments &args);

    GridView(Grid * JSGrid);
    inline grid_view_ptr get() { return this_; }

private:
    ~GridView();
    grid_view_ptr this_;
    Grid * JSGrid_;
};

#endif
