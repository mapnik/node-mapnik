
#include <node_buffer.h>
#include <node_version.h>

// mapnik
#include <mapnik/map.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/filter_factory.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/config_error.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/query.hpp>

// careful, missing include gaurds: http://trac.mapnik.org/changeset/2516 
//#include <mapnik/filter_featureset.hpp>

// not currently used...
//#include <mapnik/color_factory.hpp>
//#include <mapnik/hit_test_filter.hpp>
//#include <mapnik/memory_datasource.hpp>
//#include <mapnik/memory_featureset.hpp>
//#include <mapnik/params.hpp>
//#include <mapnik/feature_layer_desc.hpp>

#if defined(HAVE_CAIRO)
#include <mapnik/cairo_renderer.hpp>
#endif

// stl
#include <exception>

#include <mapnik/version.hpp>

#include "utils.hpp"
#include "mapnik_map.hpp"
#include "json_emitter.hpp"
#include "mapnik_layer.hpp"


Persistent<FunctionTemplate> Map::constructor;

void Map::Initialize(Handle<Object> target) {

    HandleScope scope;
  
    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Map::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Map"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "load", load);
    NODE_SET_PROTOTYPE_METHOD(constructor, "clear", clear);
    NODE_SET_PROTOTYPE_METHOD(constructor, "from_string", from_string);
    NODE_SET_PROTOTYPE_METHOD(constructor, "resize", resize);
    NODE_SET_PROTOTYPE_METHOD(constructor, "width", width);
    NODE_SET_PROTOTYPE_METHOD(constructor, "height", height);
    NODE_SET_PROTOTYPE_METHOD(constructor, "buffer_size", buffer_size);
    NODE_SET_PROTOTYPE_METHOD(constructor, "generate_hit_grid", generate_hit_grid);
    NODE_SET_PROTOTYPE_METHOD(constructor, "extent", extent);
    NODE_SET_PROTOTYPE_METHOD(constructor, "zoom_all", zoom_all);
    NODE_SET_PROTOTYPE_METHOD(constructor, "zoom_to_box", zoom_to_box);
    NODE_SET_PROTOTYPE_METHOD(constructor, "render", render);
    NODE_SET_PROTOTYPE_METHOD(constructor, "render_to_string", render_to_string);
    NODE_SET_PROTOTYPE_METHOD(constructor, "render_to_file", render_to_file);

    // layer access
    NODE_SET_PROTOTYPE_METHOD(constructor, "add_layer", add_layer);
    NODE_SET_PROTOTYPE_METHOD(constructor, "get_layer", get_layer);

    // temp hack to expose layer metadata
    NODE_SET_PROTOTYPE_METHOD(constructor, "layers", layers);
    NODE_SET_PROTOTYPE_METHOD(constructor, "features", features);
    NODE_SET_PROTOTYPE_METHOD(constructor, "describe_data", describe_data);
            
    target->Set(String::NewSymbol("Map"),constructor->GetFunction());
    //eio_set_max_poll_reqs(10);
    //eio_set_min_parallel(10);
}
      
Map::Map(int width, int height) :
  ObjectWrap(),
  map_(new mapnik::Map(width,height)) {}

Map::Map(int width, int height, std::string const& srs) :
  ObjectWrap(),
  map_(new mapnik::Map(width,height,srs)) {}

Map::~Map()
{
    // std::clog << "~Map(node)\n";
    // release is handled by boost::shared_ptr
}

Handle<Value> Map::New(const Arguments& args)
{
    HandleScope scope;
  
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    // accept a reference or v8:External?
    if (args[0]->IsExternal()) 
    { 
        return ThrowException(String::New("No support yet for passing v8:External wrapper around C++ void*"));
    } 
  
    if (args.Length() == 2)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
            return ThrowException(Exception::Error(
               String::New("'width' and 'height' must be a integers")));
        Map* m = new Map(args[0]->IntegerValue(),args[1]->IntegerValue());
        m->Wrap(args.This());
        return args.This();
    }
    else if (args.Length() == 3)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsString())
            return ThrowException(Exception::Error(
               String::New("'width' and 'height' must be a integers")));
        Map* m = new Map(args[0]->IntegerValue(),args[1]->IntegerValue(),TOSTR(args[2]));
        m->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
          String::New("please provide Map width and height and optional srs")));  
    }
    //return args.This();
    return Undefined();
}

