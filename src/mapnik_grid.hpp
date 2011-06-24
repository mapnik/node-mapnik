#ifndef __NODE_MAPNIK_GRID_H__
#define __NODE_MAPNIK_GRID_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <mapnik/grid/grid.hpp>
#include <boost/shared_ptr.hpp>

using namespace v8;
using namespace node;

typedef boost::shared_ptr<mapnik::grid> grid_ptr;

class Grid: public node::ObjectWrap {
  public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    static Handle<Value> encode(const Arguments &args);
    static Handle<Value> width(const Arguments &args);
    static Handle<Value> height(const Arguments &args);
    static Handle<Value> view(const Arguments &args);

    static Handle<Value> fields(const Arguments &args);

    static Handle<Value> get_prop(Local<String> property,
                         const AccessorInfo& info);
    static void set_prop(Local<String> property,
                         Local<Value> value,
                         const AccessorInfo& info);
    void _ref() { Ref(); }
    void _unref() { Unref(); }

    Grid(unsigned int width, unsigned int height, std::string const& key, unsigned int resolution);
    Grid(grid_ptr this_);
    inline grid_ptr get() { return this_; }

  private:
    ~Grid();
    grid_ptr this_;
};

#endif
