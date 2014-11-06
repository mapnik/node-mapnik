## Geometry

#### Constructor

- No constructor - geometries are only available via `mapnik.Feature` instances

#### Methods

- `extent()` : Array of [minx,miny,maxx,maxy]
- `toJSON()` : String of GeoJSON geometry
- `toWKB()` : Buffer of geometry in Well Known Binary format
- `toWKT()` : String of geometry in Well Known Text format
