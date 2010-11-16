# Frequently Ask Questions

Q: Why am I getting "TypeError: Object #<an Object> has no method ..." when calling a method of a mapnik.Map?
 
A: Likely you forgot to use the 'new' keyword to allocate a map. Do:

    var map = new mapnik.Map(width,height);


Q: Why am I getting "Could not create datasource. No plugin found for type ..."?

A: Mapnik loaded datasources as runtime, and the node-mapnik bindings use the
'mapnik-config' program from Mapnik trunk to find these paths as build time. If this
fails or you have custom plugins you can load them manually, see below.


Q: How are Mapnik datasource plugins and fonts loaded?

A: The paths to Mapnik's input plugins and fonts are configured at build time and
written the the 'mapnik/settings.js' file, so they can be dynamically loaded at runtime.
If you want to load custom input plugins you can use the function:

    mapnik.register_datasources("/path/to/plugins");

And if you want to load custom fonts do:

    mapnik.register_fonts("/path/to/fonts/");
