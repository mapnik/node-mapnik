#ifndef __NODE_MAPNIK_REQUEST_H__
#define __NODE_MAPNIK_REQUEST_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

#include <mapnik/request.hpp>
#include <mapnik/box2d.hpp>

// stl
#include <string>

using namespace v8;

class Request: public node::ObjectWrap {
 public:
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    Request(unsigned width, unsigned height, mapnik::box2d<double> const& extent);
    mapnik::request const& instance() const { return req_; }
 private:
    ~Request();
    mapnik::request req_;
};

#endif
