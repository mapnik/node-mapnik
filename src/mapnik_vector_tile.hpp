#ifndef __NODE_MAPNIK_VECTOR_TILE_H__
#define __NODE_MAPNIK_VECTOR_TILE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <napi.h>
#include <uv.h>
#pragma GCC diagnostic pop

// mapnik-vector-tile
#include "vector_tile_merc_tile.hpp"

// std
#include <string>
#include <set>
#include <vector>

// mapnik
#include <mapnik/feature.hpp>

// boost
#include <boost/version.hpp>

struct query_lonlat
{
    double lon;
    double lat;
};

struct query_result
{
    std::string layer;
    double distance;
    double x_hit;
    double y_hit;
    mapnik::feature_ptr feature;
    explicit query_result() :
     layer(),
     distance(0),
     x_hit(0),
     y_hit(0) {}
};

struct query_hit
{
    double distance;
    unsigned feature_id;
};

struct queryMany_result
{
    std::map<unsigned,query_result> features;
    std::map<unsigned,std::vector<query_hit> > hits;
};

class VectorTile : public Napi::ObjectWrap<VectorTile>
{
public:
    static Napi::FunctionReference constructor;
    static void Initialize(Napi::Object target);
    static Napi::Value New(Napi::CallbackInfo const& info);
    static Napi::Value getData(Napi::CallbackInfo const& info);
    static Napi::Value getDataSync(Napi::CallbackInfo const& info);
    static Napi::Value _getDataSync(Napi::CallbackInfo const& info);
    static void get_data(uv_work_t* req);
    static void after_get_data(uv_work_t* req);
    static Napi::Value render(Napi::CallbackInfo const& info);
    static Napi::Value toJSON(Napi::CallbackInfo const& info);
    static Napi::Value query(Napi::CallbackInfo const& info);
    static void EIO_Query(uv_work_t* req);
    static void EIO_AfterQuery(uv_work_t* req);
    static std::vector<query_result> _query(VectorTile* d, double lon, double lat, double tolerance, std::string const& layer_name);
    //static bool _querySort(query_result const& a, query_result const& b);
    static Napi::Array _queryResultToV8(std::vector<query_result> const& result);
    static Napi::Value queryMany(Napi::CallbackInfo const& info);
    static void _queryMany(queryMany_result & result, VectorTile* d, std::vector<query_lonlat> const& query, double tolerance, std::string const& layer_name, std::vector<std::string> const& fields);
    static bool _queryManySort(query_hit const& a, query_hit const& b);
    static Napi::Object _queryManyResultToV8(queryMany_result const& result);
    static void EIO_QueryMany(uv_work_t* req);
    static void EIO_AfterQueryMany(uv_work_t* req);
    static Napi::Value extent(Napi::CallbackInfo const& info);
    static Napi::Value bufferedExtent(Napi::CallbackInfo const& info);
    static Napi::Value names(Napi::CallbackInfo const& info);
    static Napi::Value layer(Napi::CallbackInfo const& info);
    static Napi::Value emptyLayers(Napi::CallbackInfo const& info);
    static Napi::Value paintedLayers(Napi::CallbackInfo const& info);
    static Napi::Value _toGeoJSONSync(Napi::CallbackInfo const& info);
    static Napi::Value toGeoJSON(Napi::CallbackInfo const& info);
    static Napi::Value toGeoJSONSync(Napi::CallbackInfo const& info);
    static void to_geojson(uv_work_t* req);
    static void after_to_geojson(uv_work_t* req);
    static Napi::Value addGeoJSON(Napi::CallbackInfo const& info);
    static Napi::Value addImage(Napi::CallbackInfo const& info);
    static void EIO_AddImage(uv_work_t* req);
    static void EIO_AfterAddImage(uv_work_t* req);
    static Napi::Value _addImageSync(Napi::CallbackInfo const& info);
    static Napi::Value addImageSync(Napi::CallbackInfo const& info);
    static Napi::Value addImageBuffer(Napi::CallbackInfo const& info);
    static void EIO_AddImageBuffer(uv_work_t* req);
    static void EIO_AfterAddImageBuffer(uv_work_t* req);
    static Napi::Value _addImageBufferSync(Napi::CallbackInfo const& info);
    static Napi::Value addImageBufferSync(Napi::CallbackInfo const& info);
    static void EIO_RenderTile(uv_work_t* req);
    static void EIO_AfterRenderTile(uv_work_t* req);
    static Napi::Value setData(Napi::CallbackInfo const& info);
    static void EIO_SetData(uv_work_t* req);
    static void EIO_AfterSetData(uv_work_t* req);
    static Napi::Value _setDataSync(Napi::CallbackInfo const& info);
    static Napi::Value setDataSync(Napi::CallbackInfo const& info);
    static Napi::Value addData(Napi::CallbackInfo const& info);
    static void EIO_AddData(uv_work_t* req);
    static void EIO_AfterAddData(uv_work_t* req);
    static Napi::Value _addDataSync(Napi::CallbackInfo const& info);
    static Napi::Value addDataSync(Napi::CallbackInfo const& info);
    static Napi::Value composite(Napi::CallbackInfo const& info);
    static Napi::Value compositeSync(Napi::CallbackInfo const& info);
    static Napi::Value _compositeSync(Napi::CallbackInfo const& info);
    static void EIO_Composite(uv_work_t* req);
    static void EIO_AfterComposite(uv_work_t* req);
    static Napi::Value painted(Napi::CallbackInfo const& info);
    static Napi::Value clearSync(Napi::CallbackInfo const& info);
    static Napi::Value _clearSync(Napi::CallbackInfo const& info);
    static Napi::Value clear(Napi::CallbackInfo const& info);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);
    static Napi::Value empty(Napi::CallbackInfo const& info);
    static Napi::Value info(Napi::CallbackInfo const& info);
