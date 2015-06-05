#include "mapnik_featureset.hpp"
#include "mapnik_feature.hpp"

Persistent<FunctionTemplate> Featureset::constructor;

/**
 * An iterator of {@link mapnik.Feature} objects.
 *
 * @name mapnik.Featureset
 * @class
 */
void Featureset::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Featureset::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Featureset"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "next", next);

    target->Set(NanNew("Featureset"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Featureset::Featureset() :
    node::ObjectWrap(),
    this_() {}

Featureset::~Featureset()
{
}

NAN_METHOD(Featureset::New)
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
        Featureset* fs =  static_cast<Featureset*>(ptr);
        fs->Wrap(args.This());
        NanReturnValue(args.This());
    }

    NanThrowTypeError("Sorry a Featureset cannot currently be created, only accessed via an existing datasource");
    NanReturnUndefined();
}

/**
 * Return the next Feature in this featureset if it exists, or `null` if it
 * does not.
 *
 * @name next
 * @instance
 * @memberof mapnik.Featureset
 * @returns {mapnik.Feature|null} next feature
 */
NAN_METHOD(Featureset::next)
{
    NanScope();

    Featureset* fs = node::ObjectWrap::Unwrap<Featureset>(args.Holder());

    if (fs->this_) {
        mapnik::feature_ptr fp;
        try
        {
            fp = fs->this_->next();
        }
        catch (std::exception const& ex)
        {
            NanThrowError(ex.what());
            NanReturnUndefined();
        }

        if (fp) {
            NanReturnValue(Feature::NewInstance(fp));
        }
    }
    NanReturnUndefined();
}

Handle<Value> Featureset::NewInstance(mapnik::featureset_ptr fs_ptr)
{
    NanEscapableScope();
    Featureset* fs = new Featureset();
    fs->this_ = fs_ptr;
    Handle<Value> ext = NanNew<External>(fs);
    Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
    return NanEscapeScope(obj);
}
