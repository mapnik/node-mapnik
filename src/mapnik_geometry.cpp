#include "utils.hpp"
#include "mapnik_geometry.hpp"
#include "mapnik_projection.hpp"

#include <mapnik/datasource.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>
#include <mapnik/util/geometry_to_wkb.hpp>

Persistent<FunctionTemplate> Geometry::constructor;

/**
 * Geometry: a representation of geographical features in terms of
 * shape alone. This class provides many useful functions for conversion
 * to and from formats.
 *
 * You'll never create a mapnik.Geometry instance manually: it is always
 * part of a {@link mapnik.Feature} instance, which is often a part of
 * a {@link mapnik.Featureset} instance.
 *
 * @name mapnik.Geometry
 * @class
 */
void Geometry::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Geometry::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Geometry"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "extent", extent);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toWKB", toWKB);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toWKT", toWKT);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toJSON", toJSON);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toJSONSync", toJSONSync);
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "Point",mapnik::datasource_geometry_t::Point)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "LineString",mapnik::datasource_geometry_t::LineString)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "Polygon",mapnik::datasource_geometry_t::Polygon)
    target->Set(NanNew("Geometry"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Geometry::Geometry(mapnik::feature_ptr f) :
    node::ObjectWrap(),
    feat_(f) {}

Geometry::~Geometry()
{
}

NAN_METHOD(Geometry::New)
{
    NanScope();
    if (args[0]->IsExternal())
    {
        Local<External> ext = args[0].As<External>();
        void* ptr = ext->Value();
        Geometry* g =  static_cast<Geometry*>(ptr);
        g->Wrap(args.This());
        NanReturnValue(args.This());
    }
    else
    {
        NanThrowError("a mapnik.Geometry cannot be created directly - it is only available via a mapnik.Feature instance");
        NanReturnUndefined();
    }
    NanReturnValue(args.This());
}

Handle<Value> Geometry::NewInstance(mapnik::feature_ptr f) {
    NanEscapableScope();
    Geometry* g = new Geometry(f);
    Handle<Value> ext = NanNew<External>(g);
    Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
    return NanEscapeScope(obj);
}

/**
 * Convert this geometry into a [GeoJSON](http://geojson.org/) representation,
 * synchronously.
 *
 * @returns {string} GeoJSON, string-encoded representation of this geometry.
 * @memberof mapnik.Geometry
 * @instance
 * @name toJSONSync
 */
NAN_METHOD(Geometry::toJSONSync)
{
    NanScope();
    NanReturnValue(_toJSONSync(args));
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

Local<Value> Geometry::_toJSONSync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    Geometry* g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());
    std::string json;
    if (args.Length() < 1)
    {
        if (!mapnik::util::to_geojson(json,g->feat_->get_geometry()))
        {
            // Fairly certain this situation can never be reached but
            // leaving it none the less
            /* LCOV_EXCL_START */
            NanThrowError("Failed to generate GeoJSON");
            return NanEscapeScope(NanUndefined());
            /* LCOV_EXCL_END */
        }
    }
    else
    {
        if (!args[0]->IsObject()) {
            NanThrowTypeError("optional first arg must be an options object");
            return NanEscapeScope(NanUndefined());
        }
        Local<Object> options = args[0]->ToObject();
        if (options->Has(NanNew("transform")))
        {
            Local<Value> bound_opt = options->Get(NanNew("transform"));
            if (!bound_opt->IsObject()) {
                NanThrowTypeError("'transform' must be an object");
                return NanEscapeScope(NanUndefined());
            }

            Local<Object> obj = bound_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(ProjTransform::constructor)->HasInstance(obj)) {
                NanThrowTypeError("mapnik.ProjTransform expected as first arg");
                return NanEscapeScope(NanUndefined());
            }
            ProjTransform* tr = node::ObjectWrap::Unwrap<ProjTransform>(obj);
            mapnik::proj_transform const& prj_trans = *tr->get();
            mapnik::geometry::geometry<double> const& geom = g->feat_->get_geometry();
            if (!to_geojson_projected(json,geom,prj_trans))
            {
                // Fairly certain this situation can never be reached but
                // leaving it none the less
                /* LCOV_EXCL_START */
                NanThrowError("Failed to generate GeoJSON");
                return NanEscapeScope(NanUndefined());
                /* LCOV_EXCL_END */
            }
        }
    }
    return NanEscapeScope(NanNew(json));
}

struct to_json_baton {
    uv_work_t request;
    Geometry* g;
    ProjTransform* tr;
    bool error;
    std::string result;
    Persistent<Function> cb;
};


