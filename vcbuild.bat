set PROJ_LIB=C:\dev2\proj\nad
set GDAL_DATA=C:\dev2\gdal\data
set MAPNIK_INPUT_PLUGINS_DIRECTORY="c:\\mapnik-2.0\\lib\\mapnik\\input"
set MAPNIK_FONT_DIRECTORY="c:\\mapnik-2.0\\lib\\mapnik\\fonts"
rd /q /s build
del lib\\_mapnik.node
npm install
rem test!
set NODE_PATH=lib
mocha
@rem node -e "console.log(require('mapnik'))"