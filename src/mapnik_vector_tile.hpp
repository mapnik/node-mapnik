#ifndef __NODE_MAPNIK_VECTOR_TILE_H__
#define __NODE_MAPNIK_VECTOR_TILE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
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

class VectorTile: public Nan::ObjectWrap
{
public:
    static Nan::Persistent<v8::FunctionTemplate> constructor;
    static void Initialize(v8::Local<v8::Object> target);
    static NAN_METHOD(New);
    static NAN_METHOD(getData);
    static NAN_METHOD(getDataSync);
    static v8::Local<v8::Value> _getDataSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static void get_data(uv_work_t* req);
    static void after_get_data(uv_work_t* req);
    static NAN_METHOD(render);
    static NAN_METHOD(toJSON);
    static NAN_METHOD(query);
    static void EIO_Query(uv_work_t* req);
    static void EIO_AfterQuery(uv_work_t* req);
    static std::vector<query_result> _query(VectorTile* d, double lon, double lat, double tolerance, std::string const& layer_name);
    static bool _querySort(query_result const& a, query_result const& b);
    static v8::Local<v8::Array> _queryResultToV8(std::vector<query_result> const& result);
    static NAN_METHOD(queryMany);
    static void _queryMany(queryMany_result & result, VectorTile* d, std::vector<query_lonlat> const& query, double tolerance, std::string const& layer_name, std::vector<std::string> const& fields);
    static bool _queryManySort(query_hit const& a, query_hit const& b);
    static v8::Local<v8::Object> _queryManyResultToV8(queryMany_result const& result);
    static void EIO_QueryMany(uv_work_t* req);
    static void EIO_AfterQueryMany(uv_work_t* req);
    static NAN_METHOD(extent);
    static NAN_METHOD(bufferedExtent);
    static NAN_METHOD(names);
    static NAN_METHOD(layer);
    static NAN_METHOD(emptyLayers);
    static NAN_METHOD(paintedLayers);
    static v8::Local<v8::Value> _toGeoJSONSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(toGeoJSON);
    static NAN_METHOD(toGeoJSONSync);
    static void to_geojson(uv_work_t* req);
    static void after_to_geojson(uv_work_t* req);
    static NAN_METHOD(addGeoJSON);
    static NAN_METHOD(addImage);
    static void EIO_AddImage(uv_work_t* req);
    static void EIO_AfterAddImage(uv_work_t* req);
    static v8::Local<v8::Value> _addImageSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(addImageSync);
    static NAN_METHOD(addImageBuffer);
    static void EIO_AddImageBuffer(uv_work_t* req);
    static void EIO_AfterAddImageBuffer(uv_work_t* req);
    static v8::Local<v8::Value> _addImageBufferSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(addImageBufferSync);
    static void EIO_RenderTile(uv_work_t* req);
    static void EIO_AfterRenderTile(uv_work_t* req);
    static NAN_METHOD(setData);
    static void EIO_SetData(uv_work_t* req);
    static void EIO_AfterSetData(uv_work_t* req);
    static v8::Local<v8::Value> _setDataSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(setDataSync);
    static NAN_METHOD(addData);
    static void EIO_AddData(uv_work_t* req);
    static void EIO_AfterAddData(uv_work_t* req);
    static v8::Local<v8::Value> _addDataSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(addDataSync);
    static NAN_METHOD(composite);
    static NAN_METHOD(compositeSync);
    static v8::Local<v8::Value> _compositeSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static void EIO_Composite(uv_work_t* req);
    static void EIO_AfterComposite(uv_work_t* req);
    static NAN_METHOD(painted);
    static NAN_METHOD(clearSync);
    static v8::Local<v8::Value> _clearSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(clear);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);
    static NAN_METHOD(empty);
    static NAN_METHOD(info);
#if BOOST_VERSION >= 105800
    static NAN_METHOD(reportGeometrySimplicity);
    static void EIO_ReportGeometrySimplicity(uv_work_t* req);
    static void EIO_AfterReportGeometrySimplicity(uv_work_t* req);
    static NAN_METHOD(reportGeometrySimplicitySync);
    static v8::Local<v8::Value> _reportGeometrySimplicitySync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(reportGeometryValidity);
    static void EIO_ReportGeometryValidity(uv_work_t* req);
    static void EIO_AfterReportGeometryValidity(uv_work_t* req);
    static NAN_METHOD(reportGeometryValiditySync);
    static v8::Local<v8::Value> _reportGeometryValiditySync(Nan::NAN_METHOD_ARGS_TYPE info);
#endif // BOOST_VERSION >= 105800

    static NAN_GETTER(get_tile_x);
    static NAN_SETTER(set_tile_x);
    static NAN_GETTER(get_tile_y);
    static NAN_SETTER(set_tile_y);
    static NAN_GETTER(get_tile_z);
    static NAN_SETTER(set_tile_z);
    static NAN_GETTER(get_tile_size);
    static NAN_SETTER(set_tile_size);
    static NAN_GETTER(get_buffer_size);
    static NAN_SETTER(set_buffer_size);

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
    
    using Nan::ObjectWrap::Ref;
    using Nan::ObjectWrap::Unref;

private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
    ~VectorTile();
};

#endif // __NODE_MAPNIK_VECTOR_TILE_H__
