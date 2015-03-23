#include "utils.hpp"
#include "mapnik_geometry.hpp"
#include "mapnik_projection.hpp"

#include <mapnik/util/geometry_to_geojson.hpp>
#include "proj_transform_adapter.hpp"
#include <mapnik/util/geometry_to_wkt.hpp>
#include <mapnik/util/geometry_to_wkb.hpp>

Persistent<FunctionTemplate> Geometry::constructor;

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
                                "Point",mapnik::geometry_type::types::Point)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "LineString",mapnik::geometry_type::types::LineString)
    NODE_MAPNIK_DEFINE_CONSTANT(lcons->GetFunction(),
                                "Polygon",mapnik::geometry_type::types::Polygon)
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

NAN_METHOD(Geometry::toJSONSync)
{
    NanScope();
    NanReturnValue(_toJSONSync(args));
}

Local<Value> Geometry::_toJSONSync(_NAN_METHOD_ARGS) {
    NanEscapableScope();
    Geometry* g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());
    std::string json;
    if (args.Length() < 1)
    {
        if (!mapnik::util::to_geojson(json,g->feat_->paths()))
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
            node_mapnik::proj_transform_container projected_paths;
            for (auto const& geom : g->feat_->paths())
            {
                projected_paths.push_back(new node_mapnik::proj_transform_path_type(geom,prj_trans));
            }
            using sink_type = std::back_insert_iterator<std::string>;
            static const mapnik::json::multi_geometry_generator_grammar<sink_type,node_mapnik::proj_transform_container> proj_grammar;
            sink_type sink(json);
            if (!boost::spirit::karma::generate(sink, proj_grammar, projected_paths))
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
            node_mapnik::proj_transform_container projected_paths;
            for (auto const& geom : closure->g->feat_->paths())
            {
                projected_paths.push_back(new node_mapnik::proj_transform_path_type(geom,prj_trans));
            }
            using sink_type = std::back_insert_iterator<std::string>;
            static const mapnik::json::multi_geometry_generator_grammar<sink_type,node_mapnik::proj_transform_container> proj_grammar;
            sink_type sink(closure->result);
            if (!boost::spirit::karma::generate(sink, proj_grammar, projected_paths))
            {
                // Fairly certain this situation can never be reached but 
                // leaving it none the less
                /* LCOV_EXCL_START */
                closure->error = true;
                closure->result = "Failed to generate GeoJSON";
                /* LCOV_EXCL_END */
            }
        }
        else
        {
            if (!mapnik::util::to_geojson(closure->result,closure->g->feat_->paths()))
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

NAN_METHOD(Geometry::toWKT)
{
    NanScope();
    std::string wkt;
    Geometry* g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());
    if (!mapnik::util::to_wkt(wkt, g->feat_->paths()))
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

NAN_METHOD(Geometry::toWKB)
{
    NanScope();
    std::string wkt;
    Geometry* g = node::ObjectWrap::Unwrap<Geometry>(args.Holder());
    mapnik::util::wkb_buffer_ptr wkb = mapnik::util::to_wkb(g->feat_->paths(), mapnik::util::wkbNDR);
    if (!wkb)
    {
        NanThrowError("Failed to generate WKB - geometry likely null");
        NanReturnUndefined();
    }
    NanReturnValue(NanNewBufferHandle(wkb->buffer(), wkb->size()));
}
