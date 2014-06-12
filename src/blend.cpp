#include <mapnik/image_data.hpp>
#include <mapnik/png_io.hpp>
#include <mapnik/jpeg_io.hpp>
#include <mapnik/webp_io.hpp>
#include "reader.hpp"
#include "blend.hpp"
#include "tint.hpp"

#include <sstream>

#include <node_version.h>

using namespace v8;
using namespace node;

namespace node_mapnik {

static unsigned int hexToUInt32Color(char *hex) {
    if (!hex) return 0;
    if (hex[0] == '#') hex++;
    int len = strlen(hex);
    if (len != 6 && len != 8) return 0;

    unsigned int color = 0;
    std::stringstream ss;
    ss << std::hex << hex;
    ss >> color;

    if (len == 8) {
        // Circular shift to get from RGBA to ARGB.
        return (color << 24) | ((color & 0xFF00) << 8) | ((color & 0xFF0000) >> 8) | ((color & 0xFF000000) >> 24);
    } else {
        return 0xFF000000 | ((color & 0xFF) << 16) | (color & 0xFF00) | ((color & 0xFF0000) >> 16);
    }
}

static void parseTintOps(Local<Object> const& tint, Tinter & tinter, std::string & msg) {
    HandleScope scope;
    Local<Value> hue = tint->Get(String::NewSymbol("h"));
    if (!hue.IsEmpty() && hue->IsArray()) {
        Local<Array> val_array = Local<Array>::Cast(hue);
        if (val_array->Length() != 2) {
            msg = "h array must be a pair of values";
        }
        tinter.h0 = val_array->Get(0)->NumberValue();
        tinter.h1 = val_array->Get(1)->NumberValue();
    }
    Local<Value> sat = tint->Get(String::NewSymbol("s"));
    if (!sat.IsEmpty() && sat->IsArray()) {
        Local<Array> val_array = Local<Array>::Cast(sat);
        if (val_array->Length() != 2) {
            msg = "s array must be a pair of values";
        }
        tinter.s0 = val_array->Get(0)->NumberValue();
        tinter.s1 = val_array->Get(1)->NumberValue();
    }
    Local<Value> light = tint->Get(String::NewSymbol("l"));
    if (!light.IsEmpty() && light->IsArray()) {
        Local<Array> val_array = Local<Array>::Cast(light);
        if (val_array->Length() != 2) {
            msg = "l array must be a pair of values";
        }
        tinter.l0 = val_array->Get(0)->NumberValue();
        tinter.l1 = val_array->Get(1)->NumberValue();
    }
    Local<Value> alpha = tint->Get(String::NewSymbol("a"));
    if (!alpha.IsEmpty() && alpha->IsArray()) {
        Local<Array> val_array = Local<Array>::Cast(alpha);
        if (val_array->Length() != 2) {
            msg = "a array must be a pair of values";
        }
        tinter.a0 = val_array->Get(0)->NumberValue();
        tinter.a1 = val_array->Get(1)->NumberValue();
    }
    Local<Value> debug = tint->Get(String::NewSymbol("debug"));
    if (!debug.IsEmpty()) {
        tinter.debug = debug->BooleanValue();
    }
}
Handle<Value> Blend(const Arguments& args) {
    HandleScope scope;
    std::auto_ptr<BlendBaton> baton(new BlendBaton());

    Local<Object> options;
    if (args.Length() == 0 || !args[0]->IsArray()) {
        return TYPE_EXCEPTION("First argument must be an array of Buffers.");
    } else if (args.Length() == 1) {
        return TYPE_EXCEPTION("Second argument must be a function");
    } else if (args.Length() == 2) {
        // No options provided.
        if (!args[1]->IsFunction()) {
            return TYPE_EXCEPTION("Second argument must be a function.");
        }
        baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));
    } else if (args.Length() >= 3) {
        if (!args[1]->IsObject()) {
            return TYPE_EXCEPTION("Second argument must be a an options object.");
        }
        options = Local<Object>::Cast(args[1]);

        if (!args[2]->IsFunction()) {
            return TYPE_EXCEPTION("Third argument must be a function.");
        }
        baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[2]));
    }

    // Validate options
    if (!options.IsEmpty()) {
        baton->quality = options->Get(String::NewSymbol("quality"))->Int32Value();

        Local<Value> format_val = options->Get(String::NewSymbol("format"));
        if (!format_val.IsEmpty() && format_val->IsString()) {
            if (strcmp(*String::AsciiValue(format_val), "jpeg") == 0 ||
                    strcmp(*String::AsciiValue(format_val), "jpg") == 0) {
                baton->format = BLEND_FORMAT_JPEG;
                if (baton->quality == 0) baton->quality = 80;
                else if (baton->quality < 0 || baton->quality > 100) {
                    return TYPE_EXCEPTION("JPEG quality is range 0-100.");
                }
            } else if (strcmp(*String::AsciiValue(format_val), "png") == 0) {
                if (baton->quality == 1 || baton->quality > 256) {
                    return TYPE_EXCEPTION("PNG images must be quantized between 2 and 256 colors.");
                }
            } else if (strcmp(*String::AsciiValue(format_val), "webp") == 0) {
                baton->format = BLEND_FORMAT_WEBP;
                if (baton->quality == 0) baton->quality = 80;
                else if (baton->quality < 0 || baton->quality > 100) {
                    return TYPE_EXCEPTION("WebP quality is range 0-100.");
                }
            } else {
                return TYPE_EXCEPTION("Invalid output format.");
            }
        }

        baton->reencode = options->Get(String::NewSymbol("reencode"))->BooleanValue();
        baton->width = options->Get(String::NewSymbol("width"))->Int32Value();
        baton->height = options->Get(String::NewSymbol("height"))->Int32Value();

        Local<Value> matte_val = options->Get(String::NewSymbol("matte"));
        if (!matte_val.IsEmpty() && matte_val->IsString()) {
            baton->matte = hexToUInt32Color(*String::AsciiValue(matte_val->ToString()));

            // Make sure we're reencoding in the case of single alpha PNGs
            if (baton->matte && !baton->reencode) {
                baton->reencode = true;
            }
        }

        Local<Value> mode_val = options->Get(String::NewSymbol("mode"));
        if (!mode_val.IsEmpty() && mode_val->IsString()) {
            if (strcmp(*String::AsciiValue(mode_val), "octree") == 0 ||
                strcmp(*String::AsciiValue(mode_val), "o") == 0) {
                baton->mode = BLEND_MODE_OCTREE;
            }
            else if (strcmp(*String::AsciiValue(mode_val), "hextree") == 0 ||
                strcmp(*String::AsciiValue(mode_val), "h") == 0) {
                baton->mode = BLEND_MODE_HEXTREE;
            }
        }

        Local<Value> encoder_val = options->Get(String::NewSymbol("encoder"));
        if (!encoder_val.IsEmpty() && encoder_val->IsString()) {
            if (strcmp(*String::AsciiValue(encoder_val), "miniz") == 0) {
                baton->encoder = BLEND_ENCODER_MINIZ;
            }
            // default is libpng
        }

        if (options->Has(String::NewSymbol("compression"))) {
            baton->compression = options->Get(String::NewSymbol("compression"))->Int32Value();
        }

        int min_compression = Z_NO_COMPRESSION;
        int max_compression = Z_BEST_COMPRESSION;
        if (baton->format == BLEND_FORMAT_PNG) {
            if (baton->compression < 0) baton->compression = Z_DEFAULT_COMPRESSION;
            if (baton->encoder == BLEND_ENCODER_MINIZ) max_compression = 10; // MZ_UBER_COMPRESSION
        } else if (baton->format == BLEND_FORMAT_WEBP) {
            min_compression = 0, max_compression = 6;
            if (baton->compression < 0) baton->compression = -1;
        }

        if (baton->compression > max_compression) {
            std::ostringstream msg;
            msg << "Compression level must be between "
                << min_compression << " and " << max_compression;
            return TYPE_EXCEPTION(msg.str().c_str());
        }
    }

    Local<Array> images = Local<Array>::Cast(args[0]);
    uint32_t length = images->Length();
    if (length < 1 && !baton->reencode) {
        return TYPE_EXCEPTION("First argument must contain at least one Buffer.");
    } else if (length == 1 && !baton->reencode) {
        Local<Value> buffer = images->Get(0);
        if (Buffer::HasInstance(buffer)) {
            // Directly pass through buffer if it's the only one.
            Local<Value> argv[] = {
                Local<Value>::New(Null()),
                buffer
            };
            TRY_CATCH_CALL(Context::GetCurrent()->Global(), baton->callback, 2, argv);
            return scope.Close(Undefined());
        } else {
            // Check whether the argument is a complex image with offsets etc.
            // In that case, we don't throw but continue going through the blend
            // process below.
            bool valid = false;
            if (buffer->IsObject()) {
                Local<Object> props = buffer->ToObject();
                valid = props->Has(String::NewSymbol("buffer")) &&
                        Buffer::HasInstance(props->Get(String::NewSymbol("buffer")));
            }
            if (!valid) {
                return TYPE_EXCEPTION("All elements must be Buffers or objects with a 'buffer' property.");
            }
        }
    }

    if (!(length >= 1 || (baton->width > 0 && baton->height > 0))) {
        return TYPE_EXCEPTION("Without buffers, you have to specify width and height.");
    }

    if (baton->width < 0 || baton->height < 0) {
        return TYPE_EXCEPTION("Image dimensions must be greater than 0.");
    }

    for (uint32_t i = 0; i < length; i++) {
        ImagePtr image(new BImage());
        Local<Value> buffer = images->Get(i);
        if (Buffer::HasInstance(buffer)) {
            image->buffer = Persistent<Object>::New(buffer->ToObject());
        } else if (buffer->IsObject()) {
            Local<Object> props = buffer->ToObject();
            if (props->Has(String::NewSymbol("buffer"))) {
                buffer = props->Get(String::NewSymbol("buffer"));
                if (Buffer::HasInstance(buffer)) {
                    image->buffer = Persistent<Object>::New(buffer->ToObject());
                }
            }
            image->x = props->Get(String::NewSymbol("x"))->Int32Value();
            image->y = props->Get(String::NewSymbol("y"))->Int32Value();

            Local<Value> tint_val = props->Get(String::NewSymbol("tint"));
            if (!tint_val.IsEmpty() && tint_val->IsObject()) {
                Local<Object> tint = tint_val->ToObject();
                if (!tint.IsEmpty()) {
                    baton->reencode = true;
                    std::string msg;
                    parseTintOps(tint,image->tint,msg);
                    if (!msg.empty()) {
                        return TYPE_EXCEPTION(msg.c_str());
                    }
                }
            }
        }

        if (image->buffer.IsEmpty()) {
            return TYPE_EXCEPTION("All elements must be Buffers or objects with a 'buffer' property.");
        }

        image->data = (unsigned char*)node::Buffer::Data(image->buffer);
        image->dataLength = node::Buffer::Length(image->buffer);
        baton->images.push_back(image);
    }

    QUEUE_WORK(baton.release(), Work_Blend, (uv_after_work_cb)Work_AfterBlend);

    return scope.Close(Undefined());
}


