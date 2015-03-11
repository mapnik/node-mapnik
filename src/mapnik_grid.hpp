#ifndef __NODE_MAPNIK_GRID_H__
#define __NODE_MAPNIK_GRID_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/grid/grid.hpp>
#include <memory>

using namespace v8;

typedef std::shared_ptr<mapnik::grid> grid_ptr;

class Grid: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
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
    static Local<Value> _clearSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(clearSync);
    static NAN_METHOD(clear);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);

    static NAN_GETTER(get_key);
    static NAN_SETTER(set_key);
    void _ref() { Ref(); }
    void _unref() { Unref(); }

    Grid(unsigned int width, unsigned int height, std::string const& key);
    inline grid_ptr get() { return this_; }

private:
    ~Grid();
    grid_ptr this_;
    int estimated_size_;
};

#endif
