// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_color.hpp"
#include "visitor_get_pixel.hpp"
#include "utils.hpp"

// std
#include <exception>

Nan::Persistent<v8::FunctionTemplate> Image::constructor;

/**
 * **`mapnik.Image`**
 *
 * Create a new image object (surface) that can be used for rendering data to.
 * @class Image
 * @param {number} width - width in pixels
 * @param {number} height - height in pixels
 * @param {Object} [options]
 * @param {Object<mapnik.imageType>} [options.type=mapnik.imageType.rgb8] - a {@link mapnik.imageType} object
 * @param {boolean} [options.initialize=true]
 * @param {boolean} [options.premultiplied=false]
 * @param {boolean} [options.painted=false]
 * @property {number} offset - offset number
 * @property {number} scaling - scaling number
 * @throws {TypeError} if any argument is the wrong type, like if width
 * or height is not numeric.
 * @example
 * var im = new mapnik.Image(256, 256, {
 *   premultiplied: true,
 *   type: mapnik.imageType.gray8
 * });
 */
void Image::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Image::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Image").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "getType", getType);
    Nan::SetPrototypeMethod(lcons, "getPixel", getPixel);
    Nan::SetPrototypeMethod(lcons, "setPixel", setPixel);
    Nan::SetPrototypeMethod(lcons, "encodeSync", encodeSync);
    Nan::SetPrototypeMethod(lcons, "encode", encode);
    Nan::SetPrototypeMethod(lcons, "view", view);
    Nan::SetPrototypeMethod(lcons, "saveSync", saveSync);
    Nan::SetPrototypeMethod(lcons, "save", save);
    Nan::SetPrototypeMethod(lcons, "setGrayScaleToAlpha", setGrayScaleToAlpha);
    Nan::SetPrototypeMethod(lcons, "width", width);
    Nan::SetPrototypeMethod(lcons, "height", height);
    Nan::SetPrototypeMethod(lcons, "painted", painted);
    Nan::SetPrototypeMethod(lcons, "composite", composite);
    Nan::SetPrototypeMethod(lcons, "filter", filter);
    Nan::SetPrototypeMethod(lcons, "filterSync", filterSync);
    Nan::SetPrototypeMethod(lcons, "fillSync", fillSync);
    Nan::SetPrototypeMethod(lcons, "fill", fill);
    Nan::SetPrototypeMethod(lcons, "premultiplySync", premultiplySync);
    Nan::SetPrototypeMethod(lcons, "premultiply", premultiply);
    Nan::SetPrototypeMethod(lcons, "premultiplied", premultiplied);
    Nan::SetPrototypeMethod(lcons, "demultiplySync", demultiplySync);
    Nan::SetPrototypeMethod(lcons, "demultiply", demultiply);
    Nan::SetPrototypeMethod(lcons, "clear", clear);
    Nan::SetPrototypeMethod(lcons, "clearSync", clearSync);
    Nan::SetPrototypeMethod(lcons, "compare", compare);
    Nan::SetPrototypeMethod(lcons, "isSolid", isSolid);
    Nan::SetPrototypeMethod(lcons, "isSolidSync", isSolidSync);
    Nan::SetPrototypeMethod(lcons, "copy", copy);
    Nan::SetPrototypeMethod(lcons, "copySync", copySync);
    Nan::SetPrototypeMethod(lcons, "resize", resize);
    Nan::SetPrototypeMethod(lcons, "resizeSync", resizeSync);
    Nan::SetPrototypeMethod(lcons, "data", data);

    // properties
    ATTR(lcons, "scaling", get_scaling, set_scaling);
    ATTR(lcons, "offset", get_offset, set_offset);

    // This *must* go after the ATTR setting
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(),
                    "open",
                    Image::open);
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(),
                    "fromBytes",
                    Image::fromBytes);
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(),
                    "openSync",
                    Image::openSync);
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(),
                    "fromBytesSync",
                    Image::fromBytesSync);
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(),
                    "fromBufferSync",
                    Image::fromBufferSync);
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(),
                    "fromSVG",
                    Image::fromSVG);
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(),
                    "fromSVGSync",
                    Image::fromSVGSync);
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(),
                    "fromSVGBytes",
                    Image::fromSVGBytes);
    Nan::SetMethod(lcons->GetFunction().As<v8::Object>(),
                    "fromSVGBytesSync",
                    Image::fromSVGBytesSync);
    target->Set(Nan::New("Image").ToLocalChecked(),lcons->GetFunction());
    constructor.Reset(lcons);
}

