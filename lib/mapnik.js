"use strict";

var binary = require('@mapbox/node-pre-gyp');
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

// https://github.com/mapnik/node-mapnik/issues/437
if (process.env.VRT_SHARED_SOURCE === undefined) {
    process.env.VRT_SHARED_SOURCE = 0;
}

var binding = require(binding_path);
var mapnik = module.exports = exports = binding;

exports.module_path = path.dirname(binding_path);
// add dlls to search path...
process.env["PATH"] = exports.module_path + separator + settings.env["PATH"];

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

function getGeometryType(mapnikType) {
  switch (mapnikType) {
    case mapnik.Geometry.Point:
      return 'Point';
    case mapnik.Geometry.LineString:
      return 'LineString';
    case mapnik.Geometry.Polygon:
      return 'Polygon';
    case mapnik.Geometry.MultiPoint:
      return 'MultiPoint';
    case mapnik.Geometry.MultiLineString:
      return 'MultiLineString';
    case mapnik.Geometry.MultiPolygon:
      return 'MultiPolygon';
    case mapnik.Geometry.GeometryCollection:
      return 'GeometryCollection';
    case mapnik.Geometry.Unknown:
      return 'Unknown';
    default:
      return 'Unknown';
  }
}

mapnik.Geometry.prototype.typeName = function() {
  return getGeometryType(this.type());
};

mapnik.Feature.prototype.toWKB = function() {
    return this.geometry().toWKB();
};

mapnik.Feature.prototype.toWKT = function() {
    return this.geometry().toWKT();
};
