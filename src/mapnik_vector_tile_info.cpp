#include "mapnik_vector_tile.hpp"
#include "vector_tile_load_tile.hpp"

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
Napi::Value VectorTile::info(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "must provide a buffer argument").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.IsBuffer())
    {
        Napi::TypeError::New(env, "first argument is invalid, must be a Buffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Object out = Napi::Object::New(env);
    Napi::Array layers = Napi::Array::New(env);
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
        if (mapnik::vector_tile_impl::is_gzip_compressed(obj.As<Napi::Buffer<char>>().Data(), obj.As<Napi::Buffer<char>>().Length()) ||
            mapnik::vector_tile_impl::is_zlib_compressed(obj.As<Napi::Buffer<char>>().Data(), obj.As<Napi::Buffer<char>>().Length()))
        {
            mapnik::vector_tile_impl::zlib_decompress(obj.As<Napi::Buffer<char>>().Data(), obj.As<Napi::Buffer<char>>().Length(), decompressed);
            tile_msg = protozero::pbf_reader(decompressed);
        }
        else
        {
            tile_msg = protozero::pbf_reader(obj.As<Napi::Buffer<char>>().Data(), obj.As<Napi::Buffer<char>>().Length());
        }
        while (tile_msg.next())
        {
            switch (tile_msg.tag())
            {
            case mapnik::vector_tile_impl::Tile_Encoding::LAYERS: {
                Napi::Object layer_obj = Napi::Object::New(env);
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
                    layer_obj.Set("name", layer_name);
                }
                layer_obj.Set("features", Napi::Number::New(env, feature_count));
                layer_obj.Set("point_features", Napi::Number::New(env, point_feature_count));
                layer_obj.Set("linestring_features", Napi::Number::New(env, line_feature_count));
                layer_obj.Set("polygon_features", Napi::Number::New(env, polygon_feature_count));
                layer_obj.Set("unknown_features", Napi::Number::New(env, unknown_feature_count));
                layer_obj.Set("raster_features", Napi::Number::New(env, raster_feature_count));
                layer_obj.Set("version", Napi::Number::New(env, layer_version));
                if (!layer_errors.empty())
                {
                    has_errors = true;
                    Napi::Array err_arr = Napi::Array::New(env);
                    std::size_t i = 0;
                    for (auto const& e : layer_errors)
                    {
                        err_arr.Set(i++, Napi::String::New(env, mapnik::vector_tile_impl::validity_error_to_string(e)));
                    }
                    layer_obj.Set("errors", err_arr);
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
                layers.Set(layers_size++, layer_obj);
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

    out.Set("layers", layers);
    has_errors = has_errors || !errors.empty();
    out.Set("errors", Napi::Boolean::New(env, has_errors));
    if (!errors.empty())
    {
        Napi::Array err_arr = Napi::Array::New(env);
        std::size_t i = 0;
        for (auto const& e : errors)
        {
            err_arr.Set(i++, Napi::String::New(env, mapnik::vector_tile_impl::validity_error_to_string(e)));
        }
        out.Set("tile_errors", err_arr);
    }
    return scope.Escape(out);
}
