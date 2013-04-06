
// node
#include <node.h>                       // for NODE_SET_PROTOTYPE_METHOD, etc
#include <node_object_wrap.h>           // for ObjectWrap
#include <v8.h>
#include <uv.h>
#include <node_buffer.h>

// mapnik
#include <mapnik/color.hpp>             // for color
#include <mapnik/image_view.hpp>        // for image_view, etc
#include <mapnik/image_util.hpp>
#include <mapnik/graphics.hpp>

// boost
#include <boost/make_shared.hpp>

#include "mapnik_image.hpp"
#include "mapnik_image_view.hpp"
#include "mapnik_color.hpp"
#include "mapnik_palette.hpp"
#include "utils.hpp"

// std
#include <exception>

Persistent<FunctionTemplate> ImageView::constructor;

void ImageView::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(ImageView::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("ImageView"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "encodeSync", encodeSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "encode", encode);
    NODE_SET_PROTOTYPE_METHOD(constructor, "save", save);
    NODE_SET_PROTOTYPE_METHOD(constructor, "width", width);
    NODE_SET_PROTOTYPE_METHOD(constructor, "height", height);
    NODE_SET_PROTOTYPE_METHOD(constructor, "isSolid", isSolid);
    NODE_SET_PROTOTYPE_METHOD(constructor, "isSolidSync", isSolidSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "getPixel", getPixel);

    target->Set(String::NewSymbol("ImageView"),constructor->GetFunction());
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

Handle<Value> ImageView::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "image view external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        ImageView* im =  static_cast<ImageView*>(ptr);
        im->Wrap(args.This());
        return args.This();
    } else {
        return ThrowException(String::New("Cannot create this object from Javascript"));
    }
    return Undefined();
}

Handle<Value> ImageView::New(Image * JSImage ,
                             unsigned x,
                             unsigned y,
                             unsigned w,
                             unsigned h
    )
{
    HandleScope scope;
    ImageView* imv = new ImageView(JSImage);
    imv->this_ = boost::make_shared<mapnik::image_view<mapnik::image_data_32> >(JSImage->get()->get_view(x,y,w,h));
    Handle<Value> ext = External::New(imv);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);

}

typedef struct {
    uv_work_t request;
    ImageView* im;
    Persistent<Function> cb;
    bool error;
    std::string error_name;
    bool result;
    mapnik::image_view<mapnik::image_data_32>::pixel_type pixel;
} is_solid_image_view_baton_t;

Handle<Value> ImageView::isSolid(const Arguments& args)
{
    HandleScope scope;
    ImageView* im = ObjectWrap::Unwrap<ImageView>(args.This());

    if (args.Length() == 0) {
        return isSolidSync(args);
    }
    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    is_solid_image_view_baton_t *closure = new is_solid_image_view_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->result = true;
    closure->pixel = 0;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_IsSolid, (uv_after_work_cb)EIO_AfterIsSolid);
    im->Ref();
    return Undefined();
}

void ImageView::EIO_IsSolid(uv_work_t* req)
{
    is_solid_image_view_baton_t *closure = static_cast<is_solid_image_view_baton_t *>(req->data);
    image_view_ptr view = closure->im->get();
    if (view->width() > 0 && view->height() > 0)
    {
        typedef mapnik::image_view<mapnik::image_data_32>::pixel_type pixel_type;
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
    HandleScope scope;
    is_solid_image_view_baton_t *closure = static_cast<is_solid_image_view_baton_t *>(req->data);
    TryCatch try_catch;
    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        if (closure->result)
        {
            Local<Value> argv[3] = { Local<Value>::New(Null()),
                                     Local<Value>::New(Boolean::New(closure->result)),
                                     Local<Value>::New(Number::New(closure->pixel)),
                                   };
            closure->cb->Call(Context::GetCurrent()->Global(), 3, argv);
        }
        else
        {
            Local<Value> argv[2] = { Local<Value>::New(Null()),
                                     Local<Value>::New(Boolean::New(closure->result))
                                   };
            closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        }
    }
    if (try_catch.HasCaught())
    {
        node::FatalException(try_catch);
    }
    closure->im->Unref();
    closure->cb.Dispose();
    delete closure;
}


