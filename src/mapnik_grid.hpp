#ifndef __NODE_MAPNIK_GRID_H__
#define __NODE_MAPNIK_GRID_H__

#if defined(GRID_RENDERER)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/grid/grid.hpp>
#include <memory>



typedef std::shared_ptr<mapnik::grid> grid_ptr;

class Grid: public Nan::ObjectWrap {
public:
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);

    static NAN_METHOD(encodeSync);
    static NAN_METHOD(encode);
    static void EIO_Encode(uv_work_t* req);
    static void EIO_AfterEncode(uv_work_t* req);

    static NAN_METHOD(addField);
    static NAN_METHOD(fields);
    static NAN_METHOD(view);
    static NAN_METHOD(width);
    static NAN_METHOD(height);
    static NAN_METHOD(painted);
    static v8::Local<v8::Value> _clearSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(clearSync);
    static NAN_METHOD(clear);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);

    static NAN_GETTER(get_key);
    static NAN_SETTER(set_key);

    using Nan::ObjectWrap::Ref;
    using Nan::ObjectWrap::Unref;

    Grid(unsigned int width, unsigned int height, std::string const& key);
    inline grid_ptr get() { return this_; }

private:
    ~Grid();
    grid_ptr this_;
};

#endif

#endif
