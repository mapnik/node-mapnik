#include <mapnik/image.hpp>
#include <mapnik/version.hpp>
#include <mapnik/image_reader.hpp>
#include <mapnik/safe_cast.hpp>

#include "zlib.h"

#if defined(HAVE_PNG)
#include <mapnik/png_io.hpp>
#endif

#if defined(HAVE_JPEG)
#define XMD_H
#include <mapnik/jpeg_io.hpp>
#undef XMD_H
#endif

#if defined(HAVE_WEBP)
#include <mapnik/webp_io.hpp>
#endif

#include "mapnik_palette.hpp"
#include "blend.hpp"
#include "tint.hpp"
#include "utils.hpp"

#include <sstream>
#include <cstring>
#include <cstdlib>
#include <memory>

namespace node_mapnik {

static bool hexToUInt32Color(char *hex, std::uint32_t & value) {
    if (!hex) return false;
    std::size_t len_original = strlen(hex);
    // Return is the length of the string is less then six
    // otherwise the line after this could go to some other
    // pointer in memory, resulting in strange behaviours.
    if (len_original < 6) return false;
    if (hex[0] == '#') ++hex;
    std::size_t len = strlen(hex);
    if (len != 6 && len != 8) return false;

    std::uint32_t color = 0;
    std::stringstream ss;
    ss << std::hex << hex;
    ss >> color;

    if (len == 8) {
        // Circular shift to get from RGBA to ARGB.
        value = (color << 24) | ((color & 0xFF00) << 8) | ((color & 0xFF0000) >> 8) | ((color & 0xFF000000) >> 24);
        return true;
    } else {
        value = 0xFF000000 | ((color & 0xFF) << 16) | (color & 0xFF00) | ((color & 0xFF0000) >> 16);
        return true;
    }
}

NAN_METHOD(rgb2hsl) {
    if (info.Length() != 3) {
        Nan::ThrowTypeError("Please pass r,g,b integer values as three arguments");
        return;
    }
    if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber()) {
        Nan::ThrowTypeError("Please pass r,g,b integer values as three arguments");
        return;
    }
    std::uint32_t r = Nan::To<int>(info[0]).FromJust();
    std::uint32_t g = Nan::To<int>(info[1]).FromJust();
    std::uint32_t b = Nan::To<int>(info[2]).FromJust();
    v8::Local<v8::Array> hsl = Nan::New<v8::Array>(3);
    double h,s,l;
    rgb_to_hsl(r,g,b,h,s,l);
    Nan::Set(hsl, 0, Nan::New<v8::Number>(h));
    Nan::Set(hsl, 1, Nan::New<v8::Number>(s));
    Nan::Set(hsl, 2, Nan::New<v8::Number>(l));
    info.GetReturnValue().Set(hsl);
}

NAN_METHOD(hsl2rgb) {
    if (info.Length() != 3) {
        Nan::ThrowTypeError("Please pass hsl fractional values as three arguments");
        return;
    }
    if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber()) {
        Nan::ThrowTypeError("Please pass hsl fractional values as three arguments");
        return;
    }
    double h,s,l;
    h = Nan::To<double>(info[0]).FromJust();
    s = Nan::To<double>(info[1]).FromJust();
    l = Nan::To<double>(info[2]).FromJust();
    v8::Local<v8::Array> rgb = Nan::New<v8::Array>(3);
    std::uint32_t r,g,b;
    hsl_to_rgb(h,s,l,r,g,b);
    Nan::Set(rgb, 0, Nan::New<v8::Integer>(r));
    Nan::Set(rgb, 1, Nan::New<v8::Integer>(g));
    Nan::Set(rgb, 2, Nan::New<v8::Integer>(b));
    info.GetReturnValue().Set(rgb);
}