Handle<Value> ImageView::isSolidSync(const Arguments& args)
{
    HandleScope scope;
    ImageView* im = ObjectWrap::Unwrap<ImageView>(args.This());
    image_view_ptr view = im->get();
    if (view->width() > 0 && view->height() > 0)
    {
        mapnik::image_view<mapnik::image_data_32>::pixel_type const* first_row = view->getRow(0);
        mapnik::image_view<mapnik::image_data_32>::pixel_type const first_pixel = first_row[0];
        for (unsigned y = 0; y < view->height(); ++y)
        {
            mapnik::image_view<mapnik::image_data_32>::pixel_type const * row = view->getRow(y);
            for (unsigned x = 0; x < view->width(); ++x)
            {
                if (first_pixel != row[x])
                {
                    return scope.Close(False());
                }
            }
        }
    }
    return scope.Close(True());
}


Handle<Value> ImageView::getPixel(const Arguments& args)
{
    HandleScope scope;

    unsigned x(0);
    unsigned y(0);

    if (args.Length() >= 2) {
        if (!args[0]->IsNumber())
            return ThrowException(Exception::TypeError(
                                      String::New("first arg, 'x' must be an integer")));
        if (!args[1]->IsNumber())
            return ThrowException(Exception::TypeError(
                                      String::New("second arg, 'y' must be an integer")));
        x = args[0]->IntegerValue();
        y = args[1]->IntegerValue();
    } else {
        return ThrowException(Exception::TypeError(
                                  String::New("must supply x,y to query pixel color")));
    }

    ImageView* im = ObjectWrap::Unwrap<ImageView>(args.This());
    image_view_ptr view = im->get();
    if (x < view->width() && y < view->height())
    {
        mapnik::image_view<mapnik::image_data_32>::pixel_type const * row = view->getRow(y);
        mapnik::image_view<mapnik::image_data_32>::pixel_type const pixel = row[x];
        unsigned r = pixel & 0xff;
        unsigned g = (pixel >> 8) & 0xff;
        unsigned b = (pixel >> 16) & 0xff;
        unsigned a = (pixel >> 24) & 0xff;
        return Color::New(mapnik::color(r,g,b,a));
    }
    return Undefined();
}


Handle<Value> ImageView::width(const Arguments& args)
{
    HandleScope scope;

    ImageView* im = ObjectWrap::Unwrap<ImageView>(args.This());
    return scope.Close(Integer::New(im->get()->width()));
}

Handle<Value> ImageView::height(const Arguments& args)
{
    HandleScope scope;

    ImageView* im = ObjectWrap::Unwrap<ImageView>(args.This());
    return scope.Close(Integer::New(im->get()->height()));
}


