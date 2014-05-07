set NODIST_X64=0
call nodist use stable
call node -e "console.log(process.version + ' ' + process.arch)"
set PROJ_LIB=C:\mapnik-v2.3.0\share\proj
set GDAL_DATA=C:\mapnik-v2.3.0\share\gdal
@rem rd /q /s build
rd /q /s lib\binding
@rem call npm install
call .\node_modules\.bin\node-pre-gyp build
npm test