static void parseTintOps(v8::Local<v8::Object> const& tint, Tinter & tinter, std::string & msg) {
    Nan::HandleScope scope;
    v8::Local<v8::Value> hue = Nan::Get(tint, Nan::New("h").ToLocalChecked()).ToLocalChecked();
    if (!hue.IsEmpty() && hue->IsArray()) {
        v8::Local<v8::Array> val_array = v8::Local<v8::Array>::Cast(hue);
        if (val_array->Length() != 2) {
            msg = "h array must be a pair of values";
        }
        tinter.h0 = Nan::To<double>(Nan::Get(val_array, 0).ToLocalChecked()).FromJust();
        tinter.h1 = Nan::To<double>(Nan::Get(val_array, 1).ToLocalChecked()).FromJust();
    }
    v8::Local<v8::Value> sat = Nan::Get(tint, Nan::New("s").ToLocalChecked()).ToLocalChecked();
    if (!sat.IsEmpty() && sat->IsArray()) {
        v8::Local<v8::Array> val_array = v8::Local<v8::Array>::Cast(sat);
        if (val_array->Length() != 2) {
            msg = "s array must be a pair of values";
        }
        tinter.s0 = Nan::To<double>(Nan::Get(val_array, 0).ToLocalChecked()).FromJust();
        tinter.s1 = Nan::To<double>(Nan::Get(val_array, 1).ToLocalChecked()).FromJust();
    }
    v8::Local<v8::Value> light = Nan::Get(tint, Nan::New("l").ToLocalChecked()).ToLocalChecked();
    if (!light.IsEmpty() && light->IsArray()) {
        v8::Local<v8::Array> val_array = v8::Local<v8::Array>::Cast(light);
        if (val_array->Length() != 2) {
            msg = "l array must be a pair of values";
        }
        tinter.l0 = Nan::To<double>(Nan::Get(val_array, 0).ToLocalChecked()).FromJust();
        tinter.l1 = Nan::To<double>(Nan::Get(val_array, 1).ToLocalChecked()).FromJust();
    }
    v8::Local<v8::Value> alpha = Nan::Get(tint, Nan::New("a").ToLocalChecked()).ToLocalChecked();
    if (!alpha.IsEmpty() && alpha->IsArray()) {
        v8::Local<v8::Array> val_array = v8::Local<v8::Array>::Cast(alpha);
        if (val_array->Length() != 2) {
            msg = "a array must be a pair of values";
        }
        tinter.a0 = Nan::To<double>(Nan::Get(val_array, 0).ToLocalChecked()).FromJust();
        tinter.a1 = Nan::To<double>(Nan::Get(val_array, 1).ToLocalChecked()).FromJust();
    }
}

static inline void Blend_CompositePixel(std::uint32_t & target, std::uint32_t const& source) {
    if (source <= 0x00FFFFFF) {
        // Top pixel is fully transparent.
        // <do nothing>
    } else if (source >= 0xFF000000 || target <= 0x00FFFFFF) {
        // Top pixel is fully opaque or bottom pixel is fully transparent.
        target = source;
    } else {
        // Both pixels have transparency.
        // From http://trac.mapnik.org/browser/trunk/include/mapnik/graphics.hpp#L337
        long a1 = (source >> 24) & 0xff;
        long r1 = source & 0xff;
        long g1 = (source >> 8) & 0xff;
        long b1 = (source >> 16) & 0xff;

        long a0 = (target >> 24) & 0xff;
        long r0 = (target & 0xff) * a0;
        long g0 = ((target >> 8) & 0xff) * a0;
        long b0 = ((target >> 16) & 0xff) * a0;

        a0 = ((a1 + a0) << 8) - a0 * a1;
        r0 = ((((r1 << 8) - r0) * a1 + (r0 << 8)) / a0);
        g0 = ((((g1 << 8) - g0) * a1 + (g0 << 8)) / a0);
        b0 = ((((b1 << 8) - b0) * a1 + (b0 << 8)) / a0);
        a0 = a0 >> 8;
        target = (a0 << 24) | (b0 << 16) | (g0 << 8) | (r0);
    }
}

static inline void TintPixel(std::uint32_t & r,
                             std::uint32_t & g,
                             std::uint32_t & b,
                             Tinter const& tint) {
    double h;
    double s;
    double l;
    rgb_to_hsl(r,g,b,h,s,l);
    double h2 = tint.h0 + (h * (tint.h1 - tint.h0));
    double s2 = tint.s0 + (s * (tint.s1 - tint.s0));
    double l2 = tint.l0 + (l * (tint.l1 - tint.l0));
    if (h2 > 1) h2 = 1;
    if (h2 < 0) h2 = 0;
    if (s2 > 1) s2 = 1;
    if (s2 < 0) s2 = 0;
    if (l2 > 1) l2 = 1;
    if (l2 < 0) l2 = 0;
    hsl_to_rgb(h2,s2,l2,r,g,b);
}


