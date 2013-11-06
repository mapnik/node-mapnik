set PROJ_LIB=C:\mapnik-v2.3.0\share\proj
set GDAL_DATA=C:\mapnik-v2.3.0\share\gdal
rd /q /s build
del lib\\_mapnik.node
npm install
set NODE_PATH=lib
npm test