Handle<Value> Map::add_layer(const Arguments &args) {
    HandleScope scope;
    Local<Object> obj = args[0]->ToObject();
    if (!Layer::constructor->HasInstance(obj))
      return ThrowException(Exception::TypeError(String::New("mapnik.Layer expected")));
    Layer *l = ObjectWrap::Unwrap<Layer>(obj);
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    // TODO - addLayer should be add_layer in mapnik
    m->map_->addLayer(*l->get());
    return Undefined();
}

Handle<Value> Map::get_layer(const Arguments& args)
{
    HandleScope scope;
  
    if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Please provide layer index")));
  
    if (!args[0]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("layer index must be an integer")));
  
    unsigned index = args[0]->IntegerValue();
  
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
  
    std::vector<mapnik::layer> & layers = m->map_->layers();
    
    // TODO - we don't know features.length at this point
    if ( index < layers.size())
    {      
        //mapnik::layer & lay_ref = layers[index];
        return scope.Close(Layer::New(layers[index]));
    }
    else
    {
      return ThrowException(Exception::TypeError(
        String::New("invalid layer index")));
    }
    return Undefined();

}


Handle<Value> Map::layers(const Arguments& args)
{
    HandleScope scope;
  
    // todo - optimize by allowing indexing...
    /*if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Please provide layer index")));
  
    if (!args[0]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("layer index must be an integer")));
    */
  
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
  
    std::vector<mapnik::layer> const & layers = m->map_->layers();
    Local<Array> a = Array::New(layers.size());
  
    for (unsigned i = 0; i < layers.size(); ++i )
    {
        const mapnik::layer & layer = layers[i];
        Local<Object> meta = Object::New();
        layer_as_json(meta,layer);
        a->Set(i, meta);
    }
    
    return scope.Close(a);

}

Handle<Value> Map::describe_data(const Arguments& args)
{
    HandleScope scope;
  
    // todo - optimize by allowing indexing...
    /*if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Please provide layer index")));
  
    if (!args[0]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("layer index must be an integer")));
    */
  
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
  
    std::vector<mapnik::layer> const & layers = m->map_->layers();
  
    Local<Object> meta = Object::New();
  
    for (unsigned i = 0; i < layers.size(); ++i )
    {
        const mapnik::layer & layer = layers[i];
        Local<Object> description = Object::New();
        mapnik::datasource_ptr ds = layer.datasource();
        if (ds)
        {
            describe_datasource(description,ds);
        }
        meta->Set(String::NewSymbol(layer.name().c_str()), description);
    }
    
    return scope.Close(meta);

}


Handle<Value> Map::features(const Arguments& args)
{
    HandleScope scope;
  
    if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Please provide layer index")));
  
    if (!args[0]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("layer index must be an integer")));
  
    unsigned index = args[0]->IntegerValue();
  
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
  
    std::vector<mapnik::layer> const & layers = m->map_->layers();
    
    // TODO - we don't know features.length at this point
    Local<Array> a = Array::New(0);
    if ( index < layers.size())
    {
        mapnik::layer const& layer = layers[index];
        mapnik::datasource_ptr ds = layer.datasource();
        if (ds)
        {
            datasource_features(a,ds);
        }
    }
  
    return scope.Close(a);

}

Handle<Value> Map::clear(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    m->map_->remove_all();
    return Undefined();
}

Handle<Value> Map::resize(const Arguments& args)
{
    HandleScope scope;
  
    if (!args.Length() == 2)
      return ThrowException(Exception::Error(
        String::New("Please provide width and height")));
  
    if (!args[0]->IsNumber() || !args[1]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("width and height must be integers")));
  
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    m->map_->resize(args[0]->IntegerValue(),args[1]->IntegerValue());
    return Undefined();
}


