#ifndef __NODE_MAPNIK_FEATURESET_H__
#define __NODE_MAPNIK_FEATURESET_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

#include <mapnik/featureset.hpp>
#include <memory>



typedef mapnik::featureset_ptr fs_ptr;

class Featureset : public Napi::ObjectWrap<Featureset> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(const Napi::CallbackInfo& info);
    static Napi::Value NewInstance(mapnik::featureset_ptr fs_ptr);
    static Napi::Value next(const Napi::CallbackInfo& info);

    Featureset();

private:
    ~Featureset();
    fs_ptr this_;
};

#endif
