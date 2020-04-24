#ifndef __NODE_MAPNIK_LOGGER_H__
#define __NODE_MAPNIK_LOGGER_H__

#include <napi.h>
#include <uv.h>

//Forward declaration of mapnik logger
namespace mapnik { class logger; }

class Logger : public Napi::ObjectWrap<Logger> {
public:
    // V8 way of...
    static Napi::FunctionReference constructor;

    // Initialize function is needed for all addons
    static void Initialize(Napi::Object target);

    // Nan_Method when new Logger object is instantiated
    static Napi::Value New(const Napi::CallbackInfo& info);

    // Get and set functions
    // Are these the only methods available in logger?
    static Napi::Value get_severity(const Napi::CallbackInfo& info);
    static Napi::Value set_severity(const Napi::CallbackInfo& info);
    static Napi::Value evoke_error(const Napi::CallbackInfo& info);

private:
    // Default Constructor
    Logger();
    // Deconstructor
    ~Logger();
};

#endif
