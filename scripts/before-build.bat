@echo off
SET EL=0

SET PATH=%MAPNIK_SDK%\bin;%MAPNIK_SDK%\lib;%PATH%
SET PROJ_LIB=%MAPNIK_SDK%\share\proj
SET GDAL_DATA=%MAPNIK_SDK%\share\gdal
SET ICU_DATA=%MAPNIK_SDK%\share\icu

GOTO DONE

CALL node_modules\.bin\node-pre-gyp reveal module --silent > module.txt
IF %ERRORLEVEL% NEQ 0 ECHO could not reveal module && GOTO ERROR
CALL SET /p MODULE=<module.txt
IF %ERRORLEVEL% NEQ 0 ECHO could not set module env var && GOTO ERROR
CALL del module.txt
IF %ERRORLEVEL% NEQ 0 ECHO could not delete module.txt && GOTO ERROR

call node_modules\.bin\node-pre-gyp reveal module_path --silent > binding_path.txt
IF %ERRORLEVEL% NEQ 0 ECHO could not reveal module path && GOTO ERROR
SET /p NODEMAPNIK_BINDING_DIR=<binding_path.txt
IF %ERRORLEVEL% NEQ 0 ECHO could not set binding dir env var && GOTO ERROR
del binding_path.txt
IF %ERRORLEVEL% NEQ 0 ECHO could not delete binding_path.txt && GOTO ERROR

ECHO MODULE^: %MODULE%
ECHO NODEMAPNIK_BINDING_DIR^: %NODEMAPNIK_BINDING_DIR%
::CALL python -c "import platform;print(platform.architecture())"
::CALL python -c "import ctypes,os;print('%MODULE%');print os.path.exists('%MODULE%');ctypes.CDLL('%MODULE%')" || true
::CALL .\node .\npm\bin\npm-cli.js install  --no-bin-links --verbose --unsafe-perm
:: CALL .\node .\npm\bin\npm-cli.js test || true


GOTO DONE

:ERROR
echo --------- ERROR! ------------
ECHO ERRORLEVEL %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
EXIT /b %EL%
