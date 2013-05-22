@echo off
if /i "%1"=="--libs" echo c:\\dev2\\mapnik-packaging\\windows\\build\\src\\msvc-10.0\\release\\threading-multi\\mapnik.lib icuuc.lib icuin.lib libboost_system-vc100-mt-s-1_49
if /i "%1"=="--dep-libpaths" echo c:\\dev2\\icu\\lib\\ c:\\dev2\\boost-49-vc100\\lib\\
if /i "%1"=="--defines" echo HAVE_JPEG HAVE_PNG HAVE_TIFF MAPNIK_USE_PROJ4 BOOST_REGEX_HAS_ICU MAPNIK_THREADSAFE BIGINT HAVE_LIBXML2 HAVE_CAIRO PLATFORM="win32" _WINDOWS __WINDOWS__
if /i "%1"=="--includes" echo c:\\mapnik-2.0\\include c:\\mapnik-2.0\\include\\mapnik\\agg c:\\dev2\\boost-49-vc100\\include\\boost-1_49 c:\\dev2\\icu\\include c:\\dev2\\cairo c:\\dev2\\cairo\\src c:\\dev2\\freetype c:\\dev2\\freetype\\include
if /i "%1"=="--input-plugins" echo c:\\mapnik-2.0\\lib\\mapnik\\input
if /i "%1"=="--fonts" echo c:\\mapnik-2.0\\lib\\mapnik\\fonts