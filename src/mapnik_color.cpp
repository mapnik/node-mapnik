#include "mapnik_color.hpp"

#include "utils.hpp"                    // for ATTR, TOSTR

// mapnik
#include <mapnik/color.hpp>             // for color

// stl
#include <exception>                    // for exception

Nan::Persistent<v8::FunctionTemplate> Color::constructor;

/**
 * **`mapnik.Color`**
 *
 * A `mapnik.Color` object used for handling and converting colors
 *
 * @class Color
 * @param {string|number} value either an array of [r, g, b, a],
 * a color keyword, or a CSS color in rgba() form.
 * @param {number} r - red value between `0` and `255`
 * @param {number} g - green value between `0` and `255`
 * @param {number} b - blue value between `0` and `255`
 * @param {boolean} pre - premultiplied, either `true` or `false`
 * @throws {TypeError} if a `rgb` component is outside of the 0-255 range
 * @example
 * var c = new mapnik.Color('green');
 * var c = new mapnik.Color(0, 128, 0, 255);
 * // premultiplied
 * var c = new mapnik.Color(0, 128, 0, 255, true);
 */
void Color::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Color::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Color").ToLocalChecked());

    // methods
    Nan::SetPrototypeMethod(lcons, "hex", hex);
    Nan::SetPrototypeMethod(lcons, "toString", toString);

    // properties
    ATTR(lcons, "r", get_prop, set_prop);
    ATTR(lcons, "g", get_prop, set_prop);
    ATTR(lcons, "b", get_prop, set_prop);
    ATTR(lcons, "a", get_prop, set_prop);
    ATTR(lcons, "premultiplied", get_premultiplied, set_premultiplied);

    Nan::Set(target, Nan::New("Color").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
    constructor.Reset(lcons);
}

Color::Color() :
    Nan::ObjectWrap(),
    this_() {}

Color::~Color()
{
}

NAN_METHOD(Color::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info[0]->IsExternal())
    {
        v8::Local<v8::External> ext = info[0].As<v8::External>();
        void* ptr = ext->Value();
        Color* c = static_cast<Color*>(ptr);
        c->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }

    color_ptr c_p;
    try
    {
        if (info.Length() == 1 &&
            info[0]->IsString())
        {
            c_p = std::make_shared<mapnik::color>(TOSTR(info[0]));
        }
        else if (info.Length() == 2 &&
                 info[0]->IsString() &&
                 info[1]->IsBoolean())
        {
            c_p = std::make_shared<mapnik::color>(TOSTR(info[0]),Nan::To<bool>(info[1]).FromJust());
        }
        else if (info.Length() == 3 &&
                 info[0]->IsNumber() &&
                 info[1]->IsNumber() &&
                 info[2]->IsNumber())
        {
            int r = Nan::To<int>(info[0]).FromJust();
            int g = Nan::To<int>(info[1]).FromJust();
            int b = Nan::To<int>(info[2]).FromJust();
            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
            {
                Nan::ThrowTypeError("color value out of range");
                return;
            }
            c_p = std::make_shared<mapnik::color>(r,g,b);
        }
        else if (info.Length() == 4 &&
                 info[0]->IsNumber() &&
                 info[1]->IsNumber() &&
                 info[2]->IsNumber() &&
                 info[3]->IsBoolean())
        {
            int r = Nan::To<int>(info[0]).FromJust();
            int g = Nan::To<int>(info[1]).FromJust();
            int b = Nan::To<int>(info[2]).FromJust();
            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
            {
                Nan::ThrowTypeError("color value out of range");
                return;
            }
            c_p = std::make_shared<mapnik::color>(r,g,b,255,Nan::To<bool>(info[3]).FromJust());
        }
        else if (info.Length() == 4 &&
                 info[0]->IsNumber() &&
                 info[1]->IsNumber() &&
                 info[2]->IsNumber() &&
                 info[3]->IsNumber())
        {
            int r = Nan::To<int>(info[0]).FromJust();
            int g = Nan::To<int>(info[1]).FromJust();
            int b = Nan::To<int>(info[2]).FromJust();
            int a = Nan::To<int>(info[3]).FromJust();
            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
            {
                Nan::ThrowTypeError("color value out of range");
                return;
            }
            c_p = std::make_shared<mapnik::color>(r,g,b,a);
        }
        else if (info.Length() == 5 &&
                 info[0]->IsNumber() &&
                 info[1]->IsNumber() &&
                 info[2]->IsNumber() &&
                 info[3]->IsNumber() &&
                 info[4]->IsBoolean())
        {
            int r = Nan::To<int>(info[0]).FromJust();
            int g = Nan::To<int>(info[1]).FromJust();
            int b = Nan::To<int>(info[2]).FromJust();
            int a = Nan::To<int>(info[3]).FromJust();
            if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
            {
                Nan::ThrowTypeError("color value out of range");
                return;
            }
            c_p = std::make_shared<mapnik::color>(r,g,b,a,Nan::To<bool>(info[4]).FromJust());
        }
        else
        {
            Nan::ThrowTypeError("invalid arguments: colors can be created from a string, integer r,g,b values, or integer r,g,b,a values");
            return;
        }
        // todo allow int,int,int and int,int,int,int contructor

    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }

    Color* c = new Color();
    c->Wrap(info.This());
    c->this_ = c_p;
    info.GetReturnValue().Set(info.This());
}

