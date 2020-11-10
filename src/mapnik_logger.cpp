//#include "utils.hpp"
#include "mapnik_logger.hpp"
#include <mapnik/debug.hpp>

Napi::FunctionReference Logger::constructor;

Napi::Object Logger::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Logger", {
            StaticMethod<&Logger::get_severity>("getSeverity", prop_attr),
            StaticMethod<&Logger::set_severity>("setSeverity", prop_attr),
            StaticValue("NONE", Napi::Number::New(env, mapnik::logger::severity_type::none), napi_enumerable),
            StaticValue("ERROR", Napi::Number::New(env, mapnik::logger::severity_type::error), napi_enumerable),
            StaticValue("DEBUG", Napi::Number::New(env, mapnik::logger::severity_type::debug), napi_enumerable),
            StaticValue("WARN", Napi::Number::New(env, mapnik::logger::severity_type::warn), napi_enumerable)
        });
    // clang-format on

    // What about booleans like:
    // ENABLE_STATS
    // ENABLE_LOG
    // DEFAULT_LOG_SEVERITY
    // RENDERING_STATS
    // DEBUG
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Logger", func);
    return exports;
}

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

Logger::Logger(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Logger>(info)
{
    Napi::Env env = info.Env();
    Napi::Error::New(env, "a mapnik.Logger cannot be created directly - rather you should ....").ThrowAsJavaScriptException();
}

/**
 * Returns integer which represents severity level
 * @name get_severity
 * @memberof Logger
 * @static
 * @returns {number} severity level
 */
Napi::Value Logger::get_severity(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    int severity = mapnik::logger::instance().get_severity();
    return Napi::Number::New(env, severity);
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
Napi::Value Logger::set_severity(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 1 || !info[0].IsNumber())
    {
        Napi::TypeError::New(env, "requires a severity level parameter").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int severity = info[0].As<Napi::Number>().Int32Value();
    mapnik::logger::instance().set_severity(static_cast<mapnik::logger::severity_type>(severity));
    return env.Undefined();
}
