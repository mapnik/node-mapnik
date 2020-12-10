#include "mapnik_palette.hpp"
#include <mapnik/version.hpp>
// stl
#include <vector>
#include <iomanip>
#include <sstream>

Napi::FunctionReference Palette::constructor;

Napi::Object Palette::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Palette", {
            InstanceMethod<&Palette::toBuffer>("toBuffer", prop_attr),
            InstanceMethod<&Palette::toString>("toString", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Palette", func);
    return exports;
}

Palette::Palette(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Palette>(info)
{
    Napi::Env env = info.Env();
    std::string palette_str;
    mapnik::rgba_palette::palette_type type = mapnik::rgba_palette::PALETTE_RGBA;
    if (info.Length() >= 1 && info[0].IsBuffer())
    {
        Napi::Object obj = info[0].As<Napi::Object>();
        palette_str = std::string(obj.As<Napi::Buffer<char>>().Data(), obj.As<Napi::Buffer<char>>().Length());
    }

    if (info.Length() >= 2 && info[1].IsString())
    {
        std::string obj = info[1].As<Napi::String>();
        if (obj == "rgba")
        {
            type = mapnik::rgba_palette::PALETTE_RGBA;
        }
        else if (obj == "rgb")
        {
            type = mapnik::rgba_palette::PALETTE_RGB;
        }
        else if (obj == "act")
        {
            type = mapnik::rgba_palette::PALETTE_ACT;
        }
        else
        {
            Napi::TypeError::New(env, (std::string("unknown palette type: ") + obj)).ThrowAsJavaScriptException();
            return;
        }
    }

    if (palette_str.empty())
    {
        Napi::TypeError::New(env, "First parameter must be a palette string").ThrowAsJavaScriptException();
        return;
    }

    try
    {
        palette_ = std::make_shared<mapnik::rgba_palette>(palette_str, type);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
}

Napi::Value Palette::toString(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    std::vector<mapnik::rgb> const& colors = palette_->palette();
    std::size_t length = colors.size();
#if MAPNIK_VERSION >= 300012
    std::vector<unsigned> const& alpha = palette_->alpha_table();
#else
    std::vector<unsigned> const& alpha = palette_->alphaTable();
#endif
    std::size_t alphaLength = alpha.size();

    std::ostringstream ss("");
    ss << "[Palette " << length;
    if (length == 1)
        ss << " color";
    else
        ss << " colors";

    ss << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < length; ++i)
    {
        ss << " #";
        ss << std::setw(2) << static_cast<std::uint32_t>(colors[i].r);
        ss << std::setw(2) << static_cast<std::uint32_t>(colors[i].g);
        ss << std::setw(2) << static_cast<std::uint32_t>(colors[i].b);
        if (i < alphaLength) ss << std::setw(2) << alpha[i];
    }
    ss << "]";
    return Napi::String::New(env, ss.str());
}

Napi::Value Palette::toBuffer(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    std::vector<mapnik::rgb> const& colors = palette_->palette();
    std::size_t length = colors.size();
#if MAPNIK_VERSION >= 300012
    std::vector<unsigned> const& alpha = palette_->alpha_table();
#else
    std::vector<unsigned> const& alpha = palette_->alphaTable();
#endif
    std::size_t alphaLength = alpha.size();
    char data[256 * 4];
    for (std::size_t i = 0, pos = 0; i < length; ++i)
    {
        data[pos++] = colors[i].r;
        data[pos++] = colors[i].g;
        data[pos++] = colors[i].b;
        data[pos++] = (i < alphaLength) ? alpha[i] : 0xFF;
    }
    return Napi::Buffer<char>::Copy(env, data, length * 4);
}