v8::Local<v8::Value> Color::NewInstance(mapnik::color const& color) {
    Nan::EscapableHandleScope scope;
    Color* c = new Color();
    c->this_ = std::make_shared<mapnik::color>(color);
    v8::Local<v8::Value> ext = Nan::New<v8::External>(c);
    Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::GetFunction(Nan::New(constructor)).ToLocalChecked(), 1, &ext);
    if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Color instance");
    return scope.Escape(maybe_local.ToLocalChecked());
}

NAN_GETTER(Color::get_prop)
{
    Color* c = Nan::ObjectWrap::Unwrap<Color>(info.Holder());
    std::string a = TOSTR(property);
    if (a == "a")
        info.GetReturnValue().Set(Nan::New<v8::Integer>(c->get()->alpha()));
    else if (a == "r")
        info.GetReturnValue().Set(Nan::New<v8::Integer>(c->get()->red()));
    else if (a == "g")
        info.GetReturnValue().Set(Nan::New<v8::Integer>(c->get()->green()));
    else //if (a == "b")
        info.GetReturnValue().Set(Nan::New<v8::Integer>(c->get()->blue()));
}

NAN_SETTER(Color::set_prop)
{
    Color* c = Nan::ObjectWrap::Unwrap<Color>(info.Holder());
    std::string a = TOSTR(property);
    if (!value->IsNumber())
    {
        Nan::ThrowTypeError("color channel value must be an integer");
        return;
    }
    int val = Nan::To<int>(value).FromJust();
    if (val < 0 || val > 255)
    {
        Nan::ThrowTypeError("Value out of range for color channel");
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


/**
 * Get whether this color is premultiplied
 *
 * @name get_premultiplied
 * @memberof Color
 * @instance
 * @returns {boolean} premultiplied
 */
NAN_GETTER(Color::get_premultiplied)
{
    Color* c = Nan::ObjectWrap::Unwrap<Color>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(c->get()->get_premultiplied()));
    return;
}

/**
 * Set whether this color should be premultiplied
 *
 * @name set_premultiplied
 * @memberof Color
 * @instance
 * @param {boolean} premultiplied
 * @example
 * var c = new mapnik.Color('green');
 * c.set_premultiplied(true);
 * @throws {TypeError} given a non-boolean argument
 */
NAN_SETTER(Color::set_premultiplied)
{
    Color* c = Nan::ObjectWrap::Unwrap<Color>(info.Holder());
    if (!value->IsBoolean())
    {
        Nan::ThrowTypeError("Value set to premultiplied must be a boolean");
        return;
    }
    c->get()->set_premultiplied(Nan::To<bool>(value).FromJust());
}

/**
 * Get this color's representation as a string
 *
 * @name toString
 * @memberof Color
 * @instance
 * @returns {string} color as a string
 * @example
 * var green = new mapnik.Color('green');
 * green.toString()
 * // 'rgb(0,128,0)'
 */
NAN_METHOD(Color::toString)
{
    Color* c = Nan::ObjectWrap::Unwrap<Color>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::String>(c->get()->to_string()).ToLocalChecked());
}

/**
 * Get this color represented as a hexademical string
 *
 * @name hex
 * @memberof Color
 * @instance
 * @returns {string} hex representation
 * @example
 * var c = new mapnik.Color('green');
 * c.hex();
 * // '#008000'
 */
NAN_METHOD(Color::hex)
{
    Color* c = Nan::ObjectWrap::Unwrap<Color>(info.Holder());
    std::string hex = c->get()->to_hex_string();
    info.GetReturnValue().Set(Nan::New<v8::String>(hex).ToLocalChecked());
}
