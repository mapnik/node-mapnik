#include "utils.hpp"
#include "mapnik_logger.hpp"
#include <mapnik/debug.hpp>

Nan::Persistent<v8::FunctionTemplate> Logger::constructor;

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
void Logger::Initialize(v8::Local<v8::Object> target) {
    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Logger::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Logger").ToLocalChecked());

    // Static methods
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(), "getSeverity", Logger::get_severity);
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(), "setSeverity", Logger::set_severity);

    // Constants
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),"NONE",mapnik::logger::severity_type::none);
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),"ERROR",mapnik::logger::severity_type::error);
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),"DEBUG",mapnik::logger::severity_type::debug);
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),"WARN",mapnik::logger::severity_type::warn);

    // What about booleans like:
    // ENABLE_STATS
    // ENABLE_LOG
    // DEFAULT_LOG_SEVERITY
    // RENDERING_STATS
    // DEBUG

    // Not sure if needed...
    target->Set(Nan::New("Logger").ToLocalChecked(),lcons->GetFunction());
    constructor.Reset(lcons);

}

NAN_METHOD(Logger::New){
    Nan::ThrowError("a mapnik.Logger cannot be created directly - rather you should ....");
    return;
}

/**
 * Returns integer which represents severity level
 * @name get_severity
 * @memberof Logger
 * @static
 * @returns {number} severity level
 */
NAN_METHOD(Logger::get_severity){
    int severity = mapnik::logger::instance().get_severity();
    info.GetReturnValue().Set(Nan::New(severity));
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
NAN_METHOD(Logger::set_severity){
    if (info.Length() != 1 || !info[0]->IsNumber()) {
        Nan::ThrowTypeError("requires a severity level parameter");
        return;
    }

    int severity = info[0]->IntegerValue();
    mapnik::logger::instance().set_severity(static_cast<mapnik::logger::severity_type>(severity));
    return;
}