Image::Image(unsigned int width, unsigned int height, mapnik::image_dtype type, bool initialized, bool premultiplied, bool painted) :
    Nan::ObjectWrap(),
    this_(std::make_shared<mapnik::image_any>(width,height,type,initialized,premultiplied,painted))
{
}

Image::Image(image_ptr _this) :
    Nan::ObjectWrap(),
    this_(_this)
{
}

Image::~Image()
{
}

NAN_METHOD(Image::New)
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
        Image* im =  static_cast<Image*>(ptr);
        im->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }

    if (info.Length() >= 2)
    {
        mapnik::image_dtype type = mapnik::image_dtype_rgba8;
        bool initialize = true;
        bool premultiplied = false;
        bool painted = false;
        if (!info[0]->IsNumber() || !info[1]->IsNumber())
        {
            Nan::ThrowTypeError("Image 'width' and 'height' must be a integers");
            return;
        }
        if (info.Length() >= 3)
        {
            if (info[2]->IsObject())
            {
                v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[2]);
                if (options->Has(Nan::New("type").ToLocalChecked()))
                {
                    v8::Local<v8::Value> init_val = options->Get(Nan::New("type").ToLocalChecked());

                    if (!init_val.IsEmpty() && init_val->IsNumber())
                    {
                        int int_val = init_val->IntegerValue();
                        if (int_val >= mapnik::image_dtype::IMAGE_DTYPE_MAX || int_val < 0)
                        {
                            Nan::ThrowTypeError("Image 'type' must be a valid image type");
                            return;
                        }
                        type = static_cast<mapnik::image_dtype>(init_val->IntegerValue());
                    }
                    else
                    {
                        Nan::ThrowTypeError("'type' option must be a valid 'mapnik.imageType'");
                        return;
                    }
                }

                if (options->Has(Nan::New("initialize").ToLocalChecked()))
                {
                    v8::Local<v8::Value> init_val = options->Get(Nan::New("initialize").ToLocalChecked());
                    if (!init_val.IsEmpty() && init_val->IsBoolean())
                    {
                        initialize = init_val->BooleanValue();
                    }
                    else
                    {
                        Nan::ThrowTypeError("initialize option must be a boolean");
                        return;
                    }
                }

                if (options->Has(Nan::New("premultiplied").ToLocalChecked()))
                {
                    v8::Local<v8::Value> pre_val = options->Get(Nan::New("premultiplied").ToLocalChecked());
                    if (!pre_val.IsEmpty() && pre_val->IsBoolean())
                    {
                        premultiplied = pre_val->BooleanValue();
                    }
                    else
                    {
                        Nan::ThrowTypeError("premultiplied option must be a boolean");
                        return;
                    }
                }

                if (options->Has(Nan::New("painted").ToLocalChecked()))
                {
                    v8::Local<v8::Value> painted_val = options->Get(Nan::New("painted").ToLocalChecked());
                    if (!painted_val.IsEmpty() && painted_val->IsBoolean())
                    {
                        painted = painted_val->BooleanValue();
                    }
                    else
                    {
                        Nan::ThrowTypeError("painted option must be a boolean");
                        return;
                    }
                }
            }
            else
            {
                Nan::ThrowTypeError("Options parameter must be an object");
                return;
            }
        }

        try {
            Image* im = new Image(info[0]->IntegerValue(),
                                  info[1]->IntegerValue(),
                                  type,
                                  initialize,
                                  premultiplied,
                                  painted);
            im->Wrap(info.This());
            info.GetReturnValue().Set(info.This());
        } catch (std::exception const& ex) {
            Nan::ThrowError(ex.what());
        }
        return;
    }
    else
    {
        Nan::ThrowError("please provide at least Image width and height");
        return;
    }
    return;
}

/**
 * Determine the image type
 *
 * @name getType
 * @instance
 * @returns {number} Number of the image type
 * @memberof Image
 * @example
 * var img = new mapnik.Image(256, 256, {
 *   type: mapnik.imageType.gray8
 * });
 * var type = img.getType();
 * var typeCheck = mapnik.imageType.gray8;
 * console.log(type, typeCheck); // 1, 1
 */
NAN_METHOD(Image::getType)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    unsigned type = im->this_->get_dtype();
    info.GetReturnValue().Set(Nan::New<v8::Number>(type));
}

