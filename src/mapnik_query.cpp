#include "utils.hpp"
#include "mapnik_query.hpp"

#include <boost/make_shared.hpp>

Persistent<FunctionTemplate> Query::constructor;

void Query::Initialize(Handle<Object> target) {

    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Query::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Query"));

    target->Set(String::NewSymbol("Query"),constructor->GetFunction());
}

Query::Query(mapnik::box2d<double> const& box) :
    ObjectWrap(),
    this_(boost::make_shared<mapnik::query>(box)) {}

Query::~Query()
{
}

Handle<Value> Query::New(const Arguments& args)
{
    HandleScope scope;
    // TODO - implement this
    return Undefined();
}
