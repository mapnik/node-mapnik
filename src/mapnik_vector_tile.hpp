#ifndef __NODE_MAPNIK_VECTOR_TILE_H__
#define __NODE_MAPNIK_VECTOR_TILE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include <vector>
#include <string>
#include <mapnik/feature.hpp>

using namespace v8;

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

class VectorTile: public node::ObjectWrap {
public:
    enum parsing_status {
        LAZY_DONE = 1,
        LAZY_SET = 2,
        LAZY_MERGE = 3
    };
    static Persistent<FunctionTemplate> constructor;
    static void Initialize(Handle<Object> target);
    static NAN_METHOD(New);
    static NAN_METHOD(getData);
    static NAN_METHOD(render);
    static NAN_METHOD(toJSON);
    static NAN_METHOD(query);
    static void EIO_Query(uv_work_t* req);
    static void EIO_AfterQuery(uv_work_t* req);
    static std::vector<query_result> _query(VectorTile* d, double lon, double lat, double tolerance, std::string const& layer_name);
    static bool _querySort(query_result const& a, query_result const& b);
    static Local<Array> _queryResultToV8(std::vector<query_result> const& result);
    static NAN_METHOD(queryMany);
    static queryMany_result _queryMany(VectorTile* d, std::vector<query_lonlat> const& query, double tolerance, std::string const& layer_name, std::vector<std::string> const& fields);
    static bool _queryManySort(query_hit const& a, query_hit const& b);
    static Local<Object> _queryManyResultToV8(queryMany_result const& result);
    static void EIO_QueryMany(uv_work_t* req);
    static void EIO_AfterQueryMany(uv_work_t* req);
    static NAN_METHOD(names);
    static Local<Value> _toGeoJSONSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(toGeoJSON);
    static NAN_METHOD(toGeoJSONSync);
    static void to_geojson(uv_work_t* req);
    static void after_to_geojson(uv_work_t* req);
    static NAN_METHOD(addGeoJSON);
    static NAN_METHOD(addImage);
#ifdef PROTOBUF_FULL
    static NAN_METHOD(toString);
#endif
    static void EIO_RenderTile(uv_work_t* req);
    static void EIO_AfterRenderTile(uv_work_t* req);
    static NAN_METHOD(setData);
    static void EIO_SetData(uv_work_t* req);
    static void EIO_AfterSetData(uv_work_t* req);
    static Local<Value> _setDataSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(setDataSync);
    static NAN_METHOD(parse);
    static void EIO_Parse(uv_work_t* req);
    static void EIO_AfterParse(uv_work_t* req);
    static NAN_METHOD(parseSync);
    static Local<Value> _parseSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(addData);
    static NAN_METHOD(composite);
    static NAN_METHOD(compositeSync);
    static Local<Value> _compositeSync(_NAN_METHOD_ARGS);
    static void EIO_Composite(uv_work_t* req);
    static void EIO_AfterComposite(uv_work_t* req);
    // methods common to mapnik.Image
    static NAN_METHOD(width);
    static NAN_METHOD(height);
    static NAN_METHOD(painted);
    static NAN_METHOD(clearSync);
    static Local<Value> _clearSync(_NAN_METHOD_ARGS);
    static NAN_METHOD(clear);
    static void EIO_Clear(uv_work_t* req);
    static void EIO_AfterClear(uv_work_t* req);
    static NAN_METHOD(empty);
    static NAN_METHOD(isSolid);
    static void EIO_IsSolid(uv_work_t* req);
    static void EIO_AfterIsSolid(uv_work_t* req);
    static NAN_METHOD(isSolidSync);
    static Local<Value> _isSolidSync(_NAN_METHOD_ARGS);

    VectorTile(int z, int x, int y, unsigned w, unsigned h);

    void clear() {
        tiledata_.Clear();
        buffer_.clear();
        painted(false);
        byte_size_ = 0;
    }
    vector_tile::Tile & get_tile_nonconst() {
        return tiledata_;
    }
    std::vector<std::string> lazy_names();
    bool lazy_empty();
    void parse_proto();
    vector_tile::Tile const& get_tile() {
        return tiledata_;
    }
    void cache_bytesize() {
        byte_size_ = tiledata_.ByteSize();
    }
    void painted(bool painted) {
        painted_ = painted;
    }
    bool painted() const {
        return painted_;
    }
    unsigned width() const {
        return width_;
    }
    unsigned height() const {
        return height_;
    }
    int byte_size() {
        return byte_size_;
    }
    void _ref() { Ref(); }
    void _unref() { Unref(); }
    int z_;
    int x_;
    int y_;
    std::string buffer_;
    parsing_status status_;
private:
    ~VectorTile();
    vector_tile::Tile tiledata_;
    unsigned width_;
    unsigned height_;
    bool painted_;
    int byte_size_;
};

#endif // __NODE_MAPNIK_VECTOR_TILE_H__
