// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image.hpp>             // for image types
#include <mapnik/image_any.hpp>         // for image_any
#include <mapnik/image_util.hpp>        // for save_to_string, guess_type, etc

#include <mapnik/image_compositing.hpp>
#include <mapnik/image_filter.hpp>      // filter_visitor

#include "mapnik_image.hpp"
#include "utils.hpp"

// std
#include <exception>

typedef struct {
    uv_work_t request;
    Image* im1;
    Image* im2;
    mapnik::composite_mode_e mode;
    int dx;
    int dy;
    float opacity;
    std::vector<mapnik::filter::filter_type> filters;
    bool error;
    std::string error_name;
    Nan::Persistent<v8::Function> cb;
} composite_image_baton_t;

/**
 * Overlay this image with another image, creating a layered composite as
 * a new image
 *
 * @name composite
 * @param {mapnik.Image} image - image to composite with
 * @param {Object} [options]
 * @param {mapnik.compositeOp} [options.comp_op] - compositing operation. Must be an integer
 * value that relates to a compositing operation.
 * @param {number} [options.opacity] - opacity must be a floating point number between 0-1
 * @param {number} [options.dx]
 * @param {number} [options.dy]
 * @param {string} [options.image_filters] - a string of filter names
 * @param {Function} callback
 * @instance
 * @memberof Image
 * @example
 * var img1 = new mapnik.Image.open('./path/to/image.png');
 * var img2 = new mapnik.Image.open('./path/to/another-image.png');
 * img1.composite(img2, {
 *   comp_op: mapnik.compositeOp['multiply'],
 *   dx: 0,
 *   dy: 0,
 *   opacity: 0.5,
 *   image_filters: 'invert agg-stack-blur(10,10)'
 * }, function(err, result) {
 *   if (err) throw err;
 *   // new image with `result`
 * });
 */
NAN_METHOD(Image::composite)
{
    if (info.Length() < 1){
        Nan::ThrowTypeError("requires at least one argument: an image mask");
        return;
    }

    if (!info[0]->IsObject()) {
        Nan::ThrowTypeError("first argument must be an image mask");
        return;
    }

    v8::Local<v8::Object> im2 = info[0].As<v8::Object>();
    if (im2->IsNull() || im2->IsUndefined() || !Nan::New(Image::constructor)->HasInstance(im2))
    {
        Nan::ThrowTypeError("mapnik.Image expected as first arg");
        return;
    }

    // ensure callback is a function
    v8::Local<v8::Value> callback = info[info.Length() - 1];
    if (!info[info.Length()-1]->IsFunction())
    {
        Nan::ThrowTypeError("last argument must be a callback function");
        return;
    }

    Image * dest_image = Nan::ObjectWrap::Unwrap<Image>(info.Holder());
    Image * source_image = Nan::ObjectWrap::Unwrap<Image>(im2);

    if (!dest_image->this_->get_premultiplied())
    {
        Nan::ThrowTypeError("destination image must be premultiplied");
        return;
    }

    if (!source_image->this_->get_premultiplied())
    {
        Nan::ThrowTypeError("source image must be premultiplied");
        return;
    }

    mapnik::composite_mode_e mode = mapnik::src_over;
    float opacity = 1.0;
    std::vector<mapnik::filter::filter_type> filters;
    int dx = 0;
    int dy = 0;
    if (info.Length() >= 2) {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("optional second arg must be an options object");
            return;
        }

        v8::Local<v8::Object> options = info[1].As<v8::Object>();

        if (options->Has(Nan::New("comp_op").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("comp_op").ToLocalChecked());
            if (!opt->IsNumber())
            {
                Nan::ThrowTypeError("comp_op must be a mapnik.compositeOp value");
                return;
            }
            int mode_int = opt->IntegerValue();
            if (mode_int > static_cast<int>(mapnik::composite_mode_e::divide) || mode_int < 0)
            {
                Nan::ThrowTypeError("Invalid comp_op value");
                return;
            }
            mode = static_cast<mapnik::composite_mode_e>(mode_int);
        }

        if (options->Has(Nan::New("opacity").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("opacity").ToLocalChecked());
            if (!opt->IsNumber()) {
                Nan::ThrowTypeError("opacity must be a floating point number");
                return;
            }
            opacity = opt->NumberValue();
            if (opacity < 0 || opacity > 1) {
                Nan::ThrowTypeError("opacity must be a floating point number between 0-1");
                return;
            }
        }

        if (options->Has(Nan::New("dx").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("dx").ToLocalChecked());
            if (!opt->IsNumber()) {
                Nan::ThrowTypeError("dx must be an integer");
                return;
            }
            dx = opt->IntegerValue();
        }

        if (options->Has(Nan::New("dy").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("dy").ToLocalChecked());
            if (!opt->IsNumber()) {
                Nan::ThrowTypeError("dy must be an integer");
                return;
            }
            dy = opt->IntegerValue();
        }

        if (options->Has(Nan::New("image_filters").ToLocalChecked()))
        {
            v8::Local<v8::Value> opt = options->Get(Nan::New("image_filters").ToLocalChecked());
            if (!opt->IsString()) {
                Nan::ThrowTypeError("image_filters argument must string of filter names");
                return;
            }
            std::string filter_str = TOSTR(opt);
            bool result = mapnik::filter::parse_image_filters(filter_str, filters);
            if (!result)
            {
                Nan::ThrowTypeError("could not parse image_filters");
                return;
            }
        }
    }

    composite_image_baton_t *closure = new composite_image_baton_t();
    closure->request.data = closure;
    closure->im1 = dest_image;
    closure->im2 = source_image;
    closure->mode = mode;
    closure->opacity = opacity;
    closure->filters = std::move(filters);
    closure->dx = dx;
    closure->dy = dy;
    closure->error = false;
    closure->cb.Reset(callback.As<v8::Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Composite, (uv_after_work_cb)EIO_AfterComposite);
    closure->im1->Ref();
    closure->im2->Ref();
    return;
}

void Image::EIO_Composite(uv_work_t* req)
{
    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    try
    {
        if (closure->filters.size() > 0)
        {
            // TODO: expose passing custom scale_factor: https://github.com/mapnik/mapnik/commit/b830469d2d574ac575ff24713935378894f8bdc9
            mapnik::filter::filter_visitor<mapnik::image_any> visitor(*closure->im2->this_);
            for (mapnik::filter::filter_type const& filter_tag : closure->filters)
            {
                mapnik::util::apply_visitor(visitor, filter_tag);
            }
            mapnik::premultiply_alpha(*closure->im2->this_);
        }
        mapnik::composite(*closure->im1->this_,*closure->im2->this_, closure->mode, closure->opacity, closure->dx, closure->dy);
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void Image::EIO_AfterComposite(uv_work_t* req)
{
    Nan::HandleScope scope;

    composite_image_baton_t *closure = static_cast<composite_image_baton_t *>(req->data);

    if (closure->error)
    {
        v8::Local<v8::Value> argv[1] = { Nan::Error(closure->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 1, argv);
    } else {
        v8::Local<v8::Value> argv[2] = { Nan::Null(), closure->im1->handle() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(closure->cb), 2, argv);
    }

    closure->im1->Unref();
    closure->im2->Unref();
    closure->cb.Reset();
    delete closure;
}
