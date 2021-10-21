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
#include <iostream>

namespace node_mapnik {

static bool hexToUInt32Color(char const* hex, std::uint32_t& value)
{
    if (!hex) return false;
    std::size_t len_original = strlen(hex);
    // Return if the length of the string is less than six
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

    if (len == 8)
    {
        // Circular shift to get from RGBA to ARGB.
        value = (color << 24) | ((color & 0xFF00) << 8) | ((color & 0xFF0000) >> 8) | ((color & 0xFF000000) >> 24);
        return true;
    }
    else
    {
        value = 0xFF000000 | ((color & 0xFF) << 16) | (color & 0xFF00) | ((color & 0xFF0000) >> 16);
        return true;
    }
}

Napi::Value rgb2hsl(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() != 3)
    {
        Napi::TypeError::New(env, "Please pass r,g,b integer values as three arguments").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber())
    {
        Napi::TypeError::New(env, "Please pass r,g,b integer values as three arguments").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::uint32_t r = info[0].As<Napi::Number>().Int32Value();
    std::uint32_t g = info[1].As<Napi::Number>().Int32Value();
    std::uint32_t b = info[2].As<Napi::Number>().Int32Value();
    Napi::Array hsl = Napi::Array::New(env, 3);
    double h, s, l;
    rgb_to_hsl(r, g, b, h, s, l);
    hsl.Set(0u, Napi::Number::New(env, h));
    hsl.Set(1u, Napi::Number::New(env, s));
    hsl.Set(2u, Napi::Number::New(env, l));
    return scope.Escape(hsl);
}

Napi::Value hsl2rgb(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() != 3)
    {
        Napi::TypeError::New(env, "Please pass hsl fractional values as three arguments").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (!info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber())
    {
        Napi::TypeError::New(env, "Please pass hsl fractional values as three arguments").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    double h = info[0].As<Napi::Number>().DoubleValue();
    double s = info[1].As<Napi::Number>().DoubleValue();
    double l = info[2].As<Napi::Number>().DoubleValue();
    Napi::Array rgb = Napi::Array::New(env, 3);
    std::uint32_t r, g, b;
    hsl_to_rgb(h, s, l, r, g, b);
    rgb.Set(0u, Napi::Number::New(env, r));
    rgb.Set(1u, Napi::Number::New(env, g));
    rgb.Set(2u, Napi::Number::New(env, b));
    return scope.Escape(rgb);
}

static bool parseTintOps(Napi::CallbackInfo const& info, Napi::Object const& tint, Tinter& tinter)
{
    Napi::Env env = info.Env();
    Napi::Value hue = tint.Get("h");
    if (hue.IsArray())
    {
        Napi::Array val_array = hue.As<Napi::Array>();
        if (val_array.Length() != 2)
        {
            Napi::TypeError::New(env, "h array must be a pair of values").ThrowAsJavaScriptException();
            return false;
        }
        tinter.h0 = val_array.Get(0u).As<Napi::Number>().DoubleValue();
        tinter.h1 = val_array.Get(1u).As<Napi::Number>().DoubleValue();
    }
    Napi::Value sat = tint.Get("s");
    if (sat.IsArray())
    {
        Napi::Array val_array = sat.As<Napi::Array>();
        if (val_array.Length() != 2)
        {
            Napi::TypeError::New(env, "s array must be a pair of values").ThrowAsJavaScriptException();
            return false;
        }
        tinter.s0 = val_array.Get(0u).As<Napi::Number>().DoubleValue();
        tinter.s1 = val_array.Get(1u).As<Napi::Number>().DoubleValue();
    }

    Napi::Value light = tint.Get("l");
    if (light.IsArray())
    {
        Napi::Array val_array = light.As<Napi::Array>();
        if (val_array.Length() != 2)
        {
            Napi::TypeError::New(env, "l array must be a pair of values").ThrowAsJavaScriptException();
        }
        tinter.l0 = val_array.Get(0u).As<Napi::Number>().DoubleValue();
        tinter.l1 = val_array.Get(1u).As<Napi::Number>().DoubleValue();
    }

    Napi::Value alpha = tint.Get("a");
    if (alpha.IsArray())
    {
        Napi::Array val_array = alpha.As<Napi::Array>();
        if (val_array.Length() != 2)
        {
            Napi::TypeError::New(env, "a array must be a pair of values").ThrowAsJavaScriptException();
            return false;
        }
        tinter.a0 = val_array.Get(0u).As<Napi::Number>().DoubleValue();
        tinter.a1 = val_array.Get(1u).As<Napi::Number>().DoubleValue();
    }
    return true;
}

static inline void Blend_CompositePixel(std::uint32_t& target, std::uint32_t const& source)
{
    if (source <= 0x00FFFFFF)
    {
        // Top pixel is fully transparent.
        // <do nothing>
    }
    else if (source >= 0xFF000000 || target <= 0x00FFFFFF)
    {
        // Top pixel is fully opaque or bottom pixel is fully transparent.
        target = source;
    }
    else
    {
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

static inline void TintPixel(std::uint32_t& r,
                             std::uint32_t& g,
                             std::uint32_t& b,
                             Tinter const& tint)
{
    double h;
    double s;
    double l;
    rgb_to_hsl(r, g, b, h, s, l);
    double h2 = tint.h0 + (h * (tint.h1 - tint.h0));
    double s2 = tint.s0 + (s * (tint.s1 - tint.s0));
    double l2 = tint.l0 + (l * (tint.l1 - tint.l0));
    if (h2 > 1) h2 = 1;
    if (h2 < 0) h2 = 0;
    if (s2 > 1) s2 = 1;
    if (s2 < 0) s2 = 0;
    if (l2 > 1) l2 = 1;
    if (l2 < 0) l2 = 0;
    hsl_to_rgb(h2, s2, l2, r, g, b);
}

static void Blend_Composite(int width_, int height_, std::uint32_t* target, BImage* image)
{
    const std::uint32_t* source = image->im_raw_ptr->data();

    int sourceX = std::max(0, -image->x);
    int sourceY = std::max(0, -image->y);
    int sourcePos = sourceY * image->width + sourceX;

    int width = image->width - sourceX - std::max(0, image->x + image->width - width_);
    int height = image->height - sourceY - std::max(0, image->y + image->height - height_);

    int targetX = std::max(0, image->x);
    int targetY = std::max(0, image->y);
    int targetPos = targetY * width_ + targetX;
    bool tinting = !image->tint.is_identity();
    bool set_alpha = !image->tint.is_alpha_identity();
    if (tinting || set_alpha)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                std::uint32_t const& source_pixel = source[sourcePos + x];
                std::uint32_t a = (source_pixel >> 24) & 0xff;
                if (set_alpha)
                {
                    double a2 = image->tint.a0 + (a / 255.0 * (image->tint.a1 - image->tint.a0));
                    if (a2 < 0) a2 = 0;
                    a = static_cast<std::uint32_t>(std::floor((a2 * 255.0) + .5));
                    if (a > 255) a = 255;
                }
                std::uint32_t r = source_pixel & 0xff;
                std::uint32_t g = (source_pixel >> 8) & 0xff;
                std::uint32_t b = (source_pixel >> 16) & 0xff;
                if (a > 1 && tinting)
                {
                    TintPixel(r, g, b, image->tint);
                }
                std::uint32_t new_pixel = (a << 24) | (b << 16) | (g << 8) | (r);
                Blend_CompositePixel(target[targetPos + x], new_pixel);
            }
            sourcePos += image->width;
            targetPos += width_;
        }
    }
    else
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                Blend_CompositePixel(target[targetPos + x], source[sourcePos + x]);
            }
            sourcePos += image->width;
            targetPos += width_;
        }
    }
}

