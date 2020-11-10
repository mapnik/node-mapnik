#include <mapnik/image.hpp>        // for image types
#include <mapnik/image_any.hpp>    // for image_any
#include <mapnik/image_util.hpp>   // for save_to_string, guess_type, etc
#include <mapnik/image_reader.hpp> // for get_image_reader, etc

#include "mapnik_image.hpp"
#include "pixel_utils.hpp"

namespace {

struct AsyncIsSolid : Napi::AsyncWorker
{
    using Base = Napi::AsyncWorker;
    AsyncIsSolid(image_ptr const& image, Napi::Function const& callback)
        : Base(callback),
          image_(image)
    {
    }

    void Execute() override
    {
        if (image_ && image_->width() > 0 && image_->height() > 0)
        {
            solid_ = mapnik::is_solid(*image_);
        }
        else
        {
            SetError("image does not have valid dimensions");
        }
    }
    std::vector<napi_value> GetResult(Napi::Env env) override
    {
        std::vector<napi_value> result = {env.Null(), Napi::Boolean::New(env, solid_)};
        if (solid_) result.push_back(mapnik::util::apply_visitor(detail::visitor_get_pixel<mapnik::image_any>(env, 0, 0), *image_));
        return result;
    }

  private:
    bool solid_;
    image_ptr image_;
};
} // namespace
/**
 * Test if an image's pixels are all exactly the same
 * @name isSolid
 * @memberof Image
 * @instance
 * @returns {boolean} `true` means all pixels are exactly the same
 * @example
 * var img = new mapnik.Image(2,2);
 * console.log(img.isSolid()); // true
 *
 * // change a pixel
 * img.setPixel(0,0, new mapnik.Color('green'));
 * console.log(img.isSolid()); // false
 */

Napi::Value Image::isSolid(Napi::CallbackInfo const& info)
{
    if (info.Length() == 0)
    {
        return isSolidSync(info);
    }
    Napi::Env env = info.Env();
    // ensure callback is a function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction())
    {
        Napi::TypeError::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto* worker = new AsyncIsSolid(image_, callback_val.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

/**
 * Determine whether the image is solid - whether it has alpha values of greater
 * than one.
 *
 * @name isSolidSync
 * @returns {boolean} whether the image is solid
 * @instance
 * @memberof Image
 * @example
 * var img = new mapnik.Image(256, 256);
 * var view = img.view(0, 0, 256, 256);
 * console.log(view.isSolidSync()); // true
 */

Napi::Value Image::isSolidSync(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);
    if (image_ && image_->width() > 0 && image_->height() > 0)
    {
        return scope.Escape(Napi::Boolean::New(env, mapnik::is_solid(*image_)));
    }
    Napi::Error::New(env, "image does not have valid dimensions").ThrowAsJavaScriptException();
    return env.Undefined();
}
