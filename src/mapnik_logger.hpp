#ifndef __NODE_MAPNIK_LOGGER_H__
#define __NODE_MAPNIK_LOGGER_H__

#include <nan.h>
#include "mapnik3x_compatibility.hpp"

// boost
#include MAPNIK_SHARED_INCLUDE

using namespace v8;

//Not sure if I need to create a namespace for mapnik::logger
namespace mapnik { class logger; }
typedef MAPNIK_SHARED_PTR<mapnik::logger> logger_ptr;

class Logger: public node::ObjectWrap {
public:
    // Not sure what the Persistent object is.
    static Persistent<FunctionTemplate> constructor;

    // Initialize function is needed for all addons
    static void Initialize(Handle<Object> target);

    // Nan_Method when new Logger object is instantiated?
    static NAN_METHOD(New);

    // Get and set functions
    // Are these the only methods available in logger?
    static NAN_GETTER(get_severity);
    static NAN_SETTER(set_severity);

    // Default Constructor
    Logger();
    // Returns current Logger instance
    inline logger_ptr get() { return this_; }

private:
    // Deconstructor
    ~Logger();
    logger_ptr this_;
};

#endif