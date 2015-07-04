"use strict";

function only_one(package_json) {
    // https://nodejs.org/api/modules.html#modules_caching
    var path = require('path');
    var search_path = path.join('node_modules',package_json.name,package_json.main);

    var modules = Object.keys(require.cache).filter(function(p) {
        return ((p.indexOf(search_path) > -1) && (p !== __filename));
    });

    if (modules.length == 1) {
        var previous_instance = modules[0];
        // prefer module higher in tree
        if (__filename.length < previous_instance.length) {
            console.log('warning',package_json.name,'previous imported from',previous_instance);
            console.log('replacing that instance with',__filename);
            delete require.cache[previous_instance];
        } else {
            console.log('warning',package_json.name,'already imported from',previous_instance);
            console.log('skipping require of',__filename);
            module.exports = exports = require(modules[0]);
            return true;
        }
    }
    return false;
}


var package_json = require('../package.json');
if (only_one(package_json)) {
    return;
}


var binary = require('node-pre-gyp');
var exists = require('fs').existsSync || require('path').existsSync;
var path = require('path');
var binding_path = binary.find(path.resolve(path.join(__dirname,'../package.json')));
var settings_path = path.join(path.dirname(binding_path),'mapnik_settings.js');
var settings = require(settings_path);

var separator = (process.platform === 'win32') ? ';' : ':';

// set custom env settings before loading mapnik
if (settings.env) {
    var process_keys = Object.keys(process.env);
    for (var key in settings.env) {
        if (process_keys.indexOf(key) == -1) {
            process.env[key] = settings.env[key];
        } else {
            if (key === 'PATH') {
                process.env[key] = settings.env[key] + separator + process.env[key];
            }
        }
    }
}

var binding = require(binding_path);
var mapnik = module.exports = exports = binding;

exports.module_path = path.dirname(binding_path);

process.on('exit',function() {
    if (mapnik) mapnik.shutdown();
});

exports.settings = settings;

exports.version = require('../package').version;

exports.register_default_fonts = function() {
    if (settings.paths.fonts) {
        mapnik.register_fonts(settings.paths.fonts, {recurse: true});
    }
};

exports.register_default_input_plugins = function() {
    if (settings.paths.input_plugins) {
        mapnik.register_datasources(settings.paths.input_plugins);
    }
};

exports.register_system_fonts = function() {
    var num_faces = mapnik.fonts().length;
    var dirs = [];
    if (process.platform === 'linux' ||
       (process.platform === 'sunos') ||
       (process.platform.indexOf('bsd') != -1)) {
        dirs.push('/usr/share/fonts/');
        dirs.push('/usr/local/share/fonts/');
        if (process.env.hasOwnProperty('HOME')) dirs.push(path.join(process.env.HOME, '.fonts'));
    } else if (process.platform === 'darwin') {
        dirs.push('/Library/Fonts');
        dirs.push('/System/Library/Fonts');
        if (process.env.hasOwnProperty('HOME')) dirs.push(path.join(process.env.HOME, 'Library/Fonts'));
    } else if (process.platform === 'win32') {
       dirs.push('C:\\Windows\\Fonts');
    }
    dirs.forEach(function(p) {
        if (exists(p)) {
            mapnik.register_fonts(p, {recurse: true});
        }
    });
    if (mapnik.fonts().length == num_faces) {
       return false;
    } else {
       return true;
    }
};

// register paths from MAPNIK_FONT_PATH
if (process.env.MAPNIK_FONT_PATH) {
    process.env.MAPNIK_FONT_PATH.split(separator).forEach(function(p) {
        if (exists(p)) {
            mapnik.register_fonts(p);
        }
    });
}

mapnik.Feature.prototype.toWKB = function() {
    return this.geometry().toWKB();
};

mapnik.Feature.prototype.toWKT = function() {
    return this.geometry().toWKT();
};
