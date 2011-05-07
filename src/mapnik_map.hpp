#ifndef __NODE_MAPNIK_MAP_H__
#define __NODE_MAPNIK_MAP_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

// boost
#include <boost/shared_ptr.hpp>

#include <mapnik/map.hpp>
#include "mapnik_layer.hpp"


using namespace v8;
using namespace node;

typedef boost::shared_ptr<mapnik::Map> map_ptr;

class Map: public node::ObjectWrap {
  public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);

    static Handle<Value> loadSync(const Arguments &args);
    static Handle<Value> load(const Arguments &args);
    static int EIO_Load(eio_req *req);
    static int EIO_AfterLoad(eio_req *req);

    static Handle<Value> fromStringSync(const Arguments &args);
    static Handle<Value> fromString(const Arguments &args);
    static int EIO_FromString(eio_req *req);
    static int EIO_AfterFromString(eio_req *req);

    static Handle<Value> save(const Arguments &args);
    static Handle<Value> to_string(const Arguments &args);

    static Handle<Value> clear(const Arguments &args);
    static Handle<Value> resize(const Arguments &args);
    static Handle<Value> width(const Arguments &args);
    static Handle<Value> height(const Arguments &args);
    static Handle<Value> buffer_size(const Arguments &args);
    static Handle<Value> extent(const Arguments &args);
    static Handle<Value> zoom_all(const Arguments &args);
    static Handle<Value> zoom_to_box(const Arguments &args);
    static Handle<Value> render(const Arguments &args);
    static Handle<Value> render_to_string(const Arguments &args);
    static Handle<Value> render_to_file(const Arguments &args);
    static Handle<Value> layers(const Arguments &args);
    static Handle<Value> features(const Arguments &args);
    static Handle<Value> describe_data(const Arguments &args);
    static Handle<Value> scale_denominator(const Arguments &args);
    static Handle<Value> render_grid(const Arguments &args);
    static Handle<Value> renderSync(const Arguments &args);
    static Handle<Value> renderLayerSync(const Arguments &args);

    static Handle<Value> add_layer(const Arguments &args);
    static Handle<Value> get_layer(const Arguments &args);

    static Handle<Value> get_prop(Local<String> property,
                         const AccessorInfo& info);
    static void set_prop(Local<String> property,
                         Local<Value> value,
                         const AccessorInfo& info);

    static int EIO_Render(eio_req *req);
    static int EIO_AfterRender(eio_req *req);

    static int EIO_RenderGrid(eio_req *req);
    static int EIO_AfterRenderGrid(eio_req *req);
    
    Map(int width, int height);
    Map(int width, int height, std::string const& srs);

    void acquire();
    void release();
    int active() const;

  private:
    ~Map();
    map_ptr map_;
    int in_use_;
};

#endif
