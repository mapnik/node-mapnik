#ifndef __NODE_MAPNIK_MAP_H__
#define __NODE_MAPNIK_MAP_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

// stl
#include <string>
#include <memory>



namespace mapnik { class Map; }

typedef std::shared_ptr<mapnik::Map> map_ptr;

class Map: public Nan::ObjectWrap {
public:

    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);

    static NAN_METHOD(fonts);
    static NAN_METHOD(fontFiles);
    static NAN_METHOD(fontDirectory);
    static NAN_METHOD(loadFonts);
    static NAN_METHOD(registerFonts);
    static NAN_METHOD(memoryFonts);
    static NAN_METHOD(loadSync);
    static NAN_METHOD(load);
    static void EIO_Load(uv_work_t* req);
    static void EIO_AfterLoad(uv_work_t* req);

    static NAN_METHOD(fromStringSync);
    static NAN_METHOD(fromString);
    static void EIO_FromString(uv_work_t* req);
    static void EIO_AfterFromString(uv_work_t* req);
    static NAN_METHOD(clone);

    // async rendering
    static NAN_METHOD(render);
    static void EIO_RenderImage(uv_work_t* req);
    static void EIO_AfterRenderImage(uv_work_t* req);
#if defined(GRID_RENDERER)
    static void EIO_RenderGrid(uv_work_t* req);
    static void EIO_AfterRenderGrid(uv_work_t* req);
#endif
    static void EIO_RenderVectorTile(uv_work_t* req);
    static void EIO_AfterRenderVectorTile(uv_work_t* req);

    static NAN_METHOD(renderFile);
    static void EIO_RenderFile(uv_work_t* req);
    static void EIO_AfterRenderFile(uv_work_t* req);

    // sync rendering
    static NAN_METHOD(renderSync);
    static NAN_METHOD(renderFileSync);

    static NAN_METHOD(save);
    static NAN_METHOD(toXML);

    static NAN_METHOD(clear);
    static NAN_METHOD(resize);
    static NAN_METHOD(zoomAll);
    static NAN_METHOD(zoomToBox);
    static NAN_METHOD(layers);
    static NAN_METHOD(scale);
    static NAN_METHOD(scaleDenominator);
    static NAN_METHOD(queryPoint);
    static NAN_METHOD(queryMapPoint);
    static v8::Local<v8::Value> abstractQueryPoint(Nan::NAN_METHOD_ARGS_TYPE info, bool geo_coords);
    static void EIO_QueryMap(uv_work_t* req);
    static void EIO_AfterQueryMap(uv_work_t* req);

    static NAN_METHOD(add_layer);
    static NAN_METHOD(remove_layer);
    static NAN_METHOD(get_layer);

    static NAN_GETTER(get_prop);
    static NAN_SETTER(set_prop);

    Map(int width, int height);
    Map(int width, int height, std::string const& srs);
    Map();

    bool acquire();
    void release();

    using Nan::ObjectWrap::Ref;
    using Nan::ObjectWrap::Unref;

    inline map_ptr get() { return map_; }

private:
    ~Map();
    map_ptr map_;
    bool in_use_;
};

#endif