static inline void Blend_CompositePixel(unsigned int& target, unsigned int& source) {
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

static inline void TintPixel(unsigned & r,
                      unsigned & g,
                      unsigned & b,
                      Tinter const& tint) {
    double h;
    double s;
    double l;
    rgb2hsl(r,g,b,h,s,l);
    double h2 = tint.h0 + (h * (tint.h1 - tint.h0));
    double s2 = tint.s0 + (s * (tint.s1 - tint.s0));
    double l2 = tint.l0 + (l * (tint.l1 - tint.l0));
    if (h2 > 1) h2 = 1;
    if (h2 < 0) h2 = 0;
    if (s2 > 1) s2 = 1;
    if (s2 < 0) s2 = 0;
    if (l2 > 1) l2 = 1;
    if (l2 < 0) l2 = 0;
    hsl2rgb(h2,s2,l2,r,g,b);
}


static void Blend_Composite(unsigned int *target, BlendBaton *baton, BImage *image) {
    unsigned int *source = image->reader->surface;

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
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned int& source_pixel = source[sourcePos + x];
                unsigned a = (source_pixel >> 24) & 0xff;
                if (set_alpha) {
                    double a2 = image->tint.a0 + (a/255.0 * (image->tint.a1 - image->tint.a0));
                    if (a2 < 0) a2 = 0;
                    a = static_cast<unsigned>(std::floor((a2 * 255.0)+.5));
                    if (a > 255) a = 255;
                }
                unsigned r = source_pixel & 0xff;
                unsigned g = (source_pixel >> 8 ) & 0xff;
                unsigned b = (source_pixel >> 16) & 0xff;
                if (a > 1 && tinting) {
                    TintPixel(r,g,b,image->tint);
                }
                source_pixel = (a << 24) | (b << 16) | (g << 8) | (r);
                Blend_CompositePixel(target[targetPos + x], source_pixel);
            }
            sourcePos += image->width;
            targetPos += baton->width;
        }
    } else {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Blend_CompositePixel(target[targetPos + x], source[sourcePos + x]);
            }
            sourcePos += image->width;
            targetPos += baton->width;
        }
    }
}

