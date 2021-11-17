#include "mapnik_projection.hpp"
#include "utils.hpp"

#include <mapnik/geometry/box2d.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/projection.hpp>
#include <sstream>

Napi::FunctionReference Projection::constructor;

Napi::Object Projection::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "Projection", {
            InstanceMethod<&Projection::forward>("forward", prop_attr),
            InstanceMethod<&Projection::inverse>("inverse", prop_attr)
        });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Projection", func);
    return exports;
}

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
 * var wgs84 = new mapnik.Projection('epsg:4326');
 */

Projection::Projection(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<Projection>(info)
{
    Napi::Env env = info.Env();
    if (info.Length() <= 0 || !info[0].IsString())
    {
        Napi::TypeError::New(env, "please provide a proj4 initialization string").ThrowAsJavaScriptException();
        return;
    }
    bool lazy = false;
    if (info.Length() >= 2)
    {
        if (!info[1].IsObject())
        {
            Napi::TypeError::New(env, "The second parameter provided should be an options object").ThrowAsJavaScriptException();
            return;
        }
        Napi::Object options = info[1].As<Napi::Object>();
        if (options.Has("lazy"))
        {
            Napi::Value lazy_opt = options.Get("lazy");
            if (!lazy_opt.IsBoolean())
            {
                Napi::TypeError::New(env, "'lazy' must be a Boolean").ThrowAsJavaScriptException();
                return;
            }
            lazy = lazy_opt.As<Napi::Boolean>();
        }
    }
    try
    {
        projection_ = std::make_shared<mapnik::projection>(info[0].As<Napi::String>(), lazy);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
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
 * var merc = new mapnik.Projection('epsg:3857');
 * var long_lat_coords = [-122.33517, 47.63752];
 * var projected = merc.forward(long_lat_coords);
 */
Napi::Value Projection::forward(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    try
    {
        if (info.Length() != 1)
        {
            Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        else
        {
            if (!info[0].IsArray())
            {
                Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Array arr_in = info[0].As<Napi::Array>();
            unsigned int array_length = arr_in.Length();
            if (array_length == 2)
            {
                double x = arr_in.Get(0u).As<Napi::Number>().DoubleValue();
                double y = arr_in.Get(1u).As<Napi::Number>().DoubleValue();
                projection_->forward(x, y);
                Napi::Array arr_out = Napi::Array::New(env, 2);
                arr_out.Set(0u, Napi::Number::New(env, x));
                arr_out.Set(1u, Napi::Number::New(env, y));
                return scope.Escape(arr_out);
            }
            else if (array_length == 4)
            {
                double ulx, uly, urx, ury, lrx, lry, llx, lly;
                ulx = llx = arr_in.Get(0u).As<Napi::Number>().DoubleValue();
                lry = lly = arr_in.Get(1u).As<Napi::Number>().DoubleValue();
                lrx = urx = arr_in.Get(2u).As<Napi::Number>().DoubleValue();
                uly = ury = arr_in.Get(3u).As<Napi::Number>().DoubleValue();
                projection_->forward(ulx, uly);
                projection_->forward(urx, ury);
                projection_->forward(lrx, lry);
                projection_->forward(llx, lly);
                Napi::Array arr_out = Napi::Array::New(env, 4);
                arr_out.Set(0u, Napi::Number::New(env, std::min(ulx, llx)));
                arr_out.Set(1u, Napi::Number::New(env, std::min(lry, lly)));
                arr_out.Set(2u, Napi::Number::New(env, std::max(urx, lrx)));
                arr_out.Set(3u, Napi::Number::New(env, std::max(ury, uly)));
                return scope.Escape(arr_out);
            }
            else
            {
                Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]").ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
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
Napi::Value Projection::inverse(Napi::CallbackInfo const& info)
{

    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    try
    {
        if (info.Length() != 1)
        {
            Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        else
        {
            if (!info[0].IsArray())
            {
                Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Array arr_in = info[0].As<Napi::Array>();
            unsigned int array_length = arr_in.Length();
            if (array_length == 2)
            {
                double x = arr_in.Get(0u).As<Napi::Number>().DoubleValue();
                double y = arr_in.Get(1u).As<Napi::Number>().DoubleValue();
                projection_->inverse(x, y);
                Napi::Array arr_out = Napi::Array::New(env, 2);
                arr_out.Set(0u, Napi::Number::New(env, x));
                arr_out.Set(1u, Napi::Number::New(env, y));
                return scope.Escape(arr_out);
            }
            else if (array_length == 4)
            {
                double minx = arr_in.Get(0u).As<Napi::Number>().DoubleValue();
                double miny = arr_in.Get(1u).As<Napi::Number>().DoubleValue();
                double maxx = arr_in.Get(2u).As<Napi::Number>().DoubleValue();
                double maxy = arr_in.Get(3u).As<Napi::Number>().DoubleValue();
                projection_->inverse(minx, miny);
                projection_->inverse(maxx, maxy);
                Napi::Array arr_out = Napi::Array::New(env, 4);
                arr_out.Set(0u, Napi::Number::New(env, minx));
                arr_out.Set(1u, Napi::Number::New(env, miny));
                arr_out.Set(2u, Napi::Number::New(env, maxx));
                arr_out.Set(3u, Napi::Number::New(env, maxy));
                return scope.Escape(arr_out);
            }
            else
            {
                Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::FunctionReference ProjTransform::constructor;

Napi::Object ProjTransform::Initialize(Napi::Env env, Napi::Object exports, napi_property_attributes prop_attr)
{
    // clang-format off
    Napi::Function func = DefineClass(env, "ProjTransform", {
            InstanceMethod<&ProjTransform::forward>("forward", prop_attr),
            InstanceMethod<&ProjTransform::backward>("backward", prop_attr)
         });
    // clang-format on
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("ProjTransform", func);
    return exports;
}

ProjTransform::ProjTransform(Napi::CallbackInfo const& info)
    : Napi::ObjectWrap<ProjTransform>(info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 2 || !info[0].IsObject() || !info[1].IsObject())
    {
        Napi::TypeError::New(env, "please provide two arguments: a pair of mapnik.Projection objects")
            .ThrowAsJavaScriptException();
        return;
    }

    Napi::Object src_obj = info[0].As<Napi::Object>();

    if (!src_obj.InstanceOf(Projection::constructor.Value()))
    {
        Napi::TypeError::New(env, "mapnik.Projection expected for first argument")
            .ThrowAsJavaScriptException();
        return;
    }

    Napi::Object dst_obj = info[1].As<Napi::Object>();
    if (!dst_obj.InstanceOf(Projection::constructor.Value()))
    {
        Napi::TypeError::New(env, "mapnik.Projection expected for second argument").ThrowAsJavaScriptException();
        return;
    }
    Projection* p1 = Napi::ObjectWrap<Projection>::Unwrap(src_obj);
    Projection* p2 = Napi::ObjectWrap<Projection>::Unwrap(dst_obj);

    try
    {
        proj_transform_ = std::make_shared<mapnik::proj_transform>(*p1->projection_, *p2->projection_);
    }
    catch (std::exception const& ex)
    {
        Napi::Error::New(env, ex.what()).ThrowAsJavaScriptException();
    }
}

Napi::Value ProjTransform::forward(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() != 1)
    {
        Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    else
    {
        if (!info[0].IsArray())
        {
            Napi::TypeError::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Array arr_in = info[0].As<Napi::Array>();
        unsigned int array_length = arr_in.Length();
        if (array_length == 2)
        {
            double x = arr_in.Get(0u).As<Napi::Number>().DoubleValue();
            double y = arr_in.Get(1u).As<Napi::Number>().DoubleValue();
            double z = 0;
            if (!proj_transform_->forward(x, y, z))
            {
                std::ostringstream s;
                s << "Failed to forward project "
                  << x << "," << y << " " << proj_transform_->definition();
                Napi::Error::New(env, s.str().c_str()).ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Array arr_out = Napi::Array::New(env, 2);
            arr_out.Set(0u, Napi::Number::New(env, x));
            arr_out.Set(1u, Napi::Number::New(env, y));
            return scope.Escape(arr_out);
        }
        else if (array_length == 4)
        {
            mapnik::box2d<double> box(arr_in.Get(0u).As<Napi::Number>().DoubleValue(),
                                      arr_in.Get(1u).As<Napi::Number>().DoubleValue(),
                                      arr_in.Get(2u).As<Napi::Number>().DoubleValue(),
                                      arr_in.Get(3u).As<Napi::Number>().DoubleValue());
            if (!proj_transform_->forward(box))
            {
                std::ostringstream s;
                s << "Failed to forward project "
                  << box << " " << proj_transform_->definition();
                Napi::Error::New(env, s.str()).ThrowAsJavaScriptException();
                return env.Undefined();
            }
            Napi::Array arr_out = Napi::Array::New(env, 4);
            arr_out.Set(0u, Napi::Number::New(env, box.minx()));
            arr_out.Set(1u, Napi::Number::New(env, box.miny()));
            arr_out.Set(2u, Napi::Number::New(env, box.maxx()));
            arr_out.Set(3u, Napi::Number::New(env, box.maxy()));
            return scope.Escape(arr_out);
        }
        else
        {
            Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
}

Napi::Value ProjTransform::backward(Napi::CallbackInfo const& info)
{
    Napi::Env env = info.Env();
    Napi::EscapableHandleScope scope(env);

    if (info.Length() != 1)
    {
        Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    else
    {
        if (!info[0].IsArray())
        {
            Napi::TypeError::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")
                .ThrowAsJavaScriptException();
            return env.Null();
        }

        Napi::Array arr_in = info[0].As<Napi::Array>();
        unsigned int array_length = arr_in.Length();
        if (array_length == 2)
        {
            double x = arr_in.Get(0u).As<Napi::Number>().DoubleValue();
            double y = arr_in.Get(1u).As<Napi::Number>().DoubleValue();
            double z = 0;
            if (!proj_transform_->backward(x, y, z))
            {
                std::ostringstream s;
                s << "Failed to back project "
                  << x << "," << y << " " << proj_transform_->definition();
                Napi::Error::New(env, s.str()).ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Array arr_out = Napi::Array::New(env, 2);
            arr_out.Set(0u, Napi::Number::New(env, x));
            arr_out.Set(1u, Napi::Number::New(env, y));
            return scope.Escape(arr_out);
        }
        else if (array_length == 4)
        {
            mapnik::box2d<double> box(arr_in.Get(0u).As<Napi::Number>().DoubleValue(),
                                      arr_in.Get(1u).As<Napi::Number>().DoubleValue(),
                                      arr_in.Get(2u).As<Napi::Number>().DoubleValue(),
                                      arr_in.Get(3u).As<Napi::Number>().DoubleValue());
            if (!proj_transform_->backward(box))
            {
                std::ostringstream s;
                s << "Failed to back project "
                  << box << " " << proj_transform_->definition();
                Napi::Error::New(env, s.str()).ThrowAsJavaScriptException();
                return env.Null();
            }
            Napi::Array arr_out = Napi::Array::New(env, 4);
            arr_out.Set(0u, Napi::Number::New(env, box.minx()));
            arr_out.Set(1u, Napi::Number::New(env, box.miny()));
            arr_out.Set(2u, Napi::Number::New(env, box.maxx()));
            arr_out.Set(3u, Napi::Number::New(env, box.maxy()));
            return scope.Escape(arr_out);
        }
        else
        {
            Napi::Error::New(env, "Must provide an array of either [x,y] or [minx,miny,maxx,maxy]")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
}
