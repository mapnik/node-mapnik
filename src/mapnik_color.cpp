#include "mapnik_color.hpp"

Napi::FunctionReference Color::constructor;

Napi::Object Color::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Color", {
            InstanceMethod<&Color::hex>("hex", prop_attr),
            InstanceMethod<&Color::toString>("toString", prop_attr),
            InstanceAccessor<&Color::red, &Color::red>("r", prop_attr),
            InstanceAccessor<&Color::green, &Color::green>("g", prop_attr),
            InstanceAccessor<&Color::blue, &Color::blue>("b", prop_attr),
            InstanceAccessor<&Color::alpha, &Color::alpha>("a", prop_attr),
            InstanceAccessor<&Color::premultiplied, &Color::premultiplied>("premultiplied", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Color", func);
    return exports;
}

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

Color::Color(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Color>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() == 1 && info[0].IsExternal())
    {
        auto ext = info[0].As<Napi::External<mapnik::color>>();
        if (ext) color_ = *ext.Data();
        return;
    }
    else if (info.Length() == 1 && info[0].IsString())
    {
        try
        {
            color_ = mapnik::color{info[0].As<Napi::String>()};
        }
        catch (std::exception const& ex)
        {
            Napi::TypeError::New(env, ex.what()).ThrowAsJavaScriptException();
        }
    }
    else if (info.Length() == 2 && info[0].IsString() && info[1].IsBoolean())
    {
        try
        {
            color_ = {info[0].As<Napi::String>()};
            color_.set_premultiplied(info[1].As<Napi::Boolean>());
            // FIXME: operator=(rhs) is broken in mapnik
        }
        catch (std::exception const& ex)
        {
            Napi::TypeError::New(env, ex.what()).ThrowAsJavaScriptException();
        }
    }
    else if (info.Length() == 3 && info[0].IsNumber() && info[1].IsNumber() &&
             info[2].IsNumber())
    {
        int r = info[0].As<Napi::Number>().Int32Value();
        int g = info[1].As<Napi::Number>().Int32Value();
        int b = info[2].As<Napi::Number>().Int32Value();
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
        {
            Napi::TypeError::New(env, "color value out of range").ThrowAsJavaScriptException();
        }
        color_ = mapnik::color(r, g, b);
    }
    else if (info.Length() == 4 && info[0].IsNumber() &&
             info[1].IsNumber() && info[2].IsNumber() &&
             info[3].IsBoolean())
    {
        int r = info[0].As<Napi::Number>().Int32Value();
        int g = info[1].As<Napi::Number>().Int32Value();
        int b = info[2].As<Napi::Number>().Int32Value();
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
        {
            Napi::TypeError::New(env, "color value out of range").ThrowAsJavaScriptException();
        }
        color_ = mapnik::color(r, g, b, 255);
        color_.set_premultiplied(info[3].As<Napi::Boolean>());
    }
    else if (info.Length() == 4 && info[0].IsNumber() &&
             info[1].IsNumber() && info[2].IsNumber() &&
             info[3].IsNumber())
    {
        int r = info[0].As<Napi::Number>().Int32Value();
        int g = info[1].As<Napi::Number>().Int32Value();
        int b = info[2].As<Napi::Number>().Int32Value();
        int a = info[3].As<Napi::Number>().Int32Value();
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
        {
            Napi::TypeError::New(env, "color value out of range").ThrowAsJavaScriptException();
        }
        color_ = mapnik::color(r, g, b, a);
    }
    else if (info.Length() == 5 && info[0].IsNumber() &&
             info[1].IsNumber() && info[2].IsNumber() &&
             info[3].IsNumber() && info[4].IsBoolean())
    {
        int r = info[0].As<Napi::Number>().Int32Value();
        int g = info[1].As<Napi::Number>().Int32Value();
        int b = info[2].As<Napi::Number>().Int32Value();
        int a = info[3].As<Napi::Number>().Int32Value();
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
        {
            Napi::TypeError::New(env, "color value out of range").ThrowAsJavaScriptException();
        }
        color_ = mapnik::color(r, g, b, a);
        color_.set_premultiplied(info[4].As<Napi::Boolean>());
    }
    else
    {
        Napi::TypeError::New(env, "invalid arguments: colors can be created from a string, integer r,g,b values, or integer r,g,b,a values")
            .ThrowAsJavaScriptException();
    }
}

Napi::Value Color::red(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, color_.red());
}

void Color::red(Napi::CallbackInfo const& info, Napi::Value const& val)
{
    Napi::Env env = info.Env();
    if (!val.IsNumber())
    {
        Napi::TypeError::New(env, "color channel value must be an integer").ThrowAsJavaScriptException();
    }
    int r = val.As<Napi::Number>().Int32Value();
    if (r < 0 || r > 255)
    {
        Napi::TypeError::New(env, "Value out of range for color channel").ThrowAsJavaScriptException();
    }
    color_.set_red(r);
}

Napi::Value Color::green(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, color_.green());
}

void Color::green(Napi::CallbackInfo const& info, Napi::Value const& val)
{
    Napi::Env env = info.Env();
    if (!val.IsNumber())
    {
        Napi::TypeError::New(env, "color channel value must be an integer").ThrowAsJavaScriptException();
    }
    int g = val.As<Napi::Number>().Int32Value();
    if (g < 0 || g > 255)
    {
        Napi::TypeError::New(env, "Value out of range for color channel").ThrowAsJavaScriptException();
    }
    color_.set_green(g);
}

Napi::Value Color::blue(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, color_.blue());
}

void Color::blue(Napi::CallbackInfo const& info, Napi::Value const& val)
{
    Napi::Env env = info.Env();
    if (!val.IsNumber())
    {
        Napi::TypeError::New(env, "color channel value must be an integer").ThrowAsJavaScriptException();
    }
    int b = val.As<Napi::Number>().Int32Value();
    if (b < 0 || b > 255)
    {
        Napi::TypeError::New(env, "Value out of range for color channel").ThrowAsJavaScriptException();
    }
    color_.set_blue(b);
}

Napi::Value Color::alpha(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, color_.alpha());
}

void Color::alpha(Napi::CallbackInfo const& info, Napi::Value const& val)
{
    Napi::Env env = info.Env();
    if (!val.IsNumber())
    {
        Napi::TypeError::New(env, "color channel value must be an integer").ThrowAsJavaScriptException();
    }
    int a = val.As<Napi::Number>().Int32Value();
    if (a < 0 || a > 255)
    {
        Napi::TypeError::New(env, "Value out of range for color channel").ThrowAsJavaScriptException();
    }
    color_.set_alpha(a);
}

/**
 * Get whether this color is premultiplied
 *
 * @name get_premultiplied
 * @memberof Color
 * @instance
 * @returns {boolean} premultiplied
 */

Napi::Value Color::premultiplied(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, color_.get_premultiplied());
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

void Color::premultiplied(Napi::CallbackInfo const& info, Napi::Value const& val)
{
    Napi::Env env = info.Env();
    if (!val.IsBoolean())
    {
        Napi::TypeError::New(env, "Value set to premultiplied must be a boolean").ThrowAsJavaScriptException();
    }
    color_.set_premultiplied(val.As<Napi::Boolean>());
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

Napi::Value Color::toString(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::String::New(env, color_.to_string());
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

Napi::Value Color::hex(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    return Napi::String::New(env, color_.to_hex_string());
}
