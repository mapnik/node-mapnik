#ifndef __NODE_MAPNIK_PROJECTION_H__
#define __NODE_MAPNIK_PROJECTION_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

// stl
#include <memory>
#include <string>

namespace mapnik {
class proj_transform;
}
namespace mapnik {
class projection;
}

typedef std::shared_ptr<mapnik::projection> proj_ptr;

class Projection : public Nan::ObjectWrap {
  public:
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);

    static NAN_METHOD(inverse);
    static NAN_METHOD(forward);

    explicit Projection(std::string const& name, bool defer_init);

    inline proj_ptr get() { return projection_; }

  private:
    ~Projection();
    proj_ptr projection_;
};

typedef std::shared_ptr<mapnik::proj_transform> proj_tr_ptr;

class ProjTransform : public Nan::ObjectWrap {
  public:
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);

    static NAN_METHOD(forward);
    static NAN_METHOD(backward);

    ProjTransform(mapnik::projection const& src,
                  mapnik::projection const& dest);

    inline proj_tr_ptr get() { return this_; }

    void _ref() { Ref(); }
    void _unref() { Unref(); }

  private:
    ~ProjTransform();
    proj_tr_ptr this_;
};

#endif
