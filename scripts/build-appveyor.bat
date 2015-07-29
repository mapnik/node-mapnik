@ECHO OFF
SET EL=0

ECHO =========== %~f0 ===========

ECHO renaming all node.exe, from path and cwd
FOR /F "tokens=*" %%i in ('where node') DO ECHO found: %%i && (IF EXIST %%i.bak (ECHO deleting existing %%i.bak && DEL /F "%%i.bak")) && (ECHO renaming && REN "%%i" "%%~nxi.bak")
::dont goto ERROR, also returns 1, if nothing found
::IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF  %ERRORLEVEL% NEQ 0 SET ERRORLEVEL=0 && ECHO ---- WARNING^! ERRORLEVEL^: %ERRORLEVEL%

::use 64 bit python if platform is 64 bit
IF /I "%PLATFORM%" == "x64" set PATH=C:\Python27-x64;%PATH%
SET PATH=C:\Program Files\7-Zip;%PATH%
::add local node to path (since we install it below)
SET PATH=%CD%;%PATH%;

::SET PATH=C:\Program Files (x86)\MSBuild\%msvs_toolset%.0\bin;%PATH%
::SET PATH=C:\Program Files (x86)\Microsoft Visual Studio %msvs_toolset%.0\VC\bin;%PATH%

SET MAPNIK_SDK_URL=https://mapnik.s3.amazonaws.com/dist/dev/mapnik-win-sdk-v%MAPNIK_GIT%-%platform%-%msvs_toolset%.0.7z
ECHO fetching mapnik sdk^: %MAPNIK_SDK_URL%
IF EXIST mapnik-sdk.7z (ECHO already downloaded) ELSE (powershell Invoke-WebRequest "${env:MAPNIK_SDK_URL}" -OutFile mapnik-sdk.7z)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO extracting mapnik sdk
IF EXIST mapnik-sdk (ECHO already extracted) ELSE (7z -y x mapnik-sdk.7z | %windir%\system32\FIND "ing archive")
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


::install node version per visual studio toolset
SET ARCHPATH=
IF /I %platform% == x64 (SET ARCHPATH=x64/)
SET NODE_URL=https://mapbox.s3.amazonaws.com/node-cpp11/v%nodejs_version%/%ARCHPATH%node.exe
ECHO fetching node.exe^: %NODE_URL%
powershell Invoke-WebRequest "${env:NODE_URL}" -OutFile node.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO clear out node-gyp header cache
IF "%msvs_toolset%" == "14" IF EXIST %USERPROFILE%\.node-gyp rd /s /q %USERPROFILE%\.node-gyp

::add local node-pre-gyp dir to path
SET PATH=node_modules\.bin;%PATH%

ECHO activating VS command prompt
IF /I %platform% == x64 CALL "C:\Program Files (x86)\Microsoft Visual Studio %msvs_toolset%.0\VC\vcvarsall.bat" amd64
IF /I %platform% == x86 CALL "C:\Program Files (x86)\Microsoft Visual Studio %msvs_toolset%.0\VC\vcvarsall.bat" x86

node -v
node -e "console.log(process.arch,process.execPath)"

ECHO installing and updating node-gyp
CALL npm install -g node-gyp
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
CALL npm update -g node-gyp
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

if EXIST node_modules (ECHO node_modules found) ELSE (ECHO bootstrapping modules && CALL npm install mapnik-vector-tile nan sphericalmercator mocha node-pre-gyp jshint)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


SET MAPNIK_SDK=%CD%\mapnik-sdk
SET PATH=%MAPNIK_SDK%\bin;%MAPNIK_SDK%\lib;%PATH%
SET PROJ_LIB=%MAPNIK_SDK%\share\proj
SET GDAL_DATA=%MAPNIK_SDK%\share\gdal
SET ICU_DATA=%MAPNIK_SDK%\share\icu

ECHO building node-mapnik
CALL npm install --build-from-source --msvs_version=2015 %TOOLSET_ARGS% --loglevel=http
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

FOR /F "tokens=*" %%i in ('CALL node_modules\.bin\node-pre-gyp reveal module_path --silent') DO SET NODEMAPNIK_BINDING_DIR=%%i
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

powershell scripts\build_against_sdk_02-copy-deps-to-bindingdir.ps1
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

powershell scripts\show_module_runtime.ps1 %CD%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

powershell scripts\build_against_sdk_03-write-mapnik.settings.ps1
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CALL npm test
::IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CALL node-pre-gyp package testpackage %TOOLSET_ARGS%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO APPVEYOR_REPO_COMMIT_MESSAGE^: %APPVEYOR_REPO_COMMIT_MESSAGE%
SET CM=%APPVEYOR_REPO_COMMIT_MESSAGE%
IF NOT "%CM%" == "%CM:[publish binary]=%" (ECHO publishing... && CALL node-pre-gyp --msvs_version=2015 publish %TOOLSET_ARGS%) ELSE (ECHO not publishing)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF NOT "%CM%" == "%CM:[republish binary]=%" (ECHO republishing ... && CALL node-pre-gyp --msvs_version=2015 unpublish publish %TOOLSET_ARGS%) ELSE (ECHO not republishing)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


GOTO DONE

:ERROR
ECHO =========== ERROR %~f0 ===========
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO =========== DONE %~f0 ===========

EXIT /b %EL%
