
#include "utils.hpp"
#include "mapnik_layer.hpp"
#include "json_emitter.hpp"

#include <node_buffer.h>
#include <node_version.h>

// mapnik
#include <mapnik/layer.hpp>

Persistent<FunctionTemplate> Layer::constructor;

void Layer::Initialize(Handle<Object> target) {

    HandleScope scope;
  
    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Layer::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Layer"));

    // methods
    NODE_SET_PROTOTYPE_METHOD(constructor, "describe", describe);
    NODE_SET_PROTOTYPE_METHOD(constructor, "features", features);
    NODE_SET_PROTOTYPE_METHOD(constructor, "describe_data", describe_data);

    // properties
    ATTR(constructor, "name", get_prop, set_prop);
    ATTR(constructor, "srs", get_prop, set_prop);
    ATTR(constructor, "styles", get_prop, set_prop);
    ATTR(constructor, "datasource", get_prop, set_prop);

    target->Set(String::NewSymbol("Layer"),constructor->GetFunction());
}

Layer::Layer(std::string const& name) :
  ObjectWrap(),
  layer_(new mapnik::layer(name)) {}

Layer::Layer(std::string const& name, std::string const& srs) :
  ObjectWrap(),
  layer_(new mapnik::layer(name,srs)) {}

Layer::Layer(layer_ptr l) :
  ObjectWrap(),
  layer_() {
    layer_ = l;
  }


Layer::~Layer()
{
  // std::clog << "~Layer\n";
  // release is handled by boost::shared_ptr
}

Handle<Value> Layer::New(const Arguments& args)
{
  HandleScope scope;

  if (!args.IsConstructCall())
      return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));
    
  // accept a reference?
  if (args.Length() == 1)
  {
      if (!args[0]->IsString())
          ThrowException(Exception::Error(
             String::New("'name' must be a string")));          
      Layer* l = new Layer(TOSTR(args[0]));
      l->Wrap(args.This());
  }
  else if (args.Length() == 2)
  {
      if (!args[0]->IsString() || !args[1]->IsString())
          ThrowException(Exception::Error(
             String::New("'name' and 'srs' must be a strings")));
      Layer* l = new Layer(TOSTR(args[0]),TOSTR(args[1]));
      l->Wrap(args.This());  
  }
  else
  {
      return ThrowException(Exception::TypeError(
        String::New("please provide Layer name and optional srs")));
  }
  
  return args.This();
}

/*
v8::Handle wrapPoint(Point *value) { 
  v8::HandleScope scope;
  v8::Handle cons = pointConstructor->GetFunction();
  v8::Handle external = v8::External::New(value); 
  v8::Handle result = cons->NewInstance(1, &external); 
  return scope.Close(result);
}

v8::Handle constructorCall(const v8::Arguments& args)
{
  v8::HandleScope scope;
  v8::Handle external;
  if (args[0]->IsExternal()) 
  { 
     external = v8::Handle::Cast(args[0]); 
  } 
  else 
  { 
     int x = args[0]->Int32Value();
     int y = args[1]->Int32Value();
     Point *p = new Point(x, y);
     external = v8::External::New(p); 
  } 
  args.This()->SetInternalField(0, wrapper);
  return args.This(); 
  
  }
*/

/*
Layer* Layer::New(layer_ptr lp) {
  HandleScope scope;

  Local<Value> arg = String::New("doggy");
  //Layer* ll = Layer::New(lay_ptr);
  Layer* l = new Layer(lp);
  Local<Value> ext = External::New(l);
  Local<Object> b = constructor->GetFunction()->NewInstance(1, &ext);
  
  // http://www.mail-archive.com/v8-users@googlegroups.com/msg00630.html
  //FunctionTemplate->function->Call(<your object>, 0, empty argv);
  // http://create.tpsitulsa.com/blog/2009/01/29/v8-objects/

  //return ObjectWrap::Unwrap<Layer>(b);
  return ObjectWrap::Unwrap<Layer>(b);
}
*/

