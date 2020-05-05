#ifndef __NODE_MAPNIK_MEMORY_DATASOURCE_H__
#define __NODE_MAPNIK_MEMORY_DATASOURCE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

#include <mapnik/datasource.hpp>



namespace mapnik { class transcoder; }

class MemoryDatasource : public Napi::ObjectWrap<MemoryDatasource> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);
    static Napi::Value NewInstance(mapnik::datasource_ptr ds_ptr);
    static Napi::Value parameters(Napi::CallbackInfo const& info);
    static Napi::Value describe(Napi::CallbackInfo const& info);
    static Napi::Value features(Napi::CallbackInfo const& info);
    static Napi::Value featureset(Napi::CallbackInfo const& info);
    static Napi::Value add(Napi::CallbackInfo const& info);
    static Napi::Value fields(Napi::CallbackInfo const& info);

    MemoryDatasource();
    inline mapnik::datasource_ptr get() { return datasource_; }

private:
    ~MemoryDatasource();
    mapnik::datasource_ptr datasource_;
    unsigned int feature_id_;
    mapnik::transcoder tr_;
};

#endif
