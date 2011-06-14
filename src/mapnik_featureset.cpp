#include "mapnik_featureset.hpp"
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
    
    bool include_extent = false;
    
    if ((args.Length() > 0)) {
        if (!args[0]->IsBoolean())
            return ThrowException(Exception::TypeError(
               String::New("option to include extent must be a boolean")));
        include_extent = args[0]->BooleanValue();
    }

    Featureset* fs = ObjectWrap::Unwrap<Featureset>(args.This());

    // TODO - allow filtering which fields are returned
    if (fs->this_) {
        mapnik::feature_ptr fp = fs->this_->next();
        if (fp) {
            std::map<std::string,mapnik::value> const& fprops = fp->props();
            Local<Object> feat = Object::New();
            std::map<std::string,mapnik::value>::const_iterator it = fprops.begin();
            std::map<std::string,mapnik::value>::const_iterator end = fprops.end();
            for (; it != end; ++it)
            {
                node_mapnik::params_to_object serializer( feat , it->first);
                boost::apply_visitor( serializer, it->second.base() );
            }
            
            // add feature id
            feat->Set(String::NewSymbol("__id__"), Integer::New(fp->id()));
            
            if (include_extent) {
                Local<Array> a = Array::New(4);
                mapnik::box2d<double> const& e = fp->envelope();
                a->Set(0, Number::New(e.minx()));
                a->Set(1, Number::New(e.miny()));
                a->Set(2, Number::New(e.maxx()));
                a->Set(3, Number::New(e.maxy()));
                feat->Set(String::NewSymbol("_extent"),a);
            }
            return scope.Close(feat);
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
