@echo off
SET EL=0
:: ============ USAGE
:: Powershell 3.0 or better needed.
:: Install from here: http://www.microsoft.com/en-us/download/details.aspx?id=40855
:: from the node-mapnik clone directory execute this file: scripts\build_against_sdk_00-START-HERE.bat
:: ============ Powershell Error
:: if you get "...  cannot be loaded because running scripts is disabled on this system ..."
:: open an admin prompt and enter: powershell Set-ExecutionPolicy Unrestricted
SET MAPNIK_SDK=%CD%\mapnik-sdk
set PATH=%MAPNIK_SDK%\bin;%PATH%
set PATH=%MAPNIK_SDK%\libs;%PATH%
set PROJ_LIB=%MAPNIK_SDK%\share\proj
set GDAL_DATA=%MAPNIK_SDK%\share\gdal

:: upgrade node-gyp to support --msvs_version=2013
CALL npm install node-gyp
:: clear out node-gyp header cache
IF EXIST %USERPROFILE%\.node-gyp rd /s /q %USERPROFILE%\.node-gyp

CALL .\node .\npm\bin\npm-cli.js -v
CALL set NPM_CACHE=%CD%\npm-cache
:: npm cache needs to be created beforehand otherwise you'll hit
:: the error 'uid must be an unsigned int'
CALL mkdir %NPM_CACHE%
CALL .\node .\npm\bin\npm-cli.js set cache %NPM_CACHE%
CALL .\node .\npm\bin\npm-cli.js get cache
CALL .\node .\npm\bin\npm-cli.js install node-pre-gyp
CALL node_modules\.bin\node-pre-gyp reveal module --silent > module.txt
CALL SET /p MODULE=<module.txt
CALL del module.txt
CALL python -c "import platform;print(platform.architecture())"
CALL python -c "import ctypes,os;print('%MODULE%');print os.path.exists('%MODULE%');ctypes.CDLL('%MODULE%')" || true
CALL .\node .\npm\bin\npm-cli.js install  --no-bin-links --verbose --unsafe-perm
CALL .\node .\npm\bin\npm-cli.js test || true


GOTO DONE

:ERROR
echo --------- ERROR! ------------
ECHO ERRORLEVEL %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
EXIT /b %EL%
