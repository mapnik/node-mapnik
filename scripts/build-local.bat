@ECHO OFF

ECHO =========== %~f0 ===========


SET APPVEYOR_REPO_COMMIT_MESSAGE=no commit message provided
SET MAPNIK_GIT=3.0.1-4-ga78a895
SET nodejs_version=0.10.40
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


ECHO APPVEYOR_REPO_COMMIT_MESSAGE^: %APPVEYOR_REPO_COMMIT_MESSAGE%
ECHO MAPNIK_GIT^: %MAPNIK_GIT%
ECHO nodejs_version^: %nodejs_version%
ECHO platform^: %platform%
ECHO msvs_toolset^: %msvs_toolset%
ECHO TOOLSET_ARGS^: %TOOLSET_ARGS%


ECHO calling build-appveyor.bat ....
CALL scripts\build-appveyor.bat
ECHO build-appveyor.bat finshed, ERRORLEVEL^: %ERRORLEVEL%

ECHO =========== DONE %~f0 ===========
