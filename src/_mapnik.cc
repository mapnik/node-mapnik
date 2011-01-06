// v8
#include <v8.h>

// node
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

/* dlopen(), dlsym() */
#include <dlfcn.h> 

// mapnik
#include <mapnik/map.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/font_engine_freetype.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/filter_factory.hpp>
#include <mapnik/color_factory.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/config_error.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/query.hpp>
#include <mapnik/filter_featureset.hpp>
#include <mapnik/hit_test_filter.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/memory_featureset.hpp>
#include <mapnik/version.hpp>

// boost
#include <boost/shared_ptr.hpp>
#include <boost/version.hpp>


//#ifdef MAPNIK_THREADSAFE
//#include <boost/thread/mutex.hpp>
//#endif

#if MAPNIK_VERSION >= 800
   #include <mapnik/box2d.hpp>
   #define Image32 image_32
   #define ImageData32 image_data_32
   #define Envelope box2d
#else
   #include <mapnik/envelope.hpp>
   #define get_current_extent getCurrentExtent
   #define width getWidth
   #define height getHeight
   #define zoom_to_box zoomToBox
   #define image_32 Image32
   #define image_data_32 ImageData32
   #define box2d Envelope
   #define layer Layer
   #define set_background setBackground
#endif

using namespace node;
using namespace v8;

#define TOSTR(obj) (*String::Utf8Value((obj)->ToString()))

#define FUNCTION_ARG(I, VAR)                                            \
  if (args.Length() <= (I) || !args[I]->IsFunction())                   \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a function")));  \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

typedef boost::shared_ptr<mapnik::Map> map_ptr;
//typedef mapnik::Map * map_ptr;

class Map: ObjectWrap
{
private:
  map_ptr map_;
public:

  static Persistent<FunctionTemplate> m_template;
  static void Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(NewMap);

    m_template = Persistent<FunctionTemplate>::New(t);
    m_template->InstanceTemplate()->SetInternalFieldCount(1);
    m_template->SetClassName(String::NewSymbol("Map"));

    NODE_SET_PROTOTYPE_METHOD(m_template, "load", load);
    NODE_SET_PROTOTYPE_METHOD(m_template, "from_string", from_string);
    NODE_SET_PROTOTYPE_METHOD(m_template, "resize", resize);
    NODE_SET_PROTOTYPE_METHOD(m_template, "width", width);
    NODE_SET_PROTOTYPE_METHOD(m_template, "height", height);
    NODE_SET_PROTOTYPE_METHOD(m_template, "buffer_size", buffer_size);
    NODE_SET_PROTOTYPE_METHOD(m_template, "generate_hit_grid", generate_hit_grid);
    NODE_SET_PROTOTYPE_METHOD(m_template, "extent", extent);
    NODE_SET_PROTOTYPE_METHOD(m_template, "zoom_all", zoom_all);
    NODE_SET_PROTOTYPE_METHOD(m_template, "zoom_to_box", zoom_to_box);
    NODE_SET_PROTOTYPE_METHOD(m_template, "render", render);
    NODE_SET_PROTOTYPE_METHOD(m_template, "render_to_string", render_to_string);
    NODE_SET_PROTOTYPE_METHOD(m_template, "render_to_file", render_to_file);
    //m_template->PrototypeTemplate()->SetNamedPropertyHandler(property);

