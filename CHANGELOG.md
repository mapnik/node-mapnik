# Changlog

## 0.7.25

 - Removed stale file: `build.gyp` which could in some situations break the build because npm
   tries to rename it and will blow away the main `binding.gyp`

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
