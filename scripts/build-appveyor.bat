@ECHO OFF
SETLOCAL
SET EL=0

ECHO =========== %~f0 ===========

git submodule update --init
IF %ERRORLEVEL% NEQ 0 ECHO could not update submodules && GOTO ERROR

:: only run Install-Product on AppVeyor
:: TODO: check requested node version on local builds and bail if it doesn't match
IF /I "%APPVEYOR%"=="True" powershell Install-Product node %nodejs_version% %PLATFORM%
IF %ERRORLEVEL% NEQ 0 ECHO could not install requested node version && GOTO ERROR

SET MAPNIK_GIT=
FOR /F "tokens=*" %%i in ('node -e "console.log(require(""./package.json"").mapnik_version)"') DO SET MAPNIK_GIT=%%i

:: use 64 bit python if platform is 64 bit
IF /I "%PLATFORM%" == "x64" set PATH=C:\Python27-x64;%PATH%
:: put 7z on path (needed for unpacking mapnik sdk)
SET PATH=C:\Program Files\7-Zip;%PATH%

:: TODO - dist/dev/ is intended for dev releases of mapnik
:: We ideally want to get in the habit of only using Mapnik official releases
:: which will be uploaded to dist/
SET MAPNIK_SDK_URL=https://mapnik.s3.amazonaws.com/dist/dev/mapnik-win-sdk-%MAPNIK_GIT%-%platform%-%msvs_toolset%.0.7z
ECHO fetching mapnik sdk^: %MAPNIK_SDK_URL%
IF EXIST mapnik-sdk.7z (ECHO already downloaded) ELSE (powershell Invoke-WebRequest "${env:MAPNIK_SDK_URL}" -OutFile mapnik-sdk.7z)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO extracting mapnik sdk
IF EXIST mapnik-sdk (ECHO already extracted) ELSE (7z -y x mapnik-sdk.7z | %windir%\system32\FIND "ing archive")
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:: HACK!! to make node@4.x x86 builds work
:: see: https://github.com/mapbox/node-pre-gyp/issues/209#issuecomment-217690537
CALL npm config set -g cafile=package.json
CALL npm config set -g strict-ssl=false

ECHO activating VS command prompt...
IF /I %platform% == x64 CALL "C:\Program Files (x86)\Microsoft Visual Studio %msvs_toolset%.0\VC\vcvarsall.bat" amd64
IF /I %platform% == x86 CALL "C:\Program Files (x86)\Microsoft Visual Studio %msvs_toolset%.0\VC\vcvarsall.bat" x86

ECHO available node.exe:
where node
ECHO available npm^:
where npm

ECHO node -v && CALL node -v
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO node that gets called && CALL node -e "console.log(process.arch,process.execPath)"
ECHO npm -v && CALL npm -v
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO ===== where npm puts stuff START ============
ECHO npm root && CALL npm root
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO npm root -g && CALL npm root -g
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO npm bin && CALL npm bin
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO npm bin -g && CALL npm bin -g
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET NPM_BIN_DIR=
FOR /F "tokens=*" %%i in ('CALL npm bin -g') DO SET NPM_BIN_DIR=%%i
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF /I "%NPM_BIN_DIR%"=="%CD%" ECHO ERROR npm bin -g equals local directory && SET ERRORLEVEL=1 && GOTO ERROR
ECHO ===== where npm puts stuff END ============

:: get ready to build agains the mapnik SDK
SET MAPNIK_SDK=%CD%\mapnik-sdk
SET PATH=%MAPNIK_SDK%\bin;%MAPNIK_SDK%\lib;%PATH%
SET PROJ_LIB=%MAPNIK_SDK%\share\proj
SET GDAL_DATA=%MAPNIK_SDK%\share\gdal
SET ICU_DATA=%MAPNIK_SDK%\share\icu

:: actually install deps + compile node-mapnik
ECHO building node-mapnik
:: --msvs_version=2015 is passed along to node-gyp here
CALL npm install --build-from-source --msvs_version=2015 --loglevel=http --node_shared=true
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

FOR /F "tokens=*" %%i in ('CALL node_modules\.bin\node-pre-gyp reveal module_path --silent') DO SET NODEMAPNIK_BINDING_DIR=%%i
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO NODEMAPNIK_BINDING_DIR^: %NODEMAPNIK_BINDING_DIR%
IF "%NODEMAPNIK_BINDING_DIR%"=="" ECHO ERROR could not determine binding dir && SET ERRORLEVEL=1 && GOTO ERROR

powershell scripts\build_against_sdk_02-copy-deps-to-bindingdir.ps1
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

powershell scripts\show_module_runtime.ps1 %CD%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

powershell scripts\build_against_sdk_03-write-mapnik.settings.ps1
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CALL npm test
:: uncomment to allow build to work even if tests do not pass
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO APPVEYOR_REPO_COMMIT_MESSAGE^: %APPVEYOR_REPO_COMMIT_MESSAGE%
SET CM=%APPVEYOR_REPO_COMMIT_MESSAGE%

:: publish binaries to v140 path
IF NOT "%CM%" == "%CM:[publish binary]=%" (ECHO publishing v140... && CALL node_modules\.bin\node-pre-gyp package publish --toolset=v140) ELSE (ECHO not publishing)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF NOT "%CM%" == "%CM:[republish binary]=%" (ECHO republishing v140 ... && CALL node_modules\.bin\node-pre-gyp package unpublish publish --toolset=v140) ELSE (ECHO not republishing)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO packaging ...
CALL node_modules\.bin\node-pre-gyp package
IF %ERRORLEVEL% NEQ 0 ECHO error during packaging && GOTO ERROR

:: publish binaries to default path
:: in the future this may change depending on:
:: 1) what visual studio versions we support
:: 2) how visual studio binaries are or are not compatible with each other
:: more details: https://github.com/mapnik/node-mapnik/issues/756
IF NOT "%CM%" == "%CM:[publish binary]=%" (ECHO publishing... && CALL node_modules\.bin\node-pre-gyp publish) ELSE (ECHO not publishing)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF NOT "%CM%" == "%CM:[republish binary]=%" (ECHO republishing ... && CALL node_modules\.bin\node-pre-gyp unpublish publish) ELSE (ECHO not republishing)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


GOTO DONE

:ERROR
ECHO =========== ERROR %~f0 ===========
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO =========== DONE %~f0 ===========

EXIT /b %EL%
