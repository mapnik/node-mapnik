#include "utils.hpp"
#include "mapnik_proj_transform.hpp"
#include "mapnik_projection.hpp"

#include <boost/make_shared.hpp>

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
    this_(boost::make_shared<mapnik::proj_transform>(src,dest)) {}

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

    Projection *p1 = ObjectWrap::Unwrap<Projection>(src_obj);
    Projection *p2 = ObjectWrap::Unwrap<Projection>(dest_obj);
    
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
    ProjTransform* p = ObjectWrap::Unwrap<ProjTransform>(args.This());

    if (!args.Length() == 1)
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
    ProjTransform* p = ObjectWrap::Unwrap<ProjTransform>(args.This());

    if (!args.Length() == 1)
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

