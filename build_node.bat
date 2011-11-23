set MAPNIK_INPUT_PLUGINS=c:\\mapnik-2.0\\lib\\mapnik\\input
set MAPNIK_FONTS=c:\\mapnik-2.0\\lib\\mapnik\\fonts
set target=Build
set config=Release
python gen_settings.py
python gyp/gyp build.gyp --depth=. -f msvs -G msvs_version=2010
;msbuild build.sln /t:%target% /p:Configuration=%config%
msbuild build.sln
;cp projects/makefiles/out/Default/_mapnik.node lib/_mapnik.node