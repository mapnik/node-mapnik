
// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image_view.hpp>        // for image_view, etc
#include <mapnik/image_util.hpp>
#include <mapnik/graphics.hpp>

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_color.hpp"
#include "mapnik_palette.hpp"
#include "utils.hpp"

// boost
#include MAPNIK_MAKE_SHARED_INCLUDE

// std
#include <exception>

Persistent<FunctionTemplate> ImageView::constructor;

void ImageView::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(ImageView::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("ImageView"));

    NODE_SET_PROTOTYPE_METHOD(lcons, "encodeSync", encodeSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "encode", encode);
    NODE_SET_PROTOTYPE_METHOD(lcons, "save", save);
    NODE_SET_PROTOTYPE_METHOD(lcons, "width", width);
    NODE_SET_PROTOTYPE_METHOD(lcons, "height", height);
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolid", isSolid);
    NODE_SET_PROTOTYPE_METHOD(lcons, "isSolidSync", isSolidSync);
    NODE_SET_PROTOTYPE_METHOD(lcons, "getPixel", getPixel);

    target->Set(NanNew("ImageView"),lcons->GetFunction());
    NanAssignPersistent(constructor, lcons);
}


ImageView::ImageView(Image * JSImage) :
    ObjectWrap(),
    this_(),
    JSImage_(JSImage) {
        JSImage_->_ref();
    }

ImageView::~ImageView()
{
    JSImage_->_unref();
}

NAN_METHOD(ImageView::New)
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
        ImageView* im =  static_cast<ImageView*>(ptr);
        im->Wrap(args.This());
        NanReturnValue(args.This());
    } else {
        NanThrowError("Cannot create this object from Javascript");
        NanReturnUndefined();
    }
    NanReturnUndefined();
}

Handle<Value> ImageView::New(Image * JSImage ,
                             unsigned x,
                             unsigned y,
                             unsigned w,
                             unsigned h
    )
{
    NanEscapableScope();
    ImageView* imv = new ImageView(JSImage);
    imv->this_ = MAPNIK_MAKE_SHARED<mapnik::image_view<mapnik::image_data_rgba8> >(JSImage->get()->get_view(x,y,w,h));
    Handle<Value> ext = NanNew<External>(imv);
    Handle<Object> obj = NanNew(constructor)->GetFunction()->NewInstance(1, &ext);
    return NanEscapeScope(obj);

}

typedef struct {
    uv_work_t request;
    ImageView* im;
    Persistent<Function> cb;
    bool error;
    std::string error_name;
    bool result;
    mapnik::image_view<mapnik::image_data_rgba8>::pixel_type pixel;
} is_solid_image_view_baton_t;

NAN_METHOD(ImageView::isSolid)
{
    NanScope();
    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());

    if (args.Length() == 0) {
        NanReturnValue(_isSolidSync(args));
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    is_solid_image_view_baton_t *closure = new is_solid_image_view_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->result = true;
    closure->pixel = 0;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    im->Ref();
    NanReturnUndefined();
}

void ImageView::EIO_IsSolid(uv_work_t* req)
{
    is_solid_image_view_baton_t *closure = static_cast<is_solid_image_view_baton_t *>(req->data);
    image_view_ptr view = closure->im->get();
    if (view->width() > 0 && view->height() > 0)
    {
        typedef mapnik::image_view<mapnik::image_data_rgba8>::pixel_type pixel_type;
        pixel_type const first_pixel = view->getRow(0)[0];
        closure->pixel = first_pixel;
        for (unsigned y = 0; y < view->height(); ++y)
        {
            pixel_type const * row = view->getRow(y);
            for (unsigned x = 0; x < view->width(); ++x)
            {
                if (first_pixel != row[x])
                {
                    closure->result = false;
                    return;
                }
            }
        }
    }
    else
    {
        closure->error = true;
        closure->error_name = "image does not have valid dimensions";
    }
}

