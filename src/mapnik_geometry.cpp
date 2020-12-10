#include "utils.hpp"
#include "mapnik_geometry.hpp"
#include "mapnik_projection.hpp"

#include <mapnik/datasource.hpp>
#include <mapnik/geometry/reprojection.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>
#include <mapnik/util/geometry_to_wkb.hpp>

namespace {

bool to_geojson_projected(std::string& json,
                          mapnik::geometry::geometry<double> const& geom,
                          mapnik::proj_transform const& prj_trans)
{
    unsigned int n_err = 0;
    mapnik::geometry::geometry<double> projected_geom = mapnik::geometry::reproject_copy(geom, prj_trans, n_err);
    if (n_err > 0) return false;
    return mapnik::util::to_geojson(json, projected_geom);
}

struct AsyncToJSON : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncToJSON(Geometry* geom, ProjTransform* tr, Napi::Function const& callback)
        : Base(callback),
          geom_(geom),
          tr_(tr)
    {
    }

    void Execute() override
    {
        try
        {
            if (tr_)
            {
                mapnik::proj_transform const& prj_trans = *tr_->impl();
                mapnik::geometry::geometry<double> const& geom = geom_->geometry();
                if (!to_geojson_projected(json_, geom, prj_trans))
                {
                    // Fairly certain this situation can never be reached but
                    // leaving it none the less
                    // LCOV_EXCL_START
                    SetError("Failed to generate GeoJSON");
                    // LCOV_EXCL_STOP
                }
            }
            else
            {
                if (!mapnik::util::to_geojson(json_, geom_->geometry()))
                {
                    // Fairly certain this situation can never be reached but
                    // leaving it none the less
                    /* LCOV_EXCL_START */
                    SetError("Failed to generate GeoJSON");
                    /* LCOV_EXCL_STOP */
                }
            }
        }
        catch (std::exception const& ex)
        {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        return {env.Null(), Napi::String::New(env, json_)};
    }

  private:
    Geometry* geom_;
    ProjTransform* tr_;
    std::string json_;
};

} // namespace

Napi::FunctionReference Geometry::constructor;

Napi::Object Geometry::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Geometry", {
            InstanceMethod<&Geometry::extent>("extent", prop_attr),
            InstanceMethod<&Geometry::type>("type", prop_attr),
            InstanceMethod<&Geometry::toWKB>("toWKB", prop_attr),
            InstanceMethod<&Geometry::toWKT>("toWKT", prop_attr),
            InstanceMethod<&Geometry::toJSON>("toJSON", prop_attr),
            InstanceMethod<&Geometry::toJSON>("toJSONSync", prop_attr),
            StaticValue("Unknown", Napi::Number::New(env, mapnik::geometry::geometry_types::Unknown), napi_enumerable),
            StaticValue("Point", Napi::Number::New(env, mapnik::geometry::geometry_types::Point), napi_enumerable),
            StaticValue("LineString", Napi::Number::New(env, mapnik::geometry::geometry_types::LineString), napi_enumerable),
            StaticValue("Polygon", Napi::Number::New(env, mapnik::geometry::geometry_types::Polygon), napi_enumerable),
            StaticValue("MultiPoint", Napi::Number::New(env, mapnik::geometry::geometry_types::MultiPoint), napi_enumerable),
            StaticValue("MultiLineString", Napi::Number::New(env, mapnik::geometry::geometry_types::MultiLineString), napi_enumerable),
            StaticValue("MultiPolygon", Napi::Number::New(env, mapnik::geometry::geometry_types::MultiPolygon), napi_enumerable),
            StaticValue("GeometryCollection", Napi::Number::New(env, mapnik::geometry::geometry_types::GeometryCollection), napi_enumerable)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Geometry", func);
    return exports;
}

/**
 * **`mapnik.Geometry`**
 *
 * Geometry: a representation of geographical features in terms of
 * shape alone. This class provides many useful functions for conversion
 * to and from formats.
 *
 * You'll never create a mapnik.Geometry instance manually: it is always
 * part of a {@link mapnik.Feature} instance, which is often a part of
 * a {@link mapnik.Featureset} instance.
 *
 * @class Geometry
 */

Geometry::Geometry(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Geometry>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 1 && info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<mapnik::feature_ptr>>();
        if (ext) feature_ = *ext.Data();
    }
    else
    {
        Napi::Error::New(env, "mapnik.Geometry cannot be created directly - it is only available via a mapnik.Feature instance")
            .ThrowAsJavaScriptException();
    }
}

/**
 * Get the geometry type
 *
 * @name type
 * @returns {string} type of geometry.
 * @memberof Geometry
 * @instance
 */
Napi::Value Geometry::type(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    auto const& geom = this->geometry();
    return Napi::Number::New(env, mapnik::geometry::geometry_type(geom));
}

/**
 * Convert this geometry into a [GeoJSON](http://geojson.org/) representation,
 * synchronously.
 *
 * @returns {string} GeoJSON, string-encoded representation of this geometry.
 * @memberof Geometry
 * @instance
 * @name toJSONSync
 */

