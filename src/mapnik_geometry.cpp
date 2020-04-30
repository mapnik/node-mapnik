#include "utils.hpp"
#include "mapnik_geometry.hpp"
#include "mapnik_projection.hpp"

#include <mapnik/datasource.hpp>
#include <mapnik/geometry/reprojection.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>
#include <mapnik/util/geometry_to_wkb.hpp>

Napi::FunctionReference Geometry::constructor;

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
void Geometry::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, Geometry::New);

    lcons->SetClassName(Napi::String::New(env, "Geometry"));

    InstanceMethod("extent", &extent),
    InstanceMethod("type", &type),
    InstanceMethod("toWKB", &toWKB),
    InstanceMethod("toWKT", &toWKT),
    InstanceMethod("toJSON", &toJSON),
    InstanceMethod("toJSONSync", &toJSONSync),
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),
                                "Unknown",mapnik::geometry::geometry_types::Unknown)
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),
                                "Point",mapnik::geometry::geometry_types::Point)
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),
                                "MultiPoint",mapnik::geometry::geometry_types::MultiPoint)
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),
                                "LineString",mapnik::geometry::geometry_types::LineString)
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),
                                "MultiLineString",mapnik::geometry::geometry_types::MultiLineString)
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),
                                "Polygon",mapnik::geometry::geometry_types::Polygon)
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),
                                "MultiPolygon",mapnik::geometry::geometry_types::MultiPolygon)
    NODE_MAPNIK_DEFINE_CONSTANT(Napi::GetFunction(lcons),
                                "GeometryCollection",mapnik::geometry::geometry_types::GeometryCollection)
    (target).Set(Napi::String::New(env, "Geometry"), Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

Geometry::Geometry(mapnik::feature_ptr f) : Napi::ObjectWrap<Geometry>(),
    feat_(f) {}

Geometry::~Geometry()
{
}

Napi::Value Geometry::New(const Napi::CallbackInfo& info)
{
    if (info[0].IsExternal())
    {
        Napi::External ext = info[0].As<Napi::External>();
        void* ptr = ext->Value();
        Geometry* g =  static_cast<Geometry*>(ptr);
        g->Wrap(info.This());
        return info.This();
        return;
    }
    else
    {
        Napi::Error::New(env, "a mapnik.Geometry cannot be created directly - it is only available via a mapnik.Feature instance").ThrowAsJavaScriptException();
        return env.Null();
    }
    return info.This();
}

Napi::Value Geometry::NewInstance(mapnik::feature_ptr f) {
    Napi::EscapableHandleScope scope(env);
    Geometry* g = new Geometry(f);
    Napi::Value ext = Napi::External::New(env, g);
    Napi::MaybeLocal<v8::Object> maybe_local = Napi::NewInstance(Napi::GetFunction(Napi::New(env, constructor)), 1, &ext);
    if (maybe_local.IsEmpty()) Napi::Error::New(env, "Could not create new Geometry instance").ThrowAsJavaScriptException();

    return scope.Escape(maybe_local);
}

/**
 * Get the geometry type
 *
 * @name type
 * @returns {string} type of geometry.
 * @memberof Geometry
 * @instance
 */
Napi::Value Geometry::type(const Napi::CallbackInfo& info)
{
    Geometry* g = info.Holder().Unwrap<Geometry>();
    auto const& geom = g->feat_->get_geometry();
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
Napi::Value Geometry::toJSONSync(const Napi::CallbackInfo& info)
{
    return _toJSONSync(info);
}

bool to_geojson_projected(std::string & json,
                          mapnik::geometry::geometry<double> const& geom,
                          mapnik::proj_transform const& prj_trans)
{
    unsigned int n_err = 0;
    mapnik::geometry::geometry<double> projected_geom = mapnik::geometry::reproject_copy(geom,prj_trans,n_err);
    if (n_err > 0) return false;
    return mapnik::util::to_geojson(json,projected_geom);
}

Napi::Value Geometry::_toJSONSync(const Napi::CallbackInfo& info) {
    Napi::EscapableHandleScope scope(env);
    Geometry* g = info.Holder().Unwrap<Geometry>();
    std::string json;
    if (info.Length() < 1)
    {
        if (!mapnik::util::to_geojson(json,g->feat_->get_geometry()))
        {
            // Fairly certain this situation can never be reached but
            // leaving it none the less
            /* LCOV_EXCL_START */
            Napi::Error::New(env, "Failed to generate GeoJSON").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
            /* LCOV_EXCL_STOP */
        }
    }
    else
    {
        if (!info[0].IsObject()) {
            Napi::TypeError::New(env, "optional first arg must be an options object").ThrowAsJavaScriptException();

            return scope.Escape(env.Undefined());
        }
        Napi::Object options = info[0].ToObject(Napi::GetCurrentContext());
        if ((options).Has(Napi::String::New(env, "transform")).FromMaybe(false))
        {
            Napi::Value bound_opt = (options).Get(Napi::String::New(env, "transform"));
            if (!bound_opt.IsObject()) {
                Napi::TypeError::New(env, "'transform' must be an object").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }

            Napi::Object obj = bound_opt->ToObject(Napi::GetCurrentContext());
            if (!Napi::New(env, ProjTransform::constructor)->HasInstance(obj)) {
                Napi::TypeError::New(env, "mapnik.ProjTransform expected as first arg").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
            }
            ProjTransform* tr = obj.Unwrap<ProjTransform>();
            mapnik::proj_transform const& prj_trans = *tr->get();
            mapnik::geometry::geometry<double> const& geom = g->feat_->get_geometry();
            if (!to_geojson_projected(json,geom,prj_trans))
            {
                // Fairly certain this situation can never be reached but
                // leaving it none the less
                /* LCOV_EXCL_START */
                Napi::Error::New(env, "Failed to generate GeoJSON").ThrowAsJavaScriptException();

                return scope.Escape(env.Undefined());
                /* LCOV_EXCL_STOP */
            }
        }
    }
    return scope.Escape(Napi::String::New(env, json));
}

struct to_json_baton {
    uv_work_t request;
    Geometry* g;
    ProjTransform* tr;
    bool error;
    std::string result;
    Napi::FunctionReference cb;
};


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
Napi::Value Geometry::toJSON(const Napi::CallbackInfo& info)
{
    if ((info.Length() < 1) || !info[info.Length()-1]->IsFunction()) {
        return _toJSONSync(info);
        return;
    }

    to_json_baton *closure = new to_json_baton();
    closure->request.data = closure;
    closure->g = info.Holder().Unwrap<Geometry>();
    closure->error = false;
    closure->tr = nullptr;
    if (info.Length() > 1)
    {
        if (!info[0].IsObject()) {
            Napi::TypeError::New(env, "optional first arg must be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }
        Napi::Object options = info[0].ToObject(Napi::GetCurrentContext());
        if ((options).Has(Napi::String::New(env, "transform")).FromMaybe(false))
        {
            Napi::Value bound_opt = (options).Get(Napi::String::New(env, "transform"));
            if (!bound_opt.IsObject()) {
                Napi::TypeError::New(env, "'transform' must be an object").ThrowAsJavaScriptException();
                return env.Null();
            }

            Napi::Object obj = bound_opt->ToObject(Napi::GetCurrentContext());
            if (!Napi::New(env, ProjTransform::constructor)->HasInstance(obj)) {
                Napi::TypeError::New(env, "mapnik.ProjTransform expected as first arg").ThrowAsJavaScriptException();
                return env.Null();
            }
            closure->tr = obj.Unwrap<ProjTransform>();
            closure->tr->Ref();
        }
    }
    Napi::Value callback = info[info.Length()-1];
    closure->cb.Reset(callback.As<Napi::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, to_json, (uv_after_work_cb)after_to_json);
    closure->g->Ref();
    return;
}

void Geometry::to_json(uv_work_t* req)
{
    to_json_baton *closure = static_cast<to_json_baton *>(req->data);
    try
    {
        if (closure->tr)
        {
            mapnik::proj_transform const& prj_trans = *closure->tr->get();
            mapnik::geometry::geometry<double> const& geom = closure->g->feat_->get_geometry();
            if (!to_geojson_projected(closure->result,geom,prj_trans))
            {
                // Fairly certain this situation can never be reached but
                // leaving it none the less
                // LCOV_EXCL_START
                closure->error = true;
                closure->result = "Failed to generate GeoJSON";
                // LCOV_EXCL_STOP
            }
        }
        else
        {
            if (!mapnik::util::to_geojson(closure->result,closure->g->feat_->get_geometry()))
            {
                // Fairly certain this situation can never be reached but
                // leaving it none the less
                /* LCOV_EXCL_START */
                closure->error = true;
                closure->result = "Failed to generate GeoJSON";
                /* LCOV_EXCL_STOP */
            }
        }
    }
    catch (std::exception const& ex)
    {
        // Fairly certain this situation can never be reached but
        // leaving it none the less
        /* LCOV_EXCL_START */
        closure->error = true;
        closure->result = ex.what();
        /* LCOV_EXCL_STOP */
    }
}

void Geometry::after_to_json(uv_work_t* req)
{
    Napi::HandleScope scope(env);
    to_json_baton *closure = static_cast<to_json_baton *>(req->data);
    Napi::AsyncResource async_resource(__func__);
    if (closure->error)
    {
        // Fairly certain this situation can never be reached but
        // leaving it none the less
        /* LCOV_EXCL_START */
        Napi::Value argv[1] = { Napi::Error::New(env, closure->result.c_str()) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    }
    else
    {
        Napi::Value argv[2] = { env.Null(), Napi::String::New(env, closure->result) };
        async_resource.runInAsyncScope(Napi::GetCurrentContext()->Global(), Napi::New(env, closure->cb), 2, argv);
    }
    closure->g->Unref();
    if (closure->tr) {
        closure->tr->Unref();
    }
    closure->cb.Reset();
    delete closure;
}

/**
 * Get the geometry's extent
 *
 * @name extent
 * @memberof Geometry
 * @instance
 * @returns {Array<number>} extent [minx, miny, maxx, maxy] order geometry extent.
 */
Napi::Value Geometry::extent(const Napi::CallbackInfo& info)
{
    Geometry* g = info.Holder().Unwrap<Geometry>();
    Napi::Array a = Napi::Array::New(env, 4);
    mapnik::box2d<double> const& e = g->feat_->envelope();
    (a).Set(0, Napi::Number::New(env, e.minx()));
    (a).Set(1, Napi::Number::New(env, e.miny()));
    (a).Set(2, Napi::Number::New(env, e.maxx()));
    (a).Set(3, Napi::Number::New(env, e.maxy()));
    return a;
}

/**
 * Get the geometry's representation as [Well-Known Text](http://en.wikipedia.org/wiki/Well-known_text)
 *
 * @name toWKT
 * @memberof Geometry
 * @instance
 * @returns {string} wkt representation of this geometry
 */
Napi::Value Geometry::toWKT(const Napi::CallbackInfo& info)
{
    std::string wkt;
    Geometry* g = info.Holder().Unwrap<Geometry>();
    if (!mapnik::util::to_wkt(wkt, g->feat_->get_geometry()))
    {
        // Fairly certain this situation can never be reached but
        // leaving it none the less
        /* LCOV_EXCL_START */
        Napi::Error::New(env, "Failed to generate WKT").ThrowAsJavaScriptException();
        return env.Null();
        /* LCOV_EXCL_STOP */
    }
    return Napi::String::New(env, wkt);
}

/**
 * Get the geometry's representation as Well-Known Binary
 *
 * @name toWKB
 * @memberof Geometry
 * @instance
 * @returns {string} wkb representation of this geometry
 */
Napi::Value Geometry::toWKB(const Napi::CallbackInfo& info)
{
    Geometry* g = info.Holder().Unwrap<Geometry>();
    mapnik::util::wkb_buffer_ptr wkb = mapnik::util::to_wkb(g->feat_->get_geometry(), mapnik::wkbNDR);
    if (!wkb)
    {
        Napi::Error::New(env, "Failed to generate WKB - geometry likely null").ThrowAsJavaScriptException();
        return env.Null();
    }
    return Napi::Buffer::Copy(env, wkb->buffer(), wkb->size());
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