Handle<Value> ImageView::encodeSync(const Arguments& args)
{
    HandleScope scope;

    ImageView* im = ObjectWrap::Unwrap<ImageView>(args.This());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (args.Length() >= 1) {
        if (!args[0]->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("first arg, 'format' must be a string")));
        format = TOSTR(args[0]);
    }

    // options hash
    if (args.Length() >= 2) {
        if (!args[1]->IsObject())
            return ThrowException(Exception::TypeError(
                                      String::New("optional second arg must be an options object")));

        Local<Object> options = args[1]->ToObject();

        if (options->Has(String::New("palette")))
        {
            Local<Value> format_opt = options->Get(String::New("palette"));
            if (!format_opt->IsObject())
                return ThrowException(Exception::TypeError(
                                          String::New("'palette' must be an object")));

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Palette::constructor->HasInstance(obj))
                return ThrowException(Exception::TypeError(String::New("mapnik.Palette expected as second arg")));

            palette = ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    try {
        std::string s;
        mapnik::image_view<mapnik::image_data_32> const& image = *(im->this_);
        if (palette.get())
        {
            s = save_to_string(image, format, *palette);
        }
        else {
            s = save_to_string(image, format);
        }

        node::Buffer *retbuf = node::Buffer::New((char*)s.data(),s.size());
        return scope.Close(retbuf->handle_);
    }
    catch (std::exception & ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::Error(
                                  String::New("unknown exception happened when encoding image: please file bug report")));
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


Handle<Value> ImageView::encode(const Arguments& args)
{
    HandleScope scope;

    ImageView* im = ObjectWrap::Unwrap<ImageView>(args.This());

    std::string format = "png";
    palette_ptr palette;

    // accept custom format
    if (args.Length() > 1){
        if (!args[0]->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("first arg, 'format' must be a string")));
        format = TOSTR(args[0]);
    }

    // options hash
    if (args.Length() >= 2) {
        if (!args[1]->IsObject())
            return ThrowException(Exception::TypeError(
                                      String::New("optional second arg must be an options object")));

        Local<Object> options = args[1]->ToObject();

        if (options->Has(String::New("palette")))
        {
            Local<Value> format_opt = options->Get(String::New("palette"));
            if (!format_opt->IsObject())
                return ThrowException(Exception::TypeError(
                                          String::New("'palette' must be an object")));

            Local<Object> obj = format_opt->ToObject();
            if (obj->IsNull() || obj->IsUndefined() || !Palette::constructor->HasInstance(obj))
                return ThrowException(Exception::TypeError(String::New("mapnik.Palette expected as second arg")));

            palette = ObjectWrap::Unwrap<Palette>(obj)->palette();
        }
    }

    // ensure callback is a function
    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    encode_image_view_baton_t *closure = new encode_image_view_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->format = format;
    closure->palette = palette;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, (uv_after_work_cb)EIO_AfterEncode);
    im->Ref();
    return Undefined();
}

void ImageView::EIO_Encode(uv_work_t* req)
{
    encode_image_view_baton_t *closure = static_cast<encode_image_view_baton_t *>(req->data);

    try {
        mapnik::image_view<mapnik::image_data_32> const& im = *(closure->im->this_);
        if (closure->palette.get())
        {
            closure->result = save_to_string(im, closure->format, *closure->palette);
        }
        else
        {
            closure->result = save_to_string(im, closure->format);
        }
    }
    catch (std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happened when encoding image: please file bug report";
    }
}

void ImageView::EIO_AfterEncode(uv_work_t* req)
{
    HandleScope scope;

    encode_image_view_baton_t *closure = static_cast<encode_image_view_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        node::Buffer *retbuf = node::Buffer::New((char*)closure->result.data(),closure->result.size());
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(retbuf->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->im->Unref();
    closure->cb.Dispose();
    delete closure;
}


Handle<Value> ImageView::save(const Arguments& args)
{
    HandleScope scope;

    if (args.Length() == 0 || !args[0]->IsString()){
        return ThrowException(Exception::TypeError(
                                  String::New("filename required")));
    }

    std::string filename = TOSTR(args[0]);

    std::string format("");

    if (args.Length() >= 2) {
        if (!args[1]->IsString())
            return ThrowException(Exception::TypeError(
                                      String::New("both 'filename' and 'format' arguments must be strings")));

        format = mapnik::guess_type(TOSTR(args[1]));
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << filename << "\n";
            return ThrowException(Exception::Error(
                                      String::New(s.str().c_str())));
        }
    }

    ImageView* im = ObjectWrap::Unwrap<ImageView>(args.This());
    try
    {
        save_to_file(*im->get(),filename);
    }
    catch (std::exception const& ex)
    {
        return ThrowException(Exception::Error(
                                  String::New(ex.what())));
    }
    catch (...)
    {
        return ThrowException(Exception::TypeError(
                                  String::New("unknown exception happened while saving an image, please submit a bug report")));
    }

    return Undefined();
}


