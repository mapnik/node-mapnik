var mapnik = require('./_mapnik');
var _settings = require('./settings');
var path = require('path');

function warning(value, what)
{
    var msg = 'Warning: node-mapnik initialization failed\n';
    msg += 'in "' + __dirname + '/settings.js" ';
    msg += what + '\n';
    msg += 'Auto-loading of ';
    if (value == 'fonts')
        msg += 'DejaVu fonts';
    else if (value == 'plugins')
        msg += 'Datasource input plugins';
    msg += ' will not be available.\n';
    msg += 'See the docs/FAQ on ideas for how to fix this issue.';
    console.log(msg);
}

// hack to force libmapnik symbols to be loaded with RTLD_NOW|RTLD_GLOBAL
// so that datasource plugins will have access to symbols since node.cc (process.dlopen)
// uses RTLD_LAZY in its call to dlopen and has no mechanism to set the RTLD_NOW flag.
// Not needed on darwin because mapnik input plugins are directly linked to libmapnik
if (process.platform != 'darwin') {
    var mapnik_node = path.join(__dirname,'_mapnik.node');
    if (!path.existsSync(mapnik_node))
        console.log(mapnik_node + ' does not exist, loading plugins will fail on linux');
    
    var loaded = mapnik.make_mapnik_symbols_visible(mapnik_node);
    
    if (!loaded)
        console.log('Warning, attempt to pre-load mapnik symbols did not work, see FAQ for potential solutions');
}

if (!_settings.paths.fonts) {
    warning('fonts', 'the fonts path value is missing');
} else if (!path.existsSync(_settings.paths.fonts)) {
    warning('fonts', 'the fonts directory: "' + _settings.paths.fonts + '" does not exist');
} else {
    // TODO - warn if no fonts are successfully registered
    mapnik.register_fonts(_settings.paths.fonts);
}

if (!_settings.paths.input_plugins) {
    warning('plugins', 'the datasource plugins (input) path value is missing');
} else if (!path.existsSync(_settings.paths.input_plugins)) {
    warning('plugins', 'the input_plugins directory: "' + _settings.paths.input_plugins + '" does not exist');
} else {
    mapnik.register_datasources(_settings.paths.input_plugins);
}

// push all C++ symbols into js module
for (var k in mapnik) { exports[k] = mapnik[k]; }

