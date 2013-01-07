// boost
#include <boost/make_shared.hpp>

// node-mapnik
#include "mapnik_palette.hpp"
#include "utils.hpp"

// node
#include <node_buffer.h>

// stl
#include <vector>
#include <iostream>
#include <iomanip>

Persistent<FunctionTemplate> Palette::constructor;

void Palette::Initialize(Handle<Object> target) {
    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Palette::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Palette"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "toString", ToString);
    NODE_SET_PROTOTYPE_METHOD(constructor, "toBuffer", ToBuffer);

    target->Set(String::NewSymbol("Palette"), constructor->GetFunction());
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
            String::AsciiValue obj(args[0]->ToString());
            palette = std::string(*obj, obj.length());
        }
        else if (node::Buffer::HasInstance(args[0])) {
            Local<Object> obj = args[0]->ToObject();
            palette = std::string(node::Buffer::Data(obj), node::Buffer::Length(obj));
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
    catch (std::exception const& ex) {
        return ThrowException(Exception::Error(String::New(ex.what())));
    }
}

Handle<Value> Palette::ToString(const Arguments& args)
{
    HandleScope scope;
    palette_ptr p = ObjectWrap::Unwrap<Palette>(args.This())->palette_;

    const std::vector<mapnik::rgb>& colors = p->palette();
    unsigned length = colors.size();
    const std::vector<unsigned>& alpha = p->alphaTable();
    unsigned alphaLength = alpha.size();

    std::ostringstream str("");
    str << "[Palette " << length;
    if (length == 1) str << " color";
    else str << " colors";

    str << std::hex << std::setfill('0');

    for (unsigned i = 0; i < length; i++) {
        str << " #";
        str << std::setw(2) << (unsigned)colors[i].r;
        str << std::setw(2) << (unsigned)colors[i].g;
        str << std::setw(2) << (unsigned)colors[i].b;
        if (i < alphaLength) str << std::setw(2) << alpha[i];
    }

    str << "]";
    return scope.Close(String::New(str.str().c_str()));
}

Handle<Value> Palette::ToBuffer(const Arguments& args)
{
    HandleScope scope;

    palette_ptr p = ObjectWrap::Unwrap<Palette>(args.This())->palette_;

    const std::vector<mapnik::rgb>& colors = p->palette();
    unsigned length = colors.size();
    const std::vector<unsigned>& alpha = p->alphaTable();
    unsigned alphaLength = alpha.size();

    char palette[256 * 4];
    for (unsigned i = 0, pos = 0; i < length; i++) {
        palette[pos++] = colors[i].r;
        palette[pos++] = colors[i].g;
        palette[pos++] = colors[i].b;
        palette[pos++] = (i < alphaLength) ? alpha[i] : 0xFF;
    }

    node::Buffer *buffer = node::Buffer::New(palette, length * 4);
    return scope.Close(buffer->handle_);
}