/**
 * Convert this geometry into a [GeoJSON](http://geojson.org/) representation,
 * asynchronously.
 *
 * @param {Object?} options. The only supported object is `transform`,
 * which should be a valid {@link mapnik.ProjTransform} object.
 * @param {Function} callback called with (err, result)
 * @memberof mapnik.Geometry
 * @instance
 * @name toJSON
 */
NAN_METHOD(Geometry::toJSON)
{
    NanScope();
    if ((args.Length() < 1) || !args[args.Length()-1]->IsFunction()) {
        NanReturnValue(_toJSONSync(args));
    }

    to_json_baton *closure = new to_json_baton();
    closure->request.data = closure;
    closure->g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());
    closure->error = false;
    closure->tr = nullptr;
    if (args.Length() > 1)
    {
        if (!args[0]->IsObject()) {
            NanThrowTypeError("optional first arg must be an options object");
            NanReturnUndefined();
        }
        Local<Object> options = args[0]->ToObject();
        if (options->Has(NanNew("transform")))
        {
            Local<Value> bound_opt = options->Get(NanNew("transform"));
            if (!bound_opt->IsObject()) {
                NanThrowTypeError("'transform' must be an object");
                NanReturnUndefined();
            }

            Local<Object> obj = bound_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(ProjTransform::constructor)->HasInstance(obj)) {
                NanThrowTypeError("mapnik.ProjTransform expected as first arg");
                NanReturnUndefined();
            }
            closure->tr = node::ObjectWrap::Unwrap<ProjTransform>(obj);
            closure->tr->_ref();
        }
    }
    Local<Value> callback = args[args.Length()-1];
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, to_json, (uv_after_work_cb)after_to_json);
    closure->g->Ref();
    NanReturnUndefined();
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
                // LCOV_EXCL_END
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
                /* LCOV_EXCL_END */
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
        /* LCOV_EXCL_END */
    }
}

void Geometry::after_to_json(uv_work_t* req)
{
    NanScope();
    to_json_baton *closure = static_cast<to_json_baton *>(req->data);
    if (closure->error)
    {
        // Fairly certain this situation can never be reached but
        // leaving it none the less
        /* LCOV_EXCL_START */
        Local<Value> argv[1] = { NanError(closure->result.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
        /* LCOV_EXCL_END */
    }
    else
    {
        Local<Value> argv[2] = { NanNull(), NanNew(closure->result) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }
    closure->g->Unref();
    if (closure->tr) {
        closure->tr->_unref();
    }
    NanDisposePersistent(closure->cb);
    delete closure;
}

/**
 * Get the geometry's extent
 *
 * @name extent
 * @memberof mapnik.Geometry
 * @instance
 * @returns {Array<number>} extent [minx, miny, maxx, maxy] order geometry extent.
 */
NAN_METHOD(Geometry::extent)
{
    NanScope();
    Geometry* g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());
    Local<Array> a = NanNew<Array>(4);
    mapnik::box2d<double> const& e = g->feat_->envelope();
    a->Set(0, NanNew<Number>(e.minx()));
    a->Set(1, NanNew<Number>(e.miny()));
    a->Set(2, NanNew<Number>(e.maxx()));
    a->Set(3, NanNew<Number>(e.maxy()));
    NanReturnValue(a);
}

/**
 * Get the geometry's representation as [Well-Known Text](http://en.wikipedia.org/wiki/Well-known_text)
 *
 * @name toWKT
 * @memberof mapnik.Geometry
 * @instance
 * @returns {string} wkt representation of this geometry
 */
NAN_METHOD(Geometry::toWKT)
{
    NanScope();
    std::string wkt;
    Geometry* g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());
    if (!mapnik::util::to_wkt(wkt, g->feat_->get_geometry()))
    {
        // Fairly certain this situation can never be reached but
        // leaving it none the less
        /* LCOV_EXCL_START */
        NanThrowError("Failed to generate WKT");
        NanReturnUndefined();
        /* LCOV_EXCL_END */
    }
    NanReturnValue(NanNew(wkt.c_str()));
}

/**
 * Get the geometry's representation as Well-Known Binary
 *
 * @name toWKB
 * @memberof mapnik.Geometry
 * @instance
 * @returns {string} wkb representation of this geometry
 */
NAN_METHOD(Geometry::toWKB)
{
    NanScope();
    std::string wkt;
    Geometry* g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());
    mapnik::util::wkb_buffer_ptr wkb = mapnik::util::to_wkb(g->feat_->get_geometry(), mapnik::wkbNDR);
    if (!wkb)
    {
        NanThrowError("Failed to generate WKB - geometry likely null");
        NanReturnUndefined();
    }
    NanReturnValue(NanNewBufferHandle(wkb->buffer(), wkb->size()));
}
