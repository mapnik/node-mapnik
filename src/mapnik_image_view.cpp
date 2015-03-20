
// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image_view.hpp>        // for image_view, etc
#include <mapnik/image_view_any.hpp>    // for image_view_any, etc
#include <mapnik/image_util.hpp>

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_color.hpp"
#include "mapnik_palette.hpp"
#include "utils.hpp"

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
    node::ObjectWrap(),
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

Handle<Value> ImageView::NewInstance(Image * JSImage ,
                             unsigned x,
                             unsigned y,
                             unsigned w,
                             unsigned h
    )
{
    NanEscapableScope();
    ImageView* imv = new ImageView(JSImage);
    imv->this_ = std::make_shared<mapnik::image_view_any>(mapnik::create_view(*(JSImage->get()),x,y,w,h));
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
    closure->error = false;
    NanAssignPersistent(closure->cb, callback.As<Function>());
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    im->Ref();
    NanReturnUndefined();
}

void ImageView::EIO_IsSolid(uv_work_t* req)
{
    is_solid_image_view_baton_t *closure = static_cast<is_solid_image_view_baton_t *>(req->data);
    if (closure->im->this_->width() > 0 && closure->im->this_->height() > 0)
    {
        closure->result = mapnik::is_solid(*(closure->im->this_));
    }
    else
    {
        closure->error = true;
        closure->error_name = "image does not have valid dimensions";
    }
}

struct visitor_get_pixel_view
{
    visitor_get_pixel_view(int x, int y)
        : x_(x), y_(y) {}
    
    Local<Value> operator() (mapnik::image_view_null const& data)
    {
        // This should never be reached because the width and height of 0 for a null
        // image will prevent the visitor from being called.
        /* LCOV_EXCL_START */
        NanEscapableScope();
        return NanEscapeScope(NanUndefined());
        /* LCOV_EXCL_END */
    }

    Local<Value> operator() (mapnik::image_view_gray8 const& data)
    {
        NanEscapableScope();
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Uint32>(val));
    }

    Local<Value> operator() (mapnik::image_view_gray8s const& data)
    {
        NanEscapableScope();
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Int32>(val));
    }

    Local<Value> operator() (mapnik::image_view_gray16 const& data)
    {
        NanEscapableScope();
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Uint32>(val));
    }

    Local<Value> operator() (mapnik::image_view_gray16s const& data)
    {
        NanEscapableScope();
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Int32>(val));
    }

    Local<Value> operator() (mapnik::image_view_gray32 const& data)
    {
        NanEscapableScope();
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Uint32>(val));
    }
    
    Local<Value> operator() (mapnik::image_view_gray32s const& data)
    {
        NanEscapableScope();
        std::int32_t val = mapnik::get_pixel<std::int32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Int32>(val));
    }

    Local<Value> operator() (mapnik::image_view_gray32f const& data)
    {
        NanEscapableScope();
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

    Local<Value> operator() (mapnik::image_view_gray64 const& data)
    {
        NanEscapableScope();
        std::uint64_t val = mapnik::get_pixel<std::uint64_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

    Local<Value> operator() (mapnik::image_view_gray64s const& data)
    {
        NanEscapableScope();
        std::int64_t val = mapnik::get_pixel<std::int64_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

    Local<Value> operator() (mapnik::image_view_gray64f const& data)
    {
        NanEscapableScope();
        double val = mapnik::get_pixel<double>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

    Local<Value> operator() (mapnik::image_view_rgba8 const& data)
    {
        NanEscapableScope();
        std::uint32_t val = mapnik::get_pixel<std::uint32_t>(data, x_, y_);
        return NanEscapeScope(NanNew<Number>(val));
    }

  private:
    int x_;
    int y_;
        
};

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
                                     mapnik::util::apply_visitor(visitor_get_pixel_view(0,0),*(closure->im->this_)),
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
    if (im->this_->width() > 0 && im->this_->height() > 0)
    {
        return NanEscapeScope(NanNew<Boolean>(mapnik::is_solid(*(im->this_))));
    }
    else
    {
        NanThrowTypeError("image does not have valid dimensions");
        return NanEscapeScope(NanUndefined());
    }
}


NAN_METHOD(ImageView::getPixel)
{
    NanScope();

    int x = 0;
    int y = 0;
    bool get_color = false;

    if (args.Length() >= 3) {

        if (!args[2]->IsObject()) {
            NanThrowTypeError("optional third argument must be an options object");
            NanReturnUndefined();
        }

        Local<Object> options = args[2]->ToObject();

        if (options->Has(NanNew("get_color"))) {
            Local<Value> bind_opt = options->Get(NanNew("get_color"));
            if (!bind_opt->IsBoolean()) {
                NanThrowTypeError("optional arg 'color' must be a boolean");
                NanReturnUndefined();
            }
            get_color = bind_opt->BooleanValue();
        }

    }

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
    if (x >= 0 && x < static_cast<int>(im->this_->width())
        && y >=0 && y < static_cast<int>(im->this_->height()))
    {
        if (get_color)
        {
            mapnik::color val = mapnik::get_pixel<mapnik::color>(*im->this_, x, y);
            NanReturnValue(Color::NewInstance(val));
        } else {
            visitor_get_pixel_view visitor(x, y);
            NanReturnValue(mapnik::util::apply_visitor(visitor, *im->this_));
        }
    }
    NanReturnUndefined();
}


NAN_METHOD(ImageView::width)
{
    NanScope();

    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());
    NanReturnValue(NanNew<Int32>(static_cast<std::int32_t>(im->this_->width())));
}

NAN_METHOD(ImageView::height)
{
    NanScope();

    ImageView* im = node::ObjectWrap::Unwrap<ImageView>(args.Holder());
    NanReturnValue(NanNew<Int32>(static_cast<std::int32_t>(im->this_->height())));
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
        if (palette.get())
        {
            s = save_to_string(*(im->this_), format, *palette);
        }
        else {
            s = save_to_string(*(im->this_), format);
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
        if (closure->palette.get())
        {
            closure->result = save_to_string(*(closure->im->this_), closure->format, *closure->palette);
        }
        else
        {
            closure->result = save_to_string(*(closure->im->this_), closure->format);
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
        save_to_file(*im->this_,filename);
    }
    catch (std::exception const& ex)
    {
        NanThrowError(ex.what());
        NanReturnUndefined();
    }
    NanReturnUndefined();
}


