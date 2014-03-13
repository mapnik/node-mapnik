@echo off
:: ============ USAGE
:: from the node-mapnik clone directory execute: scripts\build_against_sdk.bat
:: ============ Powershell Error
:: if you get "...  cannot be loaded because running scripts is disabled on this system ..."
:: open an admin prompt and enter: powershell Set-ExecutionPolicy Unrestricted
:: ============ ENV VARS
SET PATH=C:\Program Files\7-Zip;%PATH%
SET PATH=c:\Python27;%PATH%
SET PATH=c:\dev2\nodist\bin;%PATH%
SET PATH=C:\mapnik-v2.2.0\lib;%PATH%
SET PATH=C:\mapnik-v2.2.0\bin;%PATH%
::include node-pre-gyp
SET PATH=node_modules\.bin;%PATH%
SET PROJ_LIB=C:\mapnik-v2.2.0\share\proj
SET GDAL_DATA=C:\mapnik-v2.2.0\share\gdal
SET BASE_DIR=C:\dev2
SET DL_DIR=%BASE_DIR%\dl
SET MAPNIK_DIR=C:\mapnik-v2.2.0
SET MAPNIK_LIB_DIR=%MAPNIK_DIR%\lib
SET MAPNIK_PLUGIN_DIR=%MAPNIK_LIB_DIR%\mapnik\input
SET N_MAPNIK_BINDING_DIR=%CD%\lib\binding
SET N_MAPNIK_STAGE_DIR=%CD%\build\stage

powershell scripts\build_against_sdk_01-download-deps.ps1
IF ERRORLEVEL 1 GOTO ERROR

set NODIST_X64=0
call nodist stable 2>&1
IF ERRORLEVEL 1 GOTO ERROR
call nodist update 2>&1
IF ERRORLEVEL 1 GOTO ERROR
call node -e "console.log(process.version + ' ' + process.arch);"
IF ERRORLEVEL 1 GOTO ERROR
call npm install -g node-gyp
IF ERRORLEVEL 1 GOTO ERROR
call npm install --build-from-source 2>&1
IF ERRORLEVEL 1 GOTO ERROR

powershell scripts\build_against_sdk_02-copy-deps-to-bindingdir.ps1
IF ERRORLEVEL 1 GOTO ERROR

call npm test 2>&1
IF ERRORLEVEL 1 GOTO ERROR
call node-pre-gyp build package
IF ERRORLEVEL 1 GOTO ERROR
::call node-pre-gyp package
::IF ERRORLEVEL 1 GOTO ERROR

GOTO DONE

:ERROR
echo --------- ERROR! ------------

:DONE
