#include "mapnik_featureset.hpp"
#include "mapnik_feature.hpp"

Napi::FunctionReference Featureset::constructor;

Napi::Object Featureset::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Featureset", {
            InstanceMethod<&Featureset::next>("next", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Featureset", func);
    return exports;
}

/**
 * **`mapnik.Featureset`**
 *
 * An iterator of {@link mapnik.Feature} objects.
 *
 * @class Featureset
 */

Featureset::Featureset(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Featureset>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 1 && info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<mapnik::featureset_ptr>>();
        if (ext) featureset_ = *ext.Data();
        return;
    }
    Napi::TypeError::New(env, "Sorry a Featureset cannot currently be created, only accessed via an existing datasource")
        .ThrowAsJavaScriptException();
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
Napi::Value Featureset::next(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (featureset_)
    {
        mapnik::feature_ptr feature;
        try
        {
            feature = featureset_->next();
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
            return env.Undefined();
            /* LCOV_EXCL_STOP */
        }
        if (feature)
        {
            Napi::Value arg = Napi::External<mapnik::feature_ptr>::New(env, &feature);
            Napi::Object obj = Feature::constructor.New({arg});
            return scope.Escape(obj);
        }
    }
    return env.Null(); // Loop termination condition
}
