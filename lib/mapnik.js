var settings = require('./mapnik_settings');

// set custom env settings before loading mapnik
if (settings.env) {
    var process_keys = Object.keys(process.env);
    for (key in settings.env) {
        if (process_keys.indexOf(key) == -1) {
            process.env[key] = settings.env[key];
        }
    }
}

var exists = require('fs').existsSync || require('path').existsSync;
var path = require('path');
var mapnik = require('../build/'+process.config.target_defaults.default_configuration+'/_mapnik.node');

exports = module.exports = mapnik;
exports.settings = settings;

exports.version = require('../package').version;

exports.register_default_fonts = function() {
    if (settings.paths.fonts) {
        // TODO - make async and warn if not successful
        mapnik.register_fonts(settings.paths.fonts, {recurse: true});
    }
}

exports.register_default_input_plugins = function() {
    if (settings.paths.input_plugins) {
        // TODO - make async and warn if not successful
        mapnik.register_datasources(settings.paths.input_plugins);
    }
}

exports.register_default_input_plugins();

exports.register_system_fonts = function() {
    var num_faces = mapnik.fonts().length;
    var dirs = [];
    if (process.platform == 'linux' ||
       (process.platform.indexOf('bsd') != -1) ||
       process.platform.indexOf('bsd') == 'sunos') {
        dirs.push('/usr/share/fonts/truetype/');
        dirs.push('/usr/local/share/fonts/truetype/');
        dirs.push(path.join(process.env.HOME, '.fonts'));
    }
    else if (process.platform == 'darwin') {
        dirs.push('/Library/Fonts');
        dirs.push('/System/Library/Fonts');
        if (exists('/usr/X11R6/lib/X11/fonts/')) {
            // if xcode's X11 is installed
            dirs.push('/usr/X11R6/lib/X11/fonts/');
        }
        dirs.push(path.join(process.env.HOME, 'Library/Fonts'));
    }
    else if (process.platform == 'win32') {
       dirs.push('C:\\Windows\\Fonts');
    }
    dirs.forEach(function(p) {
        if (exists(p)) {
            mapnik.register_fonts(p, {recurse: true});
        }
    });
    if (mapnik.fonts().length == num_faces) {
       return false;
    }
    else {
       return true;
    }
};