/**
 * Get a specific pixel and its value
 * @name getPixel
 * @instance
 * @memberof Image
 * @param {number} x - position within image from top left
 * @param {number} y - position within image from top left
 * @param {Object} [options] the only valid option is `get_color`, which
 * should be a `boolean`. If set, the return is an Object with `rgba` values
 * instead of a pixel number.
 * @returns {number|Object} color number or object of rgba values
 * @example
 * // check for color after rendering image
 * var img = new mapnik.Image(4, 4);
 * var map = new mapnik.Map(4, 4);
 * map.background = new mapnik.Color('green');
 * map.render(img, {},function(err, img) {
 *   console.log(img.painted()); // false
 *   var pixel = img.getPixel(0,0);
 *   var values = img.getPixel(0,0, {get_color: true});
 *   console.log(pixel); // 4278222848
 *   console.log(values); // { premultiplied: false, a: 255, b: 0, g: 128, r: 0 }
 * });
 */
NAN_METHOD(Image::getPixel)
{
    int x = 0;
    int y = 0;
    bool get_color = false;
    if (info.Length() >= 3) {

        if (!info[2]->IsObject()) {
            Nan::ThrowTypeError("optional third argument must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[2]->ToObject();

        if (options->Has(Nan::New("get_color").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("get_color").ToLocalChecked());
            if (!bind_opt->IsBoolean()) {
                Nan::ThrowTypeError("optional arg 'color' must be a boolean");
                return;
            }
            get_color = bind_opt->BooleanValue();
        }

    }

    if (info.Length() >= 2) {
        if (!info[0]->IsNumber()) {
            Nan::ThrowTypeError("first arg, 'x' must be an integer");
            return;
        }
        if (!info[1]->IsNumber()) {
            Nan::ThrowTypeError("second arg, 'y' must be an integer");
            return;
        }
        x = info[0]->IntegerValue();
        y = info[1]->IntegerValue();
    } else {
        Nan::ThrowError("must supply x,y to query pixel color");
        return;
    }
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (x >= 0 && x < static_cast<int>(im->this_->width())
        && y >= 0 && y < static_cast<int>(im->this_->height()))
    {
        if (get_color)
        {
            mapnik::color val = mapnik::get_pixel<mapnik::color>(*im->this_, x, y);
            info.GetReturnValue().Set(Color::NewInstance(val));
        } else {
            visitor_get_pixel visitor(x, y);
            info.GetReturnValue().Set(mapnik::util::apply_visitor(visitor, *im->this_));
        }
    }
    return;
}

/**
 * Set a pixels value
 * @name setPixel
 * @instance
 * @memberof Image
 * @param {number} x position within image from top left
 * @param {number} y position within image from top left
 * @param {Object|number} numeric or object representation of a color, typically used with {@link mapnik.Color}
 * @example
 * var gray = new mapnik.Image(256, 256);
 * gray.setPixel(0,0,new mapnik.Color('white'));
 * var pixel = gray.getPixel(0,0,{get_color:true});
 * console.log(pixel); // { premultiplied: false, a: 255, b: 255, g: 255, r: 255 }
 */
NAN_METHOD(Image::setPixel)
{
    if (info.Length() < 3 || (!info[0]->IsNumber() && !info[1]->IsNumber())) {
        Nan::ThrowTypeError("expects three arguments: x, y, and pixel value");
        return;
    }
    int x = info[0]->IntegerValue();
    int y = info[1]->IntegerValue();
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (x < 0 || x >= static_cast<int>(im->this_->width()) || y < 0 || y >= static_cast<int>(im->this_->height()))
    {
        Nan::ThrowTypeError("invalid pixel requested");
        return;
    }
    if (info[2]->IsUint32())
    {
        std::uint32_t val = info[2]->Uint32Value();
        mapnik::set_pixel<std::uint32_t>(*im->this_,x,y,val);
    }
    else if (info[2]->IsInt32())
    {
        std::int32_t val = info[2]->Int32Value();
        mapnik::set_pixel<std::int32_t>(*im->this_,x,y,val);
    }
    else if (info[2]->IsNumber())
    {
        double val = info[2]->NumberValue();
        mapnik::set_pixel<double>(*im->this_,x,y,val);
    }
    else if (info[2]->IsObject())
    {
        v8::Local<v8::Object> obj = info[2]->ToObject();
        if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Color::constructor)->HasInstance(obj))
        {
            Nan::ThrowTypeError("A numeric or color value is expected as third arg");
        }
        else
        {
            Color * color = Nan::ObjectWrap::Unwrap<Color>(obj);
            mapnik::set_pixel(*im->this_,x,y,*(color->get()));
        }
    }
    else
    {
        Nan::ThrowTypeError("A numeric or color value is expected as third arg");
    }
    return;
}

