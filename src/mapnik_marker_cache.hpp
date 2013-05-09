#ifndef __NODE_MAPNIK_MARKER_CACHE_H__
#define __NODE_MAPNIK_MARKER_CACHE_H__

#include <v8.h>
#include <node_object_wrap.h>

using namespace v8;

class MarkerCache: public node::ObjectWrap {
public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(Arguments const& args);
    static Handle<Value> clear(Arguments const& args);
    static Handle<Value> remove(Arguments const& args);
    static Handle<Value> size(Arguments const& args);
    static Handle<Value> put(Arguments const& args);
    static Handle<Value> get(Arguments const& args);
    static Handle<Value> keys(Arguments const& args);
    MarkerCache();

private:
    ~MarkerCache();
};

#endif
