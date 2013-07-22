#ifndef __NODE_MAPNIK_RENDER_H__
#define __NODE_MAPNIK_RENDER_H__


// v8
#include <v8.h>

using namespace v8;

namespace node_mapnik {

Handle<Value> render(const Arguments& args);

}

#endif // __NODE_MAPNIK_RENDER_H__
