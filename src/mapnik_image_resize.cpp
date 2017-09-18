// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc
#include <mapnik/image_scaling.hpp>

#include "mapnik_image.hpp"
#include "utils.hpp"

// std
#include <exception>

typedef struct {
    uv_work_t request;
    Image* im1;
    image_ptr im2;
    mapnik::scaling_method_e scaling_method;
    std::size_t size_x;
    std::size_t size_y;
    int offset_x;
    int offset_y;
    double filter_factor;
    Nan::Persistent<v8::Function> cb;
    bool error;
    std::string error_name;
} resize_image_baton_t;

/**
 * Resize this image (makes a copy)
 *
 * @name resize
 * @instance
 * @memberof Image
 * @param {number} width - in pixels
 * @param {number} height - in pixels
 * @param {Object} [options={}]
 * @param {number} [options.offset_x=0] - offset the image horizontally in pixels
 * @param {number} [options.offset_y=0] - offset the image vertically in pixels
 * @param {mapnik.imageScaling} [options.scaling_method=mapnik.imageScaling.near] - scaling method
 * @param {number} [options.filter_factor=1.0]
 * @param {Function} callback - `function(err, result)`
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray8});
 * img.resize(8, 8, function(err, result) {
 *   if (err) throw err;
 *   // new image object as `result`
 * });
 */
NAN_METHOD(Image::resize)
{
    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction()) {
        info.GetReturnValue().Set(_resizeSync(info));
        return;
    }
    Image* im1 = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    std::size_t width = 0;
    std::size_t height = 0;
    int offset_x = 0;
    int offset_y = 0;
    double filter_factor = 1.0;
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();

    if (info.Length() >= 3)
    {
        if (info[0]->IsNumber())
        {
            auto width_tmp = info[0]->IntegerValue();
            if (width_tmp <= 0)
            {
                Nan::ThrowTypeError("Width must be a integer greater then zero");
                return;
            }
            width = static_cast<std::size_t>(width_tmp);
        }
        else
        {
            Nan::ThrowTypeError("Width must be a number");
            return;
        }
        if (info[1]->IsNumber())
        {
            auto height_tmp = info[1]->IntegerValue();
            if (height_tmp <= 0)
            {
                Nan::ThrowTypeError("Height must be a integer greater then zero");
                return;
            }
            height = static_cast<std::size_t>(height_tmp);
        }
        else
        {
            Nan::ThrowTypeError("Height must be a number");
            return;
        }
    }
    else
    {
        Nan::ThrowTypeError("resize requires a width and height parameter.");
        return;
    }
    if (info.Length() >= 4)
    {
        if (info[2]->IsObject())
        {
            options = info[2]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as third argument");
            return;
        }
    }
    if (options->Has(Nan::New("offset_x").ToLocalChecked()))
    {
        v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_x").ToLocalChecked());
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
            return;
        }
        offset_x = bind_opt->IntegerValue();
    }
    if (options->Has(Nan::New("offset_y").ToLocalChecked()))
    {
        v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_y").ToLocalChecked());
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
            return;
        }
        offset_y = bind_opt->IntegerValue();
    }
    if (options->Has(Nan::New("scaling_method").ToLocalChecked()))
    {
        v8::Local<v8::Value> scaling_val = options->Get(Nan::New("scaling_method").ToLocalChecked());
        if (scaling_val->IsNumber())
        {
            std::int64_t scaling_int = scaling_val->IntegerValue();
            if (scaling_int > mapnik::SCALING_BLACKMAN || scaling_int < 0)
            {
                Nan::ThrowTypeError("Invalid scaling_method");
                return;
            }
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_int);
        }
        else
        {
            Nan::ThrowTypeError("scaling_method argument must be an integer");
            return;
        }
    }

    if (options->Has(Nan::New("filter_factor").ToLocalChecked()))
    {
        v8::Local<v8::Value> ff_val = options->Get(Nan::New("filter_factor").ToLocalChecked());
        if (ff_val->IsNumber())
        {
            filter_factor = ff_val->NumberValue();
        }
        else
        {
            Nan::ThrowTypeError("filter_factor argument must be a number");
            return;
        }
    }
    resize_image_baton_t *closure = new resize_image_baton_t();
    closure->request.data = closure;
    closure->im1 = im1;
    closure->scaling_method = scaling_method;
    closure->size_x = width;
    closure->size_y = height;
    closure->offset_x = offset_x;
    closure->offset_y = offset_y;
    closure->filter_factor = filter_factor;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Resize, (uv_after_work_cb)EIO_AfterResize);
    closure->im1->Ref();
    return;
}

struct resize_visitor
{

    resize_visitor(mapnik::image_any const& im1,
                   mapnik::scaling_method_e scaling_method,
                   double image_ratio_x,
                   double image_ratio_y,
                   double filter_factor,
                   int offset_x,
                   int offset_y) :
        im1_(im1),
        scaling_method_(scaling_method),
        image_ratio_x_(image_ratio_x),
        image_ratio_y_(image_ratio_y),
        filter_factor_(filter_factor),
        offset_x_(offset_x),
        offset_y_(offset_y) {}

