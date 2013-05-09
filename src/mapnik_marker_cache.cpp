#include "utils.hpp"
#include "mapnik_marker_cache.hpp"
#include "mapnik_image.hpp"
#include "mapnik_svg.hpp"

// mapnik
#include <mapnik/utils.hpp>
#include <mapnik/marker.hpp>
#include <mapnik/marker_cache.hpp>
#include <mapnik/graphics.hpp>

#include <boost/make_shared.hpp>

Persistent<FunctionTemplate> MarkerCache::constructor;

void MarkerCache::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(MarkerCache::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("MarkerCache"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(constructor, "remove", remove);
    NODE_SET_PROTOTYPE_METHOD(constructor, "size", size);
    NODE_SET_PROTOTYPE_METHOD(constructor, "put", put);
    NODE_SET_PROTOTYPE_METHOD(constructor, "get", get);
    NODE_SET_PROTOTYPE_METHOD(constructor, "keys", keys);
    // todo use NODE_SET_METHOD

    target->Set(String::NewSymbol("MarkerCache"),constructor->GetFunction());
}

MarkerCache::MarkerCache() :
    ObjectWrap() {}

MarkerCache::~MarkerCache() {}

Handle<Value> MarkerCache::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));
    if (args.Length() >= 1)
    {
        return ThrowException(Exception::TypeError(
                                  String::New("accepts no arguments")));
    }

    MarkerCache* mc = new MarkerCache();
    mc->Wrap(args.This());
    return args.This();
}

Handle<Value> MarkerCache::clear(const Arguments& args)
{
    HandleScope scope;
    mapnik::marker_cache::instance().clear();
    return Undefined();
}

Handle<Value> MarkerCache::remove(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() < 1 || !args[0]->IsString())
    {
        return ThrowException(Exception::TypeError(
                                  String::New("requires at least one argument of a key (string)")));
    }
    std::string uri = TOSTR(args[0]);
    return scope.Close(Boolean::New(mapnik::marker_cache::instance().remove(uri)));
}

Handle<Value> MarkerCache::size(const Arguments& args)
{
    HandleScope scope;
    return scope.Close(Integer::New(mapnik::marker_cache::instance().size()));
}

Handle<Value> MarkerCache::put(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsObject())
        return ThrowException(Exception::TypeError(
                                  String::New("requires at least two arguments: a key and a marker (svg or image) to add to cache")));
    std::string uri = TOSTR(args[0]);
    Local<Object> obj = args[1]->ToObject();
    if (obj->IsNull() || obj->IsUndefined())
        return ThrowException(Exception::TypeError(String::New("object is not valid")));
    if (Image::constructor->HasInstance(obj))
    {
        Image *im = ObjectWrap::Unwrap<Image>(obj);
        boost::optional<mapnik::image_ptr> imagep(boost::make_shared<mapnik::image_data_32>(im->get()->data()));
        mapnik::marker_cache::instance().insert_marker(uri,boost::make_shared<mapnik::marker>(imagep),true);
    }
    else if (SVG::constructor->HasInstance(obj))
    {
        SVG *im = ObjectWrap::Unwrap<SVG>(obj);
        mapnik::svg_path_ptr marker_path(boost::make_shared<mapnik::svg_storage_type>(*im->get()));
        mapnik::marker_cache::instance().insert_marker(uri,boost::make_shared<mapnik::marker>(marker_path),true);
    }
    else
    {
        return ThrowException(Exception::TypeError(String::New("object is not valid: requires either image or svg object")));
    }

    return scope.Close(Integer::New(mapnik::marker_cache::instance().size()));
}


Handle<Value> MarkerCache::get(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() < 1 || !args[0]->IsString())
    {
        return ThrowException(Exception::TypeError(
                                  String::New("requires at least one argument of a key (string)")));
    }
    std::string uri = TOSTR(args[0]);
    mapnik::marker_cache::iterator_type itr = mapnik::marker_cache::instance().search(uri);
    mapnik::marker_cache::iterator_type end = mapnik::marker_cache::instance().end();
    if (itr != end)
    {
        if (itr->second->is_bitmap())
        {
            mapnik::image_data_32 const& im = *itr->second->get_bitmap_data()->get();
            Image* new_im = Image::New(boost::make_shared<mapnik::image_32>(im));
            return scope.Close(new_im->handle_);
        }
        else
        {
            SVG* new_im = SVG::New(*itr->second->get_vector_data());
            return scope.Close(new_im->handle_);
        }
        //return boost::python::object(*(itr->second->get_vector_data()));
    }
    return Undefined();
}

Handle<Value> MarkerCache::keys(const Arguments& args)
{
    HandleScope scope;
    Local<Array> arr = Array::New(mapnik::marker_cache::instance().size());

    mapnik::marker_cache::iterator_type itr = mapnik::marker_cache::instance().begin();
    mapnik::marker_cache::iterator_type end = mapnik::marker_cache::instance().end();
    unsigned idx = 0;
    for (;itr != end; ++itr)
    {
        arr->Set(idx,String::New(itr->first.c_str()));
        ++idx;
    }
    return scope.Close(arr);
}
