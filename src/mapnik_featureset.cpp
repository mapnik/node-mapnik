#include "mapnik_featureset.hpp"
#include "mapnik_feature.hpp"

Nan::Persistent<v8::FunctionTemplate> Featureset::constructor;

/**
 * **`mapnik.Featureset`**
 *
 * An iterator of {@link mapnik.Feature} objects.
 *
 * @class Featureset
 */
void Featureset::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Featureset::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Featureset").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "next", next);

    Nan::Set(target, Nan::New("Featureset").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

Featureset::Featureset() :
    Nan::ObjectWrap(),
    this_() {}

Featureset::~Featureset()
{
}

NAN_METHOD(Featureset::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = info[0].As<v8::External>();
        void* ptr = ext->Value();
        Featureset* fs =  static_cast<Featureset*>(ptr);
        fs->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }

    Nan::ThrowTypeError("Sorry a Featureset cannot currently be created, only accessed via an existing datasource");
    return;
}

/**
 * Return the next Feature in this featureset if it exists, or `null` if it
 * does not.
 *
 * @name next
 * @instance
 * @memberof Featureset
 * @returns {mapnik.Feature|null} next feature
 */
NAN_METHOD(Featureset::next)
{
    Featureset* fs = Nan::ObjectWrap::Unwrap<Featureset>(info.Holder());
    if (fs->this_) {
        mapnik::feature_ptr fp;
        try
        {
            fp = fs->this_->next();
        }
        catch (std::exception const& ex)
        {
            // It is not immediately obvious how this could cause an exception, a check of featureset plugin
            // implementations resulted in no obvious way that an exception could be raised. Therefore, it
            // is not obvious currently what could raise this exception. However, since a plugin could possibly
            // be developed outside of mapnik core plugins that could raise here we are probably best still
            // wrapping this in a try catch.
            /* LCOV_EXCL_START */
            Nan::ThrowError(ex.what());
            return;
            /* LCOV_EXCL_STOP */
        }

        if (fp) {
            info.GetReturnValue().Set(Feature::NewInstance(fp));
        }
    }
    return;
}

v8::Local<v8::Value> Featureset::NewInstance(mapnik::featureset_ptr fsp)
{
    Nan::EscapableHandleScope scope;
    Featureset* fs = new Featureset();
    fs->this_ = fsp;
    v8::Local<v8::Value> ext = Nan::New<v8::External>(fs);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Featureset instance");
    return scope.Escape(maybe_local.ToLocalChecked());
}
