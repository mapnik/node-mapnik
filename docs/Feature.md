## Feature

#### Constructor

- `new Feature(int id)`
- `new Feature.fromJSON(string geojson_feature)`

#### Methods

- `id()` : int
- `extent()` : Array of [minx,miny,maxx,maxy]
- `attributes()` : Object of key:value pairs representing attributes
- `geometry()` : [Geometry](Geometry.md)
- `toJSON()` : String of GeoJSON feature
