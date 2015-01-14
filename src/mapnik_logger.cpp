#include "utils.hpp"
#include "mapnik_logger.hpp"
#include <mapnik/debug.hpp>

Persistent<FunctionTemplate> Logger::constructor;

// Sets up everything for the Logger object when the addon is initialized
void Logger::Initialize(Handle<Object> target) {
    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Logger::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Logger"));

    // Static methods
    NODE_SET_METHOD(lcons->GetFunction(), "getSeverity", Logger::get_severity);
    NODE_SET_METHOD(lcons->GetFunction(), "setSeverity", Logger::set_severity);

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
    target->Set(NanNew("Logger"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);

}

NAN_METHOD(Logger::New){
    NanScope();
    NanThrowError("a mapnik.Logger cannot be created directly - rather you should ....");
    NanReturnUndefined();
}

NAN_METHOD(Logger::get_severity){
    NanScope();
    int severity = mapnik::logger::instance().get_severity();
    NanReturnValue(NanNew(severity));
}

NAN_METHOD(Logger::set_severity){
    NanScope();

    if (args.Length() < 1 || !args[0]->IsNumber()) {
        NanThrowTypeError("requires a severity level parameter");
        NanReturnUndefined();
    }

    int severity = args[0]->IntegerValue();
    mapnik::logger::instance().set_severity(static_cast<mapnik::logger::severity_type>(severity));
    NanReturnUndefined();
}