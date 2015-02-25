@echo off
SET EL=0

SET PATH=%MAPNIK_SDK%\bin;%MAPNIK_SDK%\lib;%PATH%
SET PROJ_LIB=%MAPNIK_SDK%\share\proj
SET GDAL_DATA=%MAPNIK_SDK%\share\gdal
SET ICU_DATA=%MAPNIK_SDK%\share\icu

CALL node_modules\.bin\node-pre-gyp reveal module --silent > module.txt
CALL SET /p MODULE=<module.txt
CALL del module.txt

call node_modules\.bin\node-pre-gyp reveal module_path --silent > binding_path.txt
SET /p NODEMAPNIK_BINDING_DIR=<binding_path.txt
del binding_path.txt

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
