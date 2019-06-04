#include "utils.hpp"
#include "mapnik_geometry.hpp"
#include "mapnik_projection.hpp"

#include <mapnik/datasource.hpp>
#include <mapnik/geometry/reprojection.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>
#include <mapnik/util/geometry_to_wkb.hpp>

Nan::Persistent<v8::FunctionTemplate> Geometry::constructor;

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
void Geometry::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Geometry::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Geometry").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "extent", extent);
    Nan::SetPrototypeMethod(lcons, "type", type);
    Nan::SetPrototypeMethod(lcons, "toWKB", toWKB);
    Nan::SetPrototypeMethod(lcons, "toWKT", toWKT);
    Nan::SetPrototypeMethod(lcons, "toJSON", toJSON);
    Nan::SetPrototypeMethod(lcons, "toJSONSync", toJSONSync);
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "Unknown",mapnik::geometry::geometry_types::Unknown)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "Point",mapnik::geometry::geometry_types::Point)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "MultiPoint",mapnik::geometry::geometry_types::MultiPoint)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "LineString",mapnik::geometry::geometry_types::LineString)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "MultiLineString",mapnik::geometry::geometry_types::MultiLineString)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "Polygon",mapnik::geometry::geometry_types::Polygon)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "MultiPolygon",mapnik::geometry::geometry_types::MultiPolygon)
    NODE_MAPNIK_DEFINE_CONSTANT(Nan::GetFunction(lcons).ToLocalChecked(),
                                "GeometryCollection",mapnik::geometry::geometry_types::GeometryCollection)
    Nan::Set(target, Nan::New("Geometry").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

Geometry::Geometry(mapnik::feature_ptr f) :
    Nan::ObjectWrap(),
    feat_(f) {}

Geometry::~Geometry()
{
}

NAN_METHOD(Geometry::New)
{
    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = info[0].As<v8::External>();
        void* ptr = ext->Value();
        Geometry* g =  static_cast<Geometry*>(ptr);
        g->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    else
    {
        Nan::ThrowError("a mapnik.Geometry cannot be created directly - it is only available via a mapnik.Feature instance");
        return;
    }
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Value> Geometry::NewInstance(mapnik::feature_ptr f) {
    Nan::EscapableHandleScope scope;
    Geometry* g = new Geometry(f);
    v8::Local<v8::Value> ext = Nan::New<v8::External>(g);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Geometry instance");
    return scope.Escape(maybe_local.ToLocalChecked());
}

/**
 * Get the geometry type
 *
 * @name type
 * @returns {string} type of geometry.
 * @memberof Geometry
 * @instance
 */
NAN_METHOD(Geometry::type)
{
    Geometry* g = Nan::ObjectWrap::Unwrap<Geometry>(info.Holder());
    auto const& geom = g->feat_->get_geometry();
    info.GetReturnValue().Set(Nan::New<v8::Integer>(mapnik::geometry::geometry_type(geom)));
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
NAN_METHOD(Geometry::toJSONSync)
{
    info.GetReturnValue().Set(_toJSONSync(info));
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

v8::Local<v8::Value> Geometry::_toJSONSync(Nan::NAN_METHOD_ARGS_TYPE info) {
    Nan::EscapableHandleScope scope;
    Geometry* g = Nan::ObjectWrap::Unwrap<Geometry>(info.Holder());
    std::string json;
    if (info.Length() < 1)
    {
        if (!mapnik::util::to_geojson(json,g->feat_->get_geometry()))
        {
            // Fairly certain this situation can never be reached but
            // leaving it none the less
            /* LCOV_EXCL_START */
            Nan::ThrowError("Failed to generate GeoJSON");
            return scope.Escape(Nan::Undefined());
            /* LCOV_EXCL_STOP */
        }
    }
    else
    {
        if (!info[0]->IsObject()) {
            Nan::ThrowTypeError("optional first arg must be an options object");
            return scope.Escape(Nan::Undefined());
        }
        v8::Local<v8::Object> options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("transform").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bound_opt = Nan::Get(options, Nan::New("transform").ToLocalChecked()).ToLocalChecked();
            if (!bound_opt->IsObject()) {
                Nan::ThrowTypeError("'transform' must be an object");
                return scope.Escape(Nan::Undefined());
            }

            v8::Local<v8::Object> obj = bound_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(ProjTransform::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.ProjTransform expected as first arg");
                return scope.Escape(Nan::Undefined());
            }
            ProjTransform* tr = Nan::ObjectWrap::Unwrap<ProjTransform>(obj);
            mapnik::proj_transform const& prj_trans = *tr->get();
            mapnik::geometry::geometry<double> const& geom = g->feat_->get_geometry();
            if (!to_geojson_projected(json,geom,prj_trans))
            {
                // Fairly certain this situation can never be reached but
                // leaving it none the less
                /* LCOV_EXCL_START */
                Nan::ThrowError("Failed to generate GeoJSON");
                return scope.Escape(Nan::Undefined());
                /* LCOV_EXCL_STOP */
            }
        }
    }
    return scope.Escape(Nan::New<v8::String>(json).ToLocalChecked());
}

struct to_json_baton {
    uv_work_t request;
    Geometry* g;
    ProjTransform* tr;
    bool error;
    std::string result;
    Nan::Persistent<v8::Function> cb;
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
NAN_METHOD(Geometry::toJSON)
{
    if ((info.Length() < 1) || !info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_toJSONSync(info));
        return;
    }

    to_json_baton *closure = new to_json_baton();
    closure->request.data = closure;
    closure->g = Nan::ObjectWrap::Unwrap<Geometry>(info.Holder());
    closure->error = false;
    closure->tr = nullptr;
    if (info.Length() > 1)
    {
        if (!info[0]->IsObject()) {
            Nan::ThrowTypeError("optional first arg must be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (Nan::Has(options, Nan::New("transform").ToLocalChecked()).FromMaybe(false))
        {
            v8::Local<v8::Value> bound_opt = Nan::Get(options, Nan::New("transform").ToLocalChecked()).ToLocalChecked();
            if (!bound_opt->IsObject()) {
                Nan::ThrowTypeError("'transform' must be an object");
                return;
            }

            v8::Local<v8::Object> obj = bound_opt->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            if (obj->IsNull() || obj->IsUndefined() || !Nan::New(ProjTransform::constructor)->HasInstance(obj)) {
                Nan::ThrowTypeError("mapnik.ProjTransform expected as first arg");
                return;
            }
            closure->tr = Nan::ObjectWrap::Unwrap<ProjTransform>(obj);
            closure->tr->Ref();
        }
    }
    v8::Local<v8::Value> callback = info[info.Length()-1];
    closure->cb.Reset(callback.As<v8::Function>());
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
    Nan::HandleScope scope;
    to_json_baton *closure = static_cast<to_json_baton *>(req->data);
    Nan::AsyncResource async_resource(__func__);
    if (closure->error)
    {
        // Fairly certain this situation can never be reached but
        // leaving it none the less
        /* LCOV_EXCL_START */
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->result.c_str()) };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        /* LCOV_EXCL_STOP */
    }
    else
    {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::New<v8::String>(closure->result).ToLocalChecked() };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
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
NAN_METHOD(Geometry::extent)
{
    Geometry* g = Nan::ObjectWrap::Unwrap<Geometry>(info.Holder());
    v8::Local<v8::Array> a = Nan::New<v8::Array>(4);
    mapnik::box2d<double> const& e = g->feat_->envelope();
    Nan::Set(a, 0, Nan::New<v8::Number>(e.minx()));
    Nan::Set(a, 1, Nan::New<v8::Number>(e.miny()));
    Nan::Set(a, 2, Nan::New<v8::Number>(e.maxx()));
    Nan::Set(a, 3, Nan::New<v8::Number>(e.maxy()));
    info.GetReturnValue().Set(a);
}

/**
 * Get the geometry's representation as [Well-Known Text](http://en.wikipedia.org/wiki/Well-known_text)
 *
 * @name toWKT
 * @memberof Geometry
 * @instance
 * @returns {string} wkt representation of this geometry
 */
NAN_METHOD(Geometry::toWKT)
{
    std::string wkt;
    Geometry* g = Nan::ObjectWrap::Unwrap<Geometry>(info.Holder());
    if (!mapnik::util::to_wkt(wkt, g->feat_->get_geometry()))
    {
        // Fairly certain this situation can never be reached but
        // leaving it none the less
        /* LCOV_EXCL_START */
        Nan::ThrowError("Failed to generate WKT");
        return;
        /* LCOV_EXCL_STOP */
    }
    info.GetReturnValue().Set(Nan::New<v8::String>(wkt).ToLocalChecked());
}

/**
 * Get the geometry's representation as Well-Known Binary
 *
 * @name toWKB
 * @memberof Geometry
 * @instance
 * @returns {string} wkb representation of this geometry
 */
NAN_METHOD(Geometry::toWKB)
{
    Geometry* g = Nan::ObjectWrap::Unwrap<Geometry>(info.Holder());
    mapnik::util::wkb_buffer_ptr wkb = mapnik::util::to_wkb(g->feat_->get_geometry(), mapnik::wkbNDR);
    if (!wkb)
    {
        Nan::ThrowError("Failed to generate WKB - geometry likely null");
        return;
    }
    info.GetReturnValue().Set(Nan::CopyBuffer(wkb->buffer(), wkb->size()).ToLocalChecked());
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
