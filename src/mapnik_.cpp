#include "utils.hpp"
#include "mapnik_•.hpp"

#include MAPNIK_MAKE_SHARED_INCLUDE

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
    this_(MAPNIK_MAKE_SHARED<mapnik::•>(name)) {}

•::~•()
{
}

Handle<Value> •::New(const Arguments& args)
{
    HandleScope scope;
}
