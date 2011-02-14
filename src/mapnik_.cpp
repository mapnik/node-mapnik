#include "mapnik_•.hpp"

Persistent<FunctionTemplate> •::constructor;

void •::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(•::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("•"));

    target->Set(String::NewSymbol("•"),constructor->GetFunction());
}

•::•(std::string const& name) :
  ObjectWrap(),
  this_(new mapnik::•(name)) {}

•::~•()
{
}

Handle<Value> •::New(const Arguments& args)
{
  HandleScope scope;
}
