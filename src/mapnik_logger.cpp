#include "utils.hpp"
#include "mapnik_logger.hpp"

// boost
#include MAPNIK_MAKE_SHARED_INCLUDE

Persistent<FunctionTemplate> Logger::constructor;

// Sets up everything for the Logger object when the addon is initiatlized
void Logger::Initialize(Handle<Object> target) {
	NanScope();

	Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Logger::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Logger"));

    // Static methods
    // Points to function reference of static obejct?
	NODE_SET_METHOD(lcons->GetFunction(), "get_severity", Logger::get_serverity);
    NODE_SET_METHOD(lcons->GetFunction(), "set_severity", Logger::set_serverity);

    // Constants
    NODE_MAPNIK_DEFINE_CONSTANTS(Icons->GetFunction(),"None",MAPNIK_LOG_NONE);
    NODE_MAPNIK_DEFINE_CONSTANTS(Icons->GetFunction(),"Error",MAPNIK_LOG_ERROR);
    NODE_MAPNIK_DEFINE_CONSTANTS(Icons->GetFunction(),"Debug",MAPNIK_LOG_DEBUG);
    NODE_MAPNIK_DEFINE_CONSTANTS(Icons->GetFunction(),"Warn",MAPNIK_LOG_WARN);

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

NAN_GETTER(Logger::get_serverity){
	NanScope();
	
	//NanReturnValue(severity);
}

NAN_SETTER(Logger::set_serverity){
	NanScope();

	//NanReturnUndefined();
}