static void Blend_Composite(std::uint32_t *target, BlendBaton *baton, BImage *image) {
    const std::uint32_t *source = image->im_raw_ptr->data();

    int sourceX = std::max(0, -image->x);
    int sourceY = std::max(0, -image->y);
    int sourcePos = sourceY * image->width + sourceX;

    int width = image->width - sourceX - std::max(0, image->x + image->width - baton->width);
    int height = image->height - sourceY - std::max(0, image->y + image->height - baton->height);

    int targetX = std::max(0, image->x);
    int targetY = std::max(0, image->y);
    int targetPos = targetY * baton->width + targetX;
    bool tinting = !image->tint.is_identity();
    bool set_alpha = !image->tint.is_alpha_identity();
    if (tinting || set_alpha) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                std::uint32_t const& source_pixel = source[sourcePos + x];
                std::uint32_t a = (source_pixel >> 24) & 0xff;
                if (set_alpha) {
                    double a2 = image->tint.a0 + (a/255.0 * (image->tint.a1 - image->tint.a0));
                    if (a2 < 0) a2 = 0;
                    a = static_cast<std::uint32_t>(std::floor((a2 * 255.0)+.5));
                    if (a > 255) a = 255;
                }
                std::uint32_t r = source_pixel & 0xff;
                std::uint32_t g = (source_pixel >> 8 ) & 0xff;
                std::uint32_t b = (source_pixel >> 16) & 0xff;
                if (a > 1 && tinting) {
                    TintPixel(r,g,b,image->tint);
                }
                std::uint32_t new_pixel = (a << 24) | (b << 16) | (g << 8) | (r);
                Blend_CompositePixel(target[targetPos + x], new_pixel);
            }
            sourcePos += image->width;
            targetPos += baton->width;
        }
    } else {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Blend_CompositePixel(target[targetPos + x], source[sourcePos + x]);
            }
            sourcePos += image->width;
            targetPos += baton->width;
        }
    }
}

static void Blend_Encode(mapnik::image_rgba8 const& image, BlendBaton* baton, bool alpha) {
    try {
        std::ostringstream stream(std::ios::out | std::ios::binary);
        if (baton->format == BLEND_FORMAT_JPEG) {
#if defined(HAVE_JPEG)
            if (baton->quality == 0) baton->quality = 85;
            mapnik::save_as_jpeg(stream, baton->quality, image);
#else
            baton->message = "Mapnik not built with jpeg support";
#endif
        } else if (baton->format == BLEND_FORMAT_WEBP) {
#if defined(HAVE_WEBP)
            if (baton->quality == 0) baton->quality = 80;
            WebPConfig config;
            // Default values set here will be lossless=0 and quality=75 (as least as of webp v0.3.1)
            if (!WebPConfigInit(&config)) {
                /* LCOV_EXCL_START */
                baton->message = "WebPConfigInit failed: version mismatch";
                /* LCOV_EXCL_STOP */
            } else {
                // see for more details: https://github.com/mapnik/mapnik/wiki/Image-IO#webp-output-options
                config.quality = baton->quality;
                if (baton->compression > 0) {
                    config.method = baton->compression;
                }
                mapnik::save_as_webp(stream,image,config,alpha);
            }
#else
            baton->message = "Mapnik not built with webp support";
#endif
        } else {
            // Save as PNG.
#if defined(HAVE_PNG)
            mapnik::png_options opts;
            opts.compression = baton->compression;
            if (baton->palette && baton->palette->valid()) {
                mapnik::save_as_png8_pal(stream, image, *baton->palette, opts);
            } else if (baton->quality > 0) {
                opts.colors = baton->quality;
                // Paletted PNG.
                if (alpha && baton->mode == BLEND_MODE_HEXTREE) {
                    mapnik::save_as_png8_hex(stream, image, opts);
                } else {
                    mapnik::save_as_png8_oct(stream, image, opts);
                }
            } else {
                mapnik::save_as_png(stream, image, opts);
            }
#else
            baton->message = "Mapnik not built with png support";
#endif
        }
        baton->output_data = std::make_unique<std::string>(stream.str());
    } catch (const std::exception& ex) {
        baton->message = ex.what();
    }
}

