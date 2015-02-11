#ifndef __NODE_MAPNIK_LOGGER_H__
#define __NODE_MAPNIK_LOGGER_H__

#include <nan.h>

//Forward declaration of mapnik logger
namespace mapnik { class logger; }

class Logger: public node::ObjectWrap {
public:
    // V8 way of...
    static Persistent<FunctionTemplate> constructor;

    // Initialize function is needed for all addons
    static void Initialize(Handle<Object> target);

    // Nan_Method when new Logger object is instantiated
    static NAN_METHOD(New);

    // Get and set functions
    // Are these the only methods available in logger?
    static NAN_METHOD(get_severity);
    static NAN_METHOD(set_severity);
    static NAN_METHOD(evoke_error);

private:
    // Default Constructor
    Logger();
    // Deconstructor
    ~Logger();
};

#endif