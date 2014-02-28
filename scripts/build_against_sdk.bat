@rem 1. setup mapnik sdk, Python27, protobuf (protoc.exe)
@rem 1.a. copy cairo headers to mapnik\include
@rem 2. setup nodist
@rem cd \dev2
@rem git clone git://github.com/marcelklehr/nodist.git
@rem cd nodist
@rem nodist update (important to upgrade npm!)
@rem 3. create ~/.node_pre_gyprc
@rem note, for 64 builds you may need to win7 sdk terminal
@rem https://github.com/TooTallNate/node-gyp/issues/112

set X86inputplugins=C:\mapnik-v2.2.0\lib\mapnik\input
set X86distdir=C:\Users\bergw\_TEMP\dist\x86
set X64inputplugins=C:\mapnik-v2.2.0-x64\lib\mapnik\input
set X64distdir=C:\Users\bergw\_TEMP\dist\x64
set PATH=c:\dev2\nodist\bin;%PATH%
rem set PATH=node_modules\.bin;%PATH%
rem set PATH=%PATH%;c:\Python27

@cd node-mapnik

@rem ============= 32 bit, node stable ===================
@set NODIST_X64=0
call nodist use stable
IF ERRORLEVEL 1 GOTO ERROR
@rem doesn't work, don't know why
@rem call nodist update
@rem IF ERRORLEVEL 1 GOTO ERROR
call node -e "console.log(process.version + ' ' + process.arch)"
IF ERRORLEVEL 1 GOTO ERROR
call npm install node-pre-gyp -g
IF ERRORLEVEL 1 GOTO ERROR
call node-pre-gyp clean
IF ERRORLEVEL 1 GOTO ERROR
echo Y | del %X86distdir%\*.*
IF ERRORLEVEL 1 GOTO ERROR
call npm install --build-from-source
IF ERRORLEVEL 1 GOTO ERROR
call npm test
IF ERRORLEVEL 1 GOTO ERROR
copy %X86inputplugins%\*.* lib\binding
IF ERRORLEVEL 1 GOTO ERROR
call node-pre-gyp build package
IF ERRORLEVEL 1 GOTO ERROR
rem call node-pre-gyp package publish
IF ERRORLEVEL 1 GOTO ERROR
copy lib\binding\*.* %X86distdir%
IF ERRORLEVEL 1 GOTO ERROR
call node-pre-gyp clean
IF ERRORLEVEL 1 GOTO ERROR
@echo ----- 32bit build successfull! --------


@rem ============= 64 bit, node stable ===================
@rem TODO
@echo ----- 64bit not implemented! --------
@set NODIST_X64=1
echo Y | del %X64distdir%\*.*

GOTO DONE

:ERROR
@echo -------- ERROR! ABORTED! -----------

:DONE
@cd ..
@echo done
