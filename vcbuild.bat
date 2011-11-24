set PROJ_LIB=C:\dev2\proj\nad
set GDAL_DATA=C:\dev2\gdal\data
set MAPNIK_INPUT_PLUGINS="c:\\mapnik-2.0\\lib\\mapnik\\input"
set MAPNIK_FONTS="c:\\mapnik-2.0\\lib\\mapnik\\fonts"
set target=Build
set config=Release
python gen_settings.py
rm build.sln
python gyp/gyp build.gyp --depth=. -f msvs -G msvs_version=2010
rem msbuild build.sln /t:%target% /p:Configuration=%config%
msbuild build.sln
copy Default\\_mapnik.node lib\\_mapnik.node
rem test!
set NODE_PATH=lib
node node_modules\expresso\bin\expresso
rem node -e "console.log(require('mapnik'))"