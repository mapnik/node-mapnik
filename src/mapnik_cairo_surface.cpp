#include "utils.hpp"
#include "mapnik_cairo_surface.hpp"
using namespace v8;

Persistent<FunctionTemplate> CairoSurface::constructor;

void CairoSurface::Initialize(Handle<Object> target) {
    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(CairoSurface::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("CairoSurface"));
    NODE_SET_PROTOTYPE_METHOD(lcons, "width", width);
    NODE_SET_PROTOTYPE_METHOD(lcons, "height", height);
    NODE_SET_PROTOTYPE_METHOD(lcons, "getData", getData);
    target->Set(NanNew("CairoSurface"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

CairoSurface::CairoSurface(std::string const& format, unsigned int width, unsigned int height) :
    node::ObjectWrap(),
    ss_(),
    width_(width),
    height_(height),
    format_(format)
{
}

CairoSurface::~CairoSurface()
{
}

NAN_METHOD(CairoSurface::New)
{
    NanScope();
    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args[0]->IsExternal())
    {
        Local<External> ext = args[0].As<External>();
        void* ptr = ext->Value();
        CairoSurface* im =  static_cast<CairoSurface*>(ptr);
        im->Wrap(args.This());
        NanReturnValue(args.This());
    }

    if (args.Length() == 3)
    {
        if (!args[0]->IsString())
        {
            NanThrowTypeError("CairoSurface 'format' must be a string");
            NanReturnUndefined();
        }
        std::string format = TOSTR(args[0]);
        if (!args[1]->IsNumber() || !args[2]->IsNumber())
        {
            NanThrowTypeError("CairoSurface 'width' and 'height' must be a integers");
            NanReturnUndefined();
        }
        CairoSurface* im = new CairoSurface(format, args[1]->IntegerValue(), args[2]->IntegerValue());
        im->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else
    {
        NanThrowError("CairoSurface requires three arguments: format, width, and height");
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

NAN_METHOD(CairoSurface::width)
{
    NanScope();

    CairoSurface* im = node::ObjectWrap::Unwrap<CairoSurface>(args.Holder());
    NanReturnValue(NanNew(im->width()));
}

NAN_METHOD(CairoSurface::height)
{
    NanScope();

    CairoSurface* im = node::ObjectWrap::Unwrap<CairoSurface>(args.Holder());
    NanReturnValue(NanNew(im->height()));
}

NAN_METHOD(CairoSurface::getData)
{
    NanScope();
    CairoSurface* surface = node::ObjectWrap::Unwrap<CairoSurface>(args.Holder());
    std::string s = surface->ss_.str();
    NanReturnValue(NanNewBufferHandle((char*)s.data(), s.size()));
}
