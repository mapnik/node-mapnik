
#include "mapnik_request.hpp"

//#include "mapnik3x_compatibility.hpp"
//#include MAPNIK_SHARED_INCLUDE

// mapnik
#include <mapnik/request.hpp>
#include <mapnik/box2d.hpp>

Persistent<FunctionTemplate> Request::constructor;

void Request::Initialize(Handle<Object> target)
{
    NanScope();
    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Request::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Request"));
    target->Set(NanNew("Request"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Request::Request(unsigned width, unsigned height, mapnik::box2d<double> const& extent) :
    ObjectWrap(),
    req_(width,height,extent) {}

Request::~Request() {}

NAN_METHOD(Request::New)
{
    NanScope();

    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args.Length() == 3)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
        {
            NanThrowTypeError("'width' and 'height' must be integers");
            NanReturnUndefined();
        }
        if (!args[2]->IsArray()) {
            NanThrowError("Third arg must be an array of: [minx,miny,maxx,maxy]");
            NanReturnUndefined();
        }
        Local<Array> arr = args[2].As<Array>();
        if (arr->Length() != 4) {
            NanThrowError("Third arg must be an array of: [minx,miny,maxx,maxy]");
            NanReturnUndefined();
        }
        double minx = arr->Get(0)->NumberValue();
        double miny = arr->Get(1)->NumberValue();
        double maxx = arr->Get(2)->NumberValue();
        double maxy = arr->Get(3)->NumberValue();
        mapnik::box2d<double> extent(minx,miny,maxx,maxy);
        Request* req = new Request(args[0]->IntegerValue(),args[1]->IntegerValue(),extent);
        req->Wrap(args.This());
        NanReturnValue(args.This());
    }
    NanThrowError("please provide Request width, height, and extent");
    NanReturnUndefined();
}
