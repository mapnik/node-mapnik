@ECHO OFF
SETLOCAL
SET EL=0

ECHO =========== %~f0 ===========

::build system (https://github.com/mapbox/windows-builds) has to be setup up
::and mapnik and all its dependencies have to be built already
::and settings.bat must have been run to have all env vars available
SET USE_LOCAL_MAPNIK_SDK=0

SET APPVEYOR_REPO_COMMIT_MESSAGE=no commit message provided
SET MAPNIK_GIT=
FOR /F "tokens=*" %%i in ('node -e "console.log(require(""./package.json"").mapnik_version)"') DO SET MAPNIK_GIT=%%i
SET nodejs_version=5.1.0
SET platform=x64
SET msvs_toolset=14
SET TOOLSET_ARGS=--dist-url=https://s3.amazonaws.com/mapbox/node-cpp11 --toolset=v140


:::::::::::::: OVERRIDE PARAMETERS
:NEXT-ARG

IF '%1'=='' GOTO ARGS-DONE
ECHO setting %1
SET %1
SHIFT
GOTO NEXT-ARG

:ARGS-DONE


IF %USE_LOCAL_MAPNIK_SDK% EQU 0 GOTO START_BUILD

SET LOCAL_MAPNIK_SDK_DIR=%PKGDIR%\mapnik-%MAPNIKBRANCH%\mapnik-gyp\mapnik-sdk
ECHO ----------- using local mapnik build !!!!!!!!!!!!!!!!!! && ECHO copying mapnik SDK...
ECHO %LOCAL_MAPNIK_SDK_DIR%
XCOPY /Y /Q /S /E %LOCAL_MAPNIK_SDK_DIR%\*.* .\mapnik-sdk\
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:START_BUILD

ECHO APPVEYOR_REPO_COMMIT_MESSAGE^: %APPVEYOR_REPO_COMMIT_MESSAGE%
ECHO MAPNIK_GIT^: %MAPNIK_GIT%
ECHO nodejs_version^: %nodejs_version%
ECHO platform^: %platform%
ECHO msvs_toolset^: %msvs_toolset%
ECHO TOOLSET_ARGS^: %TOOLSET_ARGS%
ECHO USE_LOCAL_MAPNIK_SDK^: %USE_LOCAL_MAPNIK_SDK%


ECHO calling build-appveyor.bat... && CALL scripts\build-appveyor.bat
ECHO build-appveyor.bat finshed, ERRORLEVEL^: %ERRORLEVEL%
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE

:ERROR
ECHO ~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~
SET EL=%ERRORLEVEL%
ECHO ERRORLEVEL^: %EL%

:DONE
ECHO =========== DONE %~f0 ===========
EXIT /B %EL%