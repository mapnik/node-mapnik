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
    static Napi::Value New(const Napi::CallbackInfo& info);

    static Napi::Value fonts(const Napi::CallbackInfo& info);
    static Napi::Value fontFiles(const Napi::CallbackInfo& info);
    static Napi::Value fontDirectory(const Napi::CallbackInfo& info);
    static Napi::Value loadFonts(const Napi::CallbackInfo& info);
    static Napi::Value registerFonts(const Napi::CallbackInfo& info);
    static Napi::Value memoryFonts(const Napi::CallbackInfo& info);
    static Napi::Value loadSync(const Napi::CallbackInfo& info);
    static Napi::Value load(const Napi::CallbackInfo& info);
    static void EIO_Load(uv_work_t* req);
    static void EIO_AfterLoad(uv_work_t* req);

    static Napi::Value fromStringSync(const Napi::CallbackInfo& info);
    static Napi::Value fromString(const Napi::CallbackInfo& info);
    static void EIO_FromString(uv_work_t* req);
    static void EIO_AfterFromString(uv_work_t* req);
    static Napi::Value clone(const Napi::CallbackInfo& info);

    // async rendering
    static Napi::Value render(const Napi::CallbackInfo& info);
    static void EIO_RenderImage(uv_work_t* req);
    static void EIO_AfterRenderImage(uv_work_t* req);
#if defined(GRID_RENDERER)
    static void EIO_RenderGrid(uv_work_t* req);
    static void EIO_AfterRenderGrid(uv_work_t* req);
#endif
    static void EIO_RenderVectorTile(uv_work_t* req);
    static void EIO_AfterRenderVectorTile(uv_work_t* req);

    static Napi::Value renderFile(const Napi::CallbackInfo& info);
    static void EIO_RenderFile(uv_work_t* req);
    static void EIO_AfterRenderFile(uv_work_t* req);

    // sync rendering
    static Napi::Value renderSync(const Napi::CallbackInfo& info);
    static Napi::Value renderFileSync(const Napi::CallbackInfo& info);

    static Napi::Value save(const Napi::CallbackInfo& info);
    static Napi::Value toXML(const Napi::CallbackInfo& info);

    static Napi::Value clear(const Napi::CallbackInfo& info);
    static Napi::Value resize(const Napi::CallbackInfo& info);
    static Napi::Value zoomAll(const Napi::CallbackInfo& info);
    static Napi::Value zoomToBox(const Napi::CallbackInfo& info);
    static Napi::Value layers(const Napi::CallbackInfo& info);
    static Napi::Value scale(const Napi::CallbackInfo& info);
    static Napi::Value scaleDenominator(const Napi::CallbackInfo& info);
    static Napi::Value queryPoint(const Napi::CallbackInfo& info);
    static Napi::Value queryMapPoint(const Napi::CallbackInfo& info);
    static Napi::Value abstractQueryPoint(const Napi::CallbackInfo& info, bool geo_coords);
    static void EIO_QueryMap(uv_work_t* req);
    static void EIO_AfterQueryMap(uv_work_t* req);

    static Napi::Value add_layer(const Napi::CallbackInfo& info);
    static Napi::Value remove_layer(const Napi::CallbackInfo& info);
    static Napi::Value get_layer(const Napi::CallbackInfo& info);

    Napi::Value get_prop(const Napi::CallbackInfo& info);
    void set_prop(const Napi::CallbackInfo& info, const Napi::Value& value);

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
