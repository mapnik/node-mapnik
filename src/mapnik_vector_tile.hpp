#ifndef __NODE_MAPNIK_VECTOR_TILE_H__
#define __NODE_MAPNIK_VECTOR_TILE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include <vector>
#include <string>
#include <mapnik/feature.hpp>
#include <boost/version.hpp>

struct query_lonlat {
    double lon;
    double lat;
};

struct query_result {
    std::string layer;
    double distance;
    mapnik::feature_ptr feature;
};

struct query_hit {
    double distance;
    unsigned feature_id;
};

struct queryMany_result {
    std::map<unsigned,query_result> features;
    std::map<unsigned,std::vector<query_hit> > hits;
};

class VectorTile: public Nan::ObjectWrap {
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
    static NAN_METHOD(names);
    static v8::Local<v8::Value> _toGeoJSONSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(toGeoJSON);
    static NAN_METHOD(toGeoJSONSync);
    static void to_geojson(uv_work_t* req);
    static void after_to_geojson(uv_work_t* req);
    static NAN_METHOD(addGeoJSON);
    static NAN_METHOD(addImage);
    static void EIO_RenderTile(uv_work_t* req);
    static void EIO_AfterRenderTile(uv_work_t* req);
    static NAN_METHOD(setData);
    static void EIO_SetData(uv_work_t* req);
    static void EIO_AfterSetData(uv_work_t* req);
    static v8::Local<v8::Value> _setDataSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(setDataSync);
    static NAN_METHOD(parse);
    static void EIO_Parse(uv_work_t* req);
    static void EIO_AfterParse(uv_work_t* req);
    static NAN_METHOD(parseSync);
    static v8::Local<v8::Value> _parseSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(addData);
    static NAN_METHOD(composite);
    static NAN_METHOD(compositeSync);
    static v8::Local<v8::Value> _compositeSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static void EIO_Composite(uv_work_t* req);
    static void EIO_AfterComposite(uv_work_t* req);
    // methods common to mapnik.Image
    static NAN_METHOD(width);
    static NAN_METHOD(height);
    static NAN_METHOD(painted);
    static NAN_METHOD(clearSync);
    static v8::Local<v8::Value> _clearSync(Nan::NAN_METHOD_ARGS_TYPE info);
    static NAN_METHOD(clear);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);
    static NAN_METHOD(empty);
    static NAN_METHOD(isSolid);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static NAN_METHOD(isSolidSync);
    static v8::Local<v8::Value> _isSolidSync(Nan::NAN_METHOD_ARGS_TYPE info);
#if BOOST_VERSION >= 105600
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
#endif // BOOST_VERSION >= 105600

    VectorTile(int z, int x, int y, unsigned w, unsigned h);

    void clear() {
        buffer_.clear();
    }
    void parse_proto();
    bool painted() const {
        return !buffer_.empty();
    }
    unsigned width() const {
        return width_;
    }
    unsigned height() const {
        return height_;
    }
    void _ref() { Ref(); }
    void _unref() { Unref(); }
    int z_;
    int x_;
    int y_;
    std::string buffer_;
private:
    ~VectorTile();
    unsigned width_;
    unsigned height_;
};

#endif // __NODE_MAPNIK_VECTOR_TILE_H__