static void Blend_Encode(mapnik::image_data_32 const& image, BlendBaton* baton, bool alpha) {
    try {
        if (baton->format == BLEND_FORMAT_JPEG) {
            if (baton->quality == 0) baton->quality = 80;
            mapnik::save_as_jpeg(baton->stream, baton->quality, image);
        } else if (baton->format == BLEND_FORMAT_WEBP) {
            if (baton->quality == 0) baton->quality = 80;
            WebPConfig config;
            // Default values set here will be lossless=0 and quality=75 (as least as of webp v0.3.1)
            if (!WebPConfigInit(&config))
            {
                throw std::runtime_error("version mismatch");
            }
            // see for more details: https://github.com/mapnik/mapnik/wiki/Image-IO#webp-output-options
            bool alpha = true;
            config.quality = baton->quality;
            config.method = baton->compression;
            mapnik::save_as_webp(baton->stream,image,config,alpha);
        } else {
            // Save as PNG.
            mapnik::png_options opts;
            opts.compression = baton->compression;
            if (baton->encoder == BLEND_ENCODER_MINIZ) opts.use_miniz = true;
            if (baton->quality > 0) {
                opts.colors = baton->quality;
                // Paletted PNG.
                if (alpha && baton->mode == BLEND_MODE_HEXTREE) {
                    save_as_png8_hex(baton->stream, image, opts);
                } else {
                    save_as_png8_oct(baton->stream, image, opts);
                }
            } else {
                mapnik::save_as_png(baton->stream, image, opts);
            }
        }
    } catch (const std::exception& ex) {
        baton->message = ex.what();
    }
}

