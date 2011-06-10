
// node
#include <node_buffer.h>
#include <node_version.h>

// mapnik
#include <mapnik/image_util.hpp>
#include <mapnik/graphics.hpp>
#include <mapnik/image_reader.hpp>

#include "mapnik_image.hpp"
#include "utils.hpp"

// std
#include <exception>

Persistent<FunctionTemplate> Image::constructor;

void Image::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Image::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Image"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "encode", encode);

    NODE_SET_METHOD(constructor->GetFunction(),
                  "open",
                  Image::open);

    target->Set(String::NewSymbol("Image"),constructor->GetFunction());
}

Image::Image(unsigned int width, unsigned int height) :
  ObjectWrap(),
  this_(new mapnik::image_32(width,height)) {}

Image::Image(image_ptr this_) :
  ObjectWrap(),
  this_(this_) {}

Image::~Image()
{
}

Handle<Value> Image::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Image* im =  static_cast<Image*>(ptr);
        im->Wrap(args.This());
        return args.This();
    }

    if (args.Length() == 2)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
            return ThrowException(Exception::Error(
               String::New("Image 'width' and 'height' must be a integers")));
        Image* im = new Image(args[0]->IntegerValue(),args[1]->IntegerValue());
        im->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
          String::New("please provide Image width and height")));
    }
    return Undefined();
}

Handle<Value> Image::open(const Arguments& args)
{
    HandleScope scope;
    
    if (!args[0]->IsString()) {
      return ThrowException(Exception::TypeError(String::New(
              "Argument must be a string")));
    }
  
    try {
    std::string filename = TOSTR(args[0]);
    boost::optional<std::string> type = mapnik::type_from_filename(filename);
    if (type)
    {
        std::auto_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(filename,*type));
        if (reader.get())
        {
            boost::shared_ptr<mapnik::image_32> image_ptr(new mapnik::image_32(reader->width(),reader->height()));
            reader->read(0,0,image_ptr->data());
            Image* im = new Image(image_ptr);
            //im->this_ = image_ptr;
            Handle<Value> ext = External::New(im);
            Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
            return scope.Close(obj);
        }
        return ThrowException(Exception::TypeError(String::New(
                ("Failed to load: " + filename).c_str())));
    }
    return ThrowException(Exception::TypeError(String::New(
            ("Unsupported image format:" + filename).c_str())));
    }
    catch (std::exception & ex)
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }

}


Handle<Value> Image::encode(const Arguments& args)
{
    HandleScope scope;

    Image* im = ObjectWrap::Unwrap<Image>(args.This());
    
    std::string format = "png8"; //default to 256 colors
    
    // accept custom format
    if (args.Length() >= 1){
        if (!args[0]->IsString())
          return ThrowException(Exception::TypeError(
            String::New("first arg, 'format' must be a string")));
        format = TOSTR(args[0]);
    }
    
    try {
        std::string s = save_to_string(*(im->this_), format);
        #if NODE_VERSION_AT_LEAST(0,3,0)
        node::Buffer *retbuf = Buffer::New((char*)s.data(),s.size());
        #else
        node::Buffer *retbuf = Buffer::New(s.size());
        memcpy(retbuf->data(), s.data(), s.size());
        #endif
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
