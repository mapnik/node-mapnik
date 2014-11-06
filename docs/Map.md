## new mapnik.Map(width, height, [srs])

Returns a new Map object.

* `width`: An integer width for the map. Normally for tiled rendering this value is some power of 2, like `256` or `512` and matches the value for `height`.

* `height`: An integer height for the map. Normally for tiled rendering this value is some power of 2, like `256` or `512` and matches the value for `width`.

* `srs` *(optional)*: A string. If provided, this sets the Map spatial reference system (aka projection). The format is the proj4 syntax. The default if you do not provide a value is `+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs` (which is equivalent to the shorthand of `+init=epsg:4326` and comes from [Mapnik core](https://github.com/mapnik/mapnik/blob/e15e64a5a94f65a125368c9eea4393fb04ffbed3/include/mapnik/well_known_srs.hpp#L56)). The most common `srs` for tiled rendering is spherical mercator which is `+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0.0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs +over` (which is equivalent to the shorthand of `+init=epsg:3857`). Note: Mapnik >= 2.2.x has special handling for transforming between these common projections and detects them [here](https://github.com/mapnik/mapnik/blob/e15e64a5a94f65a125368c9eea4393fb04ffbed3/src/well_known_srs.cpp#L39-L50).

## Map.fromStringSync(xml, [options])

## Map.fromString(xml, [options], [callback])

Options:

* `strict`: A boolean to indicate that path related errors should be raised on XML parsing. Default: false.

* `base`: A filepath (string) for Mapnik to resolve paths upon. Default: null.

## Map.render(surface, [options,] [callback])

Renders the data in the map to a given `surface`. A surface can either be a `mapnik.VectorTile`, a `mapnik.Image`, or a `mapnik.Grid`.
