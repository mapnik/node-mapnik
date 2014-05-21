#include "utils.hpp"
#include "mapnik_•.hpp"

#include MAPNIK_MAKE_SHARED_INCLUDE

Persistent<FunctionTemplate> •::constructor;

void •::Initialize(Handle<Object> target) {

    NanScope();

    Local<FunctionTemplate> lcons = NanNew<FunctionTemplate>(•::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(NanNew("•"));

    target->Set(NanNew("•"),constructor->GetFunction());
    NanAssignPersistent(constructor, lcons);
}

•::•(std::string const& name) :
ObjectWrap(),
    this_(MAPNIK_MAKE_SHARED<mapnik::•>(name)) {}

•::~•()
{
}

NAN_METHOD(•::New)
{
    NanScope();
}
