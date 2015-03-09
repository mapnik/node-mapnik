#include "mapnik_color.hpp"

#include "utils.hpp"                    // for ATTR, TOSTR

// mapnik
#include <mapnik/color.hpp>             // for color

// stl
#include <exception>                    // for exception

Persistent<FunctionTemplate> Color::constructor;

void Color::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(Color::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("Color"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(lcons, "hex", hex);
    NODE_SET_PROTOTYPE_METHOD(lcons, "toString", toString);

    // properties
    ATTR(lcons, "r", get_prop, set_prop);
    ATTR(lcons, "g", get_prop, set_prop);
    ATTR(lcons, "b", get_prop, set_prop);
    ATTR(lcons, "a", get_prop, set_prop);
    ATTR(lcons, "premultiplied", get_premultiplied, set_premultiplied);

    target->Set(NanNew("Color"), lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

Color::Color() :
    node::ObjectWrap(),
    this_() {}

Color::~Color()
{
}

NAN_METHOD(Color::New)
{
    NanScope();
    if (!args.IsConstructCall())
    {
        NanThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        NanReturnUndefined();
    }

    if (args[0]->IsExternal())
    {
        Local<External> ext = args[0].As<External>();
        void* ptr = ext->Value();
        Color* c = static_cast<Color*>(ptr);
        c->Wrap(args.This());
        NanReturnValue(args.This());
    }

    color_ptr c_p;
    try
    {
        if (args.Length() == 1 && 
            args[0]->IsString())
        {
            c_p = std::make_shared<mapnik::color>(TOSTR(args[0]));
        }
        else if (args.Length() == 2 && 
                 args[0]->IsString() &&
                 args[1]->IsBoolean())
        {
            c_p = std::make_shared<mapnik::color>(TOSTR(args[0]),args[1]->BooleanValue());
        }
        else if (args.Length() == 3 &&
                 args[0]->IsNumber() &&
                 args[1]->IsNumber() &&
                 args[2]->IsNumber()) 
        {
            int r = args[0]->IntegerValue();
            int g = args[1]->IntegerValue();
            int b = args[2]->IntegerValue();
            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
            {
                NanThrowTypeError("color value out of range");
                NanReturnUndefined();
            }
            c_p = std::make_shared<mapnik::color>(r,g,b);
        } 
        else if (args.Length() == 4 &&
                 args[0]->IsNumber() &&
                 args[1]->IsNumber() &&
                 args[2]->IsNumber() &&
                 args[3]->IsBoolean()) 
        {
            int r = args[0]->IntegerValue();
            int g = args[1]->IntegerValue();
            int b = args[2]->IntegerValue();
            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
            {
                NanThrowTypeError("color value out of range");
                NanReturnUndefined();
            }
            c_p = std::make_shared<mapnik::color>(r,g,b,255,args[3]->BooleanValue());
        } 
        else if (args.Length() == 4 &&
                 args[0]->IsNumber() &&
                 args[1]->IsNumber() &&
                 args[2]->IsNumber() &&
                 args[3]->IsNumber()) 
        {
            int r = args[0]->IntegerValue();
            int g = args[1]->IntegerValue();
            int b = args[2]->IntegerValue();
            int a = args[3]->IntegerValue();
            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
            {
                NanThrowTypeError("color value out of range");
                NanReturnUndefined();
            }
            c_p = std::make_shared<mapnik::color>(r,g,b,a);
        } 
        else if (args.Length() == 5 &&
                 args[0]->IsNumber() &&
                 args[1]->IsNumber() &&
                 args[2]->IsNumber() &&
                 args[3]->IsNumber() &&
                 args[4]->IsBoolean()) 
        {
            int r = args[0]->IntegerValue();
            int g = args[1]->IntegerValue();
            int b = args[2]->IntegerValue();
            int a = args[3]->IntegerValue();
            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
            {
                NanThrowTypeError("color value out of range");
                NanReturnUndefined();
            }
            c_p = std::make_shared<mapnik::color>(r,g,b,a,args[4]->BooleanValue());
        } 
        else 
        {
            NanThrowTypeError("invalid arguments: colors can be created from a string, integer r,g,b values, or integer r,g,b,a values");
            NanReturnUndefined();
        }
        // todo allow int,int,int and int,int,int,int contructor

    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }

    Color* c = new Color();
    c->Wrap(args.This());
    c->this_ = c_p;
    NanReturnValue(args.This());
}

Handle<Value> Color::NewInstance(mapnik::color const& color) {
    NanEscapableScope();
    Color* c = new Color();
    c->this_ = std::make_shared<mapnik::color>(color);
    Handle<Value> ext = NanNew<External>(c);
    Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
    return NanEscapeScope(obj);
}


NAN_GETTER(Color::get_prop)
{
    NanScope();
    Color* c = node::ObjectWrap::Unwrap<Color>(args.Holder());
    std::string a = TOSTR(property);
    if (a == "a")
        NanReturnValue(NanNew<Integer>(c->get()->alpha()));
    else if (a == "r")
        NanReturnValue(NanNew<Integer>(c->get()->red()));
    else if (a == "g")
        NanReturnValue(NanNew<Integer>(c->get()->green()));
    else //if (a == "b")
        NanReturnValue(NanNew<Integer>(c->get()->blue()));
}

NAN_SETTER(Color::set_prop)
{
    NanScope();
    Color* c = node::ObjectWrap::Unwrap<Color>(args.Holder());
    std::string a = TOSTR(property);
    if (!value->IsNumber())
    {
        NanThrowTypeError("color channel value must be an integer");
        return;
    }
    int val = value->IntegerValue();
    if (val < 0 || val > 255)
    {
        NanThrowTypeError("Value out of range for color channel");
        return;
    }
    if (a == "a") {
        c->get()->set_alpha(val);
    } else if (a == "r") {
        c->get()->set_red(val);
    } else if (a == "g") {
        c->get()->set_green(val);
    } else if (a == "b") {
        c->get()->set_blue(val);
    }
}

NAN_GETTER(Color::get_premultiplied)
{
    NanScope();
    Color* c = node::ObjectWrap::Unwrap<Color>(args.Holder());
    NanReturnValue(NanNew<Boolean>(c->get()->get_premultiplied()));
    NanReturnUndefined();
}

NAN_SETTER(Color::set_premultiplied)
{
    NanScope();
    Color* c = node::ObjectWrap::Unwrap<Color>(args.Holder());
    if (!value->IsBoolean())
    {
        NanThrowTypeError("Value set to premultiplied must be a boolean");
        return;
    }
    c->get()->set_premultiplied(value->BooleanValue());
}

NAN_METHOD(Color::toString)
{
    NanScope();

    Color* c = node::ObjectWrap::Unwrap<Color>(args.Holder());
    NanReturnValue(NanNew(c->get()->to_string().c_str()));
}


NAN_METHOD(Color::hex)
{
    NanScope();

    Color* c = node::ObjectWrap::Unwrap<Color>(args.Holder());
    std::string hex = c->get()->to_hex_string();
    NanReturnValue(NanNew(hex.c_str()));
}
