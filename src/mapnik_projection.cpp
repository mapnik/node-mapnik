// boost
#include <boost/make_shared.hpp>

#include "mapnik_projection.hpp"
#include "utils.hpp"

Persistent<FunctionTemplate> Projection::constructor;

void Projection::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Projection::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Projection"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "forward", forward);
    NODE_SET_PROTOTYPE_METHOD(constructor, "inverse", inverse);

    target->Set(String::NewSymbol("Projection"),constructor->GetFunction());
}

Projection::Projection(std::string const& name) :
  ObjectWrap(),
  projection_(boost::make_shared<mapnik::projection>(name)) {}

Projection::~Projection()
{
}

Handle<Value> Projection::New(const Arguments& args)
{
  HandleScope scope;

    if (!args.IsConstructCall())
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));

  if (!args.Length() > 0 || !args[0]->IsString()) {
      return ThrowException(Exception::TypeError(
        String::New("please provide a proj4 intialization string")));
  }
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

Handle<Value> Projection::forward(const Arguments& args)
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

Handle<Value> Projection::inverse(const Arguments& args)
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

