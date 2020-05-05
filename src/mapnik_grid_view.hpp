#ifndef __NODE_MAPNIK_GRID_VIEW_H__
#define __NODE_MAPNIK_GRID_VIEW_H__

#if defined(GRID_RENDERER)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

#include <mapnik/grid/grid_view.hpp>
#include <memory>

class Grid;


typedef std::shared_ptr<mapnik::grid_view> grid_view_ptr;

class GridView : public Napi::ObjectWrap<GridView> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);
    static Napi::Value NewInstance(Grid * JSGrid,
                             unsigned x,unsigned y, unsigned w, unsigned h);

    static Napi::Value encodeSync(Napi::CallbackInfo const& info);
    static Napi::Value encode(Napi::CallbackInfo const& info);
    static Napi::Value fields(Napi::CallbackInfo const& info);
    static void EIO_Encode(uv_work_t* req);
    static void EIO_AfterEncode(uv_work_t* req);

    static Napi::Value width(Napi::CallbackInfo const& info);
    static Napi::Value height(Napi::CallbackInfo const& info);
    static Napi::Value isSolid(Napi::CallbackInfo const& info);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static Napi::Value _isSolidSync(Napi::CallbackInfo const& info);
    static Napi::Value isSolidSync(Napi::CallbackInfo const& info);
    static Napi::Value getPixel(Napi::CallbackInfo const& info);

    GridView(Grid * JSGrid);
    inline grid_view_ptr get() { return this_; }

private:
    ~GridView();
    grid_view_ptr this_;
    Grid * JSGrid_;
};

#endif

#endif
