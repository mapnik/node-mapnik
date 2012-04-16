
// node
#include <node_buffer.h>

// mapnik
#include <mapnik/image_data.hpp>

// boost
#include <boost/make_shared.hpp>

#include "mapnik_grid.hpp"
#include "mapnik_grid_view.hpp"
#include "js_grid_utils.hpp"
#include "utils.hpp"

// std
#include <exception>

Persistent<FunctionTemplate> Grid::constructor;

void Grid::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Grid::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Grid"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(constructor, "encodeSync", encodeSync);
    NODE_SET_PROTOTYPE_METHOD(constructor, "encode", encode);
    NODE_SET_PROTOTYPE_METHOD(constructor, "fields", fields);
    NODE_SET_PROTOTYPE_METHOD(constructor, "view", view);
    NODE_SET_PROTOTYPE_METHOD(constructor, "width", width);
    NODE_SET_PROTOTYPE_METHOD(constructor, "height", height);
    NODE_SET_PROTOTYPE_METHOD(constructor, "painted", painted);

    // properties
    ATTR(constructor, "key", get_prop, set_prop);

    target->Set(String::NewSymbol("Grid"),constructor->GetFunction());
}

Grid::Grid(unsigned int width, unsigned int height, std::string const& key, unsigned int resolution) :
    ObjectWrap(),
    this_(boost::make_shared<mapnik::grid>(width,height,key,resolution)) {
    V8::AdjustAmountOfExternalAllocatedMemory(width * height);
}

Grid::Grid(grid_ptr this_) :
    ObjectWrap(),
    this_(this_) {
    V8::AdjustAmountOfExternalAllocatedMemory(this_->width() * this_->height());
    }

Grid::~Grid()
{
    V8::AdjustAmountOfExternalAllocatedMemory(-4 * this_->width() * this_->height());
}

Handle<Value> Grid::New(const Arguments& args)
{
    HandleScope scope;
    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

    if (args[0]->IsExternal())
    {
        //std::clog << "external!\n";
        Local<External> ext = Local<External>::Cast(args[0]);
        void* ptr = ext->Value();
        Grid* g =  static_cast<Grid*>(ptr);
        g->Wrap(args.This());
        return args.This();
    }

    if (args.Length() >= 2)
    {
        if (!args[0]->IsNumber() || !args[1]->IsNumber())
            return ThrowException(Exception::TypeError(
                                      String::New("Grid 'width' and 'height' must be a integers")));

        // defaults
        std::string key("__id__");
        unsigned int resolution = 1;

        if (args.Length() >= 3) {

            if (!args[2]->IsObject())
                return ThrowException(Exception::TypeError(
                                          String::New("optional third arg must be an options object")));
            Local<Object> options = args[2]->ToObject();

            if (options->Has(String::New("key"))) {
                Local<Value> bind_opt = options->Get(String::New("key"));
                if (!bind_opt->IsString())
                    return ThrowException(Exception::TypeError(
                                              String::New("optional arg 'key' must be an string")));

                key = TOSTR(bind_opt);
            }
            // TODO - remove, deprecated
            if (options->Has(String::New("resolution"))) {
                Local<Value> bind_opt = options->Get(String::New("resolution"));
                if (!bind_opt->IsNumber())
                    return ThrowException(Exception::TypeError(
                                              String::New("optional arg 'resolution' must be an string")));

                resolution = bind_opt->IntegerValue();
            }
        }

        Grid* g = new Grid(args[0]->IntegerValue(),args[1]->IntegerValue(),key,resolution);
        g->Wrap(args.This());
        return args.This();
    }
    else
    {
        return ThrowException(Exception::Error(
                                  String::New("please provide Grid width and height")));
    }
    return Undefined();
}

Handle<Value> Grid::painted(const Arguments& args)
{
    HandleScope scope;

    Grid* g = ObjectWrap::Unwrap<Grid>(args.This());
    return scope.Close(Boolean::New(g->get()->painted()));
}

Handle<Value> Grid::width(const Arguments& args)
{
    HandleScope scope;

    Grid* g = ObjectWrap::Unwrap<Grid>(args.This());
    return scope.Close(Integer::New(g->get()->width()));
}

Handle<Value> Grid::height(const Arguments& args)
{
    HandleScope scope;

    Grid* g = ObjectWrap::Unwrap<Grid>(args.This());
    return scope.Close(Integer::New(g->get()->height()));
}

Handle<Value> Grid::get_prop(Local<String> property,
                             const AccessorInfo& info)
{
    HandleScope scope;
    Grid* g = ObjectWrap::Unwrap<Grid>(info.Holder());
    std::string a = TOSTR(property);
    if (a == "key")
        return scope.Close(String::New(g->get()->get_key().c_str()));
    return Undefined();
}

void Grid::set_prop(Local<String> property,
                    Local<Value> value,
                    const AccessorInfo& info)
{
    HandleScope scope;
    Grid* g = ObjectWrap::Unwrap<Grid>(info.Holder());
    std::string a = TOSTR(property);
    if (a == "key") {
        if (!value->IsNumber())
            ThrowException(Exception::TypeError(
                               String::New("width must be an integer")));
        g->get()->set_key(TOSTR(value));
    }
}


Handle<Value> Grid::fields(const Arguments& args)
{
    HandleScope scope;

    Grid* g = ObjectWrap::Unwrap<Grid>(args.This());
    std::set<std::string> const& a = g->get()->property_names();
    std::set<std::string>::const_iterator itr = a.begin();
    std::set<std::string>::const_iterator end = a.end();
    Local<Array> l = Array::New(a.size());
    int idx = 0;
    for (; itr != end; ++itr)
    {
        std::string name = *itr;
        l->Set(idx, String::New(name.c_str()));
        ++idx;
    }
    return scope.Close(l);

}

Handle<Value> Grid::view(const Arguments& args)
{
    HandleScope scope;

    if ( (!args.Length() == 4) || (!args[0]->IsNumber() && !args[1]->IsNumber() && !args[2]->IsNumber() && !args[3]->IsNumber() ))
        return ThrowException(Exception::TypeError(
                                  String::New("requires 4 integer arguments: x, y, width, height")));

    unsigned x = args[0]->IntegerValue();
    unsigned y = args[1]->IntegerValue();
    unsigned w = args[2]->IntegerValue();
    unsigned h = args[3]->IntegerValue();

    Grid* g = ObjectWrap::Unwrap<Grid>(args.This());
    return scope.Close(GridView::New(g->get(),x,y,w,h));
}


Handle<Value> Grid::encodeSync(const Arguments& args) // format, resolution
{
    HandleScope scope;

    Grid* g = ObjectWrap::Unwrap<Grid>(args.This());

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

        boost::ptr_vector<uint16_t> lines;
        std::vector<mapnik::grid::lookup_type> key_order;
        node_mapnik::grid2utf<mapnik::grid>(*g->get(),lines,key_order,resolution);

        // convert key order to proper javascript array
        Local<Array> keys_a = Array::New(key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = key_order.begin(), i = 0; it < key_order.end(); ++it, ++i)
        {
            keys_a->Set(i, String::New((*it).c_str()));
        }

        mapnik::grid const& grid_type = *g->get();

        // gather feature data
        Local<Object> feature_data = Object::New();
        if (add_features) {
            node_mapnik::write_features<mapnik::grid>(*g->get(),
                                                      feature_data,
                                                      key_order);
        }

        // Create the return hash.
        Local<Object> json = Object::New();
        Local<Array> grid_array = Array::New();
        unsigned array_size = static_cast<unsigned int>(grid_type.width()/resolution);
        for (unsigned j=0;j<lines.size();++j)
        {
            grid_array->Set(j,String::New(&lines[j],array_size));
        }
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

typedef struct {
    uv_work_t request;
    Grid* g;
    std::string format;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    boost::ptr_vector<uint16_t> lines;
    unsigned int resolution;
    bool add_features;
    std::vector<mapnik::grid::lookup_type> key_order;
} encode_grid_baton_t;

Handle<Value> Grid::encode(const Arguments& args) // format, resolution
{
    HandleScope scope;

    Grid* g = ObjectWrap::Unwrap<Grid>(args.This());

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

    encode_grid_baton_t *closure = new encode_grid_baton_t();
    closure->request.data = closure;
    closure->g = g;
    closure->format = format;
    closure->error = false;
    closure->resolution = resolution;
    closure->add_features = add_features;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    // todo - reserve lines size?
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, EIO_AfterEncode);
    uv_ref(uv_default_loop());
    g->Ref();
    return Undefined();
}

void Grid::EIO_Encode(uv_work_t* req)
{
    encode_grid_baton_t *closure = static_cast<encode_grid_baton_t *>(req->data);

    try
    {
        node_mapnik::grid2utf<mapnik::grid>(*closure->g->get(),
                                            closure->lines,
                                            closure->key_order,
                                            closure->resolution);
    }
    catch (std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happened when encoding grid: please file bug report";
    }
}

void Grid::EIO_AfterEncode(uv_work_t* req)
{
    HandleScope scope;

    encode_grid_baton_t *closure = static_cast<encode_grid_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    } else {

        // convert key order to proper javascript array
        Local<Array> keys_a = Array::New(closure->key_order.size());
        std::vector<std::string>::iterator it;
        unsigned int i;
        for (it = closure->key_order.begin(), i = 0; it < closure->key_order.end(); ++it, ++i)
        {
            keys_a->Set(i, String::New((*it).c_str()));
        }

        mapnik::grid const& grid_type = *closure->g->get();
        // gather feature data
        Local<Object> feature_data = Object::New();
        if (closure->add_features) {
            node_mapnik::write_features<mapnik::grid>(grid_type,
                                                      feature_data,
                                                      closure->key_order);
        }

        // Create the return hash.
        Local<Object> json = Object::New();
        Local<Array> grid_array = Array::New(closure->lines.size());
        unsigned array_size = static_cast<unsigned int>(grid_type.width()/closure->resolution);
        for (unsigned j=0;j<closure->lines.size();++j)
        {
            grid_array->Set(j,String::New(&closure->lines[j],array_size));
        }
        json->Set(String::NewSymbol("grid"), grid_array);
        json->Set(String::NewSymbol("keys"), keys_a);
        json->Set(String::NewSymbol("data"), feature_data);

        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(json) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }

    uv_unref(uv_default_loop());
    closure->g->Unref();
    closure->cb.Dispose();
    delete closure;
}