struct AsyncBlend;
static void Blend_Encode(AsyncBlend* worker, mapnik::image_rgba8 const& image, bool alpha);

struct AsyncBlend : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncBlend(Images const& images, int quality, int width, int height,
               palette_ptr const& palette, unsigned matte, int compression,
               AlphaMode mode, BlendFormat format, bool reencode,
               Napi::Function const& callback)
        : Base(callback),
          images_(images),
          quality_(quality),
          width_(width),
          height_(height),
          palette_(palette),
          matte_(matte),
          compression_(compression),
          mode_(mode),
          format_(format),
          reencode_(reencode)
    {
    }

    void Execute() override
    {
        bool alpha = true;
        int size = 0;
        // Iterate from the last to first image because we potentially don't have
        // to decode all images if there's an opaque one.
        Images::reverse_iterator rit = images_.rbegin();
        Images::reverse_iterator rend = images_.rend();
        for (; rit != rend; ++rit)
        {
            // If an image that is higher than the current is opaque, stop all-together.
            if (!alpha) break;
            auto image = *rit;
            if (!image) continue;

            if (image->im_obj)
            {
                unsigned layer_width = image->im_obj->width();
                unsigned layer_height = image->im_obj->height();
                if (layer_width == 0 || layer_height == 0)
                {
                    SetError("zero width/height image encountered");
                    return;
                }
                int visibleWidth = static_cast<int>(layer_width) + image->x;
                int visibleHeight = static_cast<int>(layer_height) + image->y;
                // The first image that is in the viewport sets the width/height, if not user supplied.
                if (width_ <= 0) width_ = std::max(0, visibleWidth);
                if (height_ <= 0) height_ = std::max(0, visibleHeight);

                // Skip images that are outside of the viewport.
                if (visibleWidth <= 0 || visibleHeight <= 0 || image->x >= width_ || image->y >= height_)
                {
                    // Remove this layer from the list of layers we consider blending.
                    continue;
                }
                image->width = layer_width;
                image->height = layer_height;
                image->im_raw_ptr = &image->im_obj->get<mapnik::image_rgba8>();
            }
            else
            {
                std::unique_ptr<mapnik::image_reader> image_reader;
                try
                {
                    image_reader = std::unique_ptr<mapnik::image_reader>(mapnik::get_image_reader(image->data, image->dataLength));
                }
                catch (std::exception const& ex)
                {
                    SetError(ex.what());
                    return;
                }

                if (!image_reader || !image_reader.get())
                {
                    // Not quite sure anymore how the pointer would not be returned
                    // from the reader and can't find a way to make this fail.
                    // So removing from coverage
                    // LCOV_EXCL_START
                    SetError("Unknown image format");
                    return;
                    // LCOV_EXCL_STOP
                }

                unsigned layer_width = image_reader->width();
                unsigned layer_height = image_reader->height();
                // Error out on invalid images.
                if (layer_width == 0 || layer_height == 0)
                {
                    // No idea how to create a zero height or width image
                    // so removing from coverage, because I am fairly certain
                    // it is not possible in almost every image format.
                    // LCOV_EXCL_START
                    SetError("zero width/height image encountered");
                    return;
                    // LCOV_EXCL_STOP
                }

                int visibleWidth = static_cast<int>(layer_width) + image->x;
                int visibleHeight = static_cast<int>(layer_height) + image->y;
                // The first image that is in the viewport sets the width/height, if not user supplied.
                if (width_ <= 0) width_ = std::max(0, visibleWidth);
                if (height_ <= 0) height_ = std::max(0, visibleHeight);

                // Skip images that are outside of the viewport.
                if (visibleWidth <= 0 || visibleHeight <= 0 || image->x >= width_ || image->y >= height_)
                {
                    // Remove this layer from the list of layers we consider blending.
                    continue;
                }

                bool layer_has_alpha = image_reader->has_alpha();

                // Short-circuit when we're not reencoding.
                if (size == 0 && !layer_has_alpha && !reencode_ &&
                    image->x == 0 && image->y == 0 &&
                    static_cast<int>(layer_width) == width_ && static_cast<int>(layer_height) == height_)
                {
                    output_buffer_ = std::make_unique<std::string>((char*)image->data, image->dataLength);
                    return;
                }

                // allocate image for decoded pixels
                auto im_ptr = std::make_unique<mapnik::image_rgba8>(layer_width, layer_height);
                // actually decode pixels now
                try
                {
                    image_reader->read(0, 0, *im_ptr);
                }
                catch (std::exception const&)
                {
                    SetError("Could not decode image");
                    return;
                }

                bool coversWidth = image->x <= 0 && visibleWidth >= width_;
                bool coversHeight = image->y <= 0 && visibleHeight >= height_;
                if (!layer_has_alpha && coversWidth && coversHeight && image->tint.is_alpha_identity())
                {
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
        int pixels = width_ * height_;
        if (pixels <= 0)
        {
            std::ostringstream msg;
            msg << "Image dimensions " << width_ << "x" << height_ << " are invalid";
            SetError(msg.str());
            return;
        }

        mapnik::image_rgba8 target(width_, height_);
        // When we don't actually have transparent pixels, we don't need to set the matte.
        if (alpha)
        {
            target.set(matte_);
        }
        for (auto image_ptr : images_)
        {
            if (image_ptr && image_ptr->im_raw_ptr)
            {
                Blend_Composite(width_, height_, target.data(), &*image_ptr);
            }
        }
        Blend_Encode(this, target, alpha);
    }

    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        if (output_buffer_)
        {
            std::string& str = *output_buffer_;
            auto buffer = Napi::Buffer<char>::New(
                env,
                str.empty() ? nullptr : &str[0],
                str.size(),
                [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                    if (str_ptr != nullptr)
                    {
                        Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                    }
                    delete str_ptr;
                },
                output_buffer_.release());
            Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(str.size()));
            return {env.Null(), buffer};
        }
        return Base::GetResult(env);
    }
    void SetError(std::string const& err)
    {
        Base::SetError(err);
    }

    Images images_;
    int quality_;
    int width_;
    int height_;
    palette_ptr palette_;
    unsigned matte_;
    int compression_;
    AlphaMode mode_;
    BlendFormat format_;
    bool reencode_;
    std::unique_ptr<std::string> output_buffer_;
};

