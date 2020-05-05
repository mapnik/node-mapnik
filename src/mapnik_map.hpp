#ifndef __NODE_MAPNIK_MAP_H__
#define __NODE_MAPNIK_MAP_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

// stl
#include <string>
#include <memory>



namespace mapnik { class Map; }

typedef std::shared_ptr<mapnik::Map> map_ptr;

class Map : public Napi::ObjectWrap<Map> {
public:

    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);

    static Napi::Value fonts(Napi::CallbackInfo const& info);
    static Napi::Value fontFiles(Napi::CallbackInfo const& info);
    static Napi::Value fontDirectory(Napi::CallbackInfo const& info);
    static Napi::Value loadFonts(Napi::CallbackInfo const& info);
    static Napi::Value registerFonts(Napi::CallbackInfo const& info);
    static Napi::Value memoryFonts(Napi::CallbackInfo const& info);
    static Napi::Value loadSync(Napi::CallbackInfo const& info);
    static Napi::Value load(Napi::CallbackInfo const& info);
    static void EIO_Load(uv_work_t* req);
    static void EIO_AfterLoad(uv_work_t* req);

    static Napi::Value fromStringSync(Napi::CallbackInfo const& info);
    static Napi::Value fromString(Napi::CallbackInfo const& info);
    static void EIO_FromString(uv_work_t* req);
    static void EIO_AfterFromString(uv_work_t* req);
    static Napi::Value clone(Napi::CallbackInfo const& info);

    // async rendering
    static Napi::Value render(Napi::CallbackInfo const& info);
    static void EIO_RenderImage(uv_work_t* req);
    static void EIO_AfterRenderImage(uv_work_t* req);
#if defined(GRID_RENDERER)
    static void EIO_RenderGrid(uv_work_t* req);
    static void EIO_AfterRenderGrid(uv_work_t* req);
#endif
    static void EIO_RenderVectorTile(uv_work_t* req);
    static void EIO_AfterRenderVectorTile(uv_work_t* req);

    static Napi::Value renderFile(Napi::CallbackInfo const& info);
    static void EIO_RenderFile(uv_work_t* req);
    static void EIO_AfterRenderFile(uv_work_t* req);

    // sync rendering
    static Napi::Value renderSync(Napi::CallbackInfo const& info);
    static Napi::Value renderFileSync(Napi::CallbackInfo const& info);

    static Napi::Value save(Napi::CallbackInfo const& info);
    static Napi::Value toXML(Napi::CallbackInfo const& info);

    static Napi::Value clear(Napi::CallbackInfo const& info);
    static Napi::Value resize(Napi::CallbackInfo const& info);
    static Napi::Value zoomAll(Napi::CallbackInfo const& info);
    static Napi::Value zoomToBox(Napi::CallbackInfo const& info);
    static Napi::Value layers(Napi::CallbackInfo const& info);
    static Napi::Value scale(Napi::CallbackInfo const& info);
    static Napi::Value scaleDenominator(Napi::CallbackInfo const& info);
    static Napi::Value queryPoint(Napi::CallbackInfo const& info);
    static Napi::Value queryMapPoint(Napi::CallbackInfo const& info);
    static Napi::Value abstractQueryPoint(Napi::CallbackInfo const& info, bool geo_coords);
    static void EIO_QueryMap(uv_work_t* req);
    static void EIO_AfterQueryMap(uv_work_t* req);

    static Napi::Value add_layer(Napi::CallbackInfo const& info);
    static Napi::Value remove_layer(Napi::CallbackInfo const& info);
    static Napi::Value get_layer(Napi::CallbackInfo const& info);

    Napi::Value get_prop(Napi::CallbackInfo const& info);
    void set_prop(Napi::CallbackInfo const& info, const Napi::Value& value);

    Map(int width, int height);
    Map(int width, int height, std::string const& srs);
    Map();

    bool acquire();
    void release();

    using Napi::ObjectWrap::Ref;
    using Napi::ObjectWrap::Unref;

    inline map_ptr get() { return map_; }

private:
    ~Map();
    map_ptr map_;
    bool in_use_;
};

#endif
