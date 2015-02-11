#ifndef __NODE_MAPNIK_UTILS_H__
#define __NODE_MAPNIK_UTILS_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

// stl
#include <string>
#include <memory>

// core types
#include <mapnik/unicode.hpp>
#include <mapnik/value_types.hpp>
#include <mapnik/value.hpp>
#include <mapnik/version.hpp>

#define TOSTR(obj) (*String::Utf8Value((obj)->ToString()))

#define FUNCTION_ARG(I, VAR)                                            \
    if (args.Length() <= (I) || !args[I]->IsFunction()) {               \
        NanThrowTypeError("Argument " #I " must be a function");        \
        NanReturnUndefined();                                           \
    }                                                                   \
    Local<Function> VAR = args[I].As<Function>();

#define ATTR(t, name, get, set)                                         \
    t->InstanceTemplate()->SetAccessor(NanNew(name), get, set);

#define NODE_MAPNIK_DEFINE_CONSTANT(target, name, constant)             \
    (target)->Set(NanNew(name),                                         \
                  NanNew<Integer>(constant));                            \

#define NODE_MAPNIK_DEFINE_64_BIT_CONSTANT(target, name, constant)      \
    (target)->Set(NanNew(name),                                         \
                  NanNew<Number>(constant));                            \


using namespace v8;

namespace node_mapnik {

typedef mapnik::value_integer value_integer;

// adapted to work for both mapnik features and mapnik parameters
struct params_to_object : public mapnik::util::static_visitor<>
{
public:
    params_to_object( Local<Object>& ds, std::string key):
        ds_(ds),
        key_(key) {}

    void operator () ( value_integer val )
    {
        ds_->Set(NanNew(key_.c_str()), NanNew<Number>(val) );
    }

    void operator () ( bool val )
    {
        ds_->Set(NanNew(key_.c_str()), NanNew<Boolean>(val) );
    }

    void operator () ( double val )
    {
        ds_->Set(NanNew(key_.c_str()), NanNew<Number>(val) );
    }

    void operator () ( std::string const& val )
    {

        ds_->Set(NanNew(key_.c_str()), NanNew<String>(val.c_str()) );
    }

    void operator () ( mapnik::value_unicode_string const& val)
    {
        std::string buffer;
        mapnik::to_utf8(val,buffer);
        ds_->Set(NanNew(key_.c_str()), NanNew<String>(buffer.c_str()) );
    }

    void operator () ( mapnik::value_null const& )
    {
        ds_->Set(NanNew(key_.c_str()), NanNull() );
    }

private:
    Local<Object>& ds_;
    std::string key_;
};

struct value_converter: public mapnik::util::static_visitor<Handle<Value> >
{
    Handle<Value> operator () ( value_integer val ) const
    {
        return NanNew<Number>(val);
    }

    Handle<Value> operator () ( bool val ) const
    {
        return NanNew<Boolean>(val);
    }

    Handle<Value> operator () ( double val ) const
    {
        return NanNew<Number>(val);
    }

    Handle<Value> operator () ( std::string const& val ) const
    {
        return NanNew<String>(val.c_str());
    }

    Handle<Value> operator () ( mapnik::value_unicode_string const& val) const
    {
        std::string buffer;
        mapnik::to_utf8(val,buffer);
        return NanNew<String>(buffer.c_str());
    }

    Handle<Value> operator () ( mapnik::value_null const& ) const
    {
        return NanUndefined();
    }
};

}
#endif
