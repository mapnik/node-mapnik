# Changelog

## 3.2.1
 - Added an additional parameter to Projection intialization. This prevents the intialization of a proj4 object internally. This will only be useful when reprojecting from epsg:4326 to epsg:3857 and vise versa. 

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
