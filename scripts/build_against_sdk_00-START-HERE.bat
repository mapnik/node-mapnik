@echo off
:: ============ USAGE
:: Powershell 3.0 or better needed.
:: Install from here: http://www.microsoft.com/en-us/download/details.aspx?id=40855
:: from the node-mapnik clone directory execute this file: scripts\build_against_sdk_00-START-HERE.bat
:: ============ Powershell Error
:: if you get "...  cannot be loaded because running scripts is disabled on this system ..."
:: open an admin prompt and enter: powershell Set-ExecutionPolicy Unrestricted
:: ============ ENV VARS
::Supported mapnik versions: 2.2.0, 2.3.0
SET MAPNIK_VERSION=2.3.0
SET MAPNIK_DIR=C:\mapnik-v%MAPNIK_VERSION%
SET LIB_XML_STATIC=1

SET BASE_DIR=C:\dev2
SET NODIST_DIR=%BASE_DIR%\nodist
SET PATH=C:\Program Files\7-Zip;%PATH%
SET PATH=c:\Python27;%PATH%
SET PATH=%NODIST_DIR%\bin;%PATH%
SET PATH=%MAPNIK_DIR%\lib;%PATH%
SET PATH=%MAPNIK_DIR%\bin;%PATH%
::add bundled node-pre-gyp to path
SET PATH=node_modules\.bin;%PATH%
SET PATH=%BASE_DIR%;%PATH%
SET PATH=C:\Program Files (x86)\MSBuild\12.0\bin\;%PATH%

SET MAPNIK_LIB_DIR=%MAPNIK_DIR%\lib
SET MAPNIK_PLUGIN_DIR=%MAPNIK_LIB_DIR%\mapnik\input
SET N_MAPNIK_BINDING_DIR=%CD%\lib\binding
SET N_MAPNIK_LIB_MAPNIK=%N_MAPNIK_BINDING_DIR%\mapnik
SET N_MAPNIK_LIB_SHARE=%N_MAPNIK_BINDING_DIR%\share
SET N_MAPNIK_STAGE_DIR=%CD%\build\stage
SET PYTHONPATH=%MAPNIK_DIR%\python\2.7\site-packages;
SET PROJ_LIB=%MAPNIK_DIR%\share\proj
SET GDAL_DATA=%MAPNIK_DIR%\share\gdal
SET DL_DIR=%BASE_DIR%\dl

powershell scripts\build_against_sdk_01-download-deps.ps1
IF ERRORLEVEL 1 GOTO ERROR

call node -e "console.log('node version: ' + process.version + ', architecture: ' + process.arch);"
IF ERRORLEVEL 1 GOTO ERROR
call npm install aws-sdk
IF ERRORLEVEL 1 GOTO ERROR
::nodist npm node-gyp is here
::C:\dev2\nodist\bin\node_modules\npm\node_modules\node-gyp
::node-pre-gyp@0.5.4 doesn't find it
call npm install node-gyp
IF ERRORLEVEL 1 GOTO ERROR
call npm install --build-from-source --msvs_version=2013 2>&1
IF ERRORLEVEL 1 GOTO ERROR

powershell scripts\build_against_sdk_02-copy-deps-to-bindingdir.ps1
IF ERRORLEVEL 1 GOTO ERROR

call npm test 2>&1
::comment following line, if the script should continue after failed tests
::IF ERRORLEVEL 1 GOTO ERROR

powershell scripts\build_against_sdk_03-write-mapnik.settings.ps1
IF ERRORLEVEL 1 GOTO ERROR

call node-pre-gyp build package --msvs_version=2013
IF ERRORLEVEL 1 GOTO ERROR
call node-pre-gyp unpublish
IF ERRORLEVEL 1 GOTO ERROR
call node-pre-gyp publish
IF ERRORLEVEL 1 GOTO ERROR

GOTO DONE

:ERROR
echo --------- ERROR! ------------

:DONE
