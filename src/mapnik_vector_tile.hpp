#pragma once

// stl
#include <cmath> // M_PI
#include <napi.h>
// mapnik-vector-tile
#include "vector_tile_merc_tile.hpp"
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
    explicit query_result() : layer(),
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
    std::map<unsigned, query_result> features;
    std::map<unsigned, std::vector<query_hit>> hits;
};
namespace detail {
struct AsyncRenderVectorTile;
}

class VectorTile : public Napi::ObjectWrap<VectorTile>
{
  public:
    // initiaizer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr);
    // ctor
    explicit VectorTile(Napi::CallbackInfo const& info);
    // methods
    Napi::Value getData(Napi::CallbackInfo const& info);
    Napi::Value getDataSync(Napi::CallbackInfo const& info);
    Napi::Value render(Napi::CallbackInfo const& info);
    Napi::Value toJSON(Napi::CallbackInfo const& info);
    Napi::Value query(Napi::CallbackInfo const& info);
    Napi::Value queryMany(Napi::CallbackInfo const& info);
    Napi::Value extent(Napi::CallbackInfo const& info);
    Napi::Value bufferedExtent(Napi::CallbackInfo const& info);
    Napi::Value names(Napi::CallbackInfo const& info);
    Napi::Value layer(Napi::CallbackInfo const& info);
    Napi::Value emptyLayers(Napi::CallbackInfo const& info);
    Napi::Value paintedLayers(Napi::CallbackInfo const& info);
    Napi::Value toGeoJSON(Napi::CallbackInfo const& info);
    Napi::Value toGeoJSONSync(Napi::CallbackInfo const& info);
    Napi::Value addGeoJSON(Napi::CallbackInfo const& info);
    Napi::Value addImage(Napi::CallbackInfo const& info);
    Napi::Value addImageSync(Napi::CallbackInfo const& info);
    Napi::Value addImageBuffer(Napi::CallbackInfo const& info);
    Napi::Value addImageBufferSync(Napi::CallbackInfo const& info);
    Napi::Value setData(Napi::CallbackInfo const& info);
    Napi::Value setDataSync(Napi::CallbackInfo const& info);
    Napi::Value addData(Napi::CallbackInfo const& info);
    Napi::Value addDataSync(Napi::CallbackInfo const& info);
    Napi::Value composite(Napi::CallbackInfo const& info);
    Napi::Value compositeSync(Napi::CallbackInfo const& info);
    Napi::Value painted(Napi::CallbackInfo const& info);
    Napi::Value clearSync(Napi::CallbackInfo const& info);
    Napi::Value clear(Napi::CallbackInfo const& info);
    Napi::Value empty(Napi::CallbackInfo const& info);

#if BOOST_VERSION >= 105800
    Napi::Value reportGeometrySimplicity(Napi::CallbackInfo const& info);
    Napi::Value reportGeometrySimplicitySync(Napi::CallbackInfo const& info);
    Napi::Value reportGeometryValidity(Napi::CallbackInfo const& info);
    Napi::Value reportGeometryValiditySync(Napi::CallbackInfo const& info);
#endif // BOOST_VERSION >= 105800
    // static methods
    static Napi::Value info(Napi::CallbackInfo const& info);
    // accessors
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
    inline mapnik::vector_tile_impl::merc_tile_ptr impl() const { return tile_; }
    static Napi::FunctionReference constructor;

  private:
    mapnik::vector_tile_impl::merc_tile_ptr tile_;
};
