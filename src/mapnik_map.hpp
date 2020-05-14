#pragma once

#include <napi.h>
// stl
#include <memory>

namespace mapnik { class Map; }

using map_ptr = std::shared_ptr<mapnik::Map>;

namespace detail {
struct AsyncMapLoad;
struct AsyncMapFromString;
}
class Map : public Napi::ObjectWrap<Map>
{
    friend struct detail::AsyncMapLoad;
    friend struct detail::AsyncMapFromString;
public:
    // initializer
    static Napi::Object Initialize(Napi::Env env, Napi::Object exports);
    // ctor
    explicit Map(Napi::CallbackInfo const& info);
    // methods
    Napi::Value fonts(Napi::CallbackInfo const& info);
    Napi::Value fontFiles(Napi::CallbackInfo const& info);
    Napi::Value fontDirectory(Napi::CallbackInfo const& info);
    Napi::Value loadFonts(Napi::CallbackInfo const& info);
    Napi::Value registerFonts(Napi::CallbackInfo const& info);
    Napi::Value memoryFonts(Napi::CallbackInfo const& info);
    Napi::Value loadSync(Napi::CallbackInfo const& info);
    Napi::Value load(Napi::CallbackInfo const& info);
    Napi::Value fromStringSync(Napi::CallbackInfo const& info);
    Napi::Value fromString(Napi::CallbackInfo const& info);
    Napi::Value clone(Napi::CallbackInfo const& info);

    // async rendering
    Napi::Value render(Napi::CallbackInfo const& info);
    Napi::Value renderFile(Napi::CallbackInfo const& info);
    // sync rendering
    Napi::Value renderSync(Napi::CallbackInfo const& info);
    Napi::Value renderFileSync(Napi::CallbackInfo const& info);
    // export
    Napi::Value save(Napi::CallbackInfo const& info);
    Napi::Value toXML(Napi::CallbackInfo const& info);
    Napi::Value clear(Napi::CallbackInfo const& info);
    Napi::Value resize(Napi::CallbackInfo const& info);
    Napi::Value zoomAll(Napi::CallbackInfo const& info);
    Napi::Value zoomToBox(Napi::CallbackInfo const& info);
    Napi::Value layers(Napi::CallbackInfo const& info);
    Napi::Value scale(Napi::CallbackInfo const& info);
    Napi::Value scaleDenominator(Napi::CallbackInfo const& info);
    Napi::Value queryPoint(Napi::CallbackInfo const& info);
    Napi::Value queryMapPoint(Napi::CallbackInfo const& info);
    Napi::Value add_layer(Napi::CallbackInfo const& info);
    Napi::Value remove_layer(Napi::CallbackInfo const& info);
    Napi::Value get_layer(Napi::CallbackInfo const& info);

    // accessors
    Napi::Value srs(Napi::CallbackInfo const& info);
    void srs(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value width(Napi::CallbackInfo const& info);
    void width(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value height(Napi::CallbackInfo const& info);
    void height(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value bufferSize(Napi::CallbackInfo const& info);
    void bufferSize(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value extent(Napi::CallbackInfo const& info);
    void extent(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value bufferedExtent(Napi::CallbackInfo const& info);
    Napi::Value maximumExtent(Napi::CallbackInfo const& info);
    void maximumExtent(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value background(Napi::CallbackInfo const& info);
    void background(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value parameters(Napi::CallbackInfo const& info);
    void parameters(Napi::CallbackInfo const& info, Napi::Value const& value);
    Napi::Value aspect_fix_mode(Napi::CallbackInfo const& info);
    void aspect_fix_mode(Napi::CallbackInfo const& info, Napi::Value const& value);
private:

    inline bool acquire()
    {
        if (in_use_) return false;
        in_use_ = true;
        return true;
    }

    inline void release() {in_use_ = false;}

    static Napi::FunctionReference constructor;
    map_ptr map_;
    bool in_use_;
};
