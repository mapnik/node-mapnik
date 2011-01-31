# Frequently Ask Questions

Q: I get warnings when doing require('mapnik') like 'Warning: node-mapnik initialization failed...', how do I fix this?

A: Mapnik uses a plugin system for loading datasources (like shapefiles or postgis table), and for fonts. The node-mapnik build scripts attempt to auto-configure the paths needed to pass to Mapnik to auto-load these plugins and fonts by creating a 'settings.js' file with the paths, and the proper creation of this file is what failed.

A default install of Mapnik2 (aka Mapnik trunk) will have put fonts in /usr/local/lib/mapnik2/fonts and datasource plugins in /usr/local/lib/mapnik2/input. A simple solution is to edit the installed 'settings.js' file and to add these paths. A more proper solution is to rebuild the latest mapnik trunk (svn.mapnik.org/trunk) as this should contain fixes to the 'mapnik-config' script that is needed to properly build up the 'settings.js' file automatically.

Note a trick to see where Mapnik fonts and datasource plugins live is to use the Mapnik python bindings:

$ python -c "import mapnik2;print mapnik2.fontscollectionpath,mapnik2.inputpluginspath"



Q: Why am I getting "TypeError: Object #<an Object> has no method ..." when calling a method of a mapnik.Map?
 
A: Likely you forgot to use the 'new' keyword to allocate a map. Do:

    var map = new mapnik.Map(width,height);



Q: Why am I getting "Could not create datasource. No plugin found for type ..."?

A: Mapnik loads datasources at runtime, and the node-mapnik builds scripts leverage the
'mapnik-config' program from Mapnik trunk to find these paths at build time. If this
fails or you have custom plugins you can load them manually, see below.



Q: How do I load custom fonts or datasource plugins?

A: The default paths to Mapnik's input plugins and fonts are configured at build time and
written the the 'mapnik/settings.js' file, so they can be dynamically loaded at runtime.

If you want to load custom input plugins you can use the function:

    mapnik.register_datasources("/path/to/plugins");

And if you want to load custom fonts do:

    mapnik.register_fonts("/path/to/fonts/");



Q: When trying to install node-mapnik I get:

    node-waf -v configure build
    make: node-waf: Command not found
    make: *** [mapnik.node] Error 127

A: node-waf is the build tool provided with a node install, make sure you have node installed

Q: I get a compile error like:

    [1/6] cxx: src/_mapnik.cc -> build/default/src/_mapnik_1.o
    ../src/_mapnik.cc:240: error: expected constructor, destructor, or type conversion before '(' token
    ../src/_mapnik.cc:184: warning: 'void init(v8::Handle<v8::Object>)' defined but not used

A: Your node version is too old, upgrade to at least node 0.2.4
