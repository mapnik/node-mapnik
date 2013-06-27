// node
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

#include "utils.hpp"
#include "mapnik_cairo_surface.hpp"
using namespace v8;

Persistent<FunctionTemplate> CairoSurface::constructor;

void CairoSurface::Initialize(Handle<Object> target) {

    HandleScope scope;
    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(CairoSurface::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("CairoSurface"));
    NODE_SET_PROTOTYPE_METHOD(constructor, "width", width);
    NODE_SET_PROTOTYPE_METHOD(constructor, "height", height);
    NODE_SET_PROTOTYPE_METHOD(constructor, "getData", getData);
    target->Set(String::NewSymbol("CairoSurface"),constructor->GetFunction());
}

CairoSurface::CairoSurface(std::string const& format, unsigned int width, unsigned int height) :
    ObjectWrap(),
    width_(width),
    height_(height),
    format_(format)
{
}

CairoSurface::~CairoSurface()
{
}

Handle<Value> CairoSurface::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        CairoSurface* im =  static_cast<CairoSurface*>(ptr);
        im->Wrap(args.This());
        return args.This();
    }

    if (args.Length() == 3)
    {
        if (!args[0]->IsString())
            return ThrowException(Exception::Error(
                                      String::New("CairoSurface 'format' must be a string")));
        std::string format = TOSTR(args[0]);
        if (!args[1]->IsNumber() || !args[2]->IsNumber())
            return ThrowException(Exception::Error(
                                      String::New("CairoSurface 'width' and 'height' must be a integers")));
        CairoSurface* im = new CairoSurface(format,args[1]->IntegerValue(),args[2]->IntegerValue());
        im->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
                                  String::New("CairoSurface requires three arguments: format, width, and height")));
    }
    return Undefined();
}

Handle<Value> CairoSurface::width(const Arguments& args)
{
    HandleScope scope;

    CairoSurface* im = ObjectWrap::Unwrap<CairoSurface>(args.This());
    return scope.Close(Integer::New(im->width()));
}

Handle<Value> CairoSurface::height(const Arguments& args)
{
    HandleScope scope;

    CairoSurface* im = ObjectWrap::Unwrap<CairoSurface>(args.This());
    return scope.Close(Integer::New(im->height()));
}

Handle<Value> CairoSurface::getData(const Arguments& args)
{
    HandleScope scope;
    CairoSurface* surface = ObjectWrap::Unwrap<CairoSurface>(args.This());
    std::string s = surface->ss_.str();
    #if NODE_VERSION_AT_LEAST(0, 11, 0)
    return scope.Close(node::Buffer::New((char*)s.data(),s.size()));
    #else
    return scope.Close(node::Buffer::New((char*)s.data(),s.size())->handle_);
    #endif
}
