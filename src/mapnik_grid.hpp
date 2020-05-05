#ifndef __NODE_MAPNIK_GRID_H__
#define __NODE_MAPNIK_GRID_H__

#if defined(GRID_RENDERER)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

#include <mapnik/grid/grid.hpp>
#include <memory>



typedef std::shared_ptr<mapnik::grid> grid_ptr;

class Grid : public Napi::ObjectWrap<Grid> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);

    static Napi::Value encodeSync(Napi::CallbackInfo const& info);
    static Napi::Value encode(Napi::CallbackInfo const& info);
    static void EIO_Encode(uv_work_t* req);
    static void EIO_AfterEncode(uv_work_t* req);

    static Napi::Value addField(Napi::CallbackInfo const& info);
    static Napi::Value fields(Napi::CallbackInfo const& info);
    static Napi::Value view(Napi::CallbackInfo const& info);
    static Napi::Value width(Napi::CallbackInfo const& info);
    static Napi::Value height(Napi::CallbackInfo const& info);
    static Napi::Value painted(Napi::CallbackInfo const& info);
    static Napi::Value _clearSync(Napi::CallbackInfo const& info);
    static Napi::Value clearSync(Napi::CallbackInfo const& info);
    static Napi::Value clear(Napi::CallbackInfo const& info);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);

    Napi::Value get_key(Napi::CallbackInfo const& info);
    void set_key(Napi::CallbackInfo const& info, const Napi::Value& value);

    using Napi::ObjectWrap::Ref;
    using Napi::ObjectWrap::Unref;

    Grid(unsigned int width, unsigned int height, std::string const& key);
    inline grid_ptr get() { return this_; }

private:
    ~Grid();
    grid_ptr this_;
};

#endif

#endif