void ImageView::EIO_AfterIsSolid(uv_work_t* req)
{
    NanScope();
    is_solid_image_view_baton_t *closure = static_cast<is_solid_image_view_baton_t *>(req->data);
    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            Local<Value> argv[3] = { NanNull(),
                                     NanNew(closure->result),
                                     NanNew<Number>(closure->pixel),
            };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 3, argv);
        }
        else
        {
            Local<Value> argv[2] = { NanNull(), NanNew(closure->result) };
            NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
        }
    }
    closure->im->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}


NAN_METHOD(ImageView::isSolidSync)
{
    NanScope();
    NanReturnValue(_isSolidSync(args));
}

Local<Value> ImageView::_isSolidSync(_NAN_METHOD_ARGS)
{
    NanEscapableScope();
    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());
    image_view_ptr view = im->get();
    if (view->width() > 0 && view->height() > 0)
    {
        mapnik::image_view<mapnik::image_data_rgba8>::pixel_type const* first_row = view->getRow(0);
        mapnik::image_view<mapnik::image_data_rgba8>::pixel_type const first_pixel = first_row[0];
        for (unsigned y = 0; y < view->height(); ++y)
        {
            mapnik::image_view<mapnik::image_data_rgba8>::pixel_type const * row = view->getRow(y);
            for (unsigned x = 0; x < view->width(); ++x)
            {
                if (first_pixel != row[x])
                {
                    return NanEscapeScope(NanFalse());
                }
            }
        }
    }
    return NanEscapeScope(NanTrue());
}


NAN_METHOD(ImageView::getPixel)
{
    NanScope();

    int x = 0;
    int y = 0;

    if (args.Length() >= 2) {
        if (!args[0]->IsNumber()) {
            NanThrowTypeError("first arg, 'x' must be an integer");
            NanReturnUndefined();
        }
        if (!args[1]->IsNumber()) {
            NanThrowTypeError("second arg, 'y' must be an integer");
            NanReturnUndefined();
        }
        x = args[0]->IntegerValue();
        y = args[1]->IntegerValue();
    } else {
        NanThrowTypeError("must supply x,y to query pixel color");
        NanReturnUndefined();
    }

    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());
    image_view_ptr view = im->get();
    if (x >= 0 && x < static_cast<int>(view->width())
        && y >=0 && y < static_cast<int>(view->height()))
    {
        mapnik::image_view<mapnik::image_data_rgba8>::pixel_type const * row = view->getRow(y);
        mapnik::image_view<mapnik::image_data_rgba8>::pixel_type const pixel = row[x];
        unsigned r = pixel & 0xff;
        unsigned g = (pixel >> 8) & 0xff;
        unsigned b = (pixel >> 16) & 0xff;
        unsigned a = (pixel >> 24) & 0xff;
        NanReturnValue(Color::New(mapnik::color(r,g,b,a)));
    }
    NanReturnUndefined();
}


NAN_METHOD(ImageView::width)
{
    NanScope();

    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());
    NanReturnValue(NanNew<Integer>(im->get()->width()));
}

NAN_METHOD(ImageView::height)
{
    NanScope();

    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());
    NanReturnValue(NanNew<Integer>(im->get()->height()));
}