Handle<Value> Map::width(const Arguments& args)
{
    HandleScope scope;
    if (!args.Length() == 0)
      return ThrowException(Exception::Error(
        String::New("accepts no arguments")));
  
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    Local<Value> width = Integer::New(m->map_->width());
    return scope.Close(width);
}

Handle<Value> Map::height(const Arguments& args)
{
    HandleScope scope;
    if (!args.Length() == 0)
      return ThrowException(Exception::Error(
        String::New("accepts no arguments")));
  
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    Local<Value> width = Integer::New(m->map_->height());
    return scope.Close(width);
}

Handle<Value> Map::buffer_size(const Arguments& args)
{
    HandleScope scope;
    if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Please provide a buffer_size")));
  
    if (!args[0]->IsNumber())
      return ThrowException(Exception::TypeError(
        String::New("buffer_size must be an integer")));
  
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    m->map_->set_buffer_size(args[0]->IntegerValue());
    return Undefined();
}

Handle<Value> Map::load(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() != 1 || !args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a path to a mapnik stylesheet")));
        
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::string const& stylesheet = TOSTR(args[0]);
    bool strict = false;
    try
    {
        mapnik::load_map(*m->map_,stylesheet,strict);
    }
    catch (const mapnik::config_error & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (...)
    {
      return ThrowException(Exception::TypeError(
        String::New("something went wrong loading the map")));    
    }
    return Undefined();
}

Handle<Value> Map::from_string(const Arguments& args)
{
    HandleScope scope;
    if (!args.Length() >= 1) {
        return ThrowException(Exception::TypeError(
        String::New("Accepts 2 arguments: map string and base_url")));
    }
  
    if (!args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a mapnik stylesheet string")));
  
    if (!args[1]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("second argument must be a base_url to interpret any relative path from")));
        
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::string const& stylesheet = TOSTR(args[0]);
    bool strict = false;
    std::string const& base_url = TOSTR(args[1]);
    try
    {
        mapnik::load_map_string(*m->map_,stylesheet,strict,base_url);
    }
    catch (const mapnik::config_error & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (...)
    {
      return ThrowException(Exception::TypeError(
        String::New("something went wrong loading the map")));    
    }
    return Undefined();
}


Handle<Value> Map::extent(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    
    Local<Array> a = Array::New(4);
    mapnik::box2d<double> e = m->map_->get_current_extent();
    a->Set(0, Number::New(e.minx()));
    a->Set(1, Number::New(e.miny()));
    a->Set(2, Number::New(e.maxx()));
    a->Set(3, Number::New(e.maxy()));
    return scope.Close(a);
}

Handle<Value> Map::zoom_all(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    m->map_->zoom_all();
    return Undefined();
}

Handle<Value> Map::zoom_to_box(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
  
    double minx;
    double miny;
    double maxx;
    double maxy;
  
    if (args.Length() == 1)
    {
        if (!args[0]->IsArray())
            return ThrowException(Exception::Error(
               String::New("Must provide an array of: [minx,miny,maxx,maxy]")));
        Local<Array> a = Local<Array>::Cast(args[0]);
        minx = a->Get(0)->NumberValue();
        miny = a->Get(1)->NumberValue();
        maxx = a->Get(2)->NumberValue();
        maxy = a->Get(3)->NumberValue();
        
    }   
    else if (args.Length() != 4)
      return ThrowException(Exception::Error(
        String::New("Must provide 4 arguments: minx,miny,maxx,maxy")));
    else {
        minx = args[0]->NumberValue();
        miny = args[1]->NumberValue();
        maxx = args[2]->NumberValue();
        maxy = args[3]->NumberValue();
    }
    mapnik::box2d<double> box(minx,miny,maxx,maxy);
    m->map_->zoom_to_box(box);
    return Undefined();
}

typedef struct {
    Map *m;
    std::string format;
    mapnik::box2d<double> bbox;
    bool error;
    std::string error_name;
    std::string im_string;
    Persistent<Function> cb;
} closure_t;


Handle<Value> Map::render(const Arguments& args)
{
    HandleScope scope;
  
    /*
    std::clog << "eio_nreqs" << eio_nreqs() << "\n";
    std::clog << "eio_nready" << eio_nready() << "\n";
    std::clog << "eio_npending" << eio_npending() << "\n";
    std::clog << "eio_nthreads" << eio_nthreads() << "\n";
    */

    if (args.Length() < 3)
        return ThrowException(Exception::TypeError(
          String::New("requires three arguments, a extent array, a format, and a callback")));
    
    // extent array
    if (!args[0]->IsArray())
        return ThrowException(Exception::TypeError(
           String::New("first argument must be an extent array of: [minx,miny,maxx,maxy]")));

    // format
    if (!args[1]->IsString())
        return ThrowException(Exception::TypeError(
           String::New("second argument must be an format string")));

    // function callback
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                  String::New("last argument must be a callback function")));

    Local<Array> a = Local<Array>::Cast(args[0]);
    uint32_t a_length = a->Length();
    if (!a_length  == 4) {
        return ThrowException(Exception::TypeError(
           String::New("first argument must be 4 item array of: [minx,miny,maxx,maxy]")));
    }

    closure_t *closure = new closure_t();
  
    if (!closure) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate enough memory")));
    }

    double minx = a->Get(0)->NumberValue();
    double miny = a->Get(1)->NumberValue();
    double maxx = a->Get(2)->NumberValue();
    double maxy = a->Get(3)->NumberValue();
  
    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    closure->m = m;
    closure->format = TOSTR(args[1]);
    closure->error = false;
    closure->bbox = mapnik::box2d<double>(minx,miny,maxx,maxy);
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(args[args.Length()-1]));
    eio_custom(EIO_Render, EIO_PRI_DEFAULT, EIO_AfterRender, closure);
    ev_ref(EV_DEFAULT_UC);
    m->Ref();
    return Undefined();
}

