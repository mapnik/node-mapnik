## Using Fonts

Mapnik supports rendering custom fonts from a variety of formats.

Overall, to use fonts with Mapnik you:

 - Register fonts on your filesystem you wish to use
 - Reference them by name in your map styles

Fonts can either be registered globally or per map. Normally global font registration is what you need, but per map registration can help reduce memory usage and increase performance in specialized cases.

#### Supported Formats

 - TrueType (.ttf)
 - TrueType collection containing multiple faces per file (.ttc)
 - Web fonts (.woff)
 - OpenType (.otf)

And a few obscure formats: `.dfont`, `pfa`, and `pfb`.

#### Registering Fonts

Registering means telling Mapnik about the association between a face name and a font file like:

    Arial Regular -> /Library/Fonts/Arial Bold.ttf

Mapnik provides a few APIs for registering fonts:

##### Global

 - `mapnik.registerFonts(String font_directory, [Boolean recurse])` - Registers all fonts inside the directory provided and registers them globally so they will be available to all maps being rendered. The `recurse` argument is optional and if set to `true` then fonts inside subdirectories will also be registered.
 - `mapnik.register_default_fonts()` -  Globally registers any fonts inside `settings.paths.fonts` recursively. The value of `settings.paths.fonts` comes from the `mapnik_settings.js` file generated when node-mapnik is built. For pre-built packages this is a directory inside the node-mapnik package and for source-compiled node-mapnik is is the value of `mapnik-config --fonts` which is usually `/usr/local/lib/mapnik/fonts`.
 - `mapnik.register_system_fonts()` - Globally registers all fonts possible in known system font directories which are `/Library/Fonts`, `/System/Library/Fonts`, and `~/Library/Fonts` on Mac OS X, `C:\\Windows\\Fonts` on Windows, and `/usr/share/fonts/` and `/usr/local/share/fonts/` on other Unix systems.
 - `MAPNIK_FONT_PATH`. If this environment variable is set then any fonts inside the directory will be globally registered when `require('mapnik')` is called. Multiple directories can be separated by `:` on Unix and `;` on Windows.

##### Map level

 - The `<Map font-directory="..." />` parameter in XML. Any fonts within this directory path will be registered on the Map when the XML is loaded. The `font-directory` path is interpreted as relative to the location of the map being loaded. If the map is being loaded from a string using `map.fromString` then the `font-directory` path is interpreted as relative to the location of the `{base:"some/path"}` parameter and if that is not provided it is interpreted as relative to the current working directory of the calling program.
 - `map.registerFonts(String font_directory, [Boolean recurse])` - Registers all fonts inside the directory provided at the leve of the map such that these fonts will be available only on this specific map. The `recurse` argument is optional and if set to `true` then fonts inside subdirectories will also be registered.

Note: if `{strict:true}` is passed when loading a map then face names found in the stylesheet will be validated and an error will be thrown if they are not already registered by calling `registerFonts` or by the `font-directory` parameter.

#### Referencing Fonts

To use fonts for labels in your maps you reference them by:

    Family Name + Style Name

An example is `Arial Bold` where `Arial` is the Family Name and `Bold` is the Style Name.

In CartoCSS this looks like:

```css
text-face-name: "Arial Bold";
text-name: [name];
```


And in Mapnik XML this looks like:

```xml
<TextSymbolizer face-name="Arial Bold">[name]</TextSymbolizer>
```

#### Font API

Once fonts are registered there are a variety of methods to interact with them.

- `mapnik.fonts()` and `map.fonts()` - return an array of face names registered globally or on a mapnik.Map instance, respectively.
- `mapnik.fontFiles()` and `map.fontFiles()` - return an object mapping font file -> face name of fonts registered globally or on a mapnik.Map instance, respectively.
- mapnik.memoryFonts()` and `map.memoryFonts()` - return an array of font files that are fully cached in memory - this means that no i/o will be needed anymore to use them.
- `map.loadFonts()` - caches in memory (on the map instance) all fonts registered on that map. Can be called multiple times without harm: it will only cache fonts once.
- `map.fontDirectory()` - returns the value of the `font-directory` parameter on a map.

#### Map Level font usage

An optimization is possible when calling applications might create many different `mapnik.Map` instances which need to render different custom fonts. To leverage this optimization do two things:

    - Use `font-directory` in the Map XML to register any custom fonts locally to the Map.
    - After loading the map call `map.loadFonts()` to cache the fonts in memory.

Some background is needed for explaining why this is a potentially powerful optimization. By default Mapnik lazily caches font files in memory the first time their face is accessed during rendering. This makes sure that potentially slow font i/o does not hold back rendering speeds. However the drawback of this behavior is that over time if many different font faces are rendered by the same process memory usage will grow quite large and might hold on to font data that is rarely used.

A common solution to this problem in software is to use a LRU cache. If this were how Mapnik worked then  infrequently used fonts could be purged from the global cache if the cache size grew too large. However Mapnik does not currently support an LRU cache and instead has an API to enable fonts to be cached per map. Caching per map makes it possible to:

  - Load in-memory on the map instance any needed fonts before rendering `map.loadFonts()`
  - Avoids them needing to be lazily cached globally during rendering (and avoids the mutex lock to make this threadsafe).
  - Drop the map level memory cache simply by discarding the map instance.

