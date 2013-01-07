#ifndef __NODE_MAPNIK_MAP_H__
#define __NODE_MAPNIK_MAP_H__

#include <v8.h>
#include <uv.h>
#include <node_object_wrap.h>

// boost
#include <boost/shared_ptr.hpp>


using namespace v8;

namespace mapnik { class Map; }

typedef boost::shared_ptr<mapnik::Map> map_ptr;

class Map: public node::ObjectWrap {
public:

    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    static Handle<Value> loadSync(const Arguments &args);
    static Handle<Value> load(const Arguments &args);
    static void EIO_Load(uv_work_t* req);
    static void EIO_AfterLoad(uv_work_t* req);

    static Handle<Value> fromStringSync(const Arguments &args);
    static Handle<Value> fromString(const Arguments &args);
    static void EIO_FromString(uv_work_t* req);
    static void EIO_AfterFromString(uv_work_t* req);

    // async rendering
    static Handle<Value> render(const Arguments &args);
    static void EIO_RenderImage(uv_work_t* req);
    static void EIO_AfterRenderImage(uv_work_t* req);
    static void EIO_RenderGrid(uv_work_t* req);
    static void EIO_AfterRenderGrid(uv_work_t* req);

    static Handle<Value> renderFile(const Arguments &args);
    static void EIO_RenderFile(uv_work_t* req);
    static void EIO_AfterRenderFile(uv_work_t* req);

    // sync rendering
    static Handle<Value> renderSync(const Arguments &args);
    static Handle<Value> renderFileSync(const Arguments &args);

    static Handle<Value> save(const Arguments &args);
    static Handle<Value> to_string(const Arguments &args);

    static Handle<Value> clear(const Arguments &args);
    static Handle<Value> resize(const Arguments &args);
    static Handle<Value> zoomAll(const Arguments &args);
    static Handle<Value> zoomToBox(const Arguments &args);
    static Handle<Value> layers(const Arguments &args);
    static Handle<Value> scale(const Arguments &args);
    static Handle<Value> scaleDenominator(const Arguments &args);
    static Handle<Value> queryPoint(const Arguments &args);
    static Handle<Value> queryMapPoint(const Arguments &args);
    static Handle<Value> abstractQueryPoint(const Arguments &args, bool geo_coords);
    static void EIO_QueryMap(uv_work_t* req);
    static void EIO_AfterQueryMap(uv_work_t* req);

    static Handle<Value> add_layer(const Arguments &args);
    static Handle<Value> get_layer(const Arguments &args);

    static Handle<Value> get_prop(Local<String> property,
                                  const AccessorInfo& info);
    static void set_prop(Local<String> property,
                         Local<Value> value,
                         const AccessorInfo& info);

    Map(int width, int height);
    Map(int width, int height, std::string const& srs);

    static Handle<Value> size(const Arguments &args);

    void acquire();
    void release();
    int active() const;
    int estimate_map_size();
    void _ref() { Ref(); }
    void _unref() { Unref(); }

    inline map_ptr get() { return map_; }

private:
    ~Map();
    map_ptr map_;
    int in_use_;
    int estimated_size_;
};

#endif