int Map::EIO_Render(eio_req *req)
{
    closure_t *closure = static_cast<closure_t *>(req->data);
    
    // zoom to
    closure->m->map_->zoom_to_box(closure->bbox);
    try
    {
        mapnik::image_32 im(closure->m->map_->width(),closure->m->map_->height());
        mapnik::agg_renderer<mapnik::image_32> ren(*closure->m->map_,im);
        ren.apply();
        closure->im_string = save_to_string(im, closure->format);
    }
    catch (const mapnik::config_error & ex )
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (const mapnik::datasource_exception & ex )
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (const mapnik::proj_init_error & ex )
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (const std::runtime_error & ex )
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (const mapnik::ImageWriterException & ex )
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happened while rendering the map,\n this should not happen, please submit a bug report";
    }
    return 0;
}

int Map::EIO_AfterRender(eio_req *req)
{
    HandleScope scope;

    closure_t *closure = static_cast<closure_t *>(req->data);
    ev_unref(EV_DEFAULT_UC);

    TryCatch try_catch;
  
    if (closure->error) {
        // TODO - add more attributes
        // https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/Error
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {
        #if NODE_VERSION_AT_LEAST(0,3,0)
          node::Buffer *retbuf = Buffer::New((char *)closure->im_string.data(),closure->im_string.size());
        #else
          node::Buffer *retbuf = Buffer::New(closure->im_string.size());
          memcpy(retbuf->data(), closure->im_string.data(), closure->im_string.size());
        #endif
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(retbuf->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }
    
    closure->m->Unref();
    closure->cb.Dispose();
    delete closure;
    return 0;
}

Handle<Value> Map::render_to_string(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    mapnik::image_32 im(m->map_->width(),m->map_->height());
    mapnik::agg_renderer<mapnik::image_32> ren(*m->map_,im);
    try
    {
      ren.apply();
    }
    // proj_init_error
    catch (const mapnik::config_error & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (const mapnik::datasource_exception & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (const std::runtime_error & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (...)
    {
      return ThrowException(Exception::TypeError(
        String::New("unknown exception happened while rendering the map, please submit a bug report")));    
    }
    
    // TODO - expose format
    std::string s = save_to_string(im, "png");
    //std::string ss = mapnik::save_to_string<mapnik::image_data_32>(im.data(),"png");
    
  #if NODE_VERSION_AT_LEAST(0,3,0)
    node::Buffer *retbuf = Buffer::New((char*)s.data(),s.size());
  #else
    node::Buffer *retbuf = Buffer::New(s.size());
    memcpy(retbuf->data(), s.data(), s.size());
  #endif
    return scope.Close(retbuf->handle_);
}

Handle<Value> Map::render_to_file(const Arguments& args)
{
    HandleScope scope;
    if (!args.Length() >= 1 || !args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a path to a file to save")));

    if (args.Length() > 2)
      return ThrowException(Exception::TypeError(
        String::New("accepts two arguments, a required path to a file, and an optional options object, eg. {format: 'pdf'}")));

    std::string format("");
    
    if (args.Length() == 2){
      if (!args[1]->IsObject())
        return ThrowException(Exception::TypeError(
          String::New("second argument is optional, but if provided must be an object, eg. {format: 'pdf'}")));
  
        Local<Object> options = args[1]->ToObject();
        if (options->Has(String::New("format")))
        {
            Local<Value> format_opt = options->Get(String::New("format"));
            if (!format_opt->IsString())
              return ThrowException(Exception::TypeError(
                String::New("'format' must be a String")));
            
            format = TOSTR(format_opt);
        }
    }
        
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::string const& output = TOSTR(args[0]);
    
    if (format.empty()) {
        format = mapnik::guess_type(output);
        if (format == "<unknown>") {
            std::ostringstream s("");
            s << "unknown output extension for: " << output << "\n";
            return ThrowException(Exception::Error(
                String::New(s.str().c_str())));
        }
            
    }
    
    try
    {

        if (format == "pdf" || format == "svg" || format =="ps" || format == "ARGB32" || format == "RGB24")
        {
    #if defined(HAVE_CAIRO)
            mapnik::save_to_cairo_file(*m->map_,output,format);
    #else
            std::ostringstream s("");
            s << "Cairo backend is not available, cannot write to " << format << "\n";
            return ThrowException(Exception::Error(
              String::New(s.str().c_str())));
    #endif
        }
        else 
        {
            mapnik::image_32 im(m->map_->width(),m->map_->height());
            mapnik::agg_renderer<mapnik::image_32> ren(*m->map_,im);
            ren.apply();
            mapnik::save_to_file<mapnik::image_data_32>(im.data(),output);
        }
    }
    catch (const mapnik::config_error & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (const mapnik::datasource_exception & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (const std::runtime_error & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (const mapnik::ImageWriterException & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
    catch (...)
    {
      return ThrowException(Exception::TypeError(
        String::New("unknown exception happened while rendering the map, please submit a bug report")));    
    }
  
    return Undefined();
}

Handle<Value> Map::generate_hit_grid(const Arguments& args)
{
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
  
    if (args.Length() != 3)
      return ThrowException(Exception::Error(
        String::New("please provide layer idx, step, join_field")));
  
    if ((!args[0]->IsNumber() || !args[1]->IsNumber()))
        return ThrowException(Exception::TypeError(
           String::New("layer idx and step must be integers")));
  
    if ((!args[2]->IsString()))
        return ThrowException(Exception::TypeError(
           String::New("layer join_field must be a string")));
  
    // need cast from double to int
    unsigned int layer_idx = args[0]->NumberValue();
    unsigned int step = args[1]->NumberValue();
    std::string  const& join_field = TOSTR(args[2]);
    unsigned int tile_size = m->map_->width();
  
    //Local<Array> a = Array::New(step*tile_size*step);
    
    std::string prev("");
    std::string next("");
    unsigned int count(0);
    bool first = true;
    std::ostringstream s("");
  
    std::vector<mapnik::layer> const& layers = m->map_->layers();
    
    if (!layer_idx < layers.size())
        return ThrowException(Exception::Error(
           String::New("layer idx not valid")));
  
    mapnik::layer const& layer = layers[layer_idx];    
    double tol;
    double z = 0;
    mapnik::CoordTransform tr = m->map_->view_transform();
  
    const mapnik::box2d<double>&  e = m->map_->get_current_extent();
  
    double minx = e.minx();
    double miny = e.miny();
    double maxx = e.maxx();
    double maxy = e.maxy();
    
    try
    {
        mapnik::projection dest(m->map_->srs());
        mapnik::projection source(layer.srs());
        mapnik::proj_transform prj_trans(source,dest);
        prj_trans.backward(minx,miny,z);
        prj_trans.backward(maxx,maxy,z);
        
        
        tol = (maxx - minx) / m->map_->width() * 3;
        mapnik::datasource_ptr ds = layer.datasource();
        
        //mapnik::featureset_ptr fs;
    
        //mapnik::memory_datasource cache;
        mapnik::box2d<double> bbox = mapnik::box2d<double>(minx,miny,maxx,maxy);
        #if MAPNIK_VERSION >= 800
            mapnik::query q(ds->envelope());
        #else
            mapnik::query q(ds->envelope(),1.0,1.0);
        #endif
    
        q.add_property_name(join_field);
        mapnik::featureset_ptr fs = ds->features(q);
    
        /*
        if (fs)
        {   
            mapnik::feature_ptr feature;
            while ((feature = fs->next()))
            {                  
                cache.push(feature);
            }
        }
        */
        
        for (unsigned y=0;y<tile_size;y=y+step)
        {
            for (unsigned x=0;x<tile_size;x=x+step)
            {
                //std::clog << "x: " << x << " y:" << y << "\n";
                // .8 (avoid opening index) -> 1.2 sec unindexed
                // .3 indexed
                mapnik::featureset_ptr fs_hit = m->map_->query_map_point(layer_idx,x,y);
  
                std::string val;
                bool added = false;
                
                /*
                double x0 = x;
                double y0 = y;
                tr.backward(&x0,&y0);
                prj_trans.backward(x0,y0,z);
    
                mapnik::box2d<double> box(x0,y0,x0,y0);
                mapnik::featureset_ptr fs_hit;
                
                // nothing
                mapnik::featureset_ptr fs = ds->features_at_point(mapnik::coord2d(x,y));
                if (fs) 
                    fs_hit = mapnik::featureset_ptr(new mapnik::filter_featureset<mapnik::hit_test_filter>(fs,mapnik::hit_test_filter(x,y,tol)));
                    
                */
    
                // .7 sec
                //fs_hit = mapnik::featureset_ptr(new mapnik::memory_featureset(box, cache));
                if (fs_hit)
                {
                    mapnik::feature_ptr fp = fs_hit->next();
                    if (fp)
                    {
                        added = true;
                        std::map<std::string,mapnik::value> const& fprops = fp->props();
                        std::map<std::string,mapnik::value>::const_iterator const& itr = fprops.find(join_field);
                        if (itr != fprops.end())
                        {
                            val = itr->second.to_string();
                            //a->Set(x+y, String::New((const char*)itr->second.to_string().c_str()));
                        }
                        else
                        {
                            return ThrowException(Exception::Error(
                               String::New("Invalid key!")));    
                        }
                        
                    }
                }
                if (!added)
                {
                    val = "";
                }
                if (first)
                {
                    first = false;
                }
                if (!first && (val != prev))
                {
                    s << count << ":" << prev << "|";
                    count = 0;
                }
                prev = val;
                ++count;
            }
        }
    }
    catch (const mapnik::proj_init_error & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
  
    return scope.Close(String::New(s.str().c_str()));
}
