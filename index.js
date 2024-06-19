"use strict";

var exists = require('fs').existsSync || require('path').existsSync;
var os = require('os');
var path = require('path');
var binding_path = require('node-gyp-build').path();
var settings_path = path.join(path.dirname(binding_path),'mapnik_settings.js');
var settings = require(settings_path);

// https://github.com/mapnik/node-mapnik/issues/437
if (process.env.VRT_SHARED_SOURCE === undefined) {
    process.env.VRT_SHARED_SOURCE = 0;
}

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

var binding = require('node-gyp-build')(__dirname)
module.exports = binding

binding.module_path = path.dirname(binding_path);
binding.settings = settings;

binding.register_default_fonts = function() {
    if (settings.paths.fonts) {
        binding.register_fonts(settings.paths.fonts, {recurse: true});
    }
};

binding.register_default_input_plugins = function() {
    if (settings.paths.input_plugins) {
        binding.register_datasources(settings.paths.input_plugins);
    }
};

binding.register_system_fonts = function() {
    var num_faces = binding.fonts().length;
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
            binding.register_fonts(p, {recurse: true});
        }
    });
    if (binding.fonts().length == num_faces) {
       return false;
    } else {
       return true;
    }
};

// register paths from MAPNIK_FONT_PATH
if (process.env.MAPNIK_FONT_PATH) {
    process.env.MAPNIK_FONT_PATH.split(separator).forEach(function(p) {
        if (exists(p)) {
            binding.register_fonts(p);
        }
    });
}

function getGeometryType(mapnikType) {
  switch (mapnikType) {
    case binding.Geometry.Point:
      return 'Point';
    case binding.Geometry.LineString:
      return 'LineString';
    case binding.Geometry.Polygon:
      return 'Polygon';
    case binding.Geometry.MultiPoint:
      return 'MultiPoint';
    case binding.Geometry.MultiLineString:
      return 'MultiLineString';
    case binding.Geometry.MultiPolygon:
      return 'MultiPolygon';
    case binding.Geometry.GeometryCollection:
      return 'GeometryCollection';
    case binding.Geometry.Unknown:
      return 'Unknown';
    default:
      return 'Unknown';
  }
}

binding.Geometry.prototype.typeName = function() {
  return getGeometryType(this.type());
};

binding.Feature.prototype.toWKB = function() {
    return this.geometry().toWKB();
};

binding.Feature.prototype.toWKT = function() {
    return this.geometry().toWKT();
};