NAN_METHOD(ImageView::encodeSync)
{
    NanScope();

    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (args.Length() >= 1) {
        if (!args[0]->IsString()) {
            NanThrowTypeError("first arg, 'format' must be a string");
            NanReturnUndefined();
        }
        format = TOSTR(args[0]);
    }

    // options hash
    if (args.Length() >= 2) {
        if (!args[1]->IsObject()) {
            NanThrowTypeError("optional second arg must be an options object");
            NanReturnUndefined();
        }

        Local<Object> options = args[1].As<Object>();

        if (options->Has(NanNew("palette")))
        {
            Local<Value> format_opt = options->Get(NanNew("palette"));
            if (!format_opt->IsObject()) {
                NanThrowTypeError("'palette' must be an object");
                NanReturnUndefined();
            }

            Local<Object> obj = format_opt.As<Object>();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(Palette::constructor)->HasInstance(obj)) {
                NanThrowTypeError("mapnik.Palette expected as second arg");
                NanReturnUndefined();
            }

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    try {
        std::string s;
        mapnik::image_view<mapnik::image_data_rgba8> const& image = *(im->this_);
        if (palette.get())
        {
            s = save_to_string(image, format, *palette);
        }
        else {
            s = save_to_string(image, format);
        }

        NanReturnValue(NanNewBufferHandle((char*)s.data(),s.size()));
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
}

typedef struct {
    uv_work_t request;
    ImageView* im;
    std::string format;
    palette_ptr palette;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    std::string result;
} encode_image_view_baton_t;


NAN_METHOD(ImageView::encode)
{
    NanScope();

    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (args.Length() > 1){
        if (!args[0]->IsString()) {
            NanThrowTypeError("first arg, 'format' must be a string");
            NanReturnUndefined();
        }
        format = TOSTR(args[0]);
    }

    // options hash
    if (args.Length() >= 2) {
        if (!args[1]->IsObject()) {
            NanThrowTypeError("optional second arg must be an options object");
            NanReturnUndefined();
        }

        Local<Object> options = args[1].As<Object>();

        if (options->Has(NanNew("palette")))
        {
            Local<Value> format_opt = options->Get(NanNew("palette"));
            if (!format_opt->IsObject()) {
                NanThrowTypeError("'palette' must be an object");
                NanReturnUndefined();
            }

            Local<Object> obj = format_opt.As<Object>();
            if (obj->IsNull() || obj->IsUndefined() || !NanNew(Palette::constructor)->HasInstance(obj)) {
                NanThrowTypeError("mapnik.Palette expected as second arg");
                NanReturnUndefined();
            }

            palette = node::ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length() - 1];
    if (!args[args.Length()-1]->IsFunction()) {
        NanThrowTypeError("last argument must be a callback function");
        NanReturnUndefined();
    }

    encode_image_view_baton_t *closure = new encode_image_view_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->format = format;
    closure->palette = palette;
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    im->Ref();
    NanReturnUndefined();
}

void ImageView::EIO_Encode(uv_work_t* req)
{
    encode_image_view_baton_t *closure = static_cast<encode_image_view_baton_t *>(req->data);

    try {
        mapnik::image_view<mapnik::image_data_rgba8> const& im = *(closure->im->this_);
        if (closure->palette.get())
        {
            closure->result = save_to_string(im, closure->format, *closure->palette);
        }
        else
        {
            closure->result = save_to_string(im, closure->format);
        }
    }
    catch (std::exception const& ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
}

void ImageView::EIO_AfterEncode(uv_work_t* req)
{
    NanScope();

    encode_image_view_baton_t *closure = static_cast<encode_image_view_baton_t *>(req->data);

    if (closure->error) {
        Local<Value> argv[1] = { NanError(closure->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 1, argv);
    }
    else
    {
        Local<Value> argv[2] = { NanNull(), NanNewBufferHandle((char*)closure->result.data(), closure->result.size()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(closure->cb), 2, argv);
    }

    closure->im->Unref();
    NanDisposePersistent(closure->cb);
    delete closure;
}


NAN_METHOD(ImageView::save)
{
    NanScope();

    if (args.Length() == 0 || !args[0]->IsString()){
        NanThrowTypeError("filename required");
        NanReturnUndefined();
    }

    std::string filename = TOSTR(args[0]);

    std::string format("");

    if (args.Length() >= 2) {
        if (!args[1]->IsString()) {
            NanThrowTypeError("both 'filename' and 'format' arguments must be strings");
            NanReturnUndefined();
        }

        format = mapnik::guess_type(TOSTR(args[1]));
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            NanThrowError(s.str().c_str());
            NanReturnUndefined();
        }
    }

    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());
    try
    {
        save_to_file(*im->get(),filename);
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnUndefined();
}


