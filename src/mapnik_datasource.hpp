#ifndef __NODE_MAPNIK_DATASOURCE_H__
#define __NODE_MAPNIK_DATASOURCE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

#include <memory>



namespace mapnik { class datasource; }

typedef std::shared_ptr<mapnik::datasource> datasource_ptr;

class Datasource : public Napi::ObjectWrap<Datasource> {
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(const Napi::CallbackInfo& info);
    static Napi::Value NewInstance(datasource_ptr ds_ptr);

    static Napi::Value parameters(const Napi::CallbackInfo& info);
    static Napi::Value describe(const Napi::CallbackInfo& info);
    static Napi::Value featureset(const Napi::CallbackInfo& info);
    static Napi::Value extent(const Napi::CallbackInfo& info);
    static Napi::Value fields(const Napi::CallbackInfo& info);

    Datasource();
    inline datasource_ptr get() { return datasource_; }

private:
    ~Datasource();
    datasource_ptr datasource_;
};

#endif
