#include "utils.hpp"
#include "mapnik_vector_tile.hpp"

#include "vector_tile_compression.hpp"
#define MAPNIK_VECTOR_TILE_LIBRARY
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_load_tile.hpp"

// std
#include <set>                          // for set, etc
#include <string>                       // for string, char_traits, etc
#include <exception>                    // for exception
#include <vector>                       // for vector

// protozero
#include <protozero/pbf_reader.hpp>

/**
 * Return an object containing information about a vector tile buffer. Useful for
 * debugging `.mvt` files with errors.
 *
 * @name info
 * @param {Buffer} buffer - vector tile buffer
 * @returns {Object} json object with information about the vector tile buffer
 * @static
 * @memberof VectorTile
 * @instance
 * @example
 * var buffer = fs.readFileSync('./path/to/tile.mvt');
 * var info = mapnik.VectorTile.info(buffer);
 * console.log(info);
 * // { layers:
 * //   [ { name: 'world',
 * //      features: 1,
 * //      point_features: 0,
 * //      linestring_features: 0,
 * //      polygon_features: 1,
 * //      unknown_features: 0,
 * //      raster_features: 0,
 * //      version: 2 },
 * //    { name: 'world2',
 * //      features: 1,
 * //      point_features: 0,
 * //      linestring_features: 0,
 * //      polygon_features: 1,
 * //      unknown_features: 0,
 * //      raster_features: 0,
 * //      version: 2 } ],
 * //    errors: false }
 */
NAN_METHOD(VectorTile::info)
{
    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("must provide a buffer argument");
        return;
    }

    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj))
    {
        Nan::ThrowTypeError("first argument is invalid, must be a Buffer");
        return;
    }

    v8::Local<v8::Object> out = Nan::New<v8::Object>();
    v8::Local<v8::Array> layers = Nan::New<v8::Array>();
    std::set<mapnik::vector_tile_impl::validity_error> errors;
    bool has_errors = false;
    std::size_t layers_size = 0;
    bool first_layer = true;
    std::set<std::string> layer_names_set;
    std::uint32_t version = 1;
    protozero::pbf_reader tile_msg;
    std::string decompressed;
    try
    {
        if (mapnik::vector_tile_impl::is_gzip_compressed(node::Buffer::Data(obj),node::Buffer::Length(obj)) ||
            mapnik::vector_tile_impl::is_zlib_compressed(node::Buffer::Data(obj),node::Buffer::Length(obj)))
        {
            mapnik::vector_tile_impl::zlib_decompress(node::Buffer::Data(obj), node::Buffer::Length(obj), decompressed);
            tile_msg = protozero::pbf_reader(decompressed);
        }
        else
        {
            tile_msg = protozero::pbf_reader(node::Buffer::Data(obj),node::Buffer::Length(obj));
        }
        while (tile_msg.next())
        {
            switch (tile_msg.tag())
            {
                case mapnik::vector_tile_impl::Tile_Encoding::LAYERS:
                    {
                        v8::Local<v8::Object> layer_obj = Nan::New<v8::Object>();
                        std::uint64_t point_feature_count = 0;
                        std::uint64_t line_feature_count = 0;
                        std::uint64_t polygon_feature_count = 0;
                        std::uint64_t unknown_feature_count = 0;
                        std::uint64_t raster_feature_count = 0;
                        auto layer_view = tile_msg.get_view();
                        protozero::pbf_reader layer_props_msg(layer_view);
                        auto layer_info = mapnik::vector_tile_impl::get_layer_name_and_version(layer_props_msg);
                        std::string const& layer_name = layer_info.first;
                        std::uint32_t layer_version = layer_info.second;
                        std::set<mapnik::vector_tile_impl::validity_error> layer_errors;
                        if (version > 2 || version < 1)
                        {
                            layer_errors.insert(mapnik::vector_tile_impl::LAYER_HAS_UNSUPPORTED_VERSION);
                        }
                        protozero::pbf_reader layer_msg(layer_view);
                        mapnik::vector_tile_impl::layer_is_valid(layer_msg,
                                                                 layer_errors,
                                                                 point_feature_count,
                                                                 line_feature_count,
                                                                 polygon_feature_count,
                                                                 unknown_feature_count,
                                                                 raster_feature_count);
                        std::uint64_t feature_count = point_feature_count +
                                                      line_feature_count +
                                                      polygon_feature_count +
                                                      unknown_feature_count +
                                                      raster_feature_count;
                        if (!layer_name.empty())
                        {
                            auto p = layer_names_set.insert(layer_name);
                            if (!p.second)
                            {
                                errors.insert(mapnik::vector_tile_impl::TILE_REPEATED_LAYER_NAMES);
                            }
                            layer_obj->Set(Nan::New("name").ToLocalChecked(), Nan::New<v8::String>(layer_name).ToLocalChecked());
                        }
                        layer_obj->Set(Nan::New("features").ToLocalChecked(), Nan::New<v8::Number>(feature_count));
                        layer_obj->Set(Nan::New("point_features").ToLocalChecked(), Nan::New<v8::Number>(point_feature_count));
                        layer_obj->Set(Nan::New("linestring_features").ToLocalChecked(), Nan::New<v8::Number>(line_feature_count));
                        layer_obj->Set(Nan::New("polygon_features").ToLocalChecked(), Nan::New<v8::Number>(polygon_feature_count));
                        layer_obj->Set(Nan::New("unknown_features").ToLocalChecked(), Nan::New<v8::Number>(unknown_feature_count));
                        layer_obj->Set(Nan::New("raster_features").ToLocalChecked(), Nan::New<v8::Number>(raster_feature_count));
                        layer_obj->Set(Nan::New("version").ToLocalChecked(), Nan::New<v8::Number>(layer_version));
                        if (!layer_errors.empty())
                        {
                            has_errors = true;
                            v8::Local<v8::Array> err_arr = Nan::New<v8::Array>();
                            std::size_t i = 0;
                            for (auto const& e : layer_errors)
                            {
                                err_arr->Set(i++, Nan::New<v8::String>(mapnik::vector_tile_impl::validity_error_to_string(e)).ToLocalChecked());
                            }
                            layer_obj->Set(Nan::New("errors").ToLocalChecked(), err_arr);
                        }
                        if (first_layer)
                        {
                            version = layer_version;
                        }
                        else
                        {
                            if (version != layer_version)
                            {
                                errors.insert(mapnik::vector_tile_impl::TILE_HAS_DIFFERENT_VERSIONS);
                            }
                        }
                        first_layer = false;
                        layers->Set(layers_size++, layer_obj);
                    }
                    break;
                default:
                    errors.insert(mapnik::vector_tile_impl::TILE_HAS_UNKNOWN_TAG);
                    tile_msg.skip();
                    break;
            }
        }
    }
    catch (...)
    {
        errors.insert(mapnik::vector_tile_impl::INVALID_PBF_BUFFER);
    }
    out->Set(Nan::New("layers").ToLocalChecked(), layers);
    has_errors = has_errors || !errors.empty();
    out->Set(Nan::New("errors").ToLocalChecked(),  Nan::New<v8::Boolean>(has_errors));
    if (!errors.empty())
    {
        v8::Local<v8::Array> err_arr = Nan::New<v8::Array>();
        std::size_t i = 0;
        for (auto const& e : errors)
        {
            err_arr->Set(i++, Nan::New<v8::String>(mapnik::vector_tile_impl::validity_error_to_string(e)).ToLocalChecked());
        }
        out->Set(Nan::New("tile_errors").ToLocalChecked(), err_arr);
    }
    info.GetReturnValue().Set(out);
    return;
}
