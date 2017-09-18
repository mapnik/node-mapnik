#include "utils.hpp"
#include "mapnik_vector_tile.hpp"

#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_geometry_decoder.hpp"

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/geometry_is_simple.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/feature_to_geojson.hpp>

// std
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

#if BOOST_VERSION >= 105800

// LCOV_EXCL_START
struct not_simple_feature
{
    not_simple_feature(std::string const& layer_,
                       std::int64_t feature_id_)
        : layer(layer_),
          feature_id(feature_id_) {}
    std::string const layer;
    std::int64_t const feature_id;
};
// LCOV_EXCL_STOP

void layer_not_simple(protozero::pbf_reader const& layer_msg,
               unsigned x,
               unsigned y,
               unsigned z,
               std::vector<not_simple_feature> & errors)
{
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer_msg, x, y, z);
    mapnik::query q(mapnik::box2d<double>(std::numeric_limits<double>::lowest(),
                                          std::numeric_limits<double>::lowest(),
                                          std::numeric_limits<double>::max(),
                                          std::numeric_limits<double>::max()));
    mapnik::layer_descriptor ld = ds.get_descriptor();
    for (auto const& item : ld.get_descriptors())
    {
        q.add_property_name(item.get_name());
    }
    mapnik::featureset_ptr fs = ds.features(q);
    if (fs && mapnik::is_valid(fs))
    {
        mapnik::feature_ptr feature;
        while ((feature = fs->next()))
        {
            if (!mapnik::geometry::is_simple(feature->get_geometry()))
            {
                // Right now we don't have an obvious way of bypassing our validation
                // process in JS, so let's skip testing this line
                // LCOV_EXCL_START
                errors.emplace_back(ds.get_name(), feature->id());
                // LCOV_EXCL_STOP
            }
        }
    }
}

void vector_tile_not_simple(VectorTile * v,
                            std::vector<not_simple_feature> & errors)
{
    protozero::pbf_reader tile_msg(v->get_tile()->get_reader());
    while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
    {
        protozero::pbf_reader layer_msg(tile_msg.get_message());
        layer_not_simple(layer_msg,
                         v->get_tile()->x(),
                         v->get_tile()->y(),
                         v->get_tile()->z(),
                         errors);
    }
}

v8::Local<v8::Array> make_not_simple_array(std::vector<not_simple_feature> & errors)
{
    Nan::EscapableHandleScope scope;
    v8::Local<v8::Array> array = Nan::New<v8::Array>(errors.size());
    v8::Local<v8::String> layer_key = Nan::New<v8::String>("layer").ToLocalChecked();
    v8::Local<v8::String> feature_id_key = Nan::New<v8::String>("featureId").ToLocalChecked();
    std::uint32_t idx = 0;
    for (auto const& error : errors)
    {
        // LCOV_EXCL_START
        v8::Local<v8::Object> obj = Nan::New<v8::Object>();
        obj->Set(layer_key, Nan::New<v8::String>(error.layer).ToLocalChecked());
        obj->Set(feature_id_key, Nan::New<v8::Number>(error.feature_id));
        array->Set(idx++, obj);
        // LCOV_EXCL_STOP
    }
    return scope.Escape(array);
}

struct not_simple_baton
{
    uv_work_t request;
    VectorTile* v;
    bool error;
    std::vector<not_simple_feature> result;
    std::string err_msg;
    Nan::Persistent<v8::Function> cb;
};

/**
 * Count the number of geometries that are not [OGC simple]{@link http://www.iso.org/iso/catalogue_detail.htm?csnumber=40114}
 *
 * @memberof VectorTile
 * @instance
 * @name reportGeometrySimplicitySync
 * @returns {number} number of features that are not simple
 * @example
 * var simple = vectorTile.reportGeometrySimplicitySync();
 * console.log(simple); // array of non-simple geometries and their layer info
 * console.log(simple.length); // number
 */
NAN_METHOD(VectorTile::reportGeometrySimplicitySync)
{
    info.GetReturnValue().Set(_reportGeometrySimplicitySync(info));
}

v8::Local<v8::Value> VectorTile::_reportGeometrySimplicitySync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    VectorTile* d = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    try
    {
        std::vector<not_simple_feature> errors;
        vector_tile_not_simple(d, errors);
        return scope.Escape(make_not_simple_array(errors));
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        Nan::ThrowError(ex.what());
        // LCOV_EXCL_STOP
    }
    // LCOV_EXCL_START
    return scope.Escape(Nan::Undefined());
    // LCOV_EXCL_STOP
}


/**
 * Count the number of geometries that are not [OGC simple]{@link http://www.iso.org/iso/catalogue_detail.htm?csnumber=40114}
 *
 * @memberof VectorTile
 * @instance
 * @name reportGeometrySimplicity
 * @param {Function} callback
 * @example
 * vectorTile.reportGeometrySimplicity(function(err, simple) {
 *   if (err) throw err;
 *   console.log(simple); // array of non-simple geometries and their layer info
 *   console.log(simple.length); // number
 * });
 */
NAN_METHOD(VectorTile::reportGeometrySimplicity)
{
    if (info.Length() == 0)
    {
        info.GetReturnValue().Set(_reportGeometrySimplicitySync(info));
        return;
    }
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!callback->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    not_simple_baton *closure = new not_simple_baton();
    closure->request.data = closure;
    closure->v = Nan::ObjectWrap::Unwrap<VectorTile>(info.Holder());
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_ReportGeometrySimplicity, (uv_after_work_cb)EIO_AfterReportGeometrySimplicity);
    closure->v->Ref();
    return;
}

void VectorTile::EIO_ReportGeometrySimplicity(uv_work_t* req)
{
    not_simple_baton *closure = static_cast<not_simple_baton *>(req->data);
    try
    {
        vector_tile_not_simple(closure->v, closure->result);
    }
    catch (std::exception const& ex)
    {
        // LCOV_EXCL_START
        closure->error = true;
        closure->err_msg = ex.what();
        // LCOV_EXCL_STOP
    }
}

void VectorTile::EIO_AfterReportGeometrySimplicity(uv_work_t* req)
{
    Nan::HandleScope scope;
    not_simple_baton *closure = static_cast<not_simple_baton *>(req->data);
    if (closure->error)
    {
        // LCOV_EXCL_START
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->err_msg.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
        // LCOV_EXCL_STOP
    }
    else
    {
        v8::Local<v8::Array> array = make_not_simple_array(closure->result);
        v8::Local<v8::Value> argv[2] = { Nan::Null(), array };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->v->Unref();
    closure->cb.Reset();
    delete closure;
}

#endif // BOOST_VERSION >= 1.58
