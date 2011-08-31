// boost
#include <boost/make_shared.hpp>

#include "mapnik_palette.hpp"
#include "utils.hpp"

#include <node_buffer.h>

Persistent<FunctionTemplate> Palette::constructor;

void Palette::Initialize(Handle<Object> target) {
    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Palette::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Palette"));

    target->Set(String::NewSymbol("Palette"),constructor->GetFunction());
}

Palette::Palette(std::string const& palette, mapnik::rgba_palette::palette_type type) :
  ObjectWrap(),
  palette_(boost::make_shared<mapnik::rgba_palette>(palette, type)) {}

Palette::~Palette() {
}

Handle<Value> Palette::New(const Arguments& args) {
    HandleScope scope;

    if (!args.IsConstructCall()) {
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));
    }

    std::string palette;
    mapnik::rgba_palette::palette_type type = mapnik::rgba_palette::PALETTE_RGBA;
    if (args.Length() >= 1) {
        if (args[0]->IsString()) {
            String::Utf8Value obj(args[0]->ToString());
            palette = std::string(*obj, obj.length());
        }
        else if (Buffer::HasInstance(args[0])) {
            Local<Object> obj = args[0]->ToObject();
            palette = std::string(Buffer::Data(obj), Buffer::Length(obj));
        }
    }
    if (args.Length() >= 2) {
        if (args[1]->IsString()) {
            std::string obj = std::string(TOSTR(args[1]));
            if (obj == "rgb") type = mapnik::rgba_palette::PALETTE_RGB;
            else if (obj == "act") type = mapnik::rgba_palette::PALETTE_ACT;
        }
    }

    if (!palette.length()) {
        return ThrowException(Exception::TypeError(
            String::New("First parameter must be a palette string")));
    }

    try {
        Palette* p = new Palette(palette, type);
        p->Wrap(args.This());
        return args.This();
    }
    catch (const std::exception & ex) {
        return ThrowException(Exception::Error(String::New(ex.what())));
    }
}
