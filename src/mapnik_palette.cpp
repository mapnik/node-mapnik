// node-mapnik
#include "mapnik_palette.hpp"
#include "utils.hpp"

// stl
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>

Nan::Persistent<v8::FunctionTemplate> Palette::constructor;

void Palette::Initialize(v8::Local<v8::Object> target) {
    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Palette::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Palette").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "toString", ToString);
    Nan::SetPrototypeMethod(lcons, "toBuffer", ToBuffer);

    Nan::Set(target, Nan::New("Palette").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

Palette::Palette(std::string const& palette, mapnik::rgba_palette::palette_type type) :
    Nan::ObjectWrap(),
    palette_(std::make_shared<mapnik::rgba_palette>(palette, type)) {}

Palette::~Palette() {
}

NAN_METHOD(Palette::New) {
    if (!info.IsConstructCall()) {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    std::string palette;
    mapnik::rgba_palette::palette_type type = mapnik::rgba_palette::PALETTE_RGBA;
    if (info.Length() >= 1) {
        if (node::Buffer::HasInstance(info[0])) {
            v8::Local<v8::Object> obj = info[0].As<v8::Object>();
            palette = std::string(node::Buffer::Data(obj), node::Buffer::Length(obj));
        }
    }
    if (info.Length() >= 2) {
        if (info[1]->IsString()) {
            std::string obj = TOSTR(info[1]);
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
                Nan::ThrowTypeError((std::string("unknown palette type: ") + obj).c_str());
                return;
            }
        }
    }

    if (palette.empty()) {
        Nan::ThrowTypeError("First parameter must be a palette string");
        return;
    }

    try
    {

        Palette* p = new Palette(palette, type);
        p->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
}

NAN_METHOD(Palette::ToString)
{
    palette_ptr p = Nan::ObjectWrap::Unwrap<Palette>(info.Holder())->palette_;

    const std::vector<mapnik::rgb>& colors = p->palette();
    std::size_t length = colors.size();
    #if MAPNIK_VERSION >= 300012
    const std::vector<unsigned>& alpha = p->alpha_table();
    #else
    const std::vector<unsigned>& alpha = p->alphaTable();
    #endif
    std::size_t alphaLength = alpha.size();

    std::ostringstream str("");
    str << "[Palette " << length;
    if (length == 1) str << " color";
    else str << " colors";

    str << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < length; i++) {
        str << " #";
        str << std::setw(2) << (unsigned)colors[i].r;
        str << std::setw(2) << (unsigned)colors[i].g;
        str << std::setw(2) << (unsigned)colors[i].b;
        if (i < alphaLength) str << std::setw(2) << alpha[i];
    }

    str << "]";
    info.GetReturnValue().Set(Nan::New<v8::String>(str.str()).ToLocalChecked());
}

NAN_METHOD(Palette::ToBuffer)
{
    palette_ptr p = Nan::ObjectWrap::Unwrap<Palette>(info.Holder())->palette_;

    const std::vector<mapnik::rgb>& colors = p->palette();
    std::size_t length = colors.size();
    #if MAPNIK_VERSION >= 300012
    const std::vector<unsigned>& alpha = p->alpha_table();
    #else
    const std::vector<unsigned>& alpha = p->alphaTable();
    #endif
    std::size_t alphaLength = alpha.size();

    char palette[256 * 4];
    for (std::size_t i = 0, pos = 0; i < length; i++) {
        palette[pos++] = colors[i].r;
        palette[pos++] = colors[i].g;
        palette[pos++] = colors[i].b;
        palette[pos++] = (i < alphaLength) ? alpha[i] : 0xFF;
    }
    info.GetReturnValue().Set(Nan::CopyBuffer(palette, length * 4).ToLocalChecked());
}