Napi::Value Geometry::toJSONSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::string json;
    if (info.Length() < 1)
    {
        if (!mapnik::util::to_geojson(json, geometry()))
        {
            // Fairly certain this situation can never be reached but
            // leaving it none the less
            /* LCOV_EXCL_START */
            Napi::Error::New(env, "Failed to generate GeoJSON").ThrowAsJavaScriptException();
            return env.Undefined();
            /* LCOV_EXCL_STOP */
        }
    }
    else
    {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "optional first arg must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[0].As<Napi::Object>();
        if (options.Has("transform"))
        {
            Napi::Value bound_opt = options.Get("transform");
            if (!bound_opt.IsObject())
            {
                Napi::TypeError::New(env, "'transform' must be an object").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Object obj = bound_opt.As<Napi::Object>();
            if (!obj.InstanceOf(ProjTransform::constructor.Value()))
            {
                Napi::TypeError::New(env, "mapnik.ProjTransform expected as first arg").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            ProjTransform* tr = Napi::ObjectWrap<ProjTransform>::Unwrap(obj);
            //mapnik::proj_transform const& prj_trans = *tr->proj_transform;
            mapnik::geometry::geometry<double> const& geom = this->geometry();
            if (!to_geojson_projected(json, geom, *tr->impl()))
            {
                // Fairly certain this situation can never be reached but
                // leaving it none the less
                /* LCOV_EXCL_START */
                Napi::Error::New(env, "Failed to generate GeoJSON").ThrowAsJavaScriptException();
                return env.Undefined();
                /* LCOV_EXCL_STOP */
            }
        }
    }
    return scope.Escape(Napi::String::New(env, json));
}

/**
 * Convert this geometry into a [GeoJSON](http://geojson.org/) representation,
 * asynchronously.
 *
 * @param {Object} [options={}]. The only supported object is `transform`,
 * which should be a valid {@link mapnik.ProjTransform} object.
 * @param {Function} callback called with (err, result)
 * @memberof Geometry
 * @instance
 * @name toJSON
 */

Napi::Value Geometry::toJSON(Napi::CallbackInfo const& info)
{
    if ((info.Length() < 1) || !info[info.Length() - 1].IsFunction())
    {
        return toJSONSync(info);
    }

    Napi::Env env = info.Env();
    ProjTransform* transform = nullptr;
    if (info.Length() > 1)
    {
        if (!info[0].IsObject())
        {
            Napi::TypeError::New(env, "optional first arg must be an options object").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        Napi::Object options = info[0].As<Napi::Object>();
        if (options.Has("transform"))
        {
            Napi::Value bound_opt = options.Get("transform");
            if (!bound_opt.IsObject())
            {
                Napi::TypeError::New(env, "'transform' must be an object").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            Napi::Object obj = bound_opt.As<Napi::Object>();
            if (!obj.InstanceOf(ProjTransform::constructor.Value()))
            {
                Napi::TypeError::New(env, "mapnik.ProjTransform expected as first arg").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            transform = Napi::ObjectWrap<ProjTransform>::Unwrap(obj);
        }
    }
    Napi::Value callback_val = info[info.Length() - 1];
    auto* worker = new AsyncToJSON(this, transform, callback_val.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

/**
 * Get the geometry's extent
 *
 * @name extent
 * @memberof Geometry
 * @instance
 * @returns {Array<number>} extent [minx, miny, maxx, maxy] order geometry extent.
 */
Napi::Value Geometry::extent(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    Napi::Array arr = Napi::Array::New(env, 4);
    mapnik::box2d<double> const& bbox = feature_->envelope();
    arr.Set(0u, Napi::Number::New(env, bbox.minx()));
    arr.Set(1u, Napi::Number::New(env, bbox.miny()));
    arr.Set(2u, Napi::Number::New(env, bbox.maxx()));
    arr.Set(3u, Napi::Number::New(env, bbox.maxy()));
    return scope.Escape(arr);
}

/**
 * Get the geometry's representation as [Well-Known Text](http://en.wikipedia.org/wiki/Well-known_text)
 *
 * @name toWKT
 * @memberof Geometry
 * @instance
 * @returns {string} wkt representation of this geometry
 */
Napi::Value Geometry::toWKT(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    std::string wkt;
    if (!mapnik::util::to_wkt(wkt, this->geometry()))
    {
        // Fairly certain this situation can never be reached but
        // leaving it none the less
        /* LCOV_EXCL_START */
        Napi::Error::New(env, "Failed to generate WKT").ThrowAsJavaScriptException();
        return env.Undefined();
        /* LCOV_EXCL_STOP */
    }
    return scope.Escape(Napi::String::New(env, wkt));
}

/**
 * Get the geometry's representation as Well-Known Binary
 *
 * @name toWKB
 * @memberof Geometry
 * @instance
 * @returns {string} wkb representation of this geometry
 */
Napi::Value Geometry::toWKB(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    mapnik::util::wkb_buffer_ptr wkb = mapnik::util::to_wkb(this->geometry(), mapnik::wkbNDR);
    if (!wkb)
    {
        Napi::Error::New(env, "Failed to generate WKB - geometry likely null").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    return scope.Escape(Napi::Buffer<char>::Copy(env, wkb->buffer(), wkb->size()));
}

/**
 * Get the geometry's representation as a GeoJSON type
 *
 * @name typeName
 * @memberof Geometry
 * @instance
 * @returns {string} GeoJSON type representation of this geometry
 */
// Function exists in mapnik.js