void Work_Blend(uv_work_t* req)
{
    BlendBaton* baton = static_cast<BlendBaton*>(req->data);
    bool alpha = true;
    int size = 0;

    // Iterate from the last to first image because we potentially don't have
    // to decode all images if there's an opaque one.
    Images::reverse_iterator rit = baton->images.rbegin();
    Images::reverse_iterator rend = baton->images.rend();
    for (; rit != rend; ++rit)
    {
        // If an image that is higher than the current is opaque, stop all-together.
        if (!alpha) break;
        auto image = *rit;
        if (!image) continue;

        if (image->im_obj) {
            unsigned layer_width = image->im_obj->get()->width();
            unsigned layer_height = image->im_obj->get()->height();
            if (layer_width == 0 || layer_height == 0) {
                baton->message = "zero width/height image encountered";
                return;
            }
            int visibleWidth = (int)layer_width + image->x;
            int visibleHeight = (int)layer_height + image->y;
            // The first image that is in the viewport sets the width/height, if not user supplied.
            if (baton->width <= 0) baton->width = std::max(0, visibleWidth);
            if (baton->height <= 0) baton->height = std::max(0, visibleHeight);

            // Skip images that are outside of the viewport.
            if (visibleWidth <= 0 || visibleHeight <= 0 || image->x >= baton->width || image->y >= baton->height) {
                // Remove this layer from the list of layers we consider blending.
                continue;
            }
            image->width = layer_width;
            image->height = layer_height;
            image->im_raw_ptr = &image->im_obj->get()->get<mapnik::image_rgba8>();
        } else {
            std::unique_ptr<mapnik::image_reader> image_reader;
            try {
                image_reader = std::unique_ptr<mapnik::image_reader>(mapnik::get_image_reader(image->data, image->dataLength));
            } catch (std::exception const& ex) {
                baton->message = ex.what();
                return;
            }

            if (!image_reader || !image_reader.get()) {
                // Not quite sure anymore how the pointer would not be returned
                // from the reader and can't find a way to make this fail.
                // So removing from coverage
                /* LCOV_EXCL_START */
                baton->message = "Unknown image format";
                return;
                /* LCOV_EXCL_STOP */
            }

            unsigned layer_width = image_reader->width();
            unsigned layer_height = image_reader->height();
            // Error out on invalid images.
            if (layer_width == 0 || layer_height == 0) {
                // No idea how to create a zero height or width image
                // so removing from coverage, because I am fairly certain
                // it is not possible in almost every image format.
                /* LCOV_EXCL_START */
                baton->message = "zero width/height image encountered";
                return;
                /* LCOV_EXCL_STOP */
            }

            int visibleWidth = (int)layer_width + image->x;
            int visibleHeight = (int)layer_height + image->y;
            // The first image that is in the viewport sets the width/height, if not user supplied.
            if (baton->width <= 0) baton->width = std::max(0, visibleWidth);
            if (baton->height <= 0) baton->height = std::max(0, visibleHeight);

            // Skip images that are outside of the viewport.
            if (visibleWidth <= 0 || visibleHeight <= 0 || image->x >= baton->width || image->y >= baton->height) {
                // Remove this layer from the list of layers we consider blending.
                continue;
            }

            bool layer_has_alpha = image_reader->has_alpha();

            // Short-circuit when we're not reencoding.
            if (size == 0 && !layer_has_alpha && !baton->reencode &&
                image->x == 0 && image->y == 0 &&
                (int)layer_width == baton->width && (int)layer_height == baton->height)
            {
                baton->output_data = std::make_unique<std::string>((char *)image->data, image->dataLength);
                return;
            }

            // allocate image for decoded pixels
            std::unique_ptr<mapnik::image_rgba8> im_ptr(new mapnik::image_rgba8(layer_width,layer_height));
            // actually decode pixels now
            try {
                image_reader->read(0,0,*im_ptr);
            } catch (std::exception const&) {
                baton->message = "Could not decode image";
                return;
            }

            bool coversWidth = image->x <= 0 && visibleWidth >= baton->width;
            bool coversHeight = image->y <= 0 && visibleHeight >= baton->height;
            if (!layer_has_alpha && coversWidth && coversHeight && image->tint.is_alpha_identity()) {
                // Skip decoding more layers.
                alpha = false;
            }

            // Convenience aliases.
            image->width = layer_width;
            image->height = layer_height;
            image->im_ptr = std::move(im_ptr);
            image->im_raw_ptr = image->im_ptr.get();
        }
        ++size;

    }

    // Now blend images.
    int pixels = baton->width * baton->height;
    if (pixels <= 0) {
        std::ostringstream msg;
        msg << "Image dimensions " << baton->width << "x" << baton->height << " are invalid";
        baton->message = msg.str();
        return;
    }

    mapnik::image_rgba8 target(baton->width, baton->height);
    // When we don't actually have transparent pixels, we don't need to set the matte.
    if (alpha) {
        target.set(baton->matte);
    }
    for (auto image_ptr : baton->images)
    {
        if (image_ptr && image_ptr->im_raw_ptr)
        {
            Blend_Composite(target.data(), baton, &*image_ptr);
        }
    }
    Blend_Encode(target, baton, alpha);
}

