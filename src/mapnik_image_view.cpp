
// node
#include <node_buffer.h>

// mapnik
#include <mapnik/image_util.hpp>
#include <mapnik/graphics.hpp>
//#include <mapnik/image_reader.hpp>

// boost
#include <boost/make_shared.hpp>

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
    NODE_SET_PROTOTYPE_METHOD(constructor, "getPixel", getPixel);

    target->Set(String::NewSymbol("ImageView"),constructor->GetFunction());
}


ImageView::ImageView(image_view_ptr this_) :
    ObjectWrap(),
    this_(this_) {}

ImageView::~ImageView()
{
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

Handle<Value> ImageView::New(boost::shared_ptr<mapnik::image_32> image_ptr,
                             unsigned x,
                             unsigned y,
                             unsigned w,
                             unsigned h
    )
{
    HandleScope scope;
    image_view_ptr iv_ptr = boost::make_shared<mapnik::image_view<mapnik::image_data_32> >(image_ptr->get_view(x,y,w,h));
    ImageView* imv = new ImageView(iv_ptr);
    Handle<Value> ext = External::New(imv);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);

}

Handle<Value> ImageView::isSolid(const Arguments& args)
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
        if (palette.get())
        {
            s = save_to_string(*(im->this_), format, *palette);
        }
        else {
            s = save_to_string(*(im->this_), format);
        }

        node::Buffer *retbuf = Buffer::New((char*)s.data(),s.size());
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
    boost::shared_ptr<mapnik::image_view<mapnik::image_data_32> > image;
    std::string format;
    palette_ptr palette;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    std::string result;
} encode_image_baton_t;


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

    encode_image_baton_t *closure = new encode_image_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->image = im->this_;
    closure->format = format;
    closure->palette = palette;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, EIO_AfterEncode);
    uv_ref(uv_default_loop());
    im->Ref();

    return Undefined();
}

void ImageView::EIO_Encode(uv_work_t* req)
{
    encode_image_baton_t *closure = static_cast<encode_image_baton_t *>(req->data);

    try {
        if (closure->palette.get())
        {
            closure->result = save_to_string(*(closure->image), closure->format, *closure->palette);
        }
        else
        {
            closure->result = save_to_string(*(closure->image), closure->format);
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

    encode_image_baton_t *closure = static_cast<encode_image_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        node::Buffer *retbuf = Buffer::New((char*)closure->result.data(),closure->result.size());
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(retbuf->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    uv_unref(uv_default_loop());
    closure->im->Unref();
    closure->cb.Dispose();
    delete closure;
}


Handle<Value> ImageView::save(const Arguments& args)
{
    HandleScope scope;

    if (!args.Length() >= 1 || !args[0]->IsString()){
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


