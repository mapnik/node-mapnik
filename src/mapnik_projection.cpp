#include "mapnik_projection.hpp"
#include "utils.hpp"

#include <mapnik/box2d.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/projection.hpp>
#include <sstream>

Persistent<FunctionTemplate> Projection::constructor;

void Projection::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Projection::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Projection"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "forward", forward);
    NODE_SET_PROTOTYPE_METHOD(lcons, "inverse", inverse);

    target->Set(NanNew("Projection"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Projection::Projection(std::string const& name, bool defer_init) :
    node::ObjectWrap(),
    projection_(std::make_shared<mapnik::projection>(name, defer_init)) {}

Projection::~Projection()
{
}

bool Projection::HasInstance(Handle<Value> val)
{
    NanScope();
    if (!val->IsObject()) return false;
    Handle<Object> obj = val.As<Object>();

    if (NanNew(constructor)->HasInstance(obj))
        return true;

    return false;
}

NAN_METHOD(Projection::New)
{
    NanScope();

    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args.Length() <= 0 || !args[0]->IsString()) {
        NanThrowTypeError("please provide a proj4 intialization string");
        NanReturnUndefined();
    }
    bool lazy = false;
    if (args.Length() >= 2)
    {
        if (!args[1]->IsObject())
        {
            NanThrowTypeError("The second parameter provided should be an options object");
            NanReturnUndefined();
        }
        Local<Object> options = args[1].As<Object>();
        if (options->Has(NanNew("lazy")))
        {
            Local<Value> lazy_opt = options->Get(NanNew("lazy"));
            if (!lazy_opt->IsBoolean())
            {
                NanThrowTypeError("'lazy' must be a Boolean");
                NanReturnUndefined();
            }
            lazy = lazy_opt->BooleanValue();
        }
    }
            
    try
    {
        Projection* p = new Projection(TOSTR(args[0]), lazy);
        p->Wrap(args.This());
        NanReturnValue(args.This());
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
}

NAN_METHOD(Projection::forward)
{
    NanScope();
    Projection* p = node::ObjectWrap::Unwrap<Projection>(args.Holder());

    try
    {
        if (args.Length() != 1) {
            NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            NanReturnUndefined();
        } else {
            if (!args[0]->IsArray()) {
                NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
                NanReturnUndefined();
            }
            Local<Array> a = args[0].As<Array>();
            unsigned int array_length = a->Length();
            if (array_length == 2)
            {
                double x = a->Get(0)->NumberValue();
                double y = a->Get(1)->NumberValue();
                p->projection_->forward(x,y);
                Local<Array> arr = NanNew<Array>(2);
                arr->Set(0, NanNew(x));
                arr->Set(1, NanNew(y));
                NanReturnValue(arr);
            }
            else if (array_length == 4)
            {
                double minx = a->Get(0)->NumberValue();
                double miny = a->Get(1)->NumberValue();
                double maxx = a->Get(2)->NumberValue();
                double maxy = a->Get(3)->NumberValue();
                p->projection_->forward(minx,miny);
                p->projection_->forward(maxx,maxy);
                Local<Array> arr = NanNew<Array>(4);
                arr->Set(0, NanNew(minx));
                arr->Set(1, NanNew(miny));
                arr->Set(2, NanNew(maxx));
                arr->Set(3, NanNew(maxy));
                NanReturnValue(arr);
            }
            else
            {
                NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
                NanReturnUndefined();
            }
        }
    } catch (std::exception const & ex) {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
}

NAN_METHOD(Projection::inverse)
{
    NanScope();
    Projection* p = node::ObjectWrap::Unwrap<Projection>(args.Holder());

    try
    {
        if (args.Length() != 1) {
            NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            NanReturnUndefined();
        } else {
            if (!args[0]->IsArray()) {
                NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
                NanReturnUndefined();
            }
            Local<Array> a = args[0].As<Array>();
            unsigned int array_length = a->Length();
            if (array_length == 2)
            {
                double x = a->Get(0)->NumberValue();
                double y = a->Get(1)->NumberValue();
                p->projection_->inverse(x,y);
                Local<Array> arr = NanNew<Array>(2);
                arr->Set(0, NanNew(x));
                arr->Set(1, NanNew(y));
                NanReturnValue(arr);
            }
            else if (array_length == 4)
            {
                double minx = a->Get(0)->NumberValue();
                double miny = a->Get(1)->NumberValue();
                double maxx = a->Get(2)->NumberValue();
                double maxy = a->Get(3)->NumberValue();
                p->projection_->inverse(minx,miny);
                p->projection_->inverse(maxx,maxy);
                Local<Array> arr = NanNew<Array>(4);
                arr->Set(0, NanNew(minx));
                arr->Set(1, NanNew(miny));
                arr->Set(2, NanNew(maxx));
                arr->Set(3, NanNew(maxy));
                NanReturnValue(arr);
            }
            else
            {
                NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
                NanReturnUndefined();
            }
        }
    } catch (std::exception const & ex) {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
}

Persistent<FunctionTemplate> ProjTransform::constructor;

void ProjTransform::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(ProjTransform::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("ProjTransform"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "forward", forward);
    NODE_SET_PROTOTYPE_METHOD(lcons, "backward", backward);

    target->Set(NanNew("ProjTransform"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

ProjTransform::ProjTransform(mapnik::projection const& src,
                             mapnik::projection const& dest) :
    node::ObjectWrap(),
    this_(std::make_shared<mapnik::proj_transform>(src,dest)) {}

ProjTransform::~ProjTransform()
{
}

NAN_METHOD(ProjTransform::New)
{
    NanScope();
    if (!args.IsConstructCall()) {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args.Length() != 2 || !args[0]->IsObject()  || !args[1]->IsObject()) {
        NanThrowTypeError("please provide two arguments: a pair of mapnik.Projection objects");
        NanReturnUndefined();
    }

    Local<Object> src_obj = args[0].As<Object>();
    if (src_obj->IsNull() || src_obj->IsUndefined() || !Projection::HasInstance(src_obj)) {
        NanThrowTypeError("mapnik.Projection expected for first argument");
        NanReturnUndefined();
    }

    Local<Object> dest_obj = args[1].As<Object>();
    if (dest_obj->IsNull() || dest_obj->IsUndefined() || !Projection::HasInstance(dest_obj)) {
        NanThrowTypeError("mapnik.Projection expected for second argument");
        NanReturnUndefined();
    }

    Projection *p1 = node::ObjectWrap::Unwrap<Projection>(src_obj);
    Projection *p2 = node::ObjectWrap::Unwrap<Projection>(dest_obj);

    try
    {
        ProjTransform* p = new ProjTransform(*p1->get(),*p2->get());
        p->Wrap(args.This());
        NanReturnValue(args.This());
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
}

NAN_METHOD(ProjTransform::forward)
{
    NanScope();
    ProjTransform* p = node::ObjectWrap::Unwrap<ProjTransform>(args.Holder());

    if (args.Length() != 1) {
        NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
        NanReturnUndefined();
    } else {
        if (!args[0]->IsArray()) {
            NanThrowTypeError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            NanReturnUndefined();
        }

        Local<Array> a = args[0].As<Array>();
        unsigned int array_length = a->Length();
        if (array_length == 2)
        {
            double x = a->Get(0)->NumberValue();
            double y = a->Get(1)->NumberValue();
            double z = 0;
            if (!p->this_->forward(x,y,z))
            {
                std::ostringstream s;
                s << "Failed to forward project "
                  << a->Get(0)->NumberValue() << "," << a->Get(1)->NumberValue() << " from " << p->this_->source().params() << " to " << p->this_->dest().params();
                NanThrowError(s.str().c_str());
                NanReturnUndefined();

            }
            Local<Array> arr = NanNew<Array>(2);
            arr->Set(0, NanNew(x));
            arr->Set(1, NanNew(y));
            NanReturnValue(arr);
        }
        else if (array_length == 4)
        {
            mapnik::box2d<double> box(a->Get(0)->NumberValue(),
                                      a->Get(1)->NumberValue(),
                                      a->Get(2)->NumberValue(),
                                      a->Get(3)->NumberValue());
            if (!p->this_->forward(box))
            {
                std::ostringstream s;
                s << "Failed to forward project "
                  << box << " from " << p->this_->source().params() << " to " << p->this_->dest().params();
                NanThrowError(s.str().c_str());
                NanReturnUndefined();
            }
            Local<Array> arr = NanNew<Array>(4);
            arr->Set(0, NanNew(box.minx()));
            arr->Set(1, NanNew(box.miny()));
            arr->Set(2, NanNew(box.maxx()));
            arr->Set(3, NanNew(box.maxy()));
            NanReturnValue(arr);
        }
        else
        {
            NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            NanReturnUndefined();
        }
    }
}

NAN_METHOD(ProjTransform::backward)
{
    NanScope();
    ProjTransform* p = node::ObjectWrap::Unwrap<ProjTransform>(args.Holder());

    if (args.Length() != 1) {
        NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
        NanReturnUndefined();
    } else {
        if (!args[0]->IsArray())
        {
            NanThrowTypeError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            NanReturnUndefined();
        }

        Local<Array> a = args[0].As<Array>();
        unsigned int array_length = a->Length();
        if (array_length == 2)
        {
            double x = a->Get(0)->NumberValue();
            double y = a->Get(1)->NumberValue();
            double z = 0;
            if (!p->this_->backward(x,y,z))
            {
                std::ostringstream s;
                s << "Failed to back project "
                  << a->Get(0)->NumberValue() << "," << a->Get(1)->NumberValue() << " from " << p->this_->dest().params() << " to: " << p->this_->source().params();
                NanThrowError(s.str().c_str());
                NanReturnUndefined();
            }
            Local<Array> arr = NanNew<Array>(2);
            arr->Set(0, NanNew(x));
            arr->Set(1, NanNew(y));
            NanReturnValue(arr);
        }
        else if (array_length == 4)
        {
            mapnik::box2d<double> box(a->Get(0)->NumberValue(),
                                      a->Get(1)->NumberValue(),
                                      a->Get(2)->NumberValue(),
                                      a->Get(3)->NumberValue());
            if (!p->this_->backward(box))
            {
                std::ostringstream s;
                s << "Failed to back project "
                  << box << " from " << p->this_->source().params() << " to " << p->this_->dest().params();
                NanThrowError(s.str().c_str());
                NanReturnUndefined();
            }
            Local<Array> arr = NanNew<Array>(4);
            arr->Set(0, NanNew(box.minx()));
            arr->Set(1, NanNew(box.miny()));
            arr->Set(2, NanNew(box.maxx()));
            arr->Set(3, NanNew(box.maxy()));
            NanReturnValue(arr);
        }
        else
        {
            NanThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            NanReturnUndefined();
        }
    }
}
