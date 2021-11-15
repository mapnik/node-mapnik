# Changelog

## 4.5.9
- Image - fix object lifetimes/scopes in relevant async methods (#978)
- Add persistent Buffer reference to Image to ensure underlying buffer stays in scope (in situations when Image doesn't own underlyling data)
- Image - update ctor and remove ad-hoc _buffer field
- Use Napi::EscapableHandleScope in Image::buffer and Image::data

## 4.5.8
- Upgrade to mapnik@e553f55dc
  (https://github.com/mapnik/mapnik/compare/c3eda40e0...e553f55dc)
   - SVG: restore default values in `parse_svg_value` on failure to preserve "viewport/viewBox" logic when using boost > 1.65.1
   - Upgrade to boost 1.75 + ICU 58.1

## 4.5.6
- Upgrade node-addon-api (>=v3.1.0)
- Upgrade to @mapbox/node-pre-gyp >= v1.x
- Check `std::string` is not-empty before accessing internal data via operator[]
- Upgrade to mapnik@c3eda40e0
  (https://github.com/mapnik/mapnik/compare/c6fc956a7...c3eda40e0)
  - Fixed size value used to `resize` record buffer in csv.input and geojson.input
       [#4187](https://github.com/mapnik/mapnik/issues/4187)
  - Disable compiler warning using portable macros
       [#4188](https://github.com/mapnik/mapnik/issues/4188)
       [#4189](https://github.com/mapnik/mapnik/issues/4189)
       [#4194](https://github.com/mapnik/mapnik/issues/4194)
  - Upgrade travis-ci build environment to use `xenial`
  - Move to travis-ci.com

## 4.5.5
- Upgrade to mapnik@c6fc956a7
  (https://github.com/mapnik/mapnik/compare/26d3084ea...c6fc956a7)
  - SVG CSS support https://github.com/mapnik/mapnik/pull/4123
  - Use mapnik::value_integer for `id` type in feature generator
  - GeoJSON - allow 'null' properties in `Feature` objects [#4177](https://github.com/mapnik/mapnik/issues/4177)
  - Implement `is_solid` using stdlib <algorithm> `find_if`
  - Add perfect forwarding in apply_visitor alias
  - Re-implement feature_json_generator by adapting feature_impl into boost::fusion container and removing use semantic actions (simpler code + boost_1_73 support) [#4143](https://github.com/mapnik/mapnik/issues/4143)
  - Relax bounding box extracting grammar [#4140](https://github.com/mapnik/mapnik/issues/4140)
  - mapnik::color - fix operator== [#4137](https://github.com/mapnik/mapnik/issues/4137)
  - color::swap - add missing premultiplied_ [#4137](https://github.com/mapnik/mapnik/issues/4137)
  - Add `spacing-offset` option https://github.com/mapnik/mapnik/pull/4132
  - Add Int32 support for gdal driver
- Remove `reinterpret_cast` by changing `Napi::Buffer<T>` specialisations
- Replace bogus Object->Value->Object conversions e.g scope.Escape(napi_value(obj)).ToObject() => scope.Escape(obj) (ref #961)
- Remove `Escape`from `env.Undefined()` (ref #961)
- Travis CI: enable clang-format + clang-tidy targets

## 4.5.4
- Ensure `mapnik.Image` data is valid during `AsyncWorker::Execute()` (ref #960)

## 4.5.3
- Set default `quality` value for webp format to 80 (ref #957)
- Fixes utfgrid support (ref #959)

## 4.5.2
- Fixed default `scale_denominator` value in Map::render(..)` (#952)
- Set JS objects properties `napi_writable` for backward compatibility with sinon usage (#954)

## 4.5.1
- Fixed 'scale_denominator' default value in `VectorTile::render(..)`(#952)
- Fixed wrong arguments order in AsyncComposite + added test (#953)

## 4.5.0
- Ported to N-API.
- Now supporting node v14 using a "universal binary" (same binary will be used for all major node versions)
- Update mapnik@26d3084ea
- Ported tests from mocha to tape
- Upgraded to boost 1.73.0
- Binaries now compiled with clang++ 10.x

## 4.4.0

- Update mapnik@3be9ce8fa
  (https://github.com/mapnik/mapnik/compare/a0ea7db1a...3be9ce8fa)
  - SVG renderer fixes  https://github.com/mapnik/mapnik/pull/4113
  - Add "darkslategray" and "rebeccapurple" named colors (ref: https://drafts.csswg.org/css-color/#typedef-color)
  - Use std::round (ref:https://en.cppreference.com/w/cpp/numeric/math/round) mapnik@ed194a3
  - Move 0.5 up/down rounding into rounding expression (via  @lightmare https://github.com/mapnik/mapnik/pull/4113/commits/7f54e947485df0a35e0dab9b5aacea8db2cff88c#r369294323) [#4116](https://github.com/mapnik/mapnik/issues/4116)
  - SVG: only use reflection of the second control point on the previous command relative to the current point as first control point when both last and prev( NOTE: before last command in AGG logic!) are curve commands. This fixes long outstanding SVG rendering bug aka `Octocat` bug [#4115](https://github.com/mapnik/mapnik/issues/4115)
  - SVG parser: fix typo (stroke gradient was applied instead if fill gradient) mapnik@1a0b1a1e77
  - Add support for `scale-factor` parameter - useful for debugging SVG issues (ref #4112)
  - Don't attempt to rasterize ARCs with very small sweep_angles, just resort to LINETO (#4112)
  - SVG parser: ix typo in agg_bezier_arc initialisation mapnik@0420b13055
  - SVG parser: use numeric parser for arc flags mapnik@60a33a9b
  - SVG parser: parse arc and sweep flags using special single digit parser, numeric `int_` parser was over greedy and didn't handle compact notation produced by svgo (https://github.com/svg/svgo) mapnik@222835e73
  - Use official colospace HSL/HSV converters from boost source tree (BOOST_VERSION > 1_69) mapnik@7f2c8b756a
  - Only push new elements to `parenStack` when needed [#4096](https://github.com/mapnik/mapnik/issues/4096)
  - Use `& mask` for array bounds clipping (provided array size is 2^n) mapnik@1edd3b7a930
  - Avoid potential out-of-bounds array access (undefined behaviour) + add c++ `C-array` size implementation mapnik@dec6bc095081
- Update protozero to v1.6.8
- Update geometry.hpp to v1.0.0
- Update wagyu to 0.5.0
- Drop support for Node versions less than 10.
- Consistent output image size compatible with other SVG libraries (#938)

## 4.3.1
- Update to mapnik@a0ea7db1a
  (https://github.com/mapnik/mapnik/compare/da69fdf66...a0ea7db1a)
  - Accept explicit parameter "application_name" in postgis and pgraster datasources https://github.com/mapnik/mapnik/pull/3984
  - Use `ST_MakeEnvelope` https://github.com/mapnik/mapnik/pull/3319
  - postgis.input: always put decimal point in substituted tokens in SQL  https://github.com/mapnik/mapnik/pull/3942
  - Replace MAPNIK_INIT_PRIORITY workaround
  - Ensure 'scaling' and `comp-op` stored as enumeration_wrappers + fix image `scaling` property https://github.com/mapnik/mapnik/pull/4066
  - New raster colorizer mode for Terrain-RGB https://github.com/mapnik/mapnik/pull/4058
  - mapnik@831e353c5 SVG parser:  better stderr - don't assume fill/stroke ref is a <gradient>, can be a <pattern> also
  - Adaptive `smooth` https://github.com/mapnik/mapnik/pull/4031
  - CSS parser - use appropriate storage type for `hue` value (0..320) https://github.com/mapnik/mapnik/issues/4020
  - mapnik@776fa0d2f True global pattern alignment, fixed local alignment
  - mapnik@692fc7f10 render_pattern() needs its own rasterizer
- Remove remaining "SVG parse error" error message headers

## 4.3.0
- Updated NAN bindings to support node12
- Added support for `remove_layer` on map object
- Fixed various resource leaks
- Remove bogus "SVG parse error" error message header

## 4.2.1
- Performance improvement in `mapnik.blend` and `mapnik.Image` methods by having it hold the event loop less time and copying less data into buffers
- Added `Map.remove_layer` method for removing layer from the map.

## 4.2.0
- `mapnik.Image.resize` will now accept non premultiplied images and return them back as non premultiplied images

## 4.1.0
- Added `offset_width` and `offset_height` optional parameters to the `mapnik.Image.resize` and `mapnik.Image.resizeSync` methods.
- Made `mapink.blend()` now accept `mapnik.Image` objects.

## 4.0.2
- Update to mapnik@da69fdf66

## 4.0.1

- Updated mapnik-vector-tile to fix strange clipping in vector tiles https://github.com/mapnik/node-mapnik/issues/892
- Added ability to release the underlying buffer from a vector tile when using `getData` method so no memory is copied.

## 4.0.0

- Added support for node v10
- Stop bundling node-pre-gyp
- Upgraded to mapnik@a2f5969 (pre-release master)
  - See https://github.com/mapnik/mapnik/wiki/API-changes-between-v3.0-and-v3.1 for changes
- Upgraded to protozero@1.6.2 (and also now pulling via submodule)
- Upgraded to mapnik-vector-tile@2.1.1 (https://github.com/mapbox/mapnik-vector-tile/blob/master/CHANGELOG.md#211)
- Pass variables to replace tokens in query (https://github.com/mapnik/node-mapnik/pull/809)
- Changed SVG parsing behavior to respect strict mode, and default to off.

## 3.7.2
- Upgraded to Mapnik v3.0.20

## 3.7.1

- Mapnik 3.7.0 was not properly published to npm with node-pre-gyp. Releasing again with fix.
- Fix to `SSE_MATH` flag during building


## 3.7.0

Updated to 3.0.18 of mapnik. See [here](https://github.com/mapnik/mapnik/blob/master/CHANGELOG.md).

- Updated to mapnik-vector-tile@1.6.1
- Removed windows support (https://github.com/mapnik/node-mapnik/issues/848)

## 3.6.2

Updated to 3.0.15 of mapnik. The full changelog for this release is located [here](https://github.com/mapnik/mapnik/blob/master/CHANGELOG.md#3015).

## 3.6.1

Updated for a fix associated with mapnik-vector-tile where images could be requested that would have a width or height of zero resulting
in exceptions.

Several fixes associated with different mapnik by updating to use 3.0.14. Please see mapnik change log for specifics. In
general note worthy changes from mapnik include stricter geojson parsing, fixes for raster plugin, fixes to image scaling,
changing the meaning of filter-factor, and improvements to the the TIFF decoder.

Due to changes in the mapnik core version during this update you should see some changes in the image rescaling of raster and gdal plugin source data. This will definitely change the expected output. This is due to fixes in long standing bugs in the mapnik library.

- Now supporting node v8
- Updated to mapnik-vector-tile@1.4.0
- Mapnik minimum version updated to 3.0.14. Does not work with mapnik 3.1.x currently.
- Fixed tests around zlib compression and decompression when comparing to node's implementation
- Fixes rare situation of seg faults during mapnik-vector-tile image processing.
- Corrects the resolution of images in mapnik-vector-tile when using parameters from postgis plugin.
- Updated to use `font_engine` `instance()` method explicitely, reflecting on changes brought by [3688](https://github.com/mapnik/mapnik/pull/3688)

## 3.6.0

This release has major performance and stability improvements.

The biggest change is the move to https://github.com/mapbox/wagyu for clipping polygons, which is faster and results in more robust results than the previous implementation based off the "clipper" http://www.angusj.com/delphi/clipper.php. The "clipper" was known to hang on very large polygons and could output self-intersecting polygons.

The second largest change is the update of all major C/C++ dependencies. The changelogs for each are listed below. The highlights are 1) the performance improvements in webp 1.6.0, 2) the many crashes fixed in harfbuzz (https://github.com/behdad/harfbuzz/issues/139), and critical security bugs fixed in libpng, libjpeg, and libtiff.

The third most important set of changes were to node-mapnik directly: for performance many functions now can premultiply as part of another async operation (to avoid needing an additional threadpool access for async premultiply - this matters under load when the threadpool may be full since access can block). And many functions that allocate images now protect from extreme allocation that could hang a machine and result in OOM. Additionally the address sanitizer caught several cases of undefined behavior.

- Added support for node v7
- Updated to 1.3.0 of Mapnik Vector Tile (https://github.com/mapbox/mapnik-vector-tile/blob/master/CHANGELOG.md#130)
- Removed Angus Clipper and replaced with Wagyu v0.4.2 (https://github.com/mapbox/wagyu)
- Upgraded to protozero@1.5.1
- Upgraded to mapnik-vector-tile@1.3.0
- Changed build system to use mason instead of mapnik-packaging
- Added docs for Map#queryPoint and Map#queryMapPoint, #701
- Added docs for plugins
- Fixed potential abort due to unhandled error in Mapnik when passing invalid image dimensions
- Now limiting size of image internally allocated for `image.fromSVGBytes` and `image.fromSVG`, #709
  Default `max_size` is 2046x2046. Pass `max_size` option to customize.
- Added `max_size` limitation + `premultiply` option to `Image.fromBytes`, #720
- Optimized `VectorTile.query` to use fewer allocations
- Fixed potential overflow in `Image.fromSVG`, refs #740
- Fixed support for generating Vector Tiles at > z30, #730
- Fixed invalid casts detected by address sanitizer, #739
- Binaries compiled with clang-3.9 and requiring at least GLIBCXX_3.4.21 from libstdc++ (https://github.com/mapnik/node-mapnik#depends)
- Binaries updated to use mapnik `v3.0.13`, see [changelog](https://github.com/mapnik/mapnik/blob/master/CHANGELOG.md#3013).
- Updated dependency versions (also visible in `install_mason.sh`):
  - jpeg_turbo 1.5.1 (previously 1.5.0 | https://github.com/libjpeg-turbo/libjpeg-turbo/releases/tag/1.5.1)
  - libpng 1.6.28 (previously 1.6.24 | http://www.libpng.org/pub/png/libpng.html)
  - libtiff 4.0.7 (previously 4.0.6 | http://www.simplesystems.org/libtiff/v4.0.7.html)
  - icu 57.1 (previously 56.1 | http://site.icu-project.org/download/57)
  - proj 4.9.3 (previously 4.9.2 | https://github.com/OSGeo/proj.4/blob/18e6f047af7962a6da4ae3d6122034db4f8fe935/NEWS#L1)
  - pixman 0.34.0 (no change)
  - cairo 1.14.8 (previously 1.14.6 | https://www.cairographics.org/news/cairo-1.14.8/)
  - webp 0.6.0 (previously 0.5.1 | https://chromium.googlesource.com/webm/libwebp/+/v0.6.0)
  - libgdal 2.1.3 (previously 2.1.1 | https://trac.osgeo.org/gdal/wiki/Release/2.1.3-News)
  - boost 1.63.0 (previously 1.61.0 | http://www.boost.org/users/history/version_1_63_0.html)
  - freetype 2.7.1 (previously 2.6.5 | https://sourceforge.net/projects/freetype/files/freetype2/2.7.1/)
  - harfbuzz 1.4.2 (previously 1.3.0 | https://github.com/behdad/harfbuzz/blob/8568588202dd718b089e43cd6d46f689c706f665/NEWS#L29)

## 3.5.14

- Added support for node v6.x
- Now persisting image buffer in `mapnik.Image.fromBufferSync` to prevent undefined behavior if buffer were to go out of scope (#677)
- Upgraded to mapnik-vector-tile@1.2.2
- Upgraded to protozero@1.4.2
- Added `typeName()` to `mapnik.Geometry`. This returns the GeoJSON type name of a geometry (@davidtheclark).
- Fixed potential unsigned integer overflow in `mapnik.blend`
- Binaries compiled with clang-3.8 and now requiring >= GLIBCXX_3.4.21 from libstdc++ (https://github.com/mapnik/node-mapnik#depends)
- Binaries updated to use mapnik `v3.0.12`, see [changelog](https://github.com/mapnik/mapnik/blob/master/CHANGELOG.md#3012).
- Binaries updated to use mapnik-packaging@7862fb9:
 - icu 56.1
 - boost 1.61.0
 - sqlite 3140100
 - freetype 2.6.5
 - harfbuzz 1.3.0
 - proj 4.9.2
 - libpng 1.6.24
 - libtiff 4.0.6
 - webp 0.5.1
 - jpeg-turbo 1.5.0
 - libpq 9.4.5
 - cairo 1.14.6
  - pixman 0.34.0
 - gdal 2.1.1
  - expat 2.2.0

## 3.5.13

- Updated to mapnik-vector-tile `1.2.0`, includes a fix for rare decoding situation in vector tiles where a tile would be incorrectly considered invalid.
- Still using mapnik `v3.0.11`

## 3.5.12

- Fix performance regression when passing raster through vector tile (via upgrade to mapnik-vector-tile@1.1.2)
- Still using mapnik `v3.0.11`

## 3.5.11

- Fix for numerical precision issue in mapnik vector tile where valid v2 vector tiles would be thrown as invalid
- Added new exception handling for toGeoJSON
- Still using mapnik `v3.0.11`

## 3.5.10

- Fix for a segfault in the vector tile clipping library
- Still using mapnik `v3.0.11`

## 3.5.9

- Updated to mapnik-vector-tile `1.1.0`
- Automatic updating of vector tiles from v1 to v2 no longer takes place automatically when using `setData` and `addData`.
- Validation of vector tiles is now optional when using `setData` and `addData`
- Still using mapnik `v3.0.11`

## 3.5.8

- Updated to mapnik-vector-tile `1.0.6` which includes a speedup on simplification for mapnik-vector-tile
- Still using mapnik `v3.0.11`

## 3.5.7

- Fixed a situation where repeated holes on top of each other could result in self intersections in vector tile geometries
- Improved the speed of vector tile creation by removing unrequired checks in clipper library
- Fixed a situation in clipper where horizontals could result in invalid self intersections.
- Prevent intersections outside the clipper from being processed after intersections inside the clipped area as this in very rare situations would cause an intersection.
- Updated to mapnik `3.0.11`, see [changelog](https://github.com/mapnik/mapnik/blob/master/CHANGELOG.md#3011).

## 3.5.6

- Another set of fixes for clipper where it would produce invalid polygons when creating vector tiles.
- Fixed another endless loop in clipper around vector tile creation

## 3.5.5

- Fixed a situation where the clipper would get locked in an endless loop when creating vector tiles.

## 3.5.4

- Updated the angus clipper with several fixes that solve intersections issues within vetor tile polygons created when using strictly simple
- Updated version of `mapnik-vector-tile` to `1.0.5` this solves a SEGFAULT that occurs in rare situations when encoding fails
- Corrected some problems in the documentation
- Added a new optional arguments `split_multi_features` and `lat_lon` to `mapnik.VectorTile.reportGeometryValidity` and `mapnik.VectorTile.reportGeometryValiditySync` that enables validity checks against the parts of multi geometries individually.

## 3.5.3 (DEPRECATED - bad package sent to npm)

- Stopped building node v0.12 binaries. Use node v4 or v5 instead if you need node-mapnik binaries.
- No code changes: Just a rebuild of 3.5.2 but with debug binaries that can be installed with `npm install --debug`

## 3.5.2

- Fixed bug in `mapnik-inspect.js` around using old `parse()` method on vector tiles, updated it to use `mapnik.VectorTile.info`
- Updated `mapnik-vector-tile` to `1.0.3` fixing issues with non valid vector tiles being created and linking issue in mapnik-vector-tile with latest mapnik library
- Updated clipper library to fix bug mentioned in `mapnik-vector-tile`

## 3.5.1

- Added the `mapnik.VectorTile.info` command that returns an object that inspects buffers and provides information about vector tiles.
- Updated `mapnik-vector-tile` to `1.0.2`

## 3.5.0

This is a major update and reflects a large number of changes added into node-mapnik due to update of the [Mapbox Vector Tile Specification](https://github.com/mapbox/vector-tile-spec). As part of this the [mapnik-vector-tile library](https://github.com/mapbox/mapnik-vector-tile) was updated to `1.0.0`. Therefore, a large number of interfaces changes have taken place around the `mapnik.VectorTile` object.

It is important to know that the concept of `width` and `height` have been removed from `mapnik.VectorTile` objects. This is replaced by the concept of `tileSize`. While `width` and `height` were based on the concept of an Image size created from a vector tile, `tileSize` is directly related to the `extent` as defined in the `Layer` of a vector tile. For understanding what the `Layer` and `extent` is please see the [Vector Tile Specification](https://github.com/mapbox/vector-tile-spec/tree/master/2.1#41-layers). This also changed the `buffer_size` arguments that were commonly used in many Vector Tile methods, which was also based on the *Image size*. The vector tile object now contains a `bufferSize` which represents the buffer added to the layer extent in a tile.

Internally, all methods now depend on V2 tiles, however, any V1 tiles that are loaded into a `mapnik.VectorTile` object will **automatically** be updated.

Summary of changes:

 - `mapnik.VectorTile.addData` now verifies buffers validity and internally updates v1 tiles to v2
 - `mapnik.VectorTile.addDataSync` now verifies buffers validity and internally updates v1 tiles to v2
 - `mapnik.VectorTile.setData` now verifies buffers validity and internally updates v1 tiles to v2
 - `mapnik.VectorTile.setDataSync` now verifies buffers validity and internally updates v1 tiles to v2
 - `mapnik.VectorTile.addImage` now takes a `mapnik.Image` object rather then a buffer, it also takes optional arguments image_scaling and image_format.
 - `mapnik.VectorTile.addImageBuffer` replaces the old functionality of of `mapnik.VectorTile.addImage`
 - Added `mapnik.VectorTile.addImageSync` and made `mapnik.VectorTile.addImage` accept a callback.
 - Added `mapnik.VectorTile.addImageBufferSync` and made `mapnik.VectorTile.addImageBuffer` accept a callback.
 - `mapnik.VectorTile.height()` method is removed
 - `mapnik.VectorTile.width()` method is removed
 - `mapnik.VectorTile.parse()` method is removed
 - `mapnik.VectorTile.IsSolid()` method is removed
 - `mapnik.shutdown()` is removed
 - Removed the dependency on libprotobuf library
 - Lowered memory requirements for vector tile creation and vector tile operations.
 - Duplicate layer names in `mapnik.VectorTile` objects are no longer permitted.
 - Added new `mapnik.VectorTile.extent()` method which returns the bounding box of a tile in EPSG:3857
 - Added new `mapnik.VectorTile.bufferedExtent()` method which returns the bounding box including buffer of a tile in EPSG:3857
 - Added new `mapnik.VectorTile.emptyLayers()` method which returns the name of layers which were not added to a tile during any tile rendering operation.
 - Added new `mapnik.VectorTile.paintedLayers()` method which returns the name of layers which were considered painted during rendering or layers that contain data.
 - Added new `mapnik.VetorTile.tileSize` property.
 - Added new `mapnik.VetorTile.bufferSize` property.
 - Updated many of the default configuration options on `mapnik.VectorTile` class methods
 - Removed the concept of `path_multiplier` from the code entirely.
 - Added optional arguments of `tile_size` and `buffer_size` to `mapnik.VectorTile` constructor.

## 3.4.19

 - Update to mapnik-core 3.0.11 with a fix to unquoted strings

## 3.4.18

 - Fixed decoding bug that assumed tags came before geometries in vector-tile layers

## 3.4.17

 - Binaries updated to use v3.0.10 and mapnik-packaging@d6ae1fb
 - Upgraded to protozero v1.3.0
 - Fixed invalid usage of `mapbox::variant` that was causing windows compiler crash

Notable Changes in Mapnik v3.0.10 include:

 - A shapefile index now is skipped instead of causing an error to be throw. The shapefile plugin will then
   proceed by reading without using an index. It is advisable to regenerate the indexes to maintain
   top performance.

Notable changes in the Mapnik SDK include:

 - sqlite 3100000->3110000
 - libpng 1.6.20->1.6.21
 - postgres 9.4.5->9.5.1
 - sparsehash 2.0.2->2.0.3

## 3.4.16

 - Fixed `image.resize` behavior when scaling images with alpha (https://github.com/mapnik/node-mapnik/issues/585)
 - Binaries updated to use v3.0.9-125-g5e30aee and mapnik-packaging@db696ed

Notable Changes in Mapnik v3.0.9-125-g5e30aee include:

 - Compare: https://github.com/mapnik/mapnik/compare/v3.0.9-48-gbb8cd10...v3.0.9-125-g5e30aee
 - Support for rendering `dash-array` in SVGs
 - SVG parser is now stricter (fails is all input is not parsable)
 - SVG parser now correctly handles optional separator `(,)` between multiple command parts
 - Optimized parsing of `png` format string
 - The `memory_datasource` now dynamically reports correct datasource type (vector or raster)
 - Upgraded `mapbox::variant@272f91c`

Notable changes in the Mapnik SDK include:

 - none

## 3.4.15

 - `vtile.query` now returns WGS84 `x_hit` and `y_hit` values of the nearest point/vertex
 - Upgraded to nan@2.2.0
 - Upgraded to mapnik-vector-tile@0.14.4

## 3.4.14

 - Binaries updated to use Mapnik v3.0.9-57-g9494bc1 and mapnik-packaging@039aa0d

Notable Changes in Mapnik v3.0.9-57-g9494bc1 include:

 - Fixed parsing of SVG `PathElement` (https://github.com/mapnik/mapnik/issues/3225)

## 3.4.13

 - BREAKING: shapefile index files must be regenerated if using the
   node-mapnik binaries which now default to Mapnik `v3.0.9-48-gbb8cd10` (see `Notable Changes in Mapnik` below for details).
 - Upgraded to node-pre-gyp@0.6.19
 - Upgraded to mapnik-vector-tile@0.14.2
   - Fixed premultiplication bug in raster encoding (#170)
 - Binaries updated to use Mapnik v3.0.9-48-gbb8cd10 and mapnik-packaging@039aa0d

Notable Changes in Mapnik v3.0.9-48-gbb8cd10 include:

 - BREAKING: any `.index` files accompanying a `.shp` must now be regenerated otherwise
   an error will be throw like `Error: invalid index file`. To avoid this error you can
   either delete the existing `.index` files, or ideally run `shapeindex` (or [mapnik-shapeindex.js](https://github.com/mapnik/node-mapnik/blob/master/bin/mapnik-shapeindex.js)) to recreate the `.index`.
   The trigger for this change was an optimization that required a new binary format for the shapefile indexes (https://github.com/mapnik/mapnik/pull/3217). It was a mistake of @springmeyer to bring this into node-mapnik minor release (I'm sorry).
 - WARNING: index files generated with this newer Mapnik are invalid for older versions of Mapnik.
 - Compare: https://github.com/mapnik/mapnik/compare/v3.0.9...v3.0.9-48-gbb8cd10
 - The `shapeindex` command now has a `--index-parts` option
 - Upgraded mapbox::variant@3ac6e46
 - JSON parsing now supports arbitrary (nested) attributes in `geometry`

Notable changes in the Mapnik SDK include:

 - Upgrade libpng 1.6.19 -> 1.6.20
 - Upgrade webp 0.4.4 -> 0.5.0
 - Upgrade sqlite3 3.9.2 -> 3.10.0

## 3.4.12

 - Exposed `image_scaling` and `image_format` in `vtile.composite` (https://github.com/mapnik/node-mapnik/pull/572)
   - Default format is now `webp` encoding rather than `jpeg` (to support transparency)
   - Default scaling is now `bilinear` rather than `near`
 - Binaries updated to use Mapnik v3.0.9-17-g75cb954 and mapnik-packaging@e29a81e

Notable Changes in Mapnik 3.0.9-17-g75cb954 include:

 - Support arbitrary (nested) attributes in JSON Geometry
 - Fixed `shapeindex` to avoid creating an index for null shapes

## 3.4.11

 - Expose `mapnik.Geometry.type` https://github.com/mapnik/node-mapnik/issues/562
 - Travis tests now run against `osx_image: xcode7`
 - Appveyor tests now run against `nodejs_version: 5.1.0`
 - Updated nan to `~2.1.0`
 - Updated node-pre-gyp to `~0.6.16`
 - Updated npm-windows-upgrade (https://github.com/mapnik/node-mapnik/issues/566)
 - Binaries updated to use Mapnik v3.0.9 and mapnik-packaging@1aa9705

Notable Changes in Mapnik 3.0.9 include:

 - The `mapnik-index` command now has a `--validate-features` option
 - CSV - change 'quote' auto-dection logic to handle mixed cases better
 - Fixed `shapeindex` for 3dpoints (https://github.com/mapnik/mapnik/issues/3184)
 - Fixed GeoJSON ordering (https://github.com/mapnik/mapnik/issues/3182)
 - Fixed parsing of empty GeoJSON FeatureCollections (https://github.com/mapnik/mapnik/issues/3167)
 - Invalid bbox is now instantiated with `std::numeric_limits<T>::max()` (https://github.com/mapnik/mapnik/commit/4d6a735f535c27561bb40567398aba19a88243d4)
 - Fixed raster scaling/nodata handling (https://github.com/mapnik/mapnik/pull/3147)
 - For more details see entries for https://github.com/mapnik/mapnik/blob/master/CHANGELOG.md#309

Notable changes in the Mapnik SDK include:

 - Upgrade harfbuzz 1.0.6 -> 1.1.2
 - Upgrade pixman 0.32.6 -> 0.32.8
 - Upgrade cairo 1.14.2 -> 1.14.4
 - Upgrade libxml2 2.9.2 -> 2.9.3
 - Upgrade postgres 9.4.0 -> 9.4.5
 - Upgrade libpng 1.6.18 -> 1.6.19
 - Upgrade icu_version 55.1 -> 56.1
 - Upgrade icu_version2 55_1 -> 56_1


## 3.4.10

 - Now bundling the `mapnik-index` command (https://github.com/mapnik/node-mapnik/pull/545)
 - Added `process_all_rings`  option to `VectorTile.composite`, `VectorTile.addGeoJSON`, and `Map.render`.
   This option enables invalid ring to be processed (and potentially kept during re-encoding) when creating vector tiles.
   By default this is set to `false`. Use with caution.
 - Added enum for all polygon fill types under `mapnik.polygonFillType`. Options are `nonZero`, `evenOdd`, `positive`,
   and `negative`
 - Added `fill_type`  option to `VectorTile.composite`, `VectorTile.addGeoJSON`, and `Map.render`. By default
   this is set to `mapnik.polygonFillType.nonZero`
 - Added `multi_polygon_union`  option to `VectorTile.composite`, `VectorTile.addGeoJSON`, and `Map.render`. By
   default this is set to `true`. This will cause multipolygons to undergo a union operation during vector tile
   creation.
 - Added `simplify_distance`  option to `VectorTile.composite`.
 - Added `max_extent` (bbox) option to `VectorTile.composite`. By default it is unset which means no
   clipping extent will be used for the operation. If provided the data will be restricted to this extent.
     - Landed in https://github.com/mapnik/node-mapnik/commit/ef3b12a36f529a1a8fbb70f4ddd6a92e1bd22008
     - Previously compositing was using a hardcoded global extent of `-20037508.34,-20037508.34,20037508.34,20037508.34` which meant that all vector tile data was being clipped to global extents. This was harmless in all cases except when data contained data outside of global extents intentionally in order to avoid rendering of lines and blurs being visible at tile boundaries.
 - Added `reencode` (boolean) option to `VectorTile.composite`. If `true` will trigger re-rendering
   even if the z/x/y of all tiles matches. If `false` (the default) then tiles will be concatenated for
   best performance.
 - Updated mapnik-vector-tile to `v0.14.1`
 - Binaries updated to use Mapnik v3.0.9-rc2 and mapnik-packaging@6f2f178

Notable Changes in Mapnik 3.0.9-rc2/3.0.8 include:

 - Improved support for natural earth shapefiles
 - Improved CSV and JSON parsing and error handling
 - Stricter GeoJSON parsing in geojson.input (https://github.com/mapnik/mapnik/issues/3125)
 - For more details see entries for https://github.com/mapnik/mapnik/blob/master/CHANGELOG.md#308 and https://github.com/mapnik/mapnik/blob/master/CHANGELOG.md#309

Notable changes in the Mapnik SDK include:
 - Upgrade freetype 2.6 -> 2.6.1
 - Upgrade proj 4.8.0 -> 4.9.2
 - Upgrade png 1.6.17 -> 1.6.18
 - Upgrade tiff 4.0.4 -> 4.0.6
 - Upgrade jpeg-turbo 1.4.1 -> 1.4.2
 - Upgrade GDAL 2.0.0 -> 2.0.1
 - Upgrade Harfbuzz 0.9.41 -> 1.0.6
 - Upgrade sqlite 3.8.10.2 -> 3.9.2
 - Upgrade webp 0.4.3 -> 0.4.4

## 3.4.9

 - Updated to use mapnik-vector-tile `0.13.0`
 - Linestrings will no longer contain repeated points when vector tiles are created
 - Added a new method called `decode_geometry` as an optional argument to `toJSON` for `mapnik.VectorTile` object
   this option provides decoded geometry in the raw form from the vector tile.
 - Updated to use a more recent version of the angus clipper library.
 - Binaries updated to use Mapnik v3.0.7 and mapnik-packaging@9606f72ef0

Notable Changes in Mapnik 3.0.7 include:

 - Fixed bugs in the PostGIS `key_field_as_attribute` behavior

Notable changes in the Mapnik SDK include:
 - Upgrade cairo 1.12.18 -> 1.14.2
 - Upgrade boost 1.58 -> 1.59

## 3.4.8

 - Now supporting Node v4.x
 - The `new mapnik.Palette` constructor no longer accepts a string - please pass a correctly encoded buffer.
 - Added `reportGeometrySimplicity` and `reportGeometryValidity` to `mapnik.VectorTile`. These check if the geometry in the vector tile is OGC simple or valid.
 - Added `strictlySimple` option when creating vector tiles.
 - Updated to use mapnik-vector-tile `0.12.0`

Notable Changes in Mapnik 3.0.5 include:
 - PostGIS plugin: added `key_field_as_attribute` option. Defaults to `True` to preserve current behavior of having the `key_field` added both
   as an attribute and as the `feature.id` value. If `key_field_as_attribute=false` is passed then the attribute is discarded (https://github.com/mapnik/mapnik/issues/3115)
 - CSV plugin has been further optimized and has gained experimental support for on-disk indexes (https://github.com/mapnik/mapnik/issues/3089)
 - SVG parser now fallsback to using `viewbox` if explicit dimensions are lacking (https://github.com/mapnik/mapnik/issues/3081)
 - Fixed parsing colors in hexadecimal notation (https://github.com/mapnik/mapnik/pull/3075)

## 3.4.7

 - Rebuilt with Mapnik 3.0.5
 - Added ability to create an image using a Buffer object. It should be noted that this should
   be used very carefully as the lifetime of the Image object is tied to that of the Buffer. If the
   buffer object is garbage collect this could result in a segfault.

Notable Changes in Mapnik 3.0.5 include:

 - `scale-hsla` image filter: parameters are no longer limited by interval \[0, 1\] (https://github.com/mapnik/mapnik/pull/3054)
 - Windows: Fixed SVG file loading from unicode paths
 - `colorize-alpha` image filter: fixed normalization of color components (https://github.com/mapnik/mapnik/pull/3058)
 - `colorize-alpha` image filter: added support for transparent colors (https://github.com/mapnik/mapnik/pull/3061)
 - Enable reading optional `MAPNIK_LOG_FORMAT` environment variable(https://github.com/mapnik/mapnik/commit/6d1ffc8a93008b8c0a89d87d68b59afb2cb3757f)
 - CSV.input uses memory mapped file by default on *nix.
 - Updated bundled fonts to the latest version
 - Topojson.input - fixed geometry_index logic which was causing missing features
 - Fixed SVG file loading from unicode paths (https://github.com/mapnik/node-mapnik/issues/517)
 - CSV.input - improved support for LF/CR/CRLF line endings on all platforms (https://github.com/mapnik/mapnik/issues/3065)

## 3.4.6

 - Enhanced `vtile.setData` and `vtile.getData` to have async signatures if callback passed as last argument.
 - Enhanced `vtile.setData` to accept gzip and zlib compressed data.
 - Enhanced `vtile.getData` to accept options to gzip compress data before returning buffer like `vtile.getData({compression:'gzip'})`
 - Rebuilt with 0.10.0 of Mapnik Vector Tile. This changes the way that `painted` method returns in `VectorTile` classes.
 - In `VectorTile` object `parse` is no longer required please consider it depreciated.
 - `VectorTile` now utilizes the protozero library for lower memory vector tile operations.

## 3.4.5

 - Rebuilt against Mapnik 3.0.4

Notable changes in Mapnik SDK include:

 - CSV.input: plug-in has been refactored to minimise memory usage and to improve handling of larger input.
   (NOTE: [large_csv](https://github.com/mapnik/mapnik/tree/large_csv) branch adds experimental trunsduction parser with deferred string initialisation)
 - CSV.input: added internal spatial index (boost::geometry::index::tree) for fast `bounding box` queries (https://github.com/mapnik/mapnik/pull/3010)
 - Fixed deadlock in recursive datasource registration via @zerebubuth (https://github.com/mapnik/mapnik/pull/3038)

## 3.4.4

 - Rebuilt against updated Mapnik SDK to fix mysterious zlib related build issue.

## 3.4.3

 - Upgrade to mapnik-vector-tile@0.9.2
 - Added `Image.filter` and `Image.filterSync` to filter images.
 - Binaries updated to use Mapnik v3.0.3
 - Upgrade to mapnik-vector-tile@0.9.3
   - Fixed multipoint encoding
   - Optimized geometry decoding

Notable changes in the Mapnik SDK include:

 - Fixed an issue with fields over size of `int32` in `OGR` plugin (https://github.com/mapnik/node-mapnik/issues/499)
 - Added 3 new image-filters to simulate types of colorblindness (`color-blind-protanope`,`color-blind-deuteranope`,`color-blind-tritanope`)
 - Fix so that null text boxes have no bounding boxes when attempting placement ( 162f82cba5b0fb984c425586c6a4b354917abc47 )
 - Patch to add legacy method for setting JPEG quality in images ( #3024 )
 - Added `filter_image` method which can modify an image in place or return a new image that is filtered
 - Added missing typedef's in `mapnik::geometry` to allow experimenting with different containers

## 3.4.2

 - Added `Image.fromSVG`, `Image.fromSVGBytes` and the equivilent Sync functions for each
 - Binaries updated to use Mapnik v3.0.2 and mapnik-packaging@049968d24

Notable changes in the Mapnik SDK include:

 - Added container to log SVG parsing errors
 - Reimplemented to use rapidxml for parsing XML (DOM)
 - Support both xml:id and id attributes ( xml:id takes precedence )
 - Added parse_id_from_url using boost::spirit
 - Added error tracking when parsing doubles
 - Unit tests for svg_parser to improve coverage
 - Fixed rx/ry validation for rounded_rect
 - Fixed dimensions parsing
 - Remove libxml2 dependency

## 3.4.1

 - Installing like `npm install mapnik --toolset=v140` now installs windows binaries built
   with Visual Studio 2015 (rather than 2014 CTP4)
 - Added support for `buffer_size` in `addGeoJSON` (#457)
 - Fixed bug in `render` method of VectorTile where invalid parameters could cause a segfault.
 - Added `mapnik.Image.resize` method that enables images to be resized.
 - Now setting `VRT_SHARED_SOURCE=0` (#437)
 - Removed usage of `V8::AdjustAmountOfExternalAllocatedMemory` in `mapnik.Image` and `mapnik.Grid` (#136)
 - Upgraded to node-pre-gyp@0.6.9
 - Upgrade to mapnik-vector-tile@0.8.5
   - Updated vector tile clipping so that it throws out polygons outside bbox of tile
 - Binaries updated to use Mapnik v3.0.1 and mapnik-packaging@049968d24

Notable changes in the Mapnik SDK include:

 - Update gdal 1.11.2->2.0.0
 - Update freetype 2.5.5->2.6
 - Update harfbuzz 0.9.40->0.9.41
 - Changed the offset algorithm such that offsets now will always be positive to the left of the direction of the path
 - Increased performance of text rendering
 - Fixed text placement performance after #2949 (#2963)
 - Fixed rendering behavior for text-minimum-path-length which regressed in 3.0.0 (#2990)
 - Fixed handling of xml:id in SVG parsing (#2989)
 - Fixed handling of out of range rx and ry in SVG rect (#2991)
 - Fixed reporting of envelope from mapnik::memory_datasource when new features are added (#2985)
 - Fixed parsing of GeoJSON when unknown properties encountered at FeatureCollection level (#2983)
 - Fixed parsing of GeoJSON when properties contained {} (#2964)
 - Fixed potential hang due to invalid use of line-geometry-transform (6d6cb15)
 - Moved unmaintained plugins out of core: osm, occi, and rasterlite (#2980)

## 3.4.0

 - `mapnik.imageType` is now passed in options to new mapnik.Image
 - Upgrade to mapnik-vector-tile@0.8.4
  - Fixes support for decoding known degenerate polygons (from AGG clipper)
  - Fixes support for handling data in alternative projections
  - Fixes support for geometry collections
  - Fixes support for skipping out of range coordinates to avoid aborting polygon clipping
  - Includes fixes to clipper to avoid aborting on out of range coordinates
  - Fixed support for gracefully handling proj4 transformation errors
 - Upgraded to node-pre-gyp@0.6.7
 - Binaries updated to use Mapnik v3.x (master branch) at 39eab41 and mapnik-packaging@3ab051556e

Notable changes in the Mapnik SDK include:

 - Fixed potential crash when rendering metatiles to webp
 - Now throws on missing style when loading map in strict mode
 - Now handling when proj4 returns HUGE_VAL
 - Fixed crash when jpeg reader is used to open a png
 - Fixed gamma pollution for dot symbolizer
 - Purged usage of `boost::ptr_vector` and `boost::unordered_map`
 - Support for GDAL 2.0
 - Update boost 1.57.0->1.58.0
 - Update icu 1.54.1->1.55.1
 - Update sqlite 3.8.8.2->3.8.10.2
 - Update png 1.6.16->1.6.17
 - Update tiff 4.0.4beta->4.0.4
 - Update jpeg-turbo 1.4.0->1.4.1

## 3.3.0
 - Ugraded to Mapnik 3.x version with totally new geometry storage
 - Upgrade to mapnik-vector-tile@0.8.0
 - Upgraded to node-pre-gyp@0.6.5
 - Added an additional parameter to Projection initialization. This prevents the initialization
   of a proj4 object internally. This will only be useful when reprojecting from epsg:4326 to
   epsg:3857 and vise versa.
 - Binaries updated to use Mapnik v3.x (master branch) at 126c777.

## 3.2.0
 - Added support for a variety of different grayscale images and `mapnik.imageType` list
   - `mapnik.imageType.null`
   - `mapnik.imageType.rgba8`
   - `mapnik.imageType.gray8`
   - `mapnik.imageType.gray8s`
   - `mapnik.imageType.gray16`
   - `mapnik.imageType.gray16s`
   - `mapnik.imageType.gray32`
   - `mapnik.imageType.gray32s`
   - `mapnik.imageType.gray32f`
   - `mapnik.imageType.gray64`
   - `mapnik.imageType.gray64s`
   - `mapnik.imageType.gray64f`
 - Added the ability to return colors optionally with `getPixel` on `Image` objects
 - Added new constructors for `Color` object
 - Added the concept of premultiplied to `Image` and `Color` objects
 - `Image` objects no longer have a `background` property
 - Added `fill` and `fillSync` methods to `Image` objects to replace `background` property
 - Added `imageCopy` to copy an image into a new image type
 - `Image` `rgba8` objects are not automatically premultiplied prior to using `composite` operation
 - Added image view support for all new grayscale image types
 - Modified tolerance option on `query` and `queryMany` to only include features within that tolerance into the vector tile.
 - Modified the `renderSync` method on the `Map` object to only take an optional parameters object. Format can still be set by passing format as a optional argument. This was done so that it mirrors `renderFileSync`. The default format if none is provide is 'png'
 - Changed name of method `hsl2rgb2` to `hsl2rgb`
 - Changed name of method `rgb2hsl2` to `rgb2hsl`
 - Removed format parameter from `Grid` and `GridView` objects `encode` and `encodeSync` methods as it had no affect.
 - Added `active`, `queryable`, `clear_label_cache`, `minzoom`, and `maxzoom` property to `Layer` objects
 - Added `compositeSync` to `VectorTile` object.
 - Changed `composite` in `VectorTile` to accept a callback
 - Upgraded to nan@1.7.0 and mapnik-vector-tile@0.7.1
 - Changed boolean on `Parameters` for `Map` object such that 1 and 0 are no longer boolean but integers.
 - Binaries updated to use Mapnik v3.x (master branch) at 3270d42b74821ac733db169487b5cd5d5748c1e6 and mapnik-packaging@6638de9b5b

Notable changes in the Mapnik SDK include:
 - Changes: https://github.com/mapnik/mapnik/compare/30c6cf636c...5a49842952
 - Mapnik TopoJSON plugin now supports optional `bbox` property on layer
 - Various improvements to Mapnik pgraster plugin
 - Mapnik GDAL plugin now keeps datasets open for the lifetime of the datasource (rather than per featureset)
 - Mapnik GDAL plugin now has optimized nodata handling for RGB images.
 - Mapnik no longer calls `dlclose` on gdal.input (mapnik/mapnik#2716)
 - Upgraded Clipper to v6.2.8 / svn r492.
 - Upgraded libtiff to 4.0.4beta
 - Upgraded libjpeg-turbo to 1.4.0
 - Upgraded GDAL to 1.11.2
 - Upgraded harfbuzz to 0.9.38

## 3.1.6

 - Now supporting IO.js 1.x and Node v0.12.x
 - Optimized `vtile.addGeoJSON` by switching to Mapnik native GeoJSON plugin internally rather than OGR.
 - Upgraded to node-pre-gyp@0.6.4

## 3.1.5

 - Security Fix: now throwing error instead of abort when vtile.getData() is called which needs to produce a node::Buffer larger than node::Buffer::kMaxLength (bed994a). However this condition did not previously happen due to integer overflow which is now also fixed (#371)
 - Now handling C++ exceptions in vt.composite to prevent possible abort (although none could be replicated)
 - Removed nik2img from binary packages (not useful since it duplicates ./bin/mapnik-render.js)
 - Added stress test benchmarks that live in ./bench folder of git repo
 - Added `isSolid` method to `Image` object
 - When making vector tiles that are larger then 64 MB changed node so that it would no longer through an abort but rather an exception
 - Added extra meta data for some datasource associated with the use of the `describe` method on datasources

Notable changes in the Mapnik SDK include:
 - Changes: https://github.com/mapnik/mapnik/compare/8063fa0...30c6cf636c
 - `shapeindex` now works properly for point 3d shapes
 - Improved auto-detection of `geometry_table` from sql subselects for PostGIS plugin
 - Fixed hextree encoder (will produce non-visible image differences)
 - Fixed bugs in GeoJSON parser
 - GroupSymbolizer now supports MarkersSymbolizer and not PointSymbolizer

## 3.1.4

 - Fixed bugs in `VectorTile.toGeoJSON` to ensure properly formatted JSON output.
 - Cleanup of Javascript code and tests using JSLint.
 - Added preliminary support for building against Nan v1.5.0 and IO.js v1.0.1 (but still using Nan v1.4.1 for the time being)
 - Added `mapnik.versions.mapnik_git_describe` to get access to the git details of the Mapnik version node-mapnik was built against.
 - Fixed `mapnik-inspect.js` script.
 - Binaries updated to use Mapnik v3.x (master branch) at 8063fa0 and mapnik-packaging@0cc6382

Notable changes in the Mapnik SDK include:
 - Changes: https://github.com/mapnik/mapnik/compare/1faaf595...8063fa0
 - Fixed marker properties to not override svg `fill:none` or `stroke:none`, which avoids unintended colorization of svg symbols
 - Added support for `text-transform:reverse`
 - Fixed utf8 output in json properties grammar
 - Upgraded to latest [Mapbox Variant](https://github.com/mapbox/variant)
 - Upgrade freetype 2.5.4 -> 2.5.5
 - Upgrade libpng 1.6.15 -> 1.6.16
 - Upgrade cairo 1.12.16 -> 1.12.18
 - Still pinned to harfbuzz 7d5e7613ced3dd39d05df83c

## 3.1.3

 - Now vt.composite `buffer-size` defaults to `1` instead of `256` and `tolerance` defaults to `8` instead of `1`.
 - Improvements to internals of mapnik.blend
 - Fixed rare error when reading image data with the async `mapnik.Image.fromBytes`
 - Binaries updated to use Mapnik v3.x (master branch) at 1faaf595 and mapnik-packaging@5a436d45e3513

Notable changes in the Mapnik SDK include:

 - New and experimental `dot` symbolizer.
 - GeoJSON/TopoJSON plugin now returns correct ids even if rendered twice.
 - `font-feature-settings` is now respected per text item.
 - image_data internals were refactored.
 - Ignore overviews with 0 scale in pgraster (@rafatower)
 - Fixed support for handling all SQLite columns (@StevenLooman)
 - Upgrade libpng 1.6.14->1.6.15
 - Upgrade freetype 2.5.3->2.5.4
 - Upgrade sqlite 3080701->3080704
 - Upgrade postgres 9.3.4->9.4.0
 - Upgrade openssl 1.0.1i->1.0.1j
 - Upgrade harfbuzz 0.9.35->0.9.37/7d5e7613ced3dd39d05df83ca7e8952cbecd68f6

## 3.1.2

 - Now providing 32 bit windows binaries in addition to 64 bit

## 3.1.1

 - Added `Map.registerFonts()`
 - Upgraded to node-pre-gyp@0.6.1
 - Aliased `mapnik.register_fonts()` -> `mapnik.registerFonts()`, `mapnik.register_datasources()` -> `mapnik.registerDatasources()`.
 - Binaries updated to use Mapnik v3.x (master branch) at 2577a6c and mapnik-packaging@759c4a32ba

## 3.1.0

 - Added [`mapnik.Logger`](https://github.com/mapnik/node-mapnik/blob/master/docs/Logger.md)
 - Added `Map.loadFonts`, `Map.fonts()`, `Map.fontFiles()`, `Map.fontDirectory()`, and `Map.memoryFonts()`
 - Added `Feature.fromJSON` and `Feature.geometry`
 - Added `Geometry.toJSON`
 - Removed: `Feature.numGeometries`, `Feature.addAttributes`, and `Feature.addGeometry`
 - BREAKING: `VectorTile.toGeoJSON` now returns a string
 - `VectorTile.toGeoJSON` now supports multigeometries and is async if callback is passed
 - Dropped build dependency on pkg-config (protobuf headers and libs are assumed to be installed at paths reported by mapnik-config)
 - Upgraded Nan to v1.4.0
 - Upgraded to mapnik-vector-tile@v0.6.1
 - Binaries updated to use Mapnik v3.x (master branch) at bff4465 and mapnik-packaging@fdc5b659d4

Notable changes in binaries:

 - Restored support for OS X 10.8

Notable changes in the Mapnik SDK include:
 - GDAL updated to 0334c2bed93f2
 - ICU 53.1 -> 54.1
 - xml2 2.9.1 -> 2.9.2
 - webp 0.4.0 -> 0.4.2
 - libpng 1.6.13 -> 1.6.14
 - sqlite 3.8.6 -> 3.8.7.1
 - boost 1.56 -> 1.57
 - protobuf 2.5.0 -> 2.6.1


## 3.0.5

 - Binaries updated to use Mapnik v3.x (master branch) at b90763469a and mapnik-packaging@f9e1c81b39
 - shapeindex and nik2img are now bundled

## 3.0.4

 - Binaries updated to use Mapnik v3.x (master branch) at 6b1c4d00e5 and mapnik-packaging@f9ec0f8d95

## 3.0.3

 - Fixed incorrect bundled deps

## 3.0.2

 - Upgraded Nan to v1.3.0
 - Binaries updated to use Mapnik v3.x (master branch) at 7bc956e and mapnik-packaging@9c7c9de

## 3.0.1

 - Binaries updated to use Mapnik v3.x (master branch) at 5a1126b

## 3.0.0

 - New 3.x series targeting Mapnik 3.x / C++11
 - Binaries for OS X require >= 10.9
 - Binaries for Linux require >= Ubuntu 14.04
 - First release providing binaries for Windows x64

Notable changes in binaries:

 - Built with `-std=c++11` and `-fvisibility=hidden` on OS X and Linux
 - Built with `-flto` on OS X (link time optimization) and target >= OS X 10.9
 - Built with Visual Studio 2014 CTP 3 on Windows (runtimes can be installed from https://mapnik.s3.amazonaws.com/dist/dev/VS-2014-runtime/vcredist_x64.exe and https://mapnik.s3.amazonaws.com/dist/dev/VS-2014-runtime/vcredist_x86.exe)
 - Binaries updated to use Mapnik v3.x (master branch) at f81fc53cbc and mapnik-packaging@6b7345248e

Notable changes in the Mapnik SDK include:
 - GDAL updated to 3fdc6e72b6e5cba
 - Libtiff updated to 14bca0edd83
 - Upgraded libpng from 1.6.12 -> 1.6.13

## 1.4.15

 - Upgraded to mapnik-vector-tile@0.5.5 for faster raster rendering

## 1.4.14

 - IMPORTANT: changes to shield placement (see Mapnik SDK notes below)
 - Added `mapnik.VectorTile.empty()` to check if a vector tile has any features (reports true if tile contains layers without features)
 - Avoid startup error if $HOME environment is not known
 - Fixed all tests on windows
 - Experimental: `mapnik.VectorTile` now accepts `variables` object in render options.
 - Experimental: Added `mapnik.Map.clone` method to create a shallow copy of a map object (datasources are still shared)

Notable changes in binaries:

 - Binaries updated to use Mapnik v2.3.x at 5ae55a07e and mapnik-packaging@b923eda6a
 - Enabled `-DSVG_RENDERER`, libtiff, and webp support for windows binaries

Notable changes in the Mapnik SDK include:
 - Fixed support for unicode paths names to XML files on Windows
 - Fixed `avoid-edges:true` support for shields (fixing frequently clipped shields) (https://github.com/mapnik/mapnik/issues/983)
 - Fixed shield bbox calculation when when `scale_factor` > 1 is used (also a cause of clipped shields) (https://github.com/mapnik/mapnik/issues/2381)
 - Upgraded boost from 1.55 -> 1.56
 - Upgraded sqlite from 3.8.5->3.8.6

## 1.4.13

 - Added `mapnik.Map.aspect_fix_mode` (#177)

Notable changes in binaries:

 - Binaries updated to use Mapnik v2.3.x at d62365c and mapnik-packaging@f012b82e6a

Notable changes in the Mapnik SDK include:
 - OGR Plugin no longer throws if layer is empty
 - Added new `aspect_fix_mode` called `RESPECT` that is a no-op
 - Upgraded harfbuzz from 0.9.32 -> 0.9.35

## 1.4.12

 - Fixed broken `postgis.input` plugin in binary package (#286)
 - New `mapnik.VectorTile.queryMany` method (@rsudekum)
 - Fixed mismatched new/delete in UTF8 grid encoding code (#278)
 - Updated to compile against latest Mapnik 3.x development version
 - Tweaked internal tracker of map concurrent usage to release before callback (should prevent spurious warnings like at mapbox/tilelive-mapnik#83)
 - Added missing `invert-rgb` compositing option (@mojodna)

Notable changes in binaries:

 - Now built with `-DSVG_RENDERER` enabled
 - Now compiled and linked with `clang++-3.4` on linux instead of `g++`
 - Now using a versioned binary module directory within `lib/binding/`.
 - Binaries updated to use Mapnik v2.3.x at a616e9d and mapnik-packaging@a5dbe90c61

Notable changes in the Mapnik SDK include:
 - Faster font i/o
 - Fixed support for multi-face font collections (.ttc files)
 - Fixed `comp-op:color` compositing to preserve luma (@mojodna)
 - Made `png` format string mean full color png (again) rather than paletted png8
 - OGR Plugin now accepts optional `extent` parameter (@kernelsanders)
 - New, experimental `pgraster` plugin (@strk)
 - Upgraded postgres from 9.3.3 -> 9.3.4
 - Upgraded harfbuzz from 0.9.29 -> 0.9.32
 - Upgraded png from 1.6.10 -> 1.6.12
 - Upgraded pixman from 0.32.4->0.32.6
 - Removed dependence on fontconfig

## 1.4.11

 - Never happened (npm shasum for published package was busted)

## 1.4.10

 - Fixed version of bundled node-pre-gyp

## 1.4.9

 - New mapnik.blend function that implements the node-blend API (https://github.com/mapbox/node-blend#usage)
 - Now supporting node v0.11.x via Nan@1.2.0
 - Binaries updated to use Mapnik v2.3.x at 7c7da1a2f and mapnik-packaging@1bbd8d560e9

Notable changes in the Mapnik SDK include:
 - Now building GDAL master (2.x) at https://github.com/OSGeo/gdal/commit/1106e43682056645c8c5e0dbf6ebcb69f8bf23cc
 - Mapnik image-filters now support correct alpha blending.
 - Mapnik PNG miniz encoding fixed.
 - Mapnik can now register fonts from directories containing non-ascii characters on windows.

## 1.4.8

 - Never happened (npm shasum for published package was busted)

## 1.4.7
 - Added `mapnik.Image.compare` function to compare the pixels of two images.
 - Fixed build issue leading to broken `ogr.input` Mapnik plugin
 - Auto-registers fonts found in paths via the `MAPNIK_FONT_PATH` environment
   variable

## 1.4.6
 - vtile.parse no longer throws if `vtile` was previously composited but no new data resulted.
 - Fixed compile problem on some linux/bsd systems
 - Binaries updated to use Mapnik v2.3.x at g7960be5 and mapnik-packaging@f007d6f1a6

Notable changes in the Mapnik SDK include:
 - GDAL now built `--with-threads=yes`
 - GDAL dependency now included as a shared library
 - Now building GDAL master (2.x) at https://github.com/OSGeo/gdal/commit/c0410a07c8b23e65071789a484fca0f3391fe0ff

## 1.4.5

 - Updated to use Mapnik 2.3.x SDK with rapidxml parsing fix: https://github.com/mapnik/mapnik/issues/2253

## 1.4.4

 - Updated to mapnik-vector-tile@0.5.0
 - Subtle VectorTile.composite bugs fixed to handle both tiles created from `setData` and those just rendered to.
 - `VectorTile.fromGeoJSON` method changed to `VectorTile.addGeoJSON`
 - Removed initializing and cleaning up global libxml2 structures because XML2 is no longer the default in node-mapnik binaries (#239)
 - Added support for pre-pending PATH when set in `mapnik_settings.js` (#258)
 - Binaries updated to use Mapnik v2.3.x at ed3afe5 and mapnik-packaging@5f9f0e0375

Notable changes in the Mapnik SDK include:
 - GDAL now built `--with-threads=no` and with a much more minimal set of drivers (https://github.com/mapnik/mapnik-packaging/commit/c572d73818ee4c9836171ae9fd49e950c6710d58)
 - Mapnik now build with rapidxml/ptree xml parser rather than libxml2 (meaning no xincludes or entities are supported)

## 1.4.3

 - VectorTile constructor now accepts optional 4th arg of options which respects `width` or `height`
 - `VectorTile.query` now returns `Feature` objects with `layer` and `distance` properties
 - New `VectorTile.fromGeoJSON` function to turn GeoJSON into a tile layer
 - New appveyor file for continuous builds on windows
 - Binaries updated to use Mapnik v2.3.x at 94e78f2ae4 and mapnik-packaging@45536bd3c9

Notable changes in the Mapnik SDK include:
 - Libtiff upgraded to CVS head at https://github.com/vadz/libtiff/commit/f696451cb05a8f33ec477eadcadd10fae9f58c39
 - Libfiff now built with `--enable-chunky-strip-read`
 - Harfbuzz updated to v0.9.28
 - Sqlite upgraded to 3.8.4.3/3080403

## 1.4.2

 - Now initializing and cleaning up global libxml2 structures to ensure safe async map loading (#239)
 - Fix publishing of mapnik package to npm to include bundled node-pre-gyp.
 - Binaries updated to use Mapnik v2.3.x at ce1ff99 and mapnik-packaging@49d8c3b.

Notable changes in Mapnik 2.3.x include:
 - mapnik line joins are now faster by discarding more nearly coincident points
 - postgis.input now links to fewer authentication libraries by only linking to libararies libpq was built against

Notable changes in the Mapnik SDK include:
 - ICU upgraded to 53.1 from 52.1
 - jpeg-turbo 1.3.1 is now used in place of libjpeg 8d (better performance likely)
 - sqlite3 upgraded to 3080401 from 3080200
 - webp upgraded to 0.4.0 from 0.3.1 (better performance likely)
 - postgres upgraded to 9.3.3 from 9.3.1
 - fontconfig upgraded to 2.11.0 from 2.10.0
 - libpng upgraded to 1.6.10 from 1.6.8
 - gdal upgraded to git master @ 0c10ddaa71

## 1.4.1

 - Binaries updated to use Mapnik v.2.3.x at 818e87d.
 - Improved build support for OS X Mavericks by autodetecting if linking against libc++ should be preferred.
 - `mapnik.register_system_fonts()` now registers opentype fonts as well as truetype fonts on linux (#231)

Notable changes in Mapnik 2.3.x include:
 - a potential double free was fixed in mapnik::projection
 - postgis plugin error reporting was fixed
 - postgis plugin !bbox! token replacement was fixed to use max<float> instead of max<double>

## 1.4.0

 - First series to default to providing binaries with `npm install` (bundling Mapnik v2.3.x at f202197)

## 1.3.4

 - Made vt.composite reentrant

## 1.3.3

 - Updated to mapnik-vector-tile@0.4.2

## 1.3.2

 - Pinned mapnik-vector-tile dependency to v0.4.1

## 1.3.1

 - Support for Mapnik 3.x

## 1.3.0

 - Updated to mapnik-vector-tile@0.4.0 with more encoding fixes and triggers VT layers without features to no longer be encoded
 - Every layer in a vector tile that matches a layer in the mapnik.Map is now rendered and not just the first (#213)
 - Added VectorTile.composite API
 - Fixed exception handling for `VectorTile.isSolid`
 - Datasource plugins must now be explicitly registered with `mapnik.register_default_input_plugins`, `mapnik.register_datasource`, or `register_datasources` as all default plugins are no longer automatically registered at startup.
 - Disabled `mapnik.Expression` object since this is not used by any known applications
 - Added `mapnik.register_datasource` to register a single datasource plugin.

## 1.2.3

 - Upgraded to mapnik-vector-tile@0.3.4 with multipart geometry fixes
 - Added toWKT/toWKB on mapnik.Feature
 - Added getPixel/setPixel on mapnik.Image
 - Added mapnik.VectorTile.query ability - accepts lon/lat in wgs84 and tolerances (in meters) returns array of features
 - Improvements to node-gyp path resolution in auxiliary Makefile and configure wrapper
 - Added `mapnik-config --ldflags` to build by default (not just when static linking)

## 1.2.2

 - Fixed windows build against Mapnik 2.3.0
 - Fixed mapnik.Image.open call - which was string data in invalid way
 - Upgraded to mapnik-vector-tile@0.3.3
 - Fixed build on OS X against node v0.6.x (tested v0.6.22)
 - Deprecated the `Datasource.features()` call #180

## 1.2.1

 - Added more details to `mapnik.supports` API including `grid`,`proj4`,`webp`,`jpeg`,`png`,`svg`,`cairo_pdf`,`cairo_svg`, and `threadsafe`
 - Added more constants for available `comp-op` values

## 1.2.0

 - Map.render (when rendering to a VectorTile) and VectorTile.render now expect `buffer_size` option to be passed and ignores map.BufferSize (#175)
 - Removed `devDependencies` so that `mocha` and `sphericalmecator` need to be manually installed to run tests
 - Tweaked gyp `Release` configuration to ensure binaries are stripped and built with highest level of optimization
 - Added support for detecting `--runtime_link=static` flag to npm install that can trigger linking against all Mapnik dependencies (not just libmapnik)
 - Added travis.ci support

## 1.1.3

 - Gyp binding cleanups
 - Removed direct icu::UnicodString usage to ensure robust compiles against icu build with `-DUSING_ICU_NAMESPACE=0`
 - Fixed variable shadowing issue in vtile -> geojson code
 - Disabled default debug symbol generation to speed up builds

## 1.1.2

 - Upgraded to mapnik-vector-tile 3.0.x API

## 1.1.1

 - Fixed extent of vector::tile_datasource to be sensitive to map buffer (TODO - long term
   plan is to make extent optional instead of adding support for layer specific buffered extent) - this is needed for avoiding too restrictive filtering of features at render time.
 - Fixed exception handling when creating geojson from vector tile
 - Build fixes to support python 3.x
 - Now accepting `scale_denominator`, `scale`, and `format` in options passed to `map.RenderSync`

## 1.1.0

 - Added support for node v0.11.x
 - Added async versions of Image methods: `fromBytes`, `open`, `premultiply`, `demultiply`
 - Added experimental support for rendering vector tiles to SVG. `renderer` option (either `cairo` or `svg`) controls whether `cairo` or native svg renderer is used
 - Exposed  `map.bufferedExtent` property to access the buffered extent
 - Changed Image.composite function to accept offsets (`dx` and `dy`), `comp_op`, `image_filters`, and `opacity` in options.
 - Fixed missing exception translation for MemoryDatasource and Image constructors
 - Fixed invalid default for `scale` in `map.render`
 - Implemented mapnik.Image.fromBytes (#147)

## 1.0.0
 - Dropped support for Mapnik versions older than v2.2.0
 - Moved build system to node-gyp - now supports node v0.10.x
 - Fonts are not longer auto-registered. Call `mapnik.register_default_fonts()` to register "DejaVu" set
   that is often bundled by Mapnik and call `mapnik.register_system_fonts()` to register fonts are various
   known system paths.
 - New mapnik.VectorTile API

## 0.7.24

 - Fixed tests after removal of example code (tests depended on it)

## 0.7.23

 - Added node v0.10.x support by moving build system from waf to node-gyp
 - All changes are in build system with only very minor changes to code
 - Moved example code to https://github.com/mapnik/node-mapnik-sample-code

## 0.7.22

 - Header include refactoring to ensure clean compiles again Mapnik 2.0.x, 2.1.x, and 2.2.x

## 0.7.21

 - Fix compile with latest Mapnik 2.2-pre (<limits> header)
 - Exposed Map.scale() (stefanklug)
 - More fixes for 64 bit integer support

## 0.7.20

 - Fix compile with Mapnik 2.1 (stefanklug)
 - Support 64 bit integers in grid types in anticipation of mapnik/mapnik#1662

## 0.7.19

 - Adapts to mapnik master's move to supporting 64 bit integers using `mapnik::value_integer`

## 0.7.18

 - Report null values in mapnik features as javascript null rather than undefined

## 0.7.17

 - Added sync/async `clear()` method to enable re-use of mapnik.Image and mapnik.Grid
   objects from a cache
 - Made ImageView and GridView `isSolid()` methods async if a callback is passed
 - Made async `isSolid()` return pixel value as second arg
 - Fixed code examples to work with generic-pool 2.x
 - Improved error reporting when an invalid image format is requested
 - Fixed possible edge-case memory corruption when encoding grids whose width != height

## 0.7.16

 - Fixed handling of datasource exception when calculating extent

## 0.7.15

2012-10-09

 - Minor compiler warning fixes

## 0.7.14

2012-09-16

 - Keep chasing Mapnik 2.2.0-pre API changes in symbolizers

## 0.7.13

2012-09-13

 - Fixed compile with <= Mapnik 2.1.0

## 0.7.12

2012-09-7

 - Keep chasing Mapnik 2.2.0-pre API changes in singletons

## 0.7.11

2012-09-5

 - Fixed compile with Mapnik 2.2.0-pre

## 0.7.10

2012-08-17

 - Makefile wrapper around node-waf now allows NPROCS option to be set.
 - Allow configure to work against Mapnik 2.2.0-pre without warning.

## 0.7.9

2012-08-1

 - Fixed broken usage of V8::AdjustAmountOfExternalAllocatedMemory
   which could trigger unneeded garbage collection pauses. (@strk)

## 0.7.8

2012-07-18

 - Fixed compile against Mapnik 2.0.x

## 0.7.7

2012-07-13

 - remove debug output when locally customized environment settings are
   pushed into process.env

## 0.7.6

2012-07-13

 - Further optimize grid encoding
 - Allow passing scale_factor to render to file functions  (#109)
 - Reference count Image and Grid objects in use by View objects to avoid
   possible scope issues resulting in segfaults when v8 garbage collects
   (#89, #110)

## 0.7.5

2012-07-12

 - Speed up grid encoding when featuresets are large: 12s -> 4s for
   processed_p at full zoom
 - Throw upon errors in grid rendering test
 - Fix mapnik version check in grid_view pixel value test
 - Amend expected test failures per version to account for platform
   differences in whether hidden fonts are around

## 0.7.4

2012-07-04

## 0.7.3

2012-06-29

## 0.7.2

2012-06-27

## 0.7.1

2012-04-27


## 0.7.0
2012-04-16

## 0.6.7

2012-03-09

## 0.5.17

2012-03-01

## 0.5.16

2012-01-23

## 0.5.15

2012-01-10

## 0.6.5

2012-01-10

## 0.6.4

2011-12-21

## 0.5.14

2011-12-21

## 0.6.3

2011-12-16

## 0.5.13

2011-12-16

## 0.5.12

2011-12-06

## 0.5.11

2011-12-06

## 0.6.2

2011-12-06

## 0.6.1

2011-11-30

## 0.5.10

2011-11-30

## 0.6.0

2011-11-19

## 0.5.9

2011-11-18

## 0.5.8

2011-10-19

## 0.5.7

2011-10-18

## 0.5.6

2011-10-03

## 0.5.5

2011-11-30

## 0.5.4

2011-08-23

## 0.5.3

2011-08-04

## 0.5.2

2011-08-04

## 0.5.1

2011-08-04

## 0.5.0

2011-08-03

## 0.4.1

2011-07-29

## 0.4.0

2011-06-27

## 0.3.1

2011-05-04

## 0.3.0

2011-04-29

## 0.2.13

2011-03-12

## 0.2.12

2011-03-03

## 0.2.11

2011-03-01

## 0.2.10

2011-02-28

## 0.2.9

2011-02-24

## 0.2.8

2011-02-11

## 0.2.7

2011-02-08

## 0.2.6

2011-02-08

## 0.2.5

2011-02-08

## 0.2.3

2011-01-27

## 0.1.2

2011-01-27

## 0.1.1

2011-01-27

## 0.1.0

2011-01-27