/**
 * Compare the pixels of one image to the pixels of another. Returns the number
 * of pixels that are different. So, if the images are identical then it returns `0`.
 * And if the images share no common pixels it returns the total number of pixels
 * in an image which is equivalent to `im.width()*im.height()`.
 *
 * @name compare
 * @instance
 * @memberof Image
 * @param {mapnik.Image} image - another {@link mapnik.Image} instance to compare to
 * @param {Object} [options]
 * @param {number} [options.threshold=16] - A value that should be `0` or greater to
 * determine if the pixels match. Defaults to 16 which means that `rgba(0,0,0,0)`
 * would be considered the same as `rgba(15,15,15,0)`.
 * @param {boolean} [options.alpha=true] - `alpha` value, along with `rgb`, is considered
 * when comparing pixels
 * @returns {number} quantified visual difference between these two images in "number of
 * pixels" (i.e. `80` pixels are different);
 * @example
 * // start with the exact same images
 * var img1 = new mapnik.Image(2,2);
 * var img2 = new mapnik.Image(2,2);
 * console.log(img1.compare(img2)); // 0
 *
 * // change 1 pixel in img2
 * img2.setPixel(0,0, new mapnik.Color('green'));
 * console.log(img1.compare(img2)); // 1
 *
 * // difference in color at first pixel
 * img1.setPixel(0,0, new mapnik.Color('red'));
 * console.log(img1.compare(img2)); // 1
 *
 * // two pixels different
 * img2.setPixel(0,1, new mapnik.Color('red'));
 * console.log(img1.compare(img2)); // 2
 *
 * // all pixels different
 * img2.setPixel(1,1, new mapnik.Color('blue'));
 * img2.setPixel(1,0, new mapnik.Color('blue'));
 * console.log(img1.compare(img2)); // 4
 */
NAN_METHOD(Image::compare)
{
    if (info.Length() < 1 || !info[0]->IsObject()) {
        Nan::ThrowTypeError("first argument should be a mapnik.Image");
        return;
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Image::constructor)->HasInstance(obj)) {
        Nan::ThrowTypeError("mapnik.Image expected as first arg");
        return;
    }

    int threshold = 16;
    unsigned alpha = true;

    if (info.Length() > 1) {

        if (!info[1]->IsObject()) {
            Nan::ThrowTypeError("optional second argument must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[1]->ToObject();

        if (options->Has(Nan::New("threshold").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("threshold").ToLocalChecked());
            if (!bind_opt->IsNumber()) {
                Nan::ThrowTypeError("optional arg 'threshold' must be a number");
                return;
            }
            threshold = bind_opt->IntegerValue();
        }

        if (options->Has(Nan::New("alpha").ToLocalChecked())) {
            v8::Local<v8::Value> bind_opt = options->Get(Nan::New("alpha").ToLocalChecked());
            if (!bind_opt->IsBoolean()) {
                Nan::ThrowTypeError("optional arg 'alpha' must be a boolean");
                return;
            }
            alpha = bind_opt->BooleanValue();
        }

    }
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.This());
    Image* im2 = Nan::ObjectWrap::Unwrap<Image>(obj);
    if (im->this_->width() != im2->this_->width() ||
        im->this_->height() != im2->this_->height()) {
            Nan::ThrowTypeError("image dimensions do not match");
            return;
    }
    unsigned difference = mapnik::compare(*im->this_, *im2->this_, threshold, alpha);
    info.GetReturnValue().Set(Nan::New<v8::Integer>(difference));
}

/**
 * Convert all grayscale values to alpha values. Great for creating
 * a mask layer based on alpha values.
 *
 * @name setGrayScaleToAlpha
 * @memberof Image
 * @instance
 * @param {mapnik.Color} color
 * @example
 * var image = new mapnik.Image(2,2);
 * image.fillSync(new mapnik.Color('rgba(0,0,0,255)'));
 * console.log(image.getPixel(0,0, {get_color:true})); // { premultiplied: false, a: 255, b: 0, g: 0, r: 0 }
 *
 * image.setGrayScaleToAlpha();
 * // turns a black pixel into a completely transparent mask
 * console.log(image.getPixel(0,0, {get_color:true})); // { premultiplied: false, a: 0, b: 255, g: 255, r: 255 }
 */
