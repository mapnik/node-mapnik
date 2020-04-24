#include "utils.hpp"
#include "mapnik_logger.hpp"
#include <mapnik/debug.hpp>

Napi::FunctionReference Logger::constructor;

/**
 * **`mapnik.Logger`**
 * 
 * No constructor - Severity level is only available via `mapnik.Logger` static instance.
 *
 * @class Logger
 * @example
 * mapnik.Logger.setSeverity(mapnik.Logger.NONE);
 * var log = mapnik.Logger.get_severity();
 * console.log(log); // 3
 */
void Logger::Initialize(Napi::Object target) {
    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, Logger::New);

    lcons->SetClassName(Napi::String::New(env, "Logger"));

    // Static methods
    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(), "getSeverity", Logger::get_severity);
    Napi::SetMethod(Napi::GetFunction(lcons).As<Napi::Object>(), "setSeverity", Logger::set_severity);

    // Constants
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),"NONE",mapnik::logger::severity_type::none);
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),"ERROR",mapnik::logger::severity_type::error);
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),"DEBUG",mapnik::logger::severity_type::debug);
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),"WARN",mapnik::logger::severity_type::warn);

    // What about booleans like:
    // ENABLE_STATS
    // ENABLE_LOG
    // DEFAULT_LOG_SEVERITY
    // RENDERING_STATS
    // DEBUG

    // Not sure if needed...
    (target).Set(Napi::String::New(env, "Logger"), Napi::GetFunction(lcons));
    constructor.Reset(lcons);

}

Napi::Value Logger::New(const Napi::CallbackInfo& info){
    Napi::Error::New(env, "a mapnik.Logger cannot be created directly - rather you should ....").ThrowAsJavaScriptException();
    return env.Null();
}

/**
 * Returns integer which represents severity level
 * @name get_severity
 * @memberof Logger
 * @static
 * @returns {number} severity level
 */
Napi::Value Logger::get_severity(const Napi::CallbackInfo& info){
    int severity = mapnik::logger::instance().get_severity();
    return Napi::New(env, severity);
}

/**
 * Accepts level of severity as a mapnik constant
 *
 * Available security levels
 *
 * @name set_severity
 * @memberof Logger
 * @static
 * @param {number} severity - severity level
 * @returns {number} severity level
 */
Napi::Value Logger::set_severity(const Napi::CallbackInfo& info){
    if (info.Length() != 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "requires a severity level parameter").ThrowAsJavaScriptException();
        return env.Null();
    }

    int severity = info[0].As<Napi::Number>().Int32Value();
    mapnik::logger::instance().set_severity(static_cast<mapnik::logger::severity_type>(severity));
    return;
}