#if BOOST_VERSION >= 105800
    static Napi::Value reportGeometrySimplicity(Napi::CallbackInfo const& info);
    static void EIO_ReportGeometrySimplicity(uv_work_t* req);
    static void EIO_AfterReportGeometrySimplicity(uv_work_t* req);
    static Napi::Value reportGeometrySimplicitySync(Napi::CallbackInfo const& info);
    static Napi::Value _reportGeometrySimplicitySync(Napi::CallbackInfo const& info);
    static Napi::Value reportGeometryValidity(Napi::CallbackInfo const& info);
    static void EIO_ReportGeometryValidity(uv_work_t* req);
    static void EIO_AfterReportGeometryValidity(uv_work_t* req);
    static Napi::Value reportGeometryValiditySync(Napi::CallbackInfo const& info);
    static Napi::Value _reportGeometryValiditySync(Napi::CallbackInfo const& info);
#endif // BOOST_VERSION >= 105800

    Napi::Value get_tile_x(Napi::CallbackInfo const& info);
    void set_tile_x(Napi::CallbackInfo const& info, const Napi::Value& value);
    Napi::Value get_tile_y(Napi::CallbackInfo const& info);
    void set_tile_y(Napi::CallbackInfo const& info, const Napi::Value& value);
    Napi::Value get_tile_z(Napi::CallbackInfo const& info);
    void set_tile_z(Napi::CallbackInfo const& info, const Napi::Value& value);
    Napi::Value get_tile_size(Napi::CallbackInfo const& info);
    void set_tile_size(Napi::CallbackInfo const& info, const Napi::Value& value);
    Napi::Value get_buffer_size(Napi::CallbackInfo const& info);
    void set_buffer_size(Napi::CallbackInfo const& info, const Napi::Value& value);

    VectorTile(std::uint64_t z,
               std::uint64_t x,
               std::uint64_t y,
               std::uint32_t tile_size,
               std::int32_t buffer_size);

    void clear()
    {
        tile_->clear();
    }

    std::uint32_t tile_size() const
    {
        return tile_->tile_size();
    }

    void tile_size(std::uint32_t val)
    {
        tile_->tile_size(val);
    }

    std::int32_t buffer_size() const
    {
        return tile_->buffer_size();
    }

    void buffer_size(std::int32_t val)
    {
        tile_->buffer_size(val);
    }

    mapnik::vector_tile_impl::merc_tile_ptr & get_tile()
    {
        return tile_;
    }

    mapnik::vector_tile_impl::merc_tile_ptr const& get_tile() const
    {
        return tile_;
    }

    using Napi::ObjectWrap::Ref;
    using Napi::ObjectWrap::Unref;

private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    ~VectorTile();
};

#endif // __NODE_MAPNIK_VECTOR_TILE_H__