NAN_METHOD(Image::setGrayScaleToAlpha)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (info.Length() == 0) {
        mapnik::set_grayscale_to_alpha(*im->this_);
    } else {
        if (!info[0]->IsObject()) {
            Nan::ThrowTypeError("optional first arg must be a mapnik.Color");
            return;
        }

        v8::Local<v8::Object> obj = info[0]->ToObject();

        if (obj->IsNull() || obj->IsUndefined() || !Nan::New(Color::constructor)->HasInstance(obj)) {
            Nan::ThrowTypeError("mapnik.Color expected as first arg");
            return;
        }

        Color * color = Nan::ObjectWrap::Unwrap<Color>(obj);
        mapnik::set_grayscale_to_alpha(*im->this_, *color->get());
    }

    return;
}

/**
 * Check if this image is painted. "Painted" refers to if it has
 * data or not. An image created with `new mapnik.Image(4,4)` defaults to
 * `false` since we loaded a new image without rendering and have no idea
 * if it was painted or not. You can run `new mapnik.Image(4, 4, {painted: true})`
 * to manually set the `painted` value.
 *
 * @name painted
 * @instance
 * @memberof Image
 * @returns {boolean} whether it is painted or not
 * @example
 * var img = new mapnik.Image(5,5);
 * console.log(img.painted()); // false
 */
NAN_METHOD(Image::painted)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Boolean>(im->this_->painted()));
}

/**
 * Get this image's width in pixels
 *
 * @name width
 * @instance
 * @memberof Image
 * @returns {number} width
 * @example
 * var img = new mapnik.Image(4,4);
 * console.log(img.width()); // 4
 */
NAN_METHOD(Image::width)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<std::int32_t>(im->this_->width())));
}

/**
 * Get this image's height in pixels
 *
 * @name height
 * @instance
 * @memberof Image
 * @returns {number} height
 * @example
 * var img = new mapnik.Image(4,4);
 * console.log(img.height()); // 4
 */
NAN_METHOD(Image::height)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<std::int32_t>(im->this_->height())));
}

/**
 * Get a constrained view of this image given x, y, width, height parameters.
 * @memberof Image
 * @instance
 * @name view
 * @param {number} x
 * @param {number} y
 * @param {number} width
 * @param {number} height
 * @returns {mapnik.Image} an image constrained to this new view
 * @example
 * var img = new mapnik.Image(10, 10);
 * // This function says "starting from the 0/0 pixel, grab 5 pixels along
 * // the x-axis and 5 along the y-axis" which gives us a quarter of the original
 * // 10x10 pixel image
 * var img2 = img.view(0, 0, 5, 5);
 * console.log(img.width(), img2.width()); // 10, 5
 */
NAN_METHOD(Image::view)
{
    if ( (info.Length() != 4) || (!info[0]->IsNumber() && !info[1]->IsNumber() && !info[2]->IsNumber() && !info[3]->IsNumber() )) {
        Nan::ThrowTypeError("requires 4 integer arguments: x, y, width, height");
        return;
    }

    // TODO parse args
    unsigned x = info[0]->IntegerValue();
    unsigned y = info[1]->IntegerValue();
    unsigned w = info[2]->IntegerValue();
    unsigned h = info[3]->IntegerValue();

    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(ImageView::NewInstance(im,x,y,w,h));
}

NAN_GETTER(Image::get_scaling)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(im->this_->get_scaling()));
}

NAN_GETTER(Image::get_offset)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    info.GetReturnValue().Set(Nan::New<v8::Number>(im->this_->get_offset()));
}

NAN_SETTER(Image::set_scaling)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        double val = value->NumberValue();
        if (val == 0.0)
        {
            Nan::ThrowError("Scaling value can not be zero");
            return;
        }
        im->this_->set_scaling(val);
    }
}

NAN_SETTER(Image::set_offset)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    if (!value->IsNumber())
    {
        Nan::ThrowError("Must provide a number");
    }
    else
    {
        double val = value->NumberValue();
        im->this_->set_offset(val);
    }
}

/**
 * Return a copy of the pixel data in this image as a buffer
 *
 * @name data
 * @instance
 * @memberof Image
 * @returns {Buffer} pixel data as a buffer
 * @example
 * var img = new mapnik.Image.open('./path/to/image.png');
 * var buffr = img.data();
 */
NAN_METHOD(Image::data)
{
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    // TODO - make this zero copy
    info.GetReturnValue().Set(Nan::CopyBuffer(reinterpret_cast<const char *>(im->this_->bytes()), im->this_->size()).ToLocalChecked());
}
