@ECHO OFF
SETLOCAL
SET EL=0

ECHO =========== %~f0 ===========

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

:: replace installed node.exe with Mapbox node.exe
:: so that the binaries based on Visual Studio 2015
:: and built by https://travis-ci.org/mapbox/node-cpp11 are used
SET ARCHPATH=
IF /I %platform% == x64 (SET ARCHPATH=x64/)
SET NODE_URL=https://mapbox.s3.amazonaws.com/node-cpp11/v%nodejs_version%/%ARCHPATH%node.exe
ECHO fetching node.exe^: %NODE_URL%
powershell Invoke-WebRequest "${env:NODE_URL}" -OutFile node.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:: we replace the existing node.exe with our own
:: we do this in place so that the npm path logic
:: is preserved. The alternative might be to put
:: our custom node.exe on PATH from a custom location
:: and then pass `--prefix` to npm - but this is untested
ECHO deleting node.exe programfiles x64
IF EXIST "%ProgramFiles%\nodejs" IF EXIST "%ProgramFiles%\nodejs\node.exe" ECHO found "%ProgramFiles%\nodejs\node.exe", deleting... && DEL /F "%ProgramFiles%\nodejs\node.exe"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO copying node.exe to programfiles x64
IF EXIST %ProgramFiles%\nodejs ECHO copying to "%ProgramFiles%\nodejs\node.exe" && COPY /Y node.exe "%ProgramFiles%\nodejs\"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO deleting node.exe programfiles x86
IF DEFINED ProgramFiles(x86) IF EXIST "%ProgramFiles(x86)%\nodejs" IF EXIST "%ProgramFiles(x86)%\nodejs\node.exe" ECHO "found %ProgramFiles(x86)%\nodejs\node.exe", deleting... && DEL /F "%ProgramFiles(x86)%\nodejs\node.exe"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO copying node.exe to programfiles x86
IF DEFINED ProgramFiles(x86) IF EXIST "%ProgramFiles(x86)%\nodejs" ECHO copying to "%ProgramFiles(x86)%\nodejs\node.exe" && COPY /Y node.exe "%ProgramFiles(x86)%\nodejs\"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

::delete node.exe in current directory, that newer npm versions put stuff into the right directories
IF EXIST node.exe DEL node.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF EXIST node.pdb DEL node.pdb
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:: this is not needed on a fresh machine without previous installs
:: but is important on a machine that already has compiled for a given
:: node version. So, we have to clear to ensure that the Visual Studio 2015
:: binaries are used rather than the ones from joyent (which are built with Visual Studio 2013)
ECHO clear out node-gyp binary cache to ensure vs 2015 binaries are linked
IF "%msvs_toolset%" == "14" IF EXIST %USERPROFILE%\.node-gyp rd /s /q %USERPROFILE%\.node-gyp

::upgrade npm to get consistent behaviour with older node versions
powershell Set-ExecutionPolicy Unrestricted -Scope CurrentUser -Force
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
CALL npm install -g npm-windows-upgrade@0.5.3
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
CALL npm-windows-upgrade --version:3.3.2 --no-dns-check --no-prompt
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


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

:: install node-gyp globally to:
:: 1) ensure node-gyp can find it (probably optional)
:: 2) ensure we have recent enough node-gyp to understand VS 2015 (needed for node v0.10.x certainly)
ECHO installing node-gyp
CALL npm install -g node-gyp
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO ERRORLEVEL^: %ERRORLEVEL%

:: get ready to build agains the mapnik SDK
SET MAPNIK_SDK=%CD%\mapnik-sdk
SET PATH=%MAPNIK_SDK%\bin;%MAPNIK_SDK%\lib;%PATH%
SET PROJ_LIB=%MAPNIK_SDK%\share\proj
SET GDAL_DATA=%MAPNIK_SDK%\share\gdal
SET ICU_DATA=%MAPNIK_SDK%\share\icu

:: actually install deps + compile node-mapnik
ECHO building node-mapnik
CALL npm install --build-from-source --msvs_version=2015 %TOOLSET_ARGS% --loglevel=http
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

CALL node_modules\.bin\node-pre-gyp package %TOOLSET_ARGS%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CALL npm test
:: uncomment to allow build to work even if tests do not pass
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO APPVEYOR_REPO_COMMIT_MESSAGE^: %APPVEYOR_REPO_COMMIT_MESSAGE%
SET CM=%APPVEYOR_REPO_COMMIT_MESSAGE%
IF NOT "%CM%" == "%CM:[publish binary]=%" (ECHO publishing... && CALL node_modules\.bin\node-pre-gyp --msvs_version=2015 publish %TOOLSET_ARGS%) ELSE (ECHO not publishing)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF NOT "%CM%" == "%CM:[republish binary]=%" (ECHO republishing ... && CALL node_modules\.bin\node-pre-gyp --msvs_version=2015 unpublish publish %TOOLSET_ARGS%) ELSE (ECHO not republishing)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


GOTO DONE

:ERROR
ECHO =========== ERROR %~f0 ===========
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO =========== DONE %~f0 ===========

EXIT /b %EL%
