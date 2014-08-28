#include "utils.hpp"
#include "mapnik_logger.hpp"

// mapnik
#include <mapnik/wkt/wkt_factory.hpp>

// boost
#include MAPNIK_MAKE_SHARED_INCLUDE

Persistent<FunctionTemplate> Logger::constructor;

// Sets up everything for the Logger object when the addon is initiatlized
void Logger::Initialize(Handle<Object> target) {
	// Do I need to capture scope if the object will be a singleton?
	NanScope();

	Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Logger::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Logger"));

    // Statis methods
    // Points to function reference of static obejct?
	NODE_SET_METHOD(lcons->GetFunction(), "get_severity", Logger::get_serverity);
    NODE_SET_METHOD(lcons->GetFunction(), "set_severity", Logger::set_serverity);

    // Not sure if needed...
    target->Set(NanNew("Logger"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);

}

NAN_METHOD(Logger::New){
	NanScope();

	if (?)
    {
        //...
    }
    else
    {
        NanThrowError("a mapnik.Logger cannot be created directly - rather you should ....");
        NanReturnUndefined();
    }
    
    //NanReturnValue(?);

}

NAN_GETTER(Logger::get_serverity){
	NanScope();
	
	//NanReturnValue(severity);
}

NAN_SETTER(Logger::set_serverity){
	NanScope();

	//NanReturnUndefined();
}