void Work_AfterBlend(uv_work_t* req) {
    Nan::HandleScope scope;
    BlendBaton* baton = static_cast<BlendBaton*>(req->data);
    Nan::AsyncResource async_resource(__func__);

    for (auto im : baton->images) {
        if (im->im_obj) {
            im->im_obj->Unref();
        }
    }

    if (!baton->message.length()) {
        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            NewBufferFrom(std::move(baton->output_data)).ToLocalChecked()
        };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 2, argv);
    } else {
        v8::Local<v8::Value> argv[] = {
            Nan::Error(baton->message.c_str())
        };
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 1, argv);
    }
    delete baton;
}

/**
 * **`mapnik.Blend`**
 *
 * Composite multiple images on top of each other, with strong control
 * over how the images are combined, resampled, and blended.
 *
 * @name blend
 * @param {Array<Buffer>} buffers an array of buffers
 * @param {Object} options can include width, height, `compression`,
 * `reencode`, palette, mode can be either `hextree` or `octree`, quality. JPEG & WebP quality
 * quality ranges from 0-100, PNG quality from 2-256. Compression varies by platform -
 * it references the internal zlib compression algorithm.
 * @param {Function} callback called with (err, res), where a successful
 * result is a processed image as a Buffer
 * @example
 * mapnik.blend([
 *  fs.readFileSync('foo.png'),
 *  fs.readFileSync('bar.png'),
 * ], function(err, result) {
 *  if (err) throw err;
 *  fs.writeFileSync('result.png', result);
 * });
 */
