#include "utils.hpp"
#include "mapnik_svg.hpp"

#include <boost/make_shared.hpp>

Persistent<FunctionTemplate> SVG::constructor;

void SVG::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(SVG::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("SVG"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "width", width);
    NODE_SET_PROTOTYPE_METHOD(constructor, "height", height);
    NODE_SET_PROTOTYPE_METHOD(constructor, "extent", extent);

    NODE_SET_METHOD(constructor->GetFunction(),
                    "open",
                    SVG::open);
    NODE_SET_METHOD(constructor->GetFunction(),
                    "fromString",
                    SVG::fromString);

    target->Set(String::NewSymbol("SVG"),constructor->GetFunction());
}

SVG::SVG() :
ObjectWrap() {}

SVG::~SVG()
{
}

Handle<Value> SVG::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        SVG* vector =  static_cast<SVG*>(ptr);
        vector->Wrap(args.This());
        return args.This();
    }

    return ThrowException(Exception::TypeError(
                              String::New("cannot be intialized using constructor instead use static methods: open or fromString")));
}

SVG * SVG::New(mapnik::svg_path_ptr this_)
{
    HandleScope scope;
    SVG* vector = new SVG();
    vector->this_ = this_;
    Local<Value> ext = External::New(vector);
    Local<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return ObjectWrap::Unwrap<SVG>(obj);
}

Handle<Value> SVG::open(const Arguments& args)
{
    HandleScope scope;
    if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(String::New(
                                                       "Argument must be a string")));
    }
    try {
        mapnik::svg_path_ptr marker = mapnik::read_svg_marker(TOSTR(args[0]));
        SVG* vector = new SVG();
        vector->this_ = marker;
        Handle<Value> ext = External::New(vector);
        Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
        return scope.Close(obj);
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

Handle<Value> SVG::fromString(const Arguments& args)
{
    HandleScope scope;
    if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(String::New(
                                                       "Argument must be a string")));
    }
    try {
        mapnik::svg_path_ptr marker = mapnik::read_svg_marker(TOSTR(args[0]),true);
        SVG* vector = new SVG();
        vector->this_ = marker;
        Handle<Value> ext = External::New(vector);
        Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
        return scope.Close(obj);
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

Handle<Value> SVG::width(const Arguments& args)
{
    HandleScope scope;
    SVG* vector = ObjectWrap::Unwrap<SVG>(args.This());
    return scope.Close(Integer::New(vector->get()->width()));
}

Handle<Value> SVG::height(const Arguments& args)
{
    HandleScope scope;
    SVG* vector = ObjectWrap::Unwrap<SVG>(args.This());
    return scope.Close(Integer::New(vector->get()->height()));
}

Handle<Value> SVG::extent(const Arguments& args)
{
    HandleScope scope;
    SVG* vector = ObjectWrap::Unwrap<SVG>(args.This());
    Local<Array> arr = Array::New(4);
    mapnik::box2d<double> const& e = vector->get()->bounding_box();
    arr->Set(0,Number::New(e.minx()));
    arr->Set(1,Number::New(e.miny()));
    arr->Set(2,Number::New(e.maxx()));
    arr->Set(3,Number::New(e.maxy()));
    return scope.Close(arr);
}
