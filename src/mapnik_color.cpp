#include "mapnik_color.hpp"

#include "node.h"                       // for NODE_SET_PROTOTYPE_METHOD
#include "node_object_wrap.h"           // for ObjectWrap
#include "utils.hpp"                    // for ATTR, TOSTR
#include "v8.h"                         // for Handle, String, Integer, etc

// mapnik
#include <mapnik/color.hpp>             // for color

// stl
#include <exception>                    // for exception

// boost
#include <boost/make_shared.hpp>

Persistent<FunctionTemplate> Color::constructor;


void Color::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Color::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Color"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(constructor, "hex", hex);
    NODE_SET_PROTOTYPE_METHOD(constructor, "toString", toString);

    // properties
    ATTR(constructor, "r", get_prop, set_prop);
    ATTR(constructor, "g", get_prop, set_prop);
    ATTR(constructor, "b", get_prop, set_prop);
    ATTR(constructor, "a", get_prop, set_prop);


    target->Set(String::NewSymbol("Color"),constructor->GetFunction());
}

Color::Color() :
    ObjectWrap(),
    this_() {}

Color::~Color()
{
}

Handle<Value> Color::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Color* c = static_cast<Color*>(ptr);
        c->Wrap(args.This());
        return args.This();
    }

    color_ptr c_p;
    try
    {

        if (args.Length() == 1 && args[0]->IsString()){

            c_p = boost::make_shared<mapnik::color>(TOSTR(args[0]));

        } else if (args.Length() == 3) {

            int r = args[0]->IntegerValue();
            int g = args[1]->IntegerValue();
            int b = args[2]->IntegerValue();
            c_p = boost::make_shared<mapnik::color>(r,g,b);

        } else if (args.Length() == 4) {

            int r = args[0]->IntegerValue();
            int g = args[1]->IntegerValue();
            int b = args[2]->IntegerValue();
            int a = args[3]->IntegerValue();
            c_p = boost::make_shared<mapnik::color>(r,g,b,a);
        } else {
            return ThrowException(Exception::Error(
                                      String::New("invalid arguments: colors can be created from a string, integer r,g,b values, or integer r,g,b,a values")));


        }
        // todo allow int,int,int and int,int,int,int contructor

    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::Error(
                                  String::New("unknown exception happened, please file bug")));
    }

    if (c_p)
    {
        Color* c = new Color();
        c->Wrap(args.This());
        c->this_ = c_p;
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
                                  String::New("unknown exception happened, please file bug")));
    }

    return Undefined();
}

Handle<Value> Color::New(mapnik::color const& color) {
    HandleScope scope;
    Color* c = new Color();
    c->this_ = boost::make_shared<mapnik::color>(color);
    Handle<Value> ext = External::New(c);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);
}


Handle<Value> Color::get_prop(Local<String> property,
                              const AccessorInfo& info)
{
    HandleScope scope;
    Color* c = ObjectWrap::Unwrap<Color>(info.This());
    std::string a = TOSTR(property);
    if (a == "a")
        return scope.Close(Integer::New(c->get()->alpha()));
    else if (a == "r")
        return scope.Close(Integer::New(c->get()->red()));
    else if (a == "g")
        return scope.Close(Integer::New(c->get()->green()));
    else if (a == "b")
        return scope.Close(Integer::New(c->get()->blue()));
    return Undefined();
}

void Color::set_prop(Local<String> property,
                     Local<Value> value,
                     const AccessorInfo& info)
{
    HandleScope scope;
    Color* c = ObjectWrap::Unwrap<Color>(info.This());
    std::string a = TOSTR(property);
    if (!value->IsNumber())
        ThrowException(Exception::TypeError(
                           String::New("color channel value must be an integer")));
    if (a == "a") {
        c->get()->set_alpha(value->IntegerValue());
    } else if (a == "r") {
        c->get()->set_red(value->IntegerValue());
    } else if (a == "g") {
        c->get()->set_green(value->IntegerValue());
    } else if (a == "b") {
        c->get()->set_blue(value->IntegerValue());
    }
}

Handle<Value> Color::toString(const Arguments& args)
{
    HandleScope scope;

    Color* c = ObjectWrap::Unwrap<Color>(args.This());
    return scope.Close(String::New( c->get()->to_string().c_str() ));
}


Handle<Value> Color::hex(const Arguments& args)
{
    HandleScope scope;

    Color* c = ObjectWrap::Unwrap<Color>(args.This());
    std::string hex = c->get()->to_hex_string();
    return scope.Close(String::New( hex.c_str() ));
}