WORKER_BEGIN(Work_Blend) {
    BlendBaton* baton = static_cast<BlendBaton*>(req->data);

    int total = baton->images.size();
    bool alpha = true;
    int size = 0;

    // Iterate from the last to first image because we potentially don't have
    // to decode all images if there's an opaque one.
    Images::reverse_iterator rit = baton->images.rbegin();
    Images::reverse_iterator rend = baton->images.rend();
    for (int index = total - 1; rit != rend; rit++, index--) {
        // If an image that is higher than the current is opaque, stop alltogether.
        if (!alpha) break;

        BImage *image = &**rit;
        std::auto_ptr<ImageReader> layer(new ImageReader(image->data, image->dataLength));

        // Error out on invalid images.
        if (layer.get() == NULL || layer->width == 0 || layer->height == 0) {
            baton->message = layer->message;
            WORKER_END();
        }

        int visibleWidth = (int)layer->width + image->x;
        int visibleHeight = (int)layer->height + image->y;

        // The first image that is in the viewport sets the width/height, if not user supplied.
        if (baton->width <= 0) baton->width = std::max(0, visibleWidth);
        if (baton->height <= 0) baton->height = std::max(0, visibleHeight);

        // Skip images that are outside of the viewport.
        if (visibleWidth <= 0 || visibleHeight <= 0 || image->x >= baton->width || image->y >= baton->height) {
            // Remove this layer from the list of layers we consider blending.
            continue;
        }

        // Short-circuit when we're not reencoding.
        if (size == 0 && !layer->alpha && !baton->reencode &&
            image->x == 0 && image->y == 0 &&
            (int)layer->width == baton->width && (int)layer->height == baton->height)
        {
            baton->stream.write((char *)image->data, image->dataLength);
            WORKER_END();
        }

        if (!layer->decode()) {
            // Decoding failed.
            baton->message = layer->message;
            WORKER_END();
        }
        else if (layer->warnings.size()) {
            std::vector<std::string>::iterator pos = layer->warnings.begin();
            std::vector<std::string>::iterator end = layer->warnings.end();
            for (; pos != end; pos++) {
                std::ostringstream msg;
                msg << "Layer " << index << ": " << *pos;
                baton->warnings.push_back(msg.str());
            }
        }

        bool coversWidth = image->x <= 0 && visibleWidth >= baton->width;
        bool coversHeight = image->y <= 0 && visibleHeight >= baton->height;
        if (!layer->alpha && coversWidth && coversHeight && image->tint.is_alpha_identity()) {
            // Skip decoding more layers.
            alpha = false;
        }

        // Convenience aliases.
        image->width = layer->width;
        image->height = layer->height;
        image->reader = layer;
        size++;

    }

    // Now blend images.
    int pixels = baton->width * baton->height;
    if (pixels <= 0) {
        std::ostringstream msg;
        msg << "Image dimensions " << baton->width << "x" << baton->height << " are invalid";
        baton->message = msg.str();
        WORKER_END();
    }

    unsigned int *target = (unsigned int *)malloc(sizeof(unsigned int) * pixels);
    if (!target) {
        baton->message = "Memory allocation failed";
        WORKER_END();
    }

    // When we don't actually have transparent pixels, we don't need to set
    // the matte.
    if (alpha) {
        // We can't use memset here because it converts the color to a 1-byte value.
        for (int i = 0; i < pixels; i++) {
            target[i] = baton->matte;
        }
    }

    for (Images::iterator it = baton->images.begin(); it != baton->images.end(); it++) {
        BImage *image = &**it;
        if (image->reader.get()) {
            Blend_Composite(target, baton, image);
        }
    }

    mapnik::image_data_32 image(baton->width, baton->height, (unsigned int*)target);
    Blend_Encode(image, baton, alpha);
    free(target);
    target = NULL;
    WORKER_END();
}

