#ifndef __NODE_MAPNIK_PROJECTION_H__
#define __NODE_MAPNIK_PROJECTION_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

// stl
#include <string>
#include <memory>

namespace mapnik { class proj_transform; }
namespace mapnik { class projection; }



typedef std::shared_ptr<mapnik::projection> proj_ptr;

class Projection : public Napi::ObjectWrap<Projection> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);

    static Napi::Value inverse(Napi::CallbackInfo const& info);
    static Napi::Value forward(Napi::CallbackInfo const& info);

    explicit Projection(std::string const& name, bool defer_init);

    inline proj_ptr get() { return projection_; }

private:
    ~Projection();
    proj_ptr projection_;
};

typedef std::shared_ptr<mapnik::proj_transform> proj_tr_ptr;

class ProjTransform : public Napi::ObjectWrap<ProjTransform> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);

    static Napi::Value forward(Napi::CallbackInfo const& info);
    static Napi::Value backward(Napi::CallbackInfo const& info);

    ProjTransform(mapnik::projection const& src,
                  mapnik::projection const& dest);

    inline proj_tr_ptr get() { return this_; }

    using Napi::ObjectWrap::Ref;
    using Napi::ObjectWrap::Unref;

private:
    ~ProjTransform();
    proj_tr_ptr this_;
};


#endif
