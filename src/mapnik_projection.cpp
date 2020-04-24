#include "mapnik_projection.hpp"
#include "utils.hpp"

#include <mapnik/geometry/box2d.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/projection.hpp>
#include <sstream>

Napi::FunctionReference Projection::constructor;

/**
 * **`mapnik.Projection`**
 *
 * A geographical projection: this class makes it possible to translate between
 * locations in different projections
 *
 * @class Projection
 * @param {string} projection projection as a proj4 definition string
 * @param {Object} [options={lazy:false}] whether to lazily instantiate the
 * data backing this projection.
 * @throws {TypeError} if the projection string or options argument is the wrong type
 * @throws {Error} the projection could not be initialized - it was not found
 * in proj4's tables or the string was malformed
 * @example
 * var wgs84 = new mapnik.Projection('+init=epsg:4326');
 */
void Projection::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, Projection::New);

    lcons->SetClassName(Napi::String::New(env, "Projection"));

    InstanceMethod("forward", &forward),
    InstanceMethod("inverse", &inverse),

    (target).Set(Napi::String::New(env, "Projection"), Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

Projection::Projection(std::string const& name, bool defer_init) : Napi::ObjectWrap<Projection>(),
    projection_(std::make_shared<mapnik::projection>(name, defer_init)) {}

Projection::~Projection()
{
}

Napi::Value Projection::New(const Napi::CallbackInfo& info)
{
    if (!info.IsConstructCall())
    {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() <= 0 || !info[0].IsString()) {
        Napi::TypeError::New(env, "please provide a proj4 initialization string").ThrowAsJavaScriptException();
        return env.Null();
    }
    bool lazy = false;
    if (info.Length() >= 2)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "The second parameter provided should be an options object").ThrowAsJavaScriptException();
            return env.Null();
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if ((options).Has(Napi::String::New(env, "lazy")).FromMaybe(false))
        {
            Napi::Value lazy_opt = (options).Get(Napi::String::New(env, "lazy"));
            if (!lazy_opt->IsBoolean())
            {
                Napi::TypeError::New(env, "'lazy' must be a Boolean").ThrowAsJavaScriptException();
                return env.Null();
            }
            lazy = lazy_opt.As<Napi::Boolean>().Value();
        }
    }

    try
    {
        Projection* p = new Projection(TOSTR(info[0]), lazy);
        p->Wrap(info.This());
        return info.This();
        return;
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

/**
 * Project from a position in WGS84 space to a position in this projection.
 *
 * @name forward
 * @memberof Projection
 * @instance
 * @param {Array<number>} position as [x, y] or extent as [minx,miny,maxx,maxy]
 * @returns {Array<number>} projected coordinates
 * @example
 * var merc = new mapnik.Projection('+init=epsg:3857');
 * var long_lat_coords = [-122.33517, 47.63752];
 * var projected = merc.forward(long_lat_coords);
 */
Napi::Value Projection::forward(const Napi::CallbackInfo& info)
{
    Projection* p = info.Holder().Unwrap<Projection>();

    try
    {
        if (info.Length() != 1) {
            Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            if (!info[0].IsArray()) {
                Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Array a = info[0].As<Napi::Array>();
            unsigned int array_length = a->Length();
            if (array_length == 2)
            {
                double x = (a).Get(0.As<Napi::Number>().DoubleValue());
                double y = (a).Get(1.As<Napi::Number>().DoubleValue());
                p->projection_->forward(x,y);
                Napi::Array arr = Napi::Array::New(env, 2);
                (arr).Set(0, Napi::New(env, x));
                (arr).Set(1, Napi::New(env, y));
                return arr;
            }
            else if (array_length == 4)
            {
                double ulx, uly, urx, ury, lrx, lry, llx, lly;
                ulx = llx = (a).Get(0.As<Napi::Number>().DoubleValue());
                lry = lly = (a).Get(1.As<Napi::Number>().DoubleValue());
                lrx = urx = (a).Get(2.As<Napi::Number>().DoubleValue());
                uly = ury = (a).Get(3.As<Napi::Number>().DoubleValue());
                p->projection_->forward(ulx,uly);
                p->projection_->forward(urx,ury);
                p->projection_->forward(lrx,lry);
                p->projection_->forward(llx,lly);
                Napi::Array arr = Napi::Array::New(env, 4);
                (arr).Set(0, Napi::New(env, std::min(ulx,llx)));
                (arr).Set(1, Napi::New(env, std::min(lry,lly)));
                (arr).Set(2, Napi::New(env, std::max(urx,lrx)));
                (arr).Set(3, Napi::New(env, std::max(ury,uly)));
                return arr;
            }
            else
            {
                Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Null();
            }
        }
    } catch (std::exception const & ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

/**
 * Unproject from a position in this projection to the same position in WGS84
 * space.
 *
 * @name inverse
 * @memberof Projection
 * @instance
 * @param {Array<number>} position as [x, y] or extent as [minx,miny,maxx,maxy]
 * @returns {Array<number>} unprojected coordinates
 */
Napi::Value Projection::inverse(const Napi::CallbackInfo& info)
{
    Projection* p = info.Holder().Unwrap<Projection>();

    try
    {
        if (info.Length() != 1) {
            Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Null();
        } else {
            if (!info[0].IsArray()) {
                Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Array a = info[0].As<Napi::Array>();
            unsigned int array_length = a->Length();
            if (array_length == 2)
            {
                double x = (a).Get(0.As<Napi::Number>().DoubleValue());
                double y = (a).Get(1.As<Napi::Number>().DoubleValue());
                p->projection_->inverse(x,y);
                Napi::Array arr = Napi::Array::New(env, 2);
                (arr).Set(0, Napi::New(env, x));
                (arr).Set(1, Napi::New(env, y));
                return arr;
            }
            else if (array_length == 4)
            {
                double minx = (a).Get(0.As<Napi::Number>().DoubleValue());
                double miny = (a).Get(1.As<Napi::Number>().DoubleValue());
                double maxx = (a).Get(2.As<Napi::Number>().DoubleValue());
                double maxy = (a).Get(3.As<Napi::Number>().DoubleValue());
                p->projection_->inverse(minx,miny);
                p->projection_->inverse(maxx,maxy);
                Napi::Array arr = Napi::Array::New(env, 4);
                (arr).Set(0, Napi::New(env, minx));
                (arr).Set(1, Napi::New(env, miny));
                (arr).Set(2, Napi::New(env, maxx));
                (arr).Set(3, Napi::New(env, maxy));
                return arr;
            }
            else
            {
                Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Null();
            }
        }
    } catch (std::exception const & ex) {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::FunctionReference ProjTransform::constructor;

void ProjTransform::Initialize(Napi::Object target) {

    Napi::HandleScope scope(env);

    Napi::FunctionReference lcons = Napi::Function::New(env, ProjTransform::New);

    lcons->SetClassName(Napi::String::New(env, "ProjTransform"));

    InstanceMethod("forward", &forward),
    InstanceMethod("backward", &backward),

    (target).Set(Napi::String::New(env, "ProjTransform"), Napi::GetFunction(lcons));
    constructor.Reset(lcons);
}

ProjTransform::ProjTransform(mapnik::projection const& src,
                             mapnik::projection const& dest) : Napi::ObjectWrap<ProjTransform>(),
    this_(std::make_shared<mapnik::proj_transform>(src,dest)) {}

ProjTransform::~ProjTransform()
{
}

Napi::Value ProjTransform::New(const Napi::CallbackInfo& info)
{
    if (!info.IsConstructCall()) {
        Napi::Error::New(env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() != 2 || !info[0].IsObject()  || !info[1].IsObject()) {
        Napi::TypeError::New(env, "please provide two arguments: a pair of mapnik.Projection objects").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object src_obj = info[0].As<Napi::Object>();
    if (src_obj->IsNull() || src_obj->IsUndefined() || !Napi::New(env, Projection::constructor)->HasInstance(src_obj)) {
        Napi::TypeError::New(env, "mapnik.Projection expected for first argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object dest_obj = info[1].As<Napi::Object>();
    if (dest_obj->IsNull() || dest_obj->IsUndefined() || !Napi::New(env, Projection::constructor)->HasInstance(dest_obj)) {
        Napi::TypeError::New(env, "mapnik.Projection expected for second argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    Projection *p1 = src_obj.Unwrap<Projection>();
    Projection *p2 = dest_obj.Unwrap<Projection>();

    try
    {
        ProjTransform* p = new ProjTransform(*p1->get(),*p2->get());
        p->Wrap(info.This());
        return info.This();
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value ProjTransform::forward(const Napi::CallbackInfo& info)
{
    ProjTransform* p = info.Holder().Unwrap<ProjTransform>();

    if (info.Length() != 1) {
        Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
        return env.Null();
    } else {
        if (!info[0].IsArray()) {
            Napi::TypeError::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Array a = info[0].As<Napi::Array>();
        unsigned int array_length = a->Length();
        if (array_length == 2)
        {
            double x = (a).Get(0.As<Napi::Number>().DoubleValue());
            double y = (a).Get(1.As<Napi::Number>().DoubleValue());
            double z = 0;
            if (!p->this_->forward(x,y,z))
            {
                std::ostringstream s;
                s << "Failed to forward project "
                  << x <<  "," << y << " from " << p->this_->source().params() << " to " << p->this_->dest().params();
                Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
                return env.Null();

            }
            Napi::Array arr = Napi::Array::New(env, 2);
            (arr).Set(0, Napi::New(env, x));
            (arr).Set(1, Napi::New(env, y));
            return arr;
        }
        else if (array_length == 4)
        {
            mapnik::box2d<double> box((a).Get(0.As<Napi::Number>().DoubleValue()),
                                      (a).Get(1.As<Napi::Number>().DoubleValue()),
                                      (a).Get(2.As<Napi::Number>().DoubleValue()),
                                      (a).Get(3.As<Napi::Number>().DoubleValue()));
            if (!p->this_->forward(box))
            {
                std::ostringstream s;
                s << "Failed to forward project "
                  << box << " from " << p->this_->source().params() << " to " << p->this_->dest().params();
                Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Array arr = Napi::Array::New(env, 4);
            (arr).Set(0, Napi::New(env, box.minx()));
            (arr).Set(1, Napi::New(env, box.miny()));
            (arr).Set(2, Napi::New(env, box.maxx()));
            (arr).Set(3, Napi::New(env, box.maxy()));
            return arr;
        }
        else
        {
            Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Null();
        }
    }
}

Napi::Value ProjTransform::backward(const Napi::CallbackInfo& info)
{
    ProjTransform* p = info.Holder().Unwrap<ProjTransform>();

    if (info.Length() != 1) {
        Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
        return env.Null();
    } else {
        if (!info[0].IsArray())
        {
            Napi::TypeError::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Array a = info[0].As<Napi::Array>();
        unsigned int array_length = a->Length();
        if (array_length == 2)
        {
            double x = (a).Get(0.As<Napi::Number>().DoubleValue());
            double y = (a).Get(1.As<Napi::Number>().DoubleValue());
            double z = 0;
            if (!p->this_->backward(x,y,z))
            {
                std::ostringstream s;
                s << "Failed to back project "
                  << x << "," << y << " from " << p->this_->dest().params() << " to: " << p->this_->source().params();
                Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Array arr = Napi::Array::New(env, 2);
            (arr).Set(0, Napi::New(env, x));
            (arr).Set(1, Napi::New(env, y));
            return arr;
        }
        else if (array_length == 4)
        {
            mapnik::box2d<double> box((a).Get(0.As<Napi::Number>().DoubleValue()),
                                      (a).Get(1.As<Napi::Number>().DoubleValue()),
                                      (a).Get(2.As<Napi::Number>().DoubleValue()),
                                      (a).Get(3.As<Napi::Number>().DoubleValue()));
            if (!p->this_->backward(box))
            {
                std::ostringstream s;
                s << "Failed to back project "
                  << box << " from " << p->this_->source().params() << " to " << p->this_->dest().params();
                Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Array arr = Napi::Array::New(env, 4);
            (arr).Set(0, Napi::New(env, box.minx()));
            (arr).Set(1, Napi::New(env, box.miny()));
            (arr).Set(2, Napi::New(env, box.maxx()));
            (arr).Set(3, Napi::New(env, box.maxy()));
            return arr;
        }
        else
        {
            Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Null();
        }
    }
}
