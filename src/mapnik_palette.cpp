// node-mapnik
#include "mapnik_palette.hpp"
#include "utils.hpp"

// stl
#include <vector>
#include <iomanip>
#include <sstream>

Persistent<FunctionTemplate> Palette::constructor;

void Palette::Initialize(Handle<Object> target) {
    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Palette::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Palette"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "toString", ToString);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toBuffer", ToBuffer);

    target->Set(NanNew("Palette"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Palette::Palette(std::string const& palette, mapnik::rgba_palette::palette_type type) :
    node::ObjectWrap(),
    palette_(std::make_shared<mapnik::rgba_palette>(palette, type)) {}

Palette::~Palette() {
}

NAN_METHOD(Palette::New) {
    NanScope();

    if (!args.IsConstructCall()) {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    std::string palette;
    mapnik::rgba_palette::palette_type type = mapnik::rgba_palette::PALETTE_RGBA;
    if (args.Length() >= 1) {
        if (args[0]->IsString()) {
            NanAsciiString nan_string(args[0]);
            palette = std::string(*nan_string,nan_string.length());
        }
        else if (node::Buffer::HasInstance(args[0])) {
            Local<Object> obj = args[0].As<Object>();
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
        NanThrowTypeError("First parameter must be a palette string");
        NanReturnUndefined();
    }

    try
    {

        Palette* p = new Palette(palette, type);
        p->Wrap(args.This());
        NanReturnValue(args.This());
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
}

NAN_METHOD(Palette::ToString)
{
    NanScope();
    palette_ptr p = node::ObjectWrap::Unwrap<Palette>(args.Holder())->palette_;

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
    NanReturnValue(NanNew(str.str().c_str()));
}

NAN_METHOD(Palette::ToBuffer)
{
    NanScope();

    palette_ptr p = node::ObjectWrap::Unwrap<Palette>(args.Holder())->palette_;

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
    NanReturnValue(NanNewBufferHandle(palette, length * 4));
}
