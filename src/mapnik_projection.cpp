
#include "mapnik_projection.hpp"
#include "utils.hpp"

// boost
#include MAPNIK_MAKE_SHARED_INCLUDE

Persistent<FunctionTemplate> Projection::constructor;

void Projection::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Projection::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Projection"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "forward", forward);
    NODE_SET_PROTOTYPE_METHOD(constructor, "inverse", inverse);

    target->Set(String::NewSymbol("Projection"),constructor->GetFunction());
}

Projection::Projection(std::string const& name) :
    ObjectWrap(),
    projection_(MAPNIK_MAKE_SHARED<mapnik::projection>(name)) {}

Projection::~Projection()
{
}

bool Projection::HasInstance(Handle<Value> val)
{
    if (!val->IsObject()) return false;
    Local<Object> obj = val->ToObject();

    if (constructor->HasInstance(obj))
        return true;

    return false;
}

Handle<Value> Projection::New(const Arguments& args)
{
    HandleScope scope;

    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args.Length() <= 0 || !args[0]->IsString()) {
        return ThrowException(Exception::TypeError(
                                  String::New("please provide a proj4 intialization string")));
    }
    try
    {
        Projection* p = new Projection(TOSTR(args[0]));
        p->Wrap(args.This());
        return args.This();
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

Handle<Value> Projection::forward(const Arguments& args)
{
    HandleScope scope;
    Projection* p = node::ObjectWrap::Unwrap<Projection>(args.This());

    try
    {
        if (args.Length() != 1)
            return ThrowException(Exception::Error(
                                      String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
        else
        {
            if (!args[0]->IsArray())
                return ThrowException(Exception::Error(
                                          String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
            Local<Array> a = Local<Array>::Cast(args[0]);
            unsigned int array_length = a->Length();
            if (array_length == 2)
            {
                double x = a->Get(0)->NumberValue();
                double y = a->Get(1)->NumberValue();
                p->projection_->forward(x,y);
                Local<Array> arr = Array::New(2);
                arr->Set(0, Number::New(x));
                arr->Set(1, Number::New(y));
                return scope.Close(arr);
            }
            else if (array_length == 4)
            {
                double minx = a->Get(0)->NumberValue();
                double miny = a->Get(1)->NumberValue();
                double maxx = a->Get(2)->NumberValue();
                double maxy = a->Get(3)->NumberValue();
                p->projection_->forward(minx,miny);
                p->projection_->forward(maxx,maxy);
                Local<Array> arr = Array::New(4);
                arr->Set(0, Number::New(minx));
                arr->Set(1, Number::New(miny));
                arr->Set(2, Number::New(maxx));
                arr->Set(3, Number::New(maxy));
                return scope.Close(arr);
            }
            else
                return ThrowException(Exception::Error(
                                          String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
        }
    } catch (std::exception const & ex) {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

Handle<Value> Projection::inverse(const Arguments& args)
{
    HandleScope scope;
    Projection* p = node::ObjectWrap::Unwrap<Projection>(args.This());

    try
    {
        if (args.Length() != 1)
            return ThrowException(Exception::Error(
                                      String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
        else
        {
            if (!args[0]->IsArray())
                return ThrowException(Exception::Error(
                                          String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
            Local<Array> a = Local<Array>::Cast(args[0]);
            unsigned int array_length = a->Length();
            if (array_length == 2)
            {
                double x = a->Get(0)->NumberValue();
                double y = a->Get(1)->NumberValue();
                p->projection_->inverse(x,y);
                Local<Array> arr = Array::New(2);
                arr->Set(0, Number::New(x));
                arr->Set(1, Number::New(y));
                return scope.Close(arr);
            }
            else if (array_length == 4)
            {
                double minx = a->Get(0)->NumberValue();
                double miny = a->Get(1)->NumberValue();
                double maxx = a->Get(2)->NumberValue();
                double maxy = a->Get(3)->NumberValue();
                p->projection_->inverse(minx,miny);
                p->projection_->inverse(maxx,maxy);
                Local<Array> arr = Array::New(4);
                arr->Set(0, Number::New(minx));
                arr->Set(1, Number::New(miny));
                arr->Set(2, Number::New(maxx));
                arr->Set(3, Number::New(maxy));
                return scope.Close(arr);
            }
            else
                return ThrowException(Exception::Error(
                                          String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
        }
    } catch (std::exception const & ex) {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

Persistent<FunctionTemplate> ProjTransform::constructor;

void ProjTransform::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(ProjTransform::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("ProjTransform"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "forward", forward);
    NODE_SET_PROTOTYPE_METHOD(constructor, "backward", backward);

    target->Set(String::NewSymbol("ProjTransform"),constructor->GetFunction());
}

ProjTransform::ProjTransform(mapnik::projection const& src,
                             mapnik::projection const& dest) :
    ObjectWrap(),
    this_(MAPNIK_MAKE_SHARED<mapnik::proj_transform>(src,dest)) {}

ProjTransform::~ProjTransform()
{
}

Handle<Value> ProjTransform::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args.Length() != 2 || !args[0]->IsObject()  || !args[1]->IsObject()) {
        return ThrowException(Exception::TypeError(
                                  String::New("please provide two arguments: a pair of mapnik.Projection objects")));
    }

    Local<Object> src_obj = args[0]->ToObject();
    if (src_obj->IsNull() || src_obj->IsUndefined() || !Projection::HasInstance(src_obj))
        return ThrowException(Exception::TypeError(String::New("mapnik.Projection expected for first argument")));

    Local<Object> dest_obj = args[1]->ToObject();
    if (dest_obj->IsNull() || dest_obj->IsUndefined() || !Projection::HasInstance(dest_obj))
        return ThrowException(Exception::TypeError(String::New("mapnik.Projection expected for second argument")));

    Projection *p1 = node::ObjectWrap::Unwrap<Projection>(src_obj);
    Projection *p2 = node::ObjectWrap::Unwrap<Projection>(dest_obj);

    try
    {
        ProjTransform* p = new ProjTransform(*p1->get(),*p2->get());
        p->Wrap(args.This());
        return args.This();
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
}

Handle<Value> ProjTransform::forward(const Arguments& args)
{
    HandleScope scope;
    ProjTransform* p = node::ObjectWrap::Unwrap<ProjTransform>(args.This());

    if (args.Length() != 1)
        return ThrowException(Exception::Error(
                                  String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
    else
    {
        if (!args[0]->IsArray())
            return ThrowException(Exception::Error(
                                      String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));

        Local<Array> a = Local<Array>::Cast(args[0]);
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
                return ThrowException(Exception::Error(
                                          String::New(s.str().c_str())));

            }
            Local<Array> arr = Array::New(2);
            arr->Set(0, Number::New(x));
            arr->Set(1, Number::New(y));
            return scope.Close(arr);
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
                return ThrowException(Exception::Error(
                                          String::New(s.str().c_str())));

            }
            Local<Array> arr = Array::New(4);
            arr->Set(0, Number::New(box.minx()));
            arr->Set(1, Number::New(box.miny()));
            arr->Set(2, Number::New(box.maxx()));
            arr->Set(3, Number::New(box.maxy()));
            return scope.Close(arr);
        }
        else
            return ThrowException(Exception::Error(
                                      String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));

    }
}

Handle<Value> ProjTransform::backward(const Arguments& args)
{
    HandleScope scope;
    ProjTransform* p = node::ObjectWrap::Unwrap<ProjTransform>(args.This());

    if (args.Length() != 1)
        return ThrowException(Exception::Error(
                                  String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
    else
    {
        if (!args[0]->IsArray())
        {
            return ThrowException(Exception::Error(
                                      String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
        }

        Local<Array> a = Local<Array>::Cast(args[0]);
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
                return ThrowException(Exception::Error(
                                          String::New(s.str().c_str())));

            }
            Local<Array> arr = Array::New(2);
            arr->Set(0, Number::New(x));
            arr->Set(1, Number::New(y));
            return scope.Close(arr);
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
                return ThrowException(Exception::Error(
                                          String::New(s.str().c_str())));

            }
            Local<Array> arr = Array::New(4);
            arr->Set(0, Number::New(box.minx()));
            arr->Set(1, Number::New(box.miny()));
            arr->Set(2, Number::New(box.maxx()));
            arr->Set(3, Number::New(box.maxy()));
            return scope.Close(arr);
        }
        else
            return ThrowException(Exception::Error(
                                      String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));

    }
}
