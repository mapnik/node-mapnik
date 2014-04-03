# Changelog

## 1.4.2

 - Now initializing and cleaning up global libxml structures to ensure safe async map loading (#239)
 - Fix publishing of mapnik package to npm to include bundled node-pre-gyp.
 - Binaries updated to use Mapnik v2.3.x at ce1ff99 and mapnik-packaging@49d8c3b.

Notable changes in Mapnik 2.3.x include:
 - mapnik line joins are now faster by discarding more nearly coincident points
 - postgis.input now links to fewer authentication libraries by only linking to libararies libpq was built against

Notable changes in the Mapnik SDK include:
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
