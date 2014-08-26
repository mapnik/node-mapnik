call node -e "console.log(process.version + ' ' + process.arch)"
set PROJ_LIB=C:\code\mapnik-deps\packages\mapnik-sdk\share\proj
set GDAL_DATA=C:\code\mapnik-deps\packages\mapnik-sdk\share\gdal
set PATH=C:\code\mapnik-deps\packages\mapnik-sdk\bin;%PATH%
set PATH=C:\code\mapnik-deps\packages\mapnik-sdk\libs;%PATH%
::rd /q /s build
rd /q /s lib\binding
::call npm install
call .\node_modules\.bin\node-pre-gyp build --msvs_version=2013