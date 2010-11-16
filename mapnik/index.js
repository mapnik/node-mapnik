var mapnik = require('./_mapnik');
var settings = require('./settings');


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

if (settings.paths.fonts) {
   mapnik.register_fonts(settings.paths.fonts);
} else {
   warning('fonts', 'register_fonts');
}

if (settings.paths.input_plugins) {
   mapnik.register_datasources(settings.paths.input_plugins);
} else {
   warning('fonts', 'register_datasources');
}

// push all C++ symbols into js module
for (var k in mapnik) { exports[k] = mapnik[k]; }