WORKER_BEGIN(Work_AfterBlend) {
    HandleScope scope;
    BlendBaton* baton = static_cast<BlendBaton*>(req->data);

    if (!baton->message.length()) {
        Local<Array> warnings = Array::New();
        std::vector<std::string>::iterator pos = baton->warnings.begin();
        std::vector<std::string>::iterator end = baton->warnings.end();
        for (int i = 0; pos != end; pos++, i++) {
            warnings->Set(i, String::New((*pos).c_str()));
        }

        std::string result = baton->stream.str();
        Local<Value> argv[] = {
            Local<Value>::New(Null()),
#if NODE_VERSION_AT_LEAST(0, 11, 0)
            Local<Value>::New(Buffer::New((char *)result.data(), result.length())),
#else
            Local<Value>::New(Buffer::New((char *)result.data(), result.length())->handle_),
#endif
            Local<Value>::New(warnings)
        };
        TRY_CATCH_CALL(Context::GetCurrent()->Global(), baton->callback, 3, argv);
    } else {
        Local<Value> argv[] = {
            Local<Value>::New(Exception::Error(String::New(baton->message.c_str())))
        };

        assert(!baton->callback.IsEmpty());
        TRY_CATCH_CALL(Context::GetCurrent()->Global(), baton->callback, 1, argv);
    }

    delete baton;
    WORKER_END();
}

}
