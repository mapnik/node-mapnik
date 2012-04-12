#ifndef __NODE_MAPNIK_UTILS_H__
#define __NODE_MAPNIK_UTILS_H__

// v8
#include <v8.h>

// node
#include <node.h>

// stl
#include <string>

// core types
#include <mapnik/unicode.hpp>
#include <mapnik/value.hpp>

// boost
#include <boost/variant/static_visitor.hpp>

#define TOSTR(obj) (*String::Utf8Value((obj)->ToString()))

#define FUNCTION_ARG(I, VAR)                                            \
    if (args.Length() <= (I) || !args[I]->IsFunction())                 \
        return ThrowException(Exception::TypeError(                     \
                                  String::New("Argument " #I " must be a function"))); \
    Local<Function> VAR = Local<Function>::Cast(args[I]);

#define ATTR(t, name, get, set)                                         \
    t->InstanceTemplate()->SetAccessor(String::NewSymbol(name), get, set);

#define NODE_MAPNIK_DEFINE_CONSTANT(target, name, constant)     \
    (target)->Set(v8::String::NewSymbol(name),                  \
                  v8::Integer::New(constant),                   \
                  static_cast<v8::PropertyAttribute>(           \
                      v8::ReadOnly|v8::DontDelete));


using namespace v8;
using namespace node;

namespace node_mapnik {

// adapted to work for both mapnik features and mapnik parameters
struct params_to_object : public boost::static_visitor<>
{
public:
    params_to_object( Local<Object>& ds, std::string key):
        ds_(ds),
        key_(key) {}

    void operator () ( int val )
    {
        ds_->Set(String::NewSymbol(key_.c_str()), Integer::New(val) );
    }

    void operator () ( bool val )
    {
        ds_->Set(String::NewSymbol(key_.c_str()), Boolean::New(val) );
    }

    void operator () ( double val )
    {
        ds_->Set(String::NewSymbol(key_.c_str()), Number::New(val) );
    }

    void operator () ( std::string const& val )
    {

        ds_->Set(String::NewSymbol(key_.c_str()), String::New(val.c_str()) );
    }

    void operator () ( UnicodeString const& val)
    {
        std::string buffer;
        mapnik::to_utf8(val,buffer);
        ds_->Set(String::NewSymbol(key_.c_str()), String::New(buffer.c_str()) );
    }

    void operator () ( mapnik::value_null const& val )
    {
        ds_->Set(String::NewSymbol(key_.c_str()), Undefined() );
    }

private:
    Local<Object>& ds_;
    std::string key_;
};

struct value_converter: public boost::static_visitor<Handle<Value> >
{
    Handle<Value> operator () ( int val ) const
    {
        return Integer::New(val);
    }

    Handle<Value> operator () ( bool val ) const
    {
        return Boolean::New(val);
    }

    Handle<Value> operator () ( double val ) const
    {
        return Number::New(val);
    }

    Handle<Value> operator () ( std::string const& val ) const
    {
        return String::New(val.c_str());
    }

    Handle<Value> operator () ( UnicodeString const& val) const
    {
        std::string buffer;
        mapnik::to_utf8(val,buffer);
        return String::New(buffer.c_str());
    }

    Handle<Value> operator () ( mapnik::value_null const& val ) const
    {
        return Undefined();
    }
};

}
#endif
