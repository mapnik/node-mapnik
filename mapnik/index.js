var mapnik = require('./_mapnik');
var settings = require('./settings');
var path = require('path');

function warning(issue,manual_function)
{
    var msg = 'Warning: "' + __dirname + '/settings.js" is missing the default mapnik ';
    msg += issue + ' path.\n';
    msg += 'This indicates a configuration problem building node-mapnik against mapnik.\n';
    msg += 'Auto-loading of ';
    if (issue == 'fonts')
        msg += 'DejaVu fonts';
    else if (issue == 'plugins')
        msg += 'Datasource input plugins';
    msg += ' will not be available...\n';
    msg += 'Either rebuild node-mapnik or call mapnik.' + manual_function + '(/path/to/' + issue + ')';
    console.log(msg);
}

// hack to force libmapnik symbols to be loaded with RTLD_NOW|RTLD_GLOBAL
// so that datasource plugins will have access to symbols since node.cc (process.dlopen)
// uses RTLD_LAZY in its call to dlopen and has no mechanism to set the RTLD_NOW flag
// not needed on darwin because mapnik input plugins are directly linked to libmapnik
if (process.platform != 'darwin') {
    var mapnik_node = path.join(__dirname,'_mapnik.node');
    if (!path.existsSync(mapnik_node))
        console.log(mapnik_node + ' does not exist, loading plugins will fail on linux');
    
    var loaded = mapnik.make_mapnik_symbols_visible(mapnik_node);
    
    if (!loaded)
        console.log('Warning, attempt to pre-load mapnik symbols did not work, see FAQ for potential solutions');
}

if (settings.paths.fonts && path.existsSync(settings.paths.fonts)) {
   mapnik.register_fonts(settings.paths.fonts);
} else {
   warning('fonts', 'register_fonts');
}

if (settings.paths.input_plugins && path.existsSync(settings.paths.input_plugins)) {
   mapnik.register_datasources(settings.paths.input_plugins);
} else {
   warning('fonts', 'register_datasources');
}

// push all C++ symbols into js module
for (var k in mapnik) { exports[k] = mapnik[k]; }