    target->Set(String::NewSymbol("Map"),m_template->GetFunction());
    //eio_set_max_poll_reqs(10);
    //eio_set_min_parallel(10);
  }

  Map(int width, int height) :
    map_(new mapnik::Map(width,height)) {}
  
  Map(int width, int height, std::string srs) :
    map_(new mapnik::Map(width,height,srs)) {}

  ~Map()
  {
    std::clog << "~Map\n";
    //delete map_;
  }

  static Handle<Value> NewMap(const Arguments& args)
  {
    HandleScope scope;

    if (!(args.Length() > 1 && args.Length() < 4))
      return ThrowException(Exception::TypeError(
        String::New("please provide Map width and height and optional srs")));

    if (args.Length() == 2) {
        Map* m = new Map(args[0]->IntegerValue(),args[1]->IntegerValue());
        m->Wrap(args.This());
    }
    else {
        Map* m = new Map(args[0]->IntegerValue(),args[1]->IntegerValue(),TOSTR(args[2]));
        m->Wrap(args.This());
    }
    return args.This();
  }

  /*
  static Handle<Value> property(Local<String> attr,
      const AccessorInfo& name)
  {
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(name.This());
    std::string a = TOSTR(attr);
    if (a == "width")
    {
        Local<Value> width = Integer::New(m->map_->width());
        return scope.Close(width);
    }
    else if (a == "height")
    {
        Local<Value> width = Integer::New(m->map_->width());
        return scope.Close(width);
    }
    return Undefined();
  }
  */
        
  static Handle<Value> resize(const Arguments& args)
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


  static Handle<Value> width(const Arguments& args)
  {
    HandleScope scope;
    if (!args.Length() == 0)
      return ThrowException(Exception::Error(
        String::New("accepts no arguments")));

    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    Local<Value> width = Integer::New(m->map_->width());
    return scope.Close(width);
  }

  static Handle<Value> height(const Arguments& args)
  {
    HandleScope scope;
    if (!args.Length() == 0)
      return ThrowException(Exception::Error(
        String::New("accepts no arguments")));

    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    Local<Value> width = Integer::New(m->map_->height());
    return scope.Close(width);
  }

  static Handle<Value> buffer_size(const Arguments& args)
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
  
  static Handle<Value> load(const Arguments& args)
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

  static Handle<Value> from_string(const Arguments& args)
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
  
  static Handle<Value> generate_hit_grid(const Arguments& args)
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

    unsigned int layer_idx = args[0]->NumberValue();
    unsigned int step = args[1]->NumberValue();
    std::string  const& join_field = TOSTR(args[2]);
    unsigned int tile_size = m->map_->width();

    //Local<Array> a = Array::New(step*tile_size*step);
    
    std::string prev("");
    std::string next("");
    unsigned int count(0);
    bool first = true;
    std::stringstream s("");

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
        
        mapnik::featureset_ptr fs;
    
        mapnik::memory_datasource cache;
        mapnik::box2d<double> bbox = mapnik::box2d<double>(minx,miny,maxx,maxy);
        #if MAPNIK_VERSION >= 800
            mapnik::query q(ds->envelope());
        #else
            mapnik::query q(ds->envelope(),1.0,1.0);
        #endif
    
        q.add_property_name(join_field);
        fs = ds->features(q);
    
        if (fs)
        {   
            mapnik::feature_ptr feature;
            while ((feature = fs->next()))
            {                  
                cache.push(feature);
            }
        }
        
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
                count++;
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

  static Handle<Value> extent(const Arguments& args)
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

  static Handle<Value> zoom_all(const Arguments& args)
  {
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    m->map_->zoom_all();
    return Undefined();
  }

  static Handle<Value> zoom_to_box(const Arguments& args)
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

  struct map_baton_t {
    Map *m;
    std::string format;
    mapnik::box2d<double> bbox;
    std::string im_string;
    //boost::mutex mutex;
    Persistent<Function> cb;
  };

  static Handle<Value> render(const Arguments& args)
  {
    HandleScope scope;

    /*
    std::clog << "eio_nreqs" << eio_nreqs() << "\n";
    std::clog << "eio_nready" << eio_nready() << "\n";
    std::clog << "eio_npending" << eio_npending() << "\n";
    std::clog << "eio_nthreads" << eio_nthreads() << "\n";
    */
    
    FUNCTION_ARG(1, cb);

    double minx;
    double miny;
    double maxx;
    double maxy;

    if (args.Length() == 2)
    {
        if (!args[0]->IsArray())
            return ThrowException(Exception::Error(
               String::New("Must provide an array of: [minx,miny,maxx,maxy] and a callback function")));
        Local<Array> a = Local<Array>::Cast(args[0]);
        minx = a->Get(0)->NumberValue();
        miny = a->Get(1)->NumberValue();
        maxx = a->Get(2)->NumberValue();
        maxy = a->Get(3)->NumberValue();
        
    }   
    else
      return ThrowException(Exception::Error(
        String::New("Must provide an array of: [minx,miny,maxx,maxy]")));

    Map* m = ObjectWrap::Unwrap<Map>(args.This());

    map_baton_t *baton = new map_baton_t();

    if (!baton) {
      V8::LowMemoryNotification();
      return ThrowException(Exception::Error(
            String::New("Could not allocate enough memory")));
    }
  
    baton->m = m;
    baton->format = "png";
    baton->bbox = mapnik::box2d<double>(minx,miny,maxx,maxy);
    baton->cb = Persistent<Function>::New(cb);

    m->Ref();

    eio_custom(EIO_render, EIO_PRI_DEFAULT, EIO_render_follow, baton);
    ev_ref(EV_DEFAULT_UC);

    return Undefined();
  }

  static int EIO_render(eio_req *req)
  {
    //HandleScope scope;
    // no handlescope here - will cause odd segfaults
    map_baton_t *baton = static_cast<map_baton_t *>(req->data);
    //boost::mutex::scoped_lock lock(baton->mutex);
    baton->m->map_->zoom_to_box(baton->bbox);
    mapnik::image_32 im(baton->m->map_->width(),baton->m->map_->height());
    mapnik::agg_renderer<mapnik::image_32> ren(*baton->m->map_,im);
    ren.apply();
    baton->im_string = save_to_string(im, baton->format);
    return 0;
  }

  static int EIO_render_follow(eio_req *req)
  {
    HandleScope scope;
    map_baton_t *baton = static_cast<map_baton_t *>(req->data);
    ev_unref(EV_DEFAULT_UC);
    baton->m->Unref();

    Local<Value> argv[1];
    
#if NODE_VERSION_AT_LEAST(0,3,0)
    node::Buffer *retbuf = Buffer::New((char *)baton->im_string.data(),baton->im_string.size());
#else
    node::Buffer *retbuf = Buffer::New(baton->im_string.size());
    memcpy(retbuf->data(), baton->im_string.data(), baton->im_string.size());
#endif

    argv[0] = Local<Value>::New(retbuf->handle_);

    TryCatch try_catch;

    baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    baton->cb.Dispose();
    delete baton;
    scope.Close(retbuf->handle_);
    return 0;
  }
  
  static Handle<Value> render_to_string(const Arguments& args)
  {
    HandleScope scope;
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    mapnik::image_32 im(m->map_->width(),m->map_->height());
    mapnik::agg_renderer<mapnik::image_32> ren(*m->map_,im);
    ren.apply();
    
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

  static Handle<Value> render_to_file(const Arguments& args)
  {
    HandleScope scope;
    if (args.Length() != 1 || !args[0]->IsString())
      return ThrowException(Exception::TypeError(
        String::New("first argument must be a path to a mapnik stylesheet")));
        
    Map* m = ObjectWrap::Unwrap<Map>(args.This());
    std::string const& output = TOSTR(args[0]);

    mapnik::image_32 im(m->map_->width(),m->map_->height());
    mapnik::agg_renderer<mapnik::image_32> ren(*m->map_,im);
    try
    {
      ren.apply();
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

    mapnik::save_to_file<mapnik::image_data_32>(im.data(),output);
    return Undefined();
  }

};

Persistent<FunctionTemplate> Map::m_template;


class Projection: ObjectWrap
{
private:
  mapnik::projection* projection_;
public:

  static Persistent<FunctionTemplate> p_template;
  static void Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(NewProjection);

    p_template = Persistent<FunctionTemplate>::New(t);
    p_template->InstanceTemplate()->SetInternalFieldCount(1);
    p_template->SetClassName(String::NewSymbol("Projection"));

    NODE_SET_PROTOTYPE_METHOD(p_template, "forward", forward);
    NODE_SET_PROTOTYPE_METHOD(p_template, "inverse", inverse);

    target->Set(String::NewSymbol("Projection"),p_template->GetFunction());
  }

  Projection(std::string params) :
    projection_(new mapnik::projection(params)) {}

  ~Projection()
  {
    //std::clog << "~Projection\n";
    // delete projection_?
  }

  static Handle<Value> NewProjection(const Arguments& args)
  {
    HandleScope scope;

    if (!(args.Length() > 0 && args[0]->IsString()))
      return ThrowException(Exception::TypeError(
        String::New("please provide a proj4 intialization string")));
    try
    {
        Projection* p = new Projection(TOSTR(args[0]));
        p->Wrap(args.This());
        return args.This();
    }
    catch (const mapnik::proj_init_error & ex )
    {
      return ThrowException(Exception::Error(
        String::New(ex.what())));
    }
  }

  static Handle<Value> forward(const Arguments& args)
  {
    HandleScope scope;
    Projection* p = ObjectWrap::Unwrap<Projection>(args.This());

    if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
    else 
    {
        if (!args[0]->IsArray())
            return ThrowException(Exception::Error(
               String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));

        Local<Array> a = Local<Array>::Cast(args[0]);
        uint32_t array_length = a->Length();
        if (array_length == 2)
        {
            double x = a->Get(0)->NumberValue();
            double y = a->Get(1)->NumberValue();
            p->projection_->forward(x,y);
            Local<Array> a = Array::New(2);
            a->Set(0, Number::New(x));
            a->Set(1, Number::New(y));
            return scope.Close(a);
        }
        else if (array_length == 4)
        {
            double minx = a->Get(0)->NumberValue();
            double miny = a->Get(1)->NumberValue();
            double maxx = a->Get(2)->NumberValue();
            double maxy = a->Get(3)->NumberValue();
            p->projection_->forward(minx,miny);
            p->projection_->forward(maxx,maxy);
            Local<Array> a = Array::New(4);
            a->Set(0, Number::New(minx));
            a->Set(1, Number::New(miny));
            a->Set(2, Number::New(maxx));
            a->Set(3, Number::New(maxy));
            return scope.Close(a);
        }
        else
            return ThrowException(Exception::Error(
               String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
        
    }
  }

  static Handle<Value> inverse(const Arguments& args)
  {
    HandleScope scope;
    Projection* p = ObjectWrap::Unwrap<Projection>(args.This());

    if (!args.Length() == 1)
      return ThrowException(Exception::Error(
        String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
    else 
    {
        if (!args[0]->IsArray())
            return ThrowException(Exception::Error(
               String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));

        Local<Array> a = Local<Array>::Cast(args[0]);
        uint32_t array_length = a->Length();
        if (array_length == 2)
        {
            double x = a->Get(0)->NumberValue();
            double y = a->Get(1)->NumberValue();
            p->projection_->inverse(x,y);
            Local<Array> a = Array::New(2);
            a->Set(0, Number::New(x));
            a->Set(1, Number::New(y));
            return scope.Close(a);
        }
        else if (array_length == 4)
        {
            double minx = a->Get(0)->NumberValue();
            double miny = a->Get(1)->NumberValue();
            double maxx = a->Get(2)->NumberValue();
            double maxy = a->Get(3)->NumberValue();
            p->projection_->inverse(minx,miny);
            p->projection_->inverse(maxx,maxy);
            Local<Array> a = Array::New(4);
            a->Set(0, Number::New(minx));
            a->Set(1, Number::New(miny));
            a->Set(2, Number::New(maxx));
            a->Set(3, Number::New(maxy));
            return scope.Close(a);
        }
        else
            return ThrowException(Exception::Error(
               String::New("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")));
        
    }
  }
  
};

Persistent<FunctionTemplate> Projection::p_template;

static Handle<Value> make_mapnik_symbols_visible(const Arguments& args)
{
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    ThrowException(Exception::TypeError(
      String::New("first argument must be a path to a directory holding _mapnik.node")));
  String::Utf8Value filename(args[0]->ToString());
  void *handle = dlopen(*filename, RTLD_NOW|RTLD_GLOBAL);
  if (handle == NULL) {
      return False();
  }
  else
  {
      dlclose(handle);
      return True();
  }
}

static Handle<Value> register_datasources(const Arguments& args)
{
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    ThrowException(Exception::TypeError(
      String::New("first argument must be a path to a directory of mapnik input plugins")));
  std::string const& path = TOSTR(args[0]);
  mapnik::datasource_cache::instance()->register_datasources(path); 
  return Undefined();
}

static Handle<Value> available_input_plugins(const Arguments& args)
{
    HandleScope scope;
    std::vector<std::string> const& names = mapnik::datasource_cache::plugin_names();
    Local<Array> a = Array::New(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        a->Set(i, String::New(names[i].c_str()));
    }
    return scope.Close(a);
}

static Handle<Value> register_fonts(const Arguments& args)
{
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    ThrowException(Exception::TypeError(
      String::New("first argument must be a path to a directory of fonts")));
  std::string const& path = TOSTR(args[0]);
  mapnik::freetype_engine::register_fonts(path);
  return Undefined();
}

static Handle<Value> available_font_faces(const Arguments& args)
{
    HandleScope scope;
    std::vector<std::string> const& names = mapnik::freetype_engine::face_names();
    Local<Array> a = Array::New(names.size());
    for (unsigned i = 0; i < names.size(); ++i)
    {
        a->Set(i, String::New(names[i].c_str()));
    }
    return scope.Close(a);
}

std::string format_version(int version)
{
    std::ostringstream s;
    s << version/100000 << "." << version/100 % 1000  << "." << version % 100;
    return s.str();
}


extern "C" {

  static void init (Handle<Object> target)
  {
    // module level functions
    NODE_SET_METHOD(target, "make_mapnik_symbols_visible", make_mapnik_symbols_visible);
    NODE_SET_METHOD(target, "register_datasources", register_datasources);
    NODE_SET_METHOD(target, "datasources", available_input_plugins);
    NODE_SET_METHOD(target, "register_fonts", register_fonts);
    NODE_SET_METHOD(target, "fonts", available_font_faces);
    
    // clases
    
    // Map
    Map::Init(target);
    
    // Projection
    Projection::Init(target);    
    
    // node-mapnik version
    target->Set(String::NewSymbol("version"), String::New("0.1.2"));
    
    // node
    target->Set(String::NewSymbol("node_version"), String::New(NODE_VERSION));
    
    // mapnik
    target->Set(String::NewSymbol("mapnik_version"), Integer::New(MAPNIK_VERSION));
    target->Set(String::NewSymbol("mapnik_version_string"),
        String::New(format_version(MAPNIK_VERSION).c_str()));

    // boost 
    target->Set(String::NewSymbol("boost_version"), Integer::New(BOOST_VERSION));
    target->Set(String::NewSymbol("boost_version_string"),
        String::New(format_version(BOOST_VERSION).c_str()));

  }

  NODE_MODULE(_mapnik, init);
}
