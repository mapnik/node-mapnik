#include "utils.hpp"
#include "mapnik_•.hpp"
#include <memory>


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
    this_(std::make_shared<mapnik::•>(name)) {}

•::~•()
{
}

NAN_METHOD(•::New)
{
    NanScope();
}
