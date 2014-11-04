## new mapnik.VectorTile(z, x, y, options);

Returns a new VectorTile object for a given `xyz` [tile uri](http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames). The assumed projection of the data is [Spherical Mercator](http://en.wikipedia.org/wiki/Mercator_projection). Learn more about tile addressing at [Maptiler.org](http://www.maptiler.org/google-maps-coordinates-tile-bounds-projection/).

Arguments:

* `z`: An integer zoom level.  

* `x`: An integer x coordinate.  

* `y`: An integer y coordinate. 

Options (available in node-mapnik > =1.4.3):

* `width`: An integer height for the tile. Defaults to 256. Not recommended to alter this default.

* `height`: An integer height for the tile. Defaults to 256. Not recommended to alter this default.

## VectorTile.render(map, surface, [options] [callback])

Renders the data in the map to a given `surface`. A surface can either be a `mapnik.Image`, a `mapnik.Grid`, or (experimentally) a `mapnik.CairoSurface`.

Options are:

* `z`: An integer zoom level. If provided this overrides the zoom level used for rendering that would otherwise be inherited from the `VectorTile` instance being used to render

* `x`: An integer x coordinate. If provided this overrides the zoom level used for rendering that would otherwise be inherited from the `VectorTile` instance being used to render 

* `y`: An integer y coordinate. If provided this overrides the zoom level used for rendering that would otherwise be inherited from the `VectorTile` instance being used to render

* `variables`: Mapnik 3.x ONLY: A javascript object containing key value pairs that should be passed into Mapnik as variables for rendering and for datasource queries. For example if you passed `vtile.render(map,image,{ variables : {zoom:1} },cb)` then the `@zoom` variable would be usable in Mapnik symbolizers like `line-width:"@zoom"` and as a token in Mapnik `postgis` sql sub-selects like `(select * from table where some_field > @zoom) as tmp`.

* `buffer_size`: An integer buffer size to use for rendering.

* `scale`: An floating point scale factor size to use for rendering.

* `scale_denominator`: An floating point scale_denominator to be used by Mapnik when matching zoom filters. If provided this overrides the auto-calculated `scale_denominator` that is based on the map dimensions and bbox. Do not set this option unless you know what it means.

## VectorTile#getData()

Get the protobuf-encoded Buffer from the vector tile object. This should then be passed through `zlib.deflate` to compress further before storing or sending over http. Remember to set `content-encoding:deflate` if you want an http client to know to automatically uncompress. Or use `zlib.inflate` to uncompress yourself if working serverside.

## VectorTile#query(lon,lat,options)

Query the features inside a vector tile by lon/lat. Returns an array of one or more mapnik.Feature objects or an empty array if no features intersect with the lon/lat.

Arguments:

* `lon`: A longitude value in degrees (WGS 84 coordinate system)

* `lat`: A latitude value in degrees (WGS 84 coordinate system)

Options:

* `tolerance`: Allows for inexact matches. If you provide a positive value for `tolerance` you are saying that you'd like features returned in the query results that might not exactly intersect with a given lon/lat. The higher the tolerance the slower the query will run because it will do more work by comparing your query lon/lat against more potential features. However, this is an important parameter because vector tile storage, by design, results in reduced precision of coordinates. The amount of precision loss depends on the zoom level of a given vector tile and how aggressively it was simplified during encoding. So if you want at least one match - say the closest single feature to your query lon/lat - is is not possible to know the smallest tolerance that will work without experimentation. In general be prepared to provide a high tolerance (1-100) for low zoom levels while you should be okay with a low tolerance (1-10) at higher zoom levels and with vector tiles that are storing less simplified geometries. The units `tolerance` should be expressed in depend on the coordinate system of the underlying data. In the case of vector tiles this is spherical mercator so the units are meters. For points any features will be returned that contain a point which is, by distance in meters, not greater than the `tolerance` value. For lines any features will be returned that have a segment which is, by distance in meters, not greater than the `tolerance` value. For polygons `tolerance` is not supported which means that your lon/lat must fall inside a feature's polygon otherwise that feature will not be matched.

* `layer`: Pass a layer name to restrict the query results to a single layer in the vector tile. Get all possible layer names in the vector tile with `VectorTile.names()`.

The mapnik.Feature objects returned have two extra special properties attached:
 - `layer`: the name of the layer the feature came from
 -  `distance`: the distance from the query lon/lat to the features geometry that was encountered. This value is in meters based on the spherical mercator coordinate system. It represents the distance to the first geometry encountered so for multipart geometries it may not be the closed of all geometries. For polygons the value with always be 0 because only lon/lat values fully within polygons are matched.

## VectorTile#queryMany(array of lonlats,options)

Query many features inside a vector tile with an array of lon/lats. Returns an array of one or more mapnik.Feature objects or an empty array if no features intersect with the lon/lat.

Arguments:

* `lonlats`: An array of longitude and latitudes in degrees (WGS 84 coordinate system)

Options:

* `tolerance`: Allows for inexact matches. If you provide a positive value for `tolerance` you are saying that you'd like features returned in the query results that might not exactly intersect with a given lon/lat. The higher the tolerance the slower the query will run because it will do more work by comparing your query lon/lat against more potential features. However, this is an important parameter because vector tile storage, by design, results in reduced precision of coordinates. The amount of precision loss depends on the zoom level of a given vector tile and how aggressively it was simplified during encoding. So if you want at least one match - say the closest single feature to your query lon/lat - is is not possible to know the smallest tolerance that will work without experimentation. In general be prepared to provide a high tolerance (1-100) for low zoom levels while you should be okay with a low tolerance (1-10) at higher zoom levels and with vector tiles that are storing less simplified geometries. The units `tolerance` should be expressed in depend on the coordinate system of the underlying data. In the case of vector tiles this is spherical mercator so the units are meters. For points any features will be returned that contain a point which is, by distance in meters, not greater than the `tolerance` value. For lines any features will be returned that have a segment which is, by distance in meters, not greater than the `tolerance` value. For polygons `tolerance` is not supported which means that your lon/lat must fall inside a feature's polygon otherwise that feature will not be matched.

* `layer`: Pass a layer name to restrict the query results to a single layer in the vector tile. Get all possible layer names in the vector tile with `VectorTile.names()`.

* `fields`: An array of strings for the field to be queried within a specific layer.


The response has contains two main objects: `hits` and `features`. The number of `hits` returned will correspond to the number of lat lngs queried and will be returned in the order of the query. Each hit returns 1) a `distance` and a 2) `feature_id`. The `distance` is number of meters the queried latlng is from the object in the vector tile. The `feature_id` is the corresponding object in `features` object. 