Handle<Value> Layer::New(mapnik::layer & lay_ref) {
  HandleScope scope;
  //std::clog << "a\n";
  // get Buffer from global scope.
  /*
  Local<Object> global = v8::Context::GetCurrent()->Global();
  //std::clog << "b\n";
  Local<Value> bv = global->Get(String::NewSymbol("Layer"));
  std::clog << "c\n";
  assert(bv->IsFunction());
  std::clog << "d\n";
  Local<Function> b = Local<Function>::Cast(bv);
  std::clog << "e\n";
  
  */

  //Local<Value> argv[1] = { Local<Value>::New(lp) };
  //Layer* l = new Layer(lp);
  //Layer* l = new Layer("wootang");
  //std::clog << "f\n";
  //Handle<Value> ext[1];
  //ext[0] = External::New(l);
  //Handle<Value> ext = Integer::New(3);
  //std::clog << "g\n";

  //V8EXPORT Local<Object> NewInstance() const;
  //V8EXPORT Local<Object> NewInstance(int argc, Handle<Value> argv[]) const;

  //baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);
  Handle<Value> ext = String::New("ruff");
  Handle<Object> obj = constructor->GetFunction()->NewInstance(1, &ext);
  //std::clog << "h\n";
  Layer* l = ObjectWrap::Unwrap<Layer>(obj);
  //std::clog << "i\n";
  *l->layer_ = lay_ref;
  //constructor->PrototypeTemplate()
  //Layer* l = new Layer(lp);
  //Layer* l = new Layer(lp);
  //obj->SetInternalField(0, External::New(l));
  //l->Wrap(obj);
  //std::clog << "j\n";
  return obj;

  //return scope.Close(obj);

  //Local<Object> instance = constructor->GetFunction()->NewInstance(1, &ext);
  //Local<Object> instance = b->NewInstance();//b->NewInstance(1, &ext);

  //return scope.Close(instance);
}
/*
Handle<Value> Map::add_layer(const Arguments &args) {
  HandleScope scope;
  Local<Object> obj = args[0]->ToObject();
  if (!Layer::constructor->HasInstance(obj))
    return ThrowException(Exception::TypeError(String::New("mapnik.Layer expected")));
  Layer *l = ObjectWrap::Unwrap<Layer>(obj);
  Map* m = ObjectWrap::Unwrap<Map>(args.This());
  // TODO - addLayer should be add_layer in mapnik
  l->layer_->addLayer(*l->get());
  return Undefined();
}
*/

Handle<Value> Layer::get_prop(Local<String> property,
                         const AccessorInfo& info)
{
  HandleScope scope;
  Layer* l = ObjectWrap::Unwrap<Layer>(info.This());
  std::string a = TOSTR(property);
  if (a == "name")
      return scope.Close(String::New(l->layer_->name().c_str()));
  else if (a == "srs")
      return scope.Close(String::New(l->layer_->srs().c_str()));
  else if (a == "styles") {
      std::vector<std::string> const& style_names = l->layer_->styles();
      Local<Array> s = Array::New(style_names.size());
      for (unsigned i = 0; i < style_names.size(); ++i)
      {
          s->Set(i, String::New(style_names[i].c_str()) );
      }
      return scope.Close(s);  
  }
  else if (a == "datasource") {
      // TODO - return mapnik.Datasource ?
      Local<Object> params = Object::New();
      return scope.Close(params);
  }
  return Undefined();
}

void Layer::set_prop(Local<String> property,
                         Local<Value> value,
                         const AccessorInfo& info)
{
  HandleScope scope;
  Layer* l = ObjectWrap::Unwrap<Layer>(info.Holder());
  std::string a = TOSTR(property);
  if (a == "name")
  {
      if (!value->IsString())
          ThrowException(Exception::Error(
             String::New("'name' must be a string")));          
      l->layer_->set_name(TOSTR(value));
  }
  else if (a == "srs")
  {
      if (!value->IsString())
          ThrowException(Exception::Error(
             String::New("'name' must be a string")));          
      l->layer_->set_srs(TOSTR(value));
  }
  else if (a == "styles")
  {
      if (!value->IsArray())
          ThrowException(Exception::Error(
             String::New("Must provide an array of style names")));
      Local<Array> a = Local<Array>::Cast(value->ToObject());
      // todo - how to check if cast worked?
      int i = 0;
      uint32_t a_length = a->Length();
      while (i < a_length) {
          l->layer_->add_style(TOSTR(a->Get(i)));
          i++;
      }
  }
  else if (a == "datasource")
  {

      /*
      Local<Object> obj = args[0]->ToObject();
      if (!Layer::constructor->HasInstance(obj))
        return ThrowException(Exception::TypeError(String::New("mapnik.Layer expected")));
      Layer *l = ObjectWrap::Unwrap<Layer>(obj);
      //l->layer_->zoom_to_box(box);
      */
  }
}


Handle<Value> Layer::describe(const Arguments& args)
{
  HandleScope scope;

  Layer* l = ObjectWrap::Unwrap<Layer>(args.This());

  Local<Object> description = Object::New();
  layer_as_json(description,*l->layer_);
  
  return scope.Close(description);

}

Handle<Value> Layer::describe_data(const Arguments& args)
{
  HandleScope scope;

  Layer* l = ObjectWrap::Unwrap<Layer>(args.This());

  //Local<Object> meta = Object::New();

  Local<Object> description = Object::New();
  layer_data_as_json(description,*l->layer_);
  //meta->Set(String::NewSymbol(layer.name().c_str()), description);
  
  return scope.Close(description);

}

Handle<Value> Layer::features(const Arguments& args)
{

  HandleScope scope;

  Layer* l = ObjectWrap::Unwrap<Layer>(args.This());

  // TODO - we don't know features.length at this point
  Local<Array> a = Array::New(0);
  mapnik::datasource_ptr ds = l->layer_->datasource();
  if (ds)
  {
      datasource_features(a,ds);
  }

  return scope.Close(a);

}