    void operator()(mapnik::image_rgba8 & im2) const
    {
        if (!im1_.get_premultiplied())
        {
            throw std::runtime_error("RGBA8 images must be premultiplied prior to using resize");
        }
        mapnik::scale_image_agg(im2,
                                mapnik::util::get<mapnik::image_rgba8>(im1_),
                                scaling_method_,
                                image_ratio_x_,
                                image_ratio_y_,
                                offset_x_,
                                offset_y_,
                                filter_factor_);
    }

    template <typename T>
    void operator()(T & im2) const
    {
        mapnik::scale_image_agg(im2,
                                mapnik::util::get<T>(im1_),
                                scaling_method_,
                                image_ratio_x_,
                                image_ratio_y_,
                                offset_x_,
                                offset_y_,
                                filter_factor_);
    }

    void operator()(mapnik::image_null &) const
    {
        // Should be caught earlier so no test coverage should reach here.
        /* LCOV_EXCL_START */
        throw std::runtime_error("Can not resize null images");
        /* LCOV_EXCL_STOP */
    }

    void operator()(mapnik::image_gray8s &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 8 bit integer rasters");
    }

    void operator()(mapnik::image_gray16s &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 16 bit integer rasters");
    }

    void operator()(mapnik::image_gray32 &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing unsigned 32 bit integer rasters");
    }

    void operator()(mapnik::image_gray32s &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 32 bit integer rasters");
    }

    void operator()(mapnik::image_gray64 &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing unsigned 64 bit integer rasters");
    }

    void operator()(mapnik::image_gray64s &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing signed 64 bit integer rasters");
    }

    void operator()(mapnik::image_gray64f &) const
    {
        throw std::runtime_error("Mapnik currently does not support resizing 64 bit floating point rasters");
    }


  private:
    mapnik::image_any const & im1_;
    mapnik::scaling_method_e scaling_method_;
    double image_ratio_x_;
    double image_ratio_y_;
    double filter_factor_;
    int offset_x_;
    int offset_y_;

};

