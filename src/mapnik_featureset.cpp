#include "mapnik_featureset.hpp"
#include "mapnik_feature.hpp"
#include "utils.hpp"

Persistent<FunctionTemplate> Featureset::constructor;

void Featureset::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Featureset::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Featureset"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "next", next);

    target->Set(String::NewSymbol("Featureset"),constructor->GetFunction());
}

Featureset::Featureset() :
    ObjectWrap(),
    this_() {}

Featureset::~Featureset()
{
}

Handle<Value> Featureset::New(const Arguments& args)
{
    HandleScope scope;

    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Featureset* fs =  static_cast<Featureset*>(ptr);
        fs->Wrap(args.This());
        return args.This();
    }

    return ThrowException(Exception::TypeError(
                              String::New("Sorry a Featureset cannot currently be created, only accessed via an existing datasource")));
}

Handle<Value> Featureset::next(const Arguments& args)
{
    HandleScope scope;

    Featureset* fs = ObjectWrap::Unwrap<Featureset>(args.This());

    if (fs->this_) {
        mapnik::feature_ptr fp;
        try
        {
            fp = fs->this_->next();
        }
        catch (std::exception const& ex)
        {
            return ThrowException(Exception::Error(
                                      String::New(ex.what())));
        }
        catch (...)
        {
            return ThrowException(Exception::Error(
                                      String::New("unknown exception happened when accessing a feature with next(), please file bug")));
        }
        if (fp) {
            return scope.Close(Feature::New(fp));
        }
    }
    return Undefined();
}

Handle<Value> Featureset::New(mapnik::featureset_ptr fs_ptr)
{
    HandleScope scope;
    Featureset* fs = new Featureset();
    fs->this_ = fs_ptr;
    Handle<Value> ext = External::New(fs);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);
}
