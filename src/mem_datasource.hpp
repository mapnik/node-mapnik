#ifndef __NODE_MAPNIK_MEM_DATASOURCE_H__
#define __NODE_MAPNIK_MEM_DATASOURCE_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

using namespace v8;
using namespace node;

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/query.hpp>
#include <mapnik/params.hpp>
#include <mapnik/sql_utils.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/feature_factory.hpp>

// boost
#include <boost/utility.hpp>
#include <boost/scoped_ptr.hpp>

// stl
#include <vector>
#include <algorithm>



class js_datasource : public mapnik::datasource
{
    friend class js_featureset;
public:
    js_datasource(const mapnik::parameters &params, bool bind, Local<Value> cb);
    virtual ~js_datasource();
    int type() const;
    mapnik::featureset_ptr features(const mapnik::query& q) const;
    mapnik::featureset_ptr features_at_point(mapnik::coord2d const& pt) const;
    mapnik::box2d<double> envelope() const;
    mapnik::layer_descriptor get_descriptor() const;
    size_t size() const;
    Persistent<Function> cb_;
private:
    int type_;
    mutable mapnik::layer_descriptor desc_;
    mapnik::box2d<double> ext_;
}; 


js_datasource::js_datasource(const mapnik::parameters &params, bool bind, Local<Value> cb)
    : datasource (params),
      type_(datasource::Vector),
      desc_(*params.get<std::string>("type"), *params.get<std::string>("encoding","utf-8"))
{
    cb_ = Persistent<Function>::New(Handle<Function>::Cast(cb));
    boost::optional<std::string> ext = params.get<std::string>("extent");
    if (ext)
        ext_.from_string(*ext);
    else
        throw mapnik::datasource_exception("JSDatasource missing <extent> parameter");    
}

js_datasource::~js_datasource()
{
    cb_.Dispose();
}

  
int js_datasource::type() const
{
    return mapnik::datasource::Vector;
}

    
mapnik::box2d<double> js_datasource::envelope() const
{
    return ext_;
}
    
mapnik::layer_descriptor js_datasource::get_descriptor() const
{
    return mapnik::layer_descriptor("in-memory js datasource","utf-8");
}
    
size_t js_datasource::size() const
{
    return 0;//features_.size();
}


class js_featureset : public mapnik::Featureset, private boost::noncopyable
{
public:
    js_featureset( const mapnik::query& q, const js_datasource* ds)
        : q_(q),
          feature_id_(1),
          tr_(new mapnik::transcoder("utf-8")),
          ds_(ds),
          obj_(Object::New())
    {
        Local<Array> a = Array::New(4);
        mapnik::box2d<double> const& e = q_.get_bbox();
        a->Set(0, Number::New(e.minx()));
        a->Set(1, Number::New(e.miny()));
        a->Set(2, Number::New(e.maxx()));
        a->Set(3, Number::New(e.maxy()));
        obj_->Set(String::NewSymbol("extent"), a);
        
    }

    virtual ~js_featureset() {}

    mapnik::feature_ptr next()
    {

        HandleScope scope;
        
        TryCatch try_catch;
        Local<Value> argv[2] = { Integer::New(feature_id_), obj_ };
        Local<Value> val = ds_->cb_->Call(Context::GetCurrent()->Global(), 2, argv);
        if (try_catch.HasCaught()) {
            FatalException(try_catch);
        }
        else
        {
            if (!val->IsUndefined())
            {
                if (val->IsObject())
                {
                    Local<Object> obj = val->ToObject();
                    if (obj->Has(String::New("x")) && obj->Has(String::New("y")))
                    {
                        Local<Value> x = obj->Get(String::New("x"));
                        Local<Value> y = obj->Get(String::New("y"));
                        if (!x->IsUndefined() && x->IsNumber() && !y->IsUndefined() && y->IsNumber())
                        {
                            mapnik::geometry_type * pt = new mapnik::geometry_type(mapnik::Point);
                            pt->move_to(x->NumberValue(),y->NumberValue());
                            mapnik::feature_ptr feature(mapnik::feature_factory::create(feature_id_));
                            ++feature_id_;
                            feature->add_geometry(pt);
                            if (obj->Has(String::New("properties")))
                            {
                                Local<Value> props = obj->Get(String::New("properties"));
                                if (props->IsObject())
                                {
                                    Local<Object> p_obj = props->ToObject();
                                    Local<Array> names = p_obj->GetPropertyNames();
                                    uint32_t i = 0;
                                    uint32_t a_length = names->Length();
                                    while (i < a_length)
                                    {
                                        Local<Value> name = names->Get(i)->ToString();
                                        // if name in q.property_names() ?
                                        Local<Value> value = p_obj->Get(name);
                                        if (value->IsString()) {
                                            UnicodeString ustr = tr_->transcode(TOSTR(value));
                                            boost::put(*feature,TOSTR(name),ustr);
                                        } else if (value->IsNumber()) {
                                            double num = value->NumberValue();
                                            // todo - round
                                            if (num == value->IntegerValue()) {
                                                int integer = value->IntegerValue();
                                                boost::put(*feature,TOSTR(name),integer);
                                            } else {
                                                double dub_val = value->NumberValue();
                                                boost::put(*feature,TOSTR(name),dub_val);
                                            }
                                        }
                                        i++;
                                    }
                                }
                            }
                            return feature;                
                        }
                    }
                }
            }        
        }

        return mapnik::feature_ptr();
    }
        
private:
    mapnik::query const& q_;
    unsigned int feature_id_;
    boost::scoped_ptr<mapnik::transcoder> tr_;
    const js_datasource* ds_;
    Local<Object> obj_;
    
};


mapnik::featureset_ptr js_datasource::features(const mapnik::query& q) const
{
    return mapnik::featureset_ptr(new js_featureset(q,this));
}

mapnik::featureset_ptr js_datasource::features_at_point(mapnik::coord2d const& pt) const
{
/*
    box2d<double> box = box2d<double>(pt.x, pt.y, pt.x, pt.y);
#ifdef MAPNIK_DEBUG
    std::clog << "box=" << box << ", pt x=" << pt.x << ", y=" << pt.y << "\n";
#endif
    return featureset_ptr(new memory_featureset(box,*this));
*/
    return mapnik::featureset_ptr();
}

#endif // __NODE_MAPNIK_MEM_DATASOURCE_H__