The values for the query is contained in the `features` object. Use `attributes()` to extract a value.

Example Response: 
```
{
    'hits': {
        '0': [
            [{
                'distance': 93.20766022375953,
                'feature_id': 0
            }, {
                'distance': 76.01178352331199,
                'feature_id': 2
            }, {
                'distance': 64.06993666527835,
                'feature_id': 4
            }, {
                'distance': 55.71101086662423,
                'feature_id': 6
            }, {
                'distance': 49.14361145721912,
                'feature_id': 8
            }]
        ],
        '1': [{
            'distance': 97.50389910208807,
            'feature_id': 1
        }, {
            'distance': 84.65866290631075,
            'feature_id': 3
        }, {
            'distance': 74.89160242941259,
            'feature_id': 5
        }, {
            'distance': 66.87466986910316,
            'feature_id': 7
        }, {
            'distance': 59.971307141140755,
            'feature_id': 9
        }]
    },
    'features': [{
        'layer': 'contour'
    }, {
        'layer': 'contour'
    }, {
        'layer': 'contour'
    }, {
        'layer': 'contour'
    }, {
        'layer': 'contour'
    }, {
        'layer': 'contour'
    }, {
        'layer': 'contour'
    }, {
        'layer': 'contour'
    }, {
        'layer': 'contour'
    }]
}
```

## new mapnik.Image(width, height)

Create a new image object that can be rendered to.

## Image.width()

Returns the width of the image in pixels

## Image.height()

Returns the height of the image in pixels

## Image.encode(format, callback)

Encode an image into a given format, like `'png'` and call callback with `(err, result)`.

## Image.encodeSync(format)

Encode an image into a given format, like `'png'` and return buffer of data.

TODO (rest of api docs - tracking at https://github.com/mapnik/node-mapnik/issues/235).

You can also take a look at the [node-mapnik typescript definition file](https://github.com/calinseciu/DefinitelyTyped/blob/master/node-mapnik/node-mapnik.d.ts) which offers [intellisense in IDEs](http://stackoverflow.com/questions/22367328/intellisense-and-code-complete-for-definitelytyped-typescript-type-definitions).

## Image.compare(image,options)

Available in >= 1.4.7.

Compare the pixels of one image to the pixels of another. Returns the number of pixels that are different. So, if the images are identical then it returns `0`. And if the images share no common pixels it returns the total number of pixels in an image which is equivalent to `im.width()*im.height()`.

Arguments:

* `image`: An image instance to compare to.

Options:

* `threshold`: A value that should be 0 or greater to determine if the pixels match. Defaults to 16 which means that `rgba(0,0,0,0)` would be considered the same as `rgba(15,15,15,0)`.

* `alpha`: Boolean that can be set to `false` so that alpha is ignored in the comparison. Default is `true` which means that alpha is considered in the pixel comparison along with the rgb channels.