static void Blend_Encode(AsyncBlend* worker, mapnik::image_rgba8 const& image, bool alpha)
{
    try
    {
        std::ostringstream stream(std::ios::out | std::ios::binary);
        if (worker->format_ == BLEND_FORMAT_JPEG)
        {
#if defined(HAVE_JPEG)
            if (worker->quality_ == 0) worker->quality_ = 85;
            mapnik::save_as_jpeg(stream, worker->quality_, image);
#else
            worker->SetError("Mapnik not built with jpeg support");
#endif
        }
        else if (worker->format_ == BLEND_FORMAT_WEBP)
        {
#if defined(HAVE_WEBP)
            if (worker->quality_ == 0) worker->quality_ = 80;
            WebPConfig config;
            // Default values set here will be lossless=0 and quality=75 (as least as of webp v0.3.1)
            if (!WebPConfigInit(&config))
            {
                // LCOV_EXCL_START
                worker->SetError("WebPConfigInit failed: version mismatch");
                // LCOV_EXCL_STOP
            }
            else
            {
                // see for more details: https://github.com/mapnik/mapnik/wiki/Image-IO#webp-output-options
                config.quality = worker->quality_;
                if (worker->compression_ > 0)
                {
                    config.method = worker->compression_;
                }
                mapnik::save_as_webp(stream, image, config, alpha);
            }
#else
            worker->SetError("Mapnik not built with webp support");
#endif
        }
        else
        {
            // Save as PNG.
#if defined(HAVE_PNG)
            mapnik::png_options opts;
            opts.compression = worker->compression_;
            if (worker->palette_ && worker->palette_->valid())
            {
                mapnik::save_as_png8_pal(stream, image, *worker->palette_, opts);
            }
            else if (worker->quality_ > 0)
            {
                opts.colors = worker->quality_;
                // Paletted PNG.
                if (alpha && worker->mode_ == BLEND_MODE_HEXTREE)
                {
                    mapnik::save_as_png8_hex(stream, image, opts);
                }
                else
                {
                    mapnik::save_as_png8_oct(stream, image, opts);
                }
            }
            else
            {
                mapnik::save_as_png(stream, image, opts);
            }
#else
            worker->SetError("Mapnik not built with png support");
#endif
        }
        worker->output_buffer_ = std::make_unique<std::string>(stream.str());
    }
    catch (const std::exception& ex)
    {
        worker->SetError(ex.what());
    }
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
Napi::Value blend(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Images images;
    int quality = 0;
    int width = 0;
    int height = 0;
    palette_ptr palette;
    unsigned matte = 0;
    int compression = -1;
    AlphaMode mode = BLEND_MODE_HEXTREE;
    BlendFormat format = BLEND_FORMAT_PNG;
    bool reencode = false;
    Napi::Function callback;

    Napi::Object options;
    if (info.Length() == 0 || !info[0].IsArray())
    {
        Napi::TypeError::New(env, "First argument must be an array of Buffers.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    else if (info.Length() == 1)
    {
        Napi::TypeError::New(env, "Second argument must be a function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    else if (info.Length() == 2)
    {
        // No options provided.
        if (!info[1].IsFunction())
        {
            Napi::TypeError::New(env, "Second argument must be a function.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        callback = info[1].As<Napi::Function>();
    }
    else if (info.Length() >= 3)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "Second argument must be a an options object.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        options = info[1].As<Napi::Object>();

        if (!info[2].IsFunction())
        {
            Napi::TypeError::New(env, "Third argument must be a function.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        callback = info[2].As<Napi::Function>();
    }

    // Validate options
    if (!options.IsEmpty())
    {
        if (options.Has("quality"))
        {
            Napi::Value quality_val = options.Get("quality");
            if (!quality_val.IsNumber())
            {
                Napi::TypeError::New(env, "quality - expected an integer value").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            quality = quality_val.As<Napi::Number>().Int32Value();
        }
        Napi::Value format_val = options.Get("format");

        if (!format_val.IsEmpty() && format_val.IsString())
        {
            std::string format_val_string = format_val.As<Napi::String>();

            if (format_val_string == "jpeg" || format_val_string == "jpg")
            {
                format = BLEND_FORMAT_JPEG;
                if (quality == 0)
                    quality = 85; // 85 is same default as mapnik core jpeg
                else if (quality < 0 || quality > 100)
                {
                    Napi::TypeError::New(env, "JPEG quality is range 0-100.").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
            else if (format_val_string == "png")
            {
                if (quality == 1 || quality > 256)
                {
                    Napi::TypeError::New(env, "PNG images must be quantized between 2 and 256 colors.").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
            else if (format_val_string == "webp")
            {
                format = BLEND_FORMAT_WEBP;
                if (quality == 0)
                    quality = 80;
                else if (quality < 0 || quality > 100)
                {
                    Napi::TypeError::New(env, "WebP quality is range 0-100.").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
            else
            {
                Napi::TypeError::New(env, "Invalid output format.").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        if (options.Has("reencode"))
        {
            reencode = options.Get("reencode").As<Napi::Boolean>();
        }
        if (options.Has("width"))
        {
            width = options.Get("width").As<Napi::Number>().Int32Value();
        }
        if (options.Has("height"))
        {
            height = options.Get("height").As<Napi::Number>().Int32Value();
        }
        if (options.Has("matte"))
        {
            Napi::Value matte_val = options.Get("matte");

            if (matte_val.IsString())
            {
                if (!hexToUInt32Color(matte_val.ToString().Utf8Value().c_str(), matte))
                {
                    Napi::TypeError::New(env, "Invalid matte provided.").ThrowAsJavaScriptException();
                    return env.Undefined();
                }

                // Make sure we're reencoding in the case of single alpha PNGs
                if (matte && !reencode)
                {
                    reencode = true;
                }
            }
        }
        if (options.Has("palette"))
        {
            Napi::Value palette_val = options.Get("palette");
            if (palette_val.IsObject())
            {
                palette = Napi::ObjectWrap<Palette>::Unwrap(palette_val.As<Napi::Object>())->palette();
            }
        }
        if (options.Has("mode"))
        {
            Napi::Value mode_val = options.Get("mode");
            if (mode_val.IsString())
            {
                std::string mode_string = mode_val.As<Napi::String>();
                if (mode_string == "octree" || mode_string == "o")
                {
                    mode = BLEND_MODE_OCTREE;
                }
                else if (mode_string == "hextree" || mode_string == "h")
                {
                    mode = BLEND_MODE_HEXTREE;
                }
            }
        }
        if (options.Has("compression"))
        {
            Napi::Value compression_val = options.Get("compression");
            if (!compression_val.IsEmpty() && compression_val.IsNumber())
            {
                compression = compression_val.As<Napi::Number>().Int32Value();
            }
            else
            {
                Napi::TypeError::New(env, "Compression option must be a number").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }

        int min_compression = Z_NO_COMPRESSION;
        int max_compression = Z_BEST_COMPRESSION;
        if (format == BLEND_FORMAT_PNG)
        {
            if (compression < 0) compression = Z_DEFAULT_COMPRESSION;
        }
        else if (format == BLEND_FORMAT_WEBP)
        {
            min_compression = 0, max_compression = 6;
            if (compression < 0) compression = -1;
        }

        if (compression > max_compression)
        {
            std::ostringstream msg;
            msg << "Compression level must be between "
                << min_compression << " and " << max_compression;
            Napi::TypeError::New(env, msg.str().c_str()).ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }

    Napi::Array js_images = info[0].As<Napi::Array>();
    std::size_t length = js_images.Length();
    if (length < 1 && !reencode)
    {
        Napi::TypeError::New(env, "First argument must contain at least one Buffer.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    else if (length == 1 && !reencode)
    {
        Napi::Value buffer = js_images.Get(0u);
        if (buffer.IsBuffer())
        {
            // Directly pass through buffer if it's the only one.
            Napi::AsyncContext context(env, __func__);
            callback.MakeCallback(Napi::Object::New(env),
                                  std::initializer_list<napi_value>{env.Null(), buffer}, context);
            return env.Undefined();
        }
    }

    if (!(length >= 1 || (width > 0 && height > 0)))
    {
        Napi::TypeError::New(env, "Without buffers, you have to specify width and height.").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (width < 0 || height < 0)
    {
        Napi::TypeError::New(env, "Image dimensions must be greater than 0.").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    for (std::size_t i = 0; i < length; ++i)
    {
        ImagePtr image = std::make_shared<BImage>();
        Napi::Value buffer = js_images.Get(i);
        if (buffer.IsBuffer())
        {
            image->buffer = Napi::Persistent(buffer.As<Napi::Buffer<char>>());
        }
        else if (buffer.IsObject())
        {
            Napi::Object obj = buffer.As<Napi::Object>();
            if (obj.InstanceOf(Image::constructor.Value()))
            {
                Image* im = Napi::ObjectWrap<Image>::Unwrap(obj);

                if (im->impl()->get_dtype() == mapnik::image_dtype_rgba8)
                {
                    image->im_obj = im->impl();
                }
                else
                {
                    Napi::TypeError::New(env, "Only mapnik.Image types that are rgba8 can be passed to blend").ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
            else
            {
                if (obj.Has("buffer"))
                {
                    buffer = obj.Get("buffer");
                    if (buffer.IsBuffer())
                    {
                        image->buffer = Napi::Persistent(buffer.As<Napi::Buffer<char>>());
                    }
                    else if (buffer.IsObject())
                    {
                        Napi::Object possible_im = buffer.As<Napi::Object>();
                        if (possible_im.InstanceOf(Image::constructor.Value()))
                        {
                            Image* im = Napi::ObjectWrap<Image>::Unwrap(possible_im);
                            if (im->impl()->get_dtype() == mapnik::image_dtype_rgba8)
                            {
                                image->im_obj = im->impl();
                            }
                            else
                            {
                                Napi::TypeError::New(env, "Only mapnik.Image types that are rgba8 can be passed to blend").ThrowAsJavaScriptException();
                                return env.Undefined();
                            }
                        }
                    }
                }
                if (obj.Has("x") && obj.Has("y"))
                {
                    image->x = obj.Get("x").As<Napi::Number>().Int32Value();
                    image->y = obj.Get("y").As<Napi::Number>().Int32Value();
                }
                Napi::Value tint_val = obj.Get("tint");
                if (tint_val.IsObject())
                {
                    Napi::Object tint = tint_val.As<Napi::Object>();
                    if (!tint.IsEmpty())
                    {
                        reencode = true;
                        std::string msg;
                        if (!parseTintOps(info, tint, image->tint))
                            return env.Undefined();
                    }
                }
            }
        }

        if (image->buffer.IsEmpty() && !image->im_obj)
        {
            Napi::TypeError::New(env, "All elements must be Buffers or RGBA Mapnik Image objects or objects with a 'buffer' property.")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        if (!image->im_obj)
        {
            image->data = buffer.As<Napi::Buffer<char>>().Data();
            image->dataLength = buffer.As<Napi::Buffer<char>>().Length();
        }
        images.push_back(image);
    }
    auto* worker = new AsyncBlend(images, quality, width, height, palette, matte, compression, mode, format, reencode, callback);
    worker->Queue();
    return env.Undefined();
}

} // namespace node_mapnik
