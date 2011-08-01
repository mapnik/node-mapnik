
// node
#include <node_buffer.h>
#include <node_version.h>

// mapnik
#include <mapnik/grid/grid_view.hpp>

// boost
#include <boost/make_shared.hpp>

#include "mapnik_grid_view.hpp"
#include "js_grid_utils.hpp"
#include "utils.hpp"

// std
#include <exception>

Persistent<FunctionTemplate> GridView::constructor;

void GridView::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(GridView::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("GridView"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "encodeSync", encodeSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "encode", encode);
    NODE_SET_PROTOTYPE_METHOD(constructor, "width", width);
    NODE_SET_PROTOTYPE_METHOD(constructor, "height", height);

    target->Set(String::NewSymbol("GridView"),constructor->GetFunction());
}


GridView::GridView(grid_view_ptr gp) :
  ObjectWrap(),
  this_(gp) {}

GridView::~GridView()
{
}

Handle<Value> GridView::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        GridView* g =  static_cast<GridView*>(ptr);
        g->Wrap(args.This());
        return args.This();
    } else {
        return ThrowException(String::New("Cannot create this object from Javascript"));
    }

    return Undefined();
}

Handle<Value> GridView::New(boost::shared_ptr<mapnik::grid> grid_ptr,
    unsigned x,
    unsigned y,
    unsigned w,
    unsigned h
    )
{
    HandleScope scope;
    typedef boost::shared_ptr<mapnik::grid_view> grid_view_ptr_type;
    grid_view_ptr_type grid_view_ptr = boost::make_shared<mapnik::grid_view>(grid_ptr->get_view(x,y,w,h));
    GridView* gv = new GridView(grid_view_ptr);
    Handle<Value> ext = External::New(gv);
    Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
    return scope.Close(obj);
}

Handle<Value> GridView::width(const Arguments& args)
{
    HandleScope scope;

    GridView* g = ObjectWrap::Unwrap<GridView>(args.This());
    return scope.Close(Integer::New(g->get()->width()));
}

Handle<Value> GridView::height(const Arguments& args)
{
    HandleScope scope;

    GridView* g = ObjectWrap::Unwrap<GridView>(args.This());
    return scope.Close(Integer::New(g->get()->height()));
}


Handle<Value> GridView::encodeSync(const Arguments& args)
{
    HandleScope scope;

    GridView* g = ObjectWrap::Unwrap<GridView>(args.This());
    
    // defaults
    std::string format("utf");
    unsigned int resolution = 4;
    bool add_features = true;
    
    // accept custom format
    if (args.Length() >= 1){
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

        if (options->Has(String::New("resolution")))
        {
            Local<Value> bind_opt = options->Get(String::New("resolution"));
            if (!bind_opt->IsNumber())
              return ThrowException(Exception::TypeError(
                String::New("'resolution' must be an Integer")));
    
            resolution = bind_opt->IntegerValue();
        }

        if (options->Has(String::New("features")))
        {
            Local<Value> bind_opt = options->Get(String::New("features"));
            if (!bind_opt->IsBoolean())
              return ThrowException(Exception::TypeError(
                String::New("'features' must be an Boolean")));
    
            add_features = bind_opt->BooleanValue();
        }
    }
    
    try {
    
        Local<Array> grid_array = Array::New();
        std::vector<mapnik::grid_view::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid_view>(*g->get(),grid_array,key_order,resolution);
    
        // convert key order to proper javascript array
        Local<Array> keys_a = Array::New(key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it < key_order.end(); ++it, ++i)
        {
            keys_a->Set(i, String::New((*it).c_str()));
        }
    
        // gather feature data
        Local<Object> feature_data = Object::New();
        if (add_features) {
            node_mapnik::write_features<mapnik::grid_view>(*g->get(),
                           feature_data,
                           key_order
                           );
        }
        
        // Create the return hash.
        Local<Object> json = Object::New();
        json->Set(String::NewSymbol("grid"), grid_array);
        json->Set(String::NewSymbol("keys"), keys_a);
        json->Set(String::NewSymbol("data"), feature_data);
        return json;
        
    }
    catch (std::exception & ex)
    {
        return ThrowException(Exception::Error(
          String::New(ex.what())));
    }

}

Handle<Value> GridView::encode(const Arguments& args)
{
    HandleScope scope;

    GridView* g = ObjectWrap::Unwrap<GridView>(args.This());

    // defaults
    std::string format("utf");
    unsigned int resolution = 4;
    bool add_features = true;

    // accept custom format
    if (args.Length() >= 1){
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

        if (options->Has(String::New("resolution")))
        {
            Local<Value> bind_opt = options->Get(String::New("resolution"));
            if (!bind_opt->IsNumber())
              return ThrowException(Exception::TypeError(
                String::New("'resolution' must be an Integer")));

            resolution = bind_opt->IntegerValue();
        }

        if (options->Has(String::New("features")))
        {
            Local<Value> bind_opt = options->Get(String::New("features"));
            if (!bind_opt->IsBoolean())
              return ThrowException(Exception::TypeError(
                String::New("'features' must be an Boolean")));

            add_features = bind_opt->BooleanValue();
        }
    }

    // ensure callback is a function
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                  String::New("last argument must be a callback function")));
    Local<Function> callback = Local<Function>::Cast(args[args.Length()-1]);

    try {

        Local<Array> grid_array = Array::New();
        std::vector<mapnik::grid_view::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid_view>(*g->get(),grid_array,key_order,resolution);

        // convert key order to proper javascript array
        Local<Array> keys_a = Array::New(key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it < key_order.end(); ++it, ++i)
        {
            keys_a->Set(i, String::New((*it).c_str()));
        }

        // gather feature data
        Local<Object> feature_data = Object::New();
        if (add_features) {
            node_mapnik::write_features<mapnik::grid_view>(*g->get(),
                           feature_data,
                           key_order
                           );
        }

        // Create the return hash.
        Local<Object> json = Object::New();
        json->Set(String::NewSymbol("grid"), grid_array);
        json->Set(String::NewSymbol("keys"), keys_a);
        json->Set(String::NewSymbol("data"), feature_data);

        TryCatch try_catch;
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(json) };
        callback->Call(Context::GetCurrent()->Global(), 2, argv);
        if (try_catch.HasCaught()) {
            FatalException(try_catch);
        }
    }
    catch (std::exception & ex)
    {
        Local<Value> argv[1] = { Exception::Error(String::New(ex.what())) };
        callback->Call(Context::GetCurrent()->Global(), 1, argv);
    }

    return scope.Close(Undefined());
}