void Image::EIO_Resize(uv_work_t* req)
{
    resize_image_baton_t *closure = static_cast<resize_image_baton_t *>(req->data);
    if (closure->im1->this_->is<mapnik::image_null>())
    {
        closure->error = true;
        closure->error_name = "Can not resize a null image.";
        return;
    }
    try
    {
        double offset = closure->im1->this_->get_offset();
        double scaling = closure->im1->this_->get_scaling();

        closure->im2 = std::make_shared<mapnik::image_any>(closure->size_x,
                                                           closure->size_y,
                                                           closure->im1->this_->get_dtype(),
                                                           true,
                                                           true,
                                                           false);
        closure->im2->set_offset(offset);
        closure->im2->set_scaling(scaling);
        int im_width = closure->im1->this_->width();
        int im_height = closure->im1->this_->height();
        if (im_width <= 0 || im_height <= 0)
        {
            closure->error = true;
            closure->error_name = "Image width or height is zero or less then zero.";
            return;
        }
        double image_ratio_x = static_cast<double>(closure->size_x) / im_width;
        double image_ratio_y = static_cast<double>(closure->size_y) / im_height;
        resize_visitor visit(*(closure->im1->this_),
                             closure->scaling_method,
                             image_ratio_x,
                             image_ratio_y,
                             closure->filter_factor,
                             closure->offset_x,
                             closure->offset_y);
        mapnik::util::apply_visitor(visit, *(closure->im2));
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterResize(uv_work_t* req)
{
    Nan::HandleScope scope;
    resize_image_baton_t *closure = static_cast<resize_image_baton_t *>(req->data);
    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    }
    else
    {
        Image* im = new Image(closure->im2);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        v8::Local<v8::Value> argv[2] = { Nan::Null(), maybe_local.ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }
    closure->im1->Unref();
    closure->cb.Reset();
    delete closure;
}

/**
 * Resize this image (makes a copy). Synchronous version of {@link mapnik.Image.resize}.
 *
 * @name resizeSync
 * @instance
 * @memberof Image
 * @param {number} width
 * @param {number} height
 * @param {Object} [options={}]
 * @param {number} [options.offset_x=0] - offset the image horizontally in pixels
 * @param {number} [options.offset_y=0] - offset the image vertically in pixels
 * @param {mapnik.imageScaling} [options.scaling_method=mapnik.imageScaling.near] - scaling method
 * @param {number} [options.filter_factor=1.0]
 * @returns {mapnik.Image} copy
 * @example
 * var img = new mapnik.Image(4, 4, {type: mapnik.imageType.gray8});
 * var img2 = img.resizeSync(8, 8);
 * // new copy as `img2`
 */
NAN_METHOD(Image::resizeSync)
{
    info.GetReturnValue().Set(_resizeSync(info));
}

v8::Local<v8::Value> Image::_resizeSync(Nan::NAN_METHOD_ARGS_TYPE info)
{
    Nan::EscapableHandleScope scope;
    Image* im = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    std::size_t width = 0;
    std::size_t height = 0;
    double filter_factor = 1.0;
    int offset_x = 0;
    int offset_y = 0;
    mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
    v8::Local<v8::Object> options = Nan::New<v8::Object>();
    if (info.Length() >= 2)
    {
        if (info[0]->IsNumber())
        {
            int width_tmp = info[0]->IntegerValue();
            if (width_tmp <= 0)
            {
                Nan::ThrowTypeError("Width parameter must be an integer greater then zero");
                return scope.Escape(Nan::Undefined());
            }
            width = static_cast<std::size_t>(width_tmp);
        }
        else
        {
            Nan::ThrowTypeError("Width must be a number");
            return scope.Escape(Nan::Undefined());
        }
        if (info[1]->IsNumber())
        {
            int height_tmp = info[1]->IntegerValue();
            if (height_tmp <= 0)
            {
                Nan::ThrowTypeError("Height parameter must be an integer greater then zero");
                return scope.Escape(Nan::Undefined());
            }
            height = static_cast<std::size_t>(height_tmp);
        }
        else
        {
            Nan::ThrowTypeError("Height must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }
    else
    {
        Nan::ThrowTypeError("Resize requires at least a width and height parameter");
        return scope.Escape(Nan::Undefined());
    }
    if (info.Length() >= 3)
    {
        if (info[2]->IsObject())
        {
            options = info[2]->ToObject();
        }
        else
        {
            Nan::ThrowTypeError("Expected options object as third argument");
            return scope.Escape(Nan::Undefined());
        }
    }
    if (options->Has(Nan::New("offset_x").ToLocalChecked()))
    {
        v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_x").ToLocalChecked());
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_x' must be a number");
            return scope.Escape(Nan::Undefined());
        }
        offset_x = bind_opt->IntegerValue();
    }
    if (options->Has(Nan::New("offset_y").ToLocalChecked()))
    {
        v8::Local<v8::Value> bind_opt = options->Get(Nan::New("offset_y").ToLocalChecked());
        if (!bind_opt->IsNumber())
        {
            Nan::ThrowTypeError("optional arg 'offset_y' must be a number");
            return scope.Escape(Nan::Undefined());
        }
        offset_y = bind_opt->IntegerValue();
    }

    if (options->Has(Nan::New("scaling_method").ToLocalChecked()))
    {
        v8::Local<v8::Value> scaling_val = options->Get(Nan::New("scaling_method").ToLocalChecked());
        if (scaling_val->IsNumber())
        {
            std::int64_t scaling_int = scaling_val->IntegerValue();
            if (scaling_int > mapnik::SCALING_BLACKMAN || scaling_int < 0)
            {
                Nan::ThrowTypeError("Invalid scaling_method");
                return scope.Escape(Nan::Undefined());
            }
            scaling_method = static_cast<mapnik::scaling_method_e>(scaling_int);
        }
        else
        {
            Nan::ThrowTypeError("scaling_method argument must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (options->Has(Nan::New("filter_factor").ToLocalChecked()))
    {
        v8::Local<v8::Value> ff_val = options->Get(Nan::New("filter_factor").ToLocalChecked());
        if (ff_val->IsNumber())
        {
            filter_factor = ff_val->NumberValue();
        }
        else
        {
            Nan::ThrowTypeError("filter_factor argument must be a number");
            return scope.Escape(Nan::Undefined());
        }
    }

    if (im->this_->is<mapnik::image_null>())
    {
        Nan::ThrowTypeError("Can not resize a null image");
        return scope.Escape(Nan::Undefined());
    }
    int im_width = im->this_->width();
    int im_height = im->this_->height();
    if (im_width <= 0 || im_height <= 0)
    {
        Nan::ThrowTypeError("Image width or height is zero or less then zero.");
        return scope.Escape(Nan::Undefined());
    }
    try
    {
        double offset = im->this_->get_offset();
        double scaling = im->this_->get_scaling();

        image_ptr imagep = std::make_shared<mapnik::image_any>(width,
                                                           height,
                                                           im->this_->get_dtype(),
                                                           true,
                                                           true,
                                                           false);
        imagep->set_offset(offset);
        imagep->set_scaling(scaling);
        double image_ratio_x = static_cast<double>(width) / im_width;
        double image_ratio_y = static_cast<double>(height) / im_height;
        resize_visitor visit(*(im->this_),
                             scaling_method,
                             image_ratio_x,
                             image_ratio_y,
                             filter_factor,
                             offset_x,
                             offset_y);
        mapnik::util::apply_visitor(visit, *imagep);
        Image* new_im = new Image(imagep);
        v8::Local<v8::Value> ext = Nan::New<v8::External>(new_im);
        Nan::MaybeLocal<v8::Object> maybe_local = Nan::NewInstance(Nan::New(constructor)->GetFunction(), 1, &ext);
        if (maybe_local.IsEmpty()) Nan::ThrowError("Could not create new Image instance");
        return scope.Escape(maybe_local.ToLocalChecked());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return scope.Escape(Nan::Undefined());
    }
}