NAN_METHOD(Blend) {
    std::unique_ptr<BlendBaton> baton(new BlendBaton());
    v8::Local<v8::Object> options;
    if (info.Length() == 0 || !info[0]->IsArray()) {
        Nan::ThrowTypeError("First argument must be an array of Buffers.");
        return;
    } else if (info.Length() == 1) {
        Nan::ThrowTypeError("Second argument must be a function");
        return;
    } else if (info.Length() == 2) {
        // No options provided.
        if (!info[1]->IsFunction()) {
            Nan::ThrowTypeError("Second argument must be a function.");
            return;
        }
        baton->callback.Reset(info[1].As<v8::Function>());
    } else if (info.Length() >= 3) {
        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("Second argument must be a an options object.");
            return;
        }
        options = v8::Local<v8::Object>::Cast(info[1]);

        if (!info[2]->IsFunction()) {
            Nan::ThrowTypeError("Third argument must be a function.");
            return;
        }
        baton->callback.Reset(info[2].As<v8::Function>());
    }

    // Validate options
    if (!options.IsEmpty()) {
        baton->quality = Nan::To<int>(Nan::Get(options, Nan::New("quality").ToLocalChecked()).ToLocalChecked()).FromJust();

        v8::Local<v8::Value> format_val = Nan::Get(options, Nan::New("format").ToLocalChecked()).ToLocalChecked();
        if (!format_val.IsEmpty() && format_val->IsString()) {
            std::string format_val_string = TOSTR(format_val);
            if (format_val_string == "jpeg" || format_val_string == "jpg") {
                baton->format = BLEND_FORMAT_JPEG;
                if (baton->quality == 0) baton->quality = 85; // 85 is same default as mapnik core jpeg
                else if (baton->quality < 0 || baton->quality > 100) {
                    Nan::ThrowTypeError("JPEG quality is range 0-100.");
                    return;
                }
            } else if (format_val_string == "png") {
                if (baton->quality == 1 || baton->quality > 256) {
                    Nan::ThrowTypeError("PNG images must be quantized between 2 and 256 colors.");
                    return;
                }
            } else if (format_val_string == "webp") {
                baton->format = BLEND_FORMAT_WEBP;
                if (baton->quality == 0) baton->quality = 80;
                else if (baton->quality < 0 || baton->quality > 100) {
                    Nan::ThrowTypeError("WebP quality is range 0-100.");
                    return;
                }
            } else {
                Nan::ThrowTypeError("Invalid output format.");
                return;
            }
        }

        baton->reencode = Nan::To<bool>(Nan::Get(options, Nan::New("reencode").ToLocalChecked()).ToLocalChecked()).FromJust();
        baton->width = Nan::To<int>(Nan::Get(options, Nan::New("width").ToLocalChecked()).ToLocalChecked()).FromJust();
        baton->height = Nan::To<int>(Nan::Get(options, Nan::New("height").ToLocalChecked()).ToLocalChecked()).FromJust();

        v8::Local<v8::Value> matte_val = Nan::Get(options, Nan::New("matte").ToLocalChecked()).ToLocalChecked();
        if (!matte_val.IsEmpty() && matte_val->IsString()) {
            if (!hexToUInt32Color(TOSTR(matte_val->ToString(Nan::GetCurrentContext()).ToLocalChecked()), baton->matte))
            {
                Nan::ThrowTypeError("Invalid batte provided.");
                return;
            }

            // Make sure we're reencoding in the case of single alpha PNGs
            if (baton->matte && !baton->reencode) {
                baton->reencode = true;
            }
        }

        v8::Local<v8::Value> palette_val = Nan::Get(options, Nan::New("palette").ToLocalChecked()).ToLocalChecked();
        if (!palette_val.IsEmpty() && palette_val->IsObject()) {
            baton->palette = Nan::ObjectWrap::Unwrap<Palette>(palette_val->ToObject(Nan::GetCurrentContext()).ToLocalChecked())->palette();
        }

        v8::Local<v8::Value> mode_val = Nan::Get(options, Nan::New("mode").ToLocalChecked()).ToLocalChecked();
        if (!mode_val.IsEmpty() && mode_val->IsString()) {
            std::string mode_string = TOSTR(mode_val);
            if (mode_string == "octree" || mode_string == "o") {
                baton->mode = BLEND_MODE_OCTREE;
            }
            else if (mode_string == "hextree" || mode_string == "h") {
                baton->mode = BLEND_MODE_HEXTREE;
            }
        }

        if (Nan::Has(options, Nan::New("compression").ToLocalChecked()).FromMaybe(false)) {
            v8::Local<v8::Value> compression_val = Nan::Get(options, Nan::New("compression").ToLocalChecked()).ToLocalChecked();
            if (!compression_val.IsEmpty() && compression_val->IsNumber())
            {
                baton->compression = Nan::To<int>(compression_val).FromJust();
            }
            else
            {
                Nan::ThrowTypeError("Compression option must be a number");
                return;
            }
        }

        int min_compression = Z_NO_COMPRESSION;
        int max_compression = Z_BEST_COMPRESSION;
        if (baton->format == BLEND_FORMAT_PNG) {
            if (baton->compression < 0) baton->compression = Z_DEFAULT_COMPRESSION;
        } else if (baton->format == BLEND_FORMAT_WEBP) {
            min_compression = 0, max_compression = 6;
            if (baton->compression < 0) baton->compression = -1;
        }

        if (baton->compression > max_compression) {
            std::ostringstream msg;
            msg << "Compression level must be between "
                << min_compression << " and " << max_compression;
            Nan::ThrowTypeError(msg.str().c_str());
            return;
        }
    }

    v8::Local<v8::Array> js_images = v8::Local<v8::Array>::Cast(info[0]);
    uint32_t length = js_images->Length();
    if (length < 1 && !baton->reencode) {
        Nan::ThrowTypeError("First argument must contain at least one Buffer.");
        return;
    } else if (length == 1 && !baton->reencode) {
        v8::Local<v8::Value> buffer = Nan::Get(js_images, 0).ToLocalChecked();
        if (node::Buffer::HasInstance(buffer)) {
            // Directly pass through buffer if it's the only one.
            v8::Local<v8::Value> argv[] = {
                Nan::Null(),
                buffer
            };
            Nan::AsyncResource async_resource(__func__);
            async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 2, argv);
            return;
        }
    }

    if (!(length >= 1 || (baton->width > 0 && baton->height > 0))) {
        Nan::ThrowTypeError("Without buffers, you have to specify width and height.");
        return;
    }

    if (baton->width < 0 || baton->height < 0) {
        Nan::ThrowTypeError("Image dimensions must be greater than 0.");
        return;
    }

    for (uint32_t i = 0; i < length; ++i) {
        ImagePtr image = std::make_shared<BImage>();
        v8::Local<v8::Value> buffer = Nan::Get(js_images, i).ToLocalChecked();
        if (node::Buffer::HasInstance(buffer)) {
            image->buffer.Reset(buffer.As<v8::Object>());
        } else if (buffer->IsObject()) {
            v8::Local<v8::Object> props = buffer->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
            if (Nan::New(Image::constructor)->HasInstance(props)) {
                Image * im = Nan::ObjectWrap::Unwrap<Image>(props);
                if (im->get()->get_dtype() == mapnik::image_dtype_rgba8) {
                    image->im_obj = im;
                } else {
                    Nan::ThrowTypeError("Only mapnik.Image types that are rgba8 can be passed to blend");
                    return;
                }
            } else {
                if (Nan::Has(props, Nan::New("buffer").ToLocalChecked()).FromMaybe(false)) {
                    buffer = Nan::Get(props, Nan::New("buffer").ToLocalChecked()).ToLocalChecked();
                    if (node::Buffer::HasInstance(buffer)) {
                        image->buffer.Reset(buffer.As<v8::Object>());
                    } else if (buffer->IsObject()) {
                        v8::Local<v8::Object> possible_im = buffer.As<v8::Object>();
                        if (Nan::New(Image::constructor)->HasInstance(possible_im)) {
                            Image * im = Nan::ObjectWrap::Unwrap<Image>(possible_im);
                            if (im->get()->get_dtype() == mapnik::image_dtype_rgba8) {
                                image->im_obj = im;
                            } else {
                                Nan::ThrowTypeError("Only mapnik.Image types that are rgba8 can be passed to blend");
                                return;
                            }
                        }
                    }
                }
                image->x = Nan::To<int>(Nan::Get(props, Nan::New("x").ToLocalChecked()).ToLocalChecked()).FromJust();
                image->y = Nan::To<int>(Nan::Get(props, Nan::New("y").ToLocalChecked()).ToLocalChecked()).FromJust();

                v8::Local<v8::Value> tint_val = Nan::Get(props, Nan::New("tint").ToLocalChecked()).ToLocalChecked();
                if (!tint_val.IsEmpty() && tint_val->IsObject()) {
                    v8::Local<v8::Object> tint = tint_val->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
                    if (!tint.IsEmpty()) {
                        baton->reencode = true;
                        std::string msg;
                        parseTintOps(tint,image->tint,msg);
                        if (!msg.empty()) {
                            Nan::ThrowTypeError(msg.c_str());
                            return;
                        }
                    }
                }
            }
        }

        if (image->buffer.IsEmpty() && !image->im_obj) {
            Nan::ThrowTypeError("All elements must be Buffers or RGBA Mapnik Image objects or objects with a 'buffer' property.");
            return;
        }
        if (!image->im_obj) {
            image->data = node::Buffer::Data(buffer);
            image->dataLength = node::Buffer::Length(buffer);
        }
        baton->images.push_back(image);
    }

    for (auto im : baton->images) {
        if (im->im_obj) {
            im->im_obj->Ref();
        }
    }

    uv_queue_work(uv_default_loop(), &(baton.release())->request, Work_Blend, (uv_after_work_cb)Work_AfterBlend);

    return;
}

}
