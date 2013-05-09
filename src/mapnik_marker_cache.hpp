#ifndef __NODE_MAPNIK_MARKER_CACHE_H__
#define __NODE_MAPNIK_MARKER_CACHE_H__

#include <v8.h>

using namespace v8;

class MarkerCache {
public:
    static Persistent<Object> constructor;
    static void Initialize(Handle<Object> target);
    static Handle<Value> New(Arguments const& args);
    static Handle<Value> clear(Arguments const& args);
    static Handle<Value> remove(Arguments const& args);
    static Handle<Value> size(Arguments const& args);
    static Handle<Value> put(Arguments const& args);
    static Handle<Value> get(Arguments const& args);
    static Handle<Value> keys(Arguments const& args);
private:
    MarkerCache();
    ~MarkerCache();
};

#endif
