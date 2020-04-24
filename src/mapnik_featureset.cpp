#include "mapnik_featureset.hpp"
#include "mapnik_feature.hpp"

Napi::FunctionReference Featureset::constructor;

/**
 * **`mapnik.Featureset`**
 *
 * An iterator of {@link mapnik.Feature} objects.
 *
 * @class Featureset
 */
void Featureset::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, Featureset::New);

    lcons->SetClassName(Napi::String::New(env, "Featureset"));

    InstanceMethod("next", &next),

    (target).Set(Napi::String::New(env, "Featureset"), Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

Featureset::Featureset() : Napi::ObjectWrap<Featureset>(),
    this_() {}

Featureset::~Featureset()
{
}

Napi::Value Featureset::New(const Napi::CallbackInfo& info)
{
    if (!info.IsConstructCall())
    {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info[0].IsExternal())
    {
        Napi::External ext = info[0].As<Napi::External>();
        void* ptr = ext->Value();
        Featureset* fs =  static_cast<Featureset*>(ptr);
        fs->Wrap(info.This());
        return info.This();
        return;
    }

    Napi::TypeError::New(env, "Sorry a Featureset cannot currently be created, only accessed via an existing datasource").ThrowAsJavaScriptException();
    return env.Null();
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
Napi::Value Featureset::next(const Napi::CallbackInfo& info)
{
    Featureset* fs = info.Holder().Unwrap<Featureset>();
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
            Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
            return env.Null();
            /* LCOV_EXCL_STOP */
        }

        if (fp) {
            return Feature::NewInstance(fp);
        }
    }
    return;
}

Napi::Value Featureset::NewInstance(mapnik::featureset_ptr fsp)
{
    Napi::EscapableHandleScope scope(env);
    Featureset* fs = new Featureset();
    fs->this_ = fsp;
    Napi::Value ext = Napi::External::New(env, fs);
    Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
    if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Featureset instance").ThrowAsJavaScriptException();

    return scope.Escape(maybe_local);
}
