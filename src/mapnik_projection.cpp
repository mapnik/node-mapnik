#include "mapnik_projection.hpp"
#include "utils.hpp"

#include <mapnik/box2d.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/projection.hpp>
#include <sstream>

Nan::Persistent<v8::FunctionTemplate> Projection::constructor;

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
void Projection::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(Projection::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("Projection").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "forward", forward);
    Nan::SetPrototypeMethod(lcons, "inverse", inverse);

    target->Set(Nan::New("Projection").ToLocalChecked(), lcons->GetFunction());
    constructor.Reset(lcons);
}

Projection::Projection(std::string const& name, bool defer_init) :
    Nan::ObjectWrap(),
    projection_(std::make_shared<mapnik::projection>(name, defer_init)) {}

Projection::~Projection()
{
}

NAN_METHOD(Projection::New)
{
    if (!info.IsConstructCall())
    {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info.Length() <= 0 || !info[0]->IsString()) {
        Nan::ThrowTypeError("please provide a proj4 initialization string");
        return;
    }
    bool lazy = false;
    if (info.Length() >= 2)
    {
        if (!info[1]->IsObject())
        {
            Nan::ThrowTypeError("The second parameter provided should be an options object");
            return;
        }
        v8::Local<v8::Object> options = info[1].As<v8::Object>();
        if (options->Has(Nan::New("lazy").ToLocalChecked()))
        {
            v8::Local<v8::Value> lazy_opt = options->Get(Nan::New("lazy").ToLocalChecked());
            if (!lazy_opt->IsBoolean())
            {
                Nan::ThrowTypeError("'lazy' must be a Boolean");
                return;
            }
            lazy = lazy_opt->BooleanValue();
        }
    }
            
    try
    {
        Projection* p = new Projection(TOSTR(info[0]), lazy);
        p->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
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
NAN_METHOD(Projection::forward)
{
    Projection* p = Nan::ObjectWrap::Unwrap<Projection>(info.Holder());

    try
    {
        if (info.Length() != 1) {
            Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            return;
        } else {
            if (!info[0]->IsArray()) {
                Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
                return;
            }
            v8::Local<v8::Array> a = info[0].As<v8::Array>();
            unsigned int array_length = a->Length();
            if (array_length == 2)
            {
                double x = a->Get(0)->NumberValue();
                double y = a->Get(1)->NumberValue();
                p->projection_->forward(x,y);
                v8::Local<v8::Array> arr = Nan::New<v8::Array>(2);
                arr->Set(0, Nan::New(x));
                arr->Set(1, Nan::New(y));
                info.GetReturnValue().Set(arr);
            }
            else if (array_length == 4)
            {
                double ulx, uly, urx, ury, lrx, lry, llx, lly;
                ulx = llx = a->Get(0)->NumberValue();
                lry = lly = a->Get(1)->NumberValue();
                lrx = urx = a->Get(2)->NumberValue();
                uly = ury = a->Get(3)->NumberValue();
                p->projection_->forward(ulx,uly);
                p->projection_->forward(urx,ury);
                p->projection_->forward(lrx,lry);
                p->projection_->forward(llx,lly);
                v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
                arr->Set(0, Nan::New(std::min(ulx,llx)));
                arr->Set(1, Nan::New(std::min(lry,lly)));
                arr->Set(2, Nan::New(std::max(urx,lrx)));
                arr->Set(3, Nan::New(std::max(ury,uly)));
                info.GetReturnValue().Set(arr);
            }
            else
            {
                Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
                return;
            }
        }
    } catch (std::exception const & ex) {
        Nan::ThrowError(ex.what());
        return;
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
NAN_METHOD(Projection::inverse)
{
    Projection* p = Nan::ObjectWrap::Unwrap<Projection>(info.Holder());

    try
    {
        if (info.Length() != 1) {
            Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            return;
        } else {
            if (!info[0]->IsArray()) {
                Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
                return;
            }
            v8::Local<v8::Array> a = info[0].As<v8::Array>();
            unsigned int array_length = a->Length();
            if (array_length == 2)
            {
                double x = a->Get(0)->NumberValue();
                double y = a->Get(1)->NumberValue();
                p->projection_->inverse(x,y);
                v8::Local<v8::Array> arr = Nan::New<v8::Array>(2);
                arr->Set(0, Nan::New(x));
                arr->Set(1, Nan::New(y));
                info.GetReturnValue().Set(arr);
            }
            else if (array_length == 4)
            {
                double minx = a->Get(0)->NumberValue();
                double miny = a->Get(1)->NumberValue();
                double maxx = a->Get(2)->NumberValue();
                double maxy = a->Get(3)->NumberValue();
                p->projection_->inverse(minx,miny);
                p->projection_->inverse(maxx,maxy);
                v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
                arr->Set(0, Nan::New(minx));
                arr->Set(1, Nan::New(miny));
                arr->Set(2, Nan::New(maxx));
                arr->Set(3, Nan::New(maxy));
                info.GetReturnValue().Set(arr);
            }
            else
            {
                Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
                return;
            }
        }
    } catch (std::exception const & ex) {
        Nan::ThrowError(ex.what());
        return;
    }
}

Nan::Persistent<v8::FunctionTemplate> ProjTransform::constructor;

void ProjTransform::Initialize(v8::Local<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(ProjTransform::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New("ProjTransform").ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "forward", forward);
    Nan::SetPrototypeMethod(lcons, "backward", backward);

    target->Set(Nan::New("ProjTransform").ToLocalChecked(), lcons->GetFunction());
    constructor.Reset(lcons);
}

ProjTransform::ProjTransform(mapnik::projection const& src,
                             mapnik::projection const& dest) :
    Nan::ObjectWrap(),
    this_(std::make_shared<mapnik::proj_transform>(src,dest)) {}

ProjTransform::~ProjTransform()
{
}

NAN_METHOD(ProjTransform::New)
{
    if (!info.IsConstructCall()) {
        Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
        return;
    }

    if (info.Length() != 2 || !info[0]->IsObject()  || !info[1]->IsObject()) {
        Nan::ThrowTypeError("please provide two arguments: a pair of mapnik.Projection objects");
        return;
    }

    v8::Local<v8::Object> src_obj = info[0].As<v8::Object>();
    if (src_obj->IsNull() || src_obj->IsUndefined() || !Nan::New(Projection::constructor)->HasInstance(src_obj)) {
        Nan::ThrowTypeError("mapnik.Projection expected for first argument");
        return;
    }

    v8::Local<v8::Object> dest_obj = info[1].As<v8::Object>();
    if (dest_obj->IsNull() || dest_obj->IsUndefined() || !Nan::New(Projection::constructor)->HasInstance(dest_obj)) {
        Nan::ThrowTypeError("mapnik.Projection expected for second argument");
        return;
    }

    Projection *p1 = Nan::ObjectWrap::Unwrap<Projection>(src_obj);
    Projection *p2 = Nan::ObjectWrap::Unwrap<Projection>(dest_obj);

    try
    {
        ProjTransform* p = new ProjTransform(*p1->get(),*p2->get());
        p->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
    }
    catch (std::exception const& ex)
    {
        Nan::ThrowError(ex.what());
        return;
    }
}

NAN_METHOD(ProjTransform::forward)
{
    ProjTransform* p = Nan::ObjectWrap::Unwrap<ProjTransform>(info.Holder());

    if (info.Length() != 1) {
        Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
        return;
    } else {
        if (!info[0]->IsArray()) {
            Nan::ThrowTypeError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            return;
        }

        v8::Local<v8::Array> a = info[0].As<v8::Array>();
        unsigned int array_length = a->Length();
        if (array_length == 2)
        {
            double x = a->Get(0)->NumberValue();
            double y = a->Get(1)->NumberValue();
            double z = 0;
            if (!p->this_->forward(x,y,z))
            {
                std::ostringstream s;
                s << "Failed to forward project "
                  << a->Get(0)->NumberValue() << "," << a->Get(1)->NumberValue() << " from " << p->this_->source().params() << " to " << p->this_->dest().params();
                Nan::ThrowError(s.str().c_str());
                return;

            }
            v8::Local<v8::Array> arr = Nan::New<v8::Array>(2);
            arr->Set(0, Nan::New(x));
            arr->Set(1, Nan::New(y));
            info.GetReturnValue().Set(arr);
        }
        else if (array_length == 4)
        {
            mapnik::box2d<double> box(a->Get(0)->NumberValue(),
                                      a->Get(1)->NumberValue(),
                                      a->Get(2)->NumberValue(),
                                      a->Get(3)->NumberValue());
            if (!p->this_->forward(box))
            {
                std::ostringstream s;
                s << "Failed to forward project "
                  << box << " from " << p->this_->source().params() << " to " << p->this_->dest().params();
                Nan::ThrowError(s.str().c_str());
                return;
            }
            v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
            arr->Set(0, Nan::New(box.minx()));
            arr->Set(1, Nan::New(box.miny()));
            arr->Set(2, Nan::New(box.maxx()));
            arr->Set(3, Nan::New(box.maxy()));
            info.GetReturnValue().Set(arr);
        }
        else
        {
            Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            return;
        }
    }
}

NAN_METHOD(ProjTransform::backward)
{
    ProjTransform* p = Nan::ObjectWrap::Unwrap<ProjTransform>(info.Holder());

    if (info.Length() != 1) {
        Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
        return;
    } else {
        if (!info[0]->IsArray())
        {
            Nan::ThrowTypeError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            return;
        }

        v8::Local<v8::Array> a = info[0].As<v8::Array>();
        unsigned int array_length = a->Length();
        if (array_length == 2)
        {
            double x = a->Get(0)->NumberValue();
            double y = a->Get(1)->NumberValue();
            double z = 0;
            if (!p->this_->backward(x,y,z))
            {
                std::ostringstream s;
                s << "Failed to back project "
                  << a->Get(0)->NumberValue() << "," << a->Get(1)->NumberValue() << " from " << p->this_->dest().params() << " to: " << p->this_->source().params();
                Nan::ThrowError(s.str().c_str());
                return;
            }
            v8::Local<v8::Array> arr = Nan::New<v8::Array>(2);
            arr->Set(0, Nan::New(x));
            arr->Set(1, Nan::New(y));
            info.GetReturnValue().Set(arr);
        }
        else if (array_length == 4)
        {
            mapnik::box2d<double> box(a->Get(0)->NumberValue(),
                                      a->Get(1)->NumberValue(),
                                      a->Get(2)->NumberValue(),
                                      a->Get(3)->NumberValue());
            if (!p->this_->backward(box))
            {
                std::ostringstream s;
                s << "Failed to back project "
                  << box << " from " << p->this_->source().params() << " to " << p->this_->dest().params();
                Nan::ThrowError(s.str().c_str());
                return;
            }
            v8::Local<v8::Array> arr = Nan::New<v8::Array>(4);
            arr->Set(0, Nan::New(box.minx()));
            arr->Set(1, Nan::New(box.miny()));
            arr->Set(2, Nan::New(box.maxx()));
            arr->Set(3, Nan::New(box.maxy()));
            info.GetReturnValue().Set(arr);
        }
        else
        {
            Nan::ThrowError("Must provide an array of either [x,y] or [minx,miny,maxx,maxy]");
            return;
        }
    }
}
