"use strict";

var mapnik = require('../');
var assert = require('assert');
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));

describe('mapnik.Geometry ', function() {
    it('should throw with invalid usage', function() {
        // geometry cannot be created directly for now
        assert.throws(function() { mapnik.Geometry(); });
    });

    it('should access a geometry from a feature', function() {
        var feature = new mapnik.Feature(1);
        var point = {
         "type": "MultiPoint",
         "coordinates": [[0,0],[1,1]]
        };
        var input = {
            type: "Feature",
            properties: {},
            geometry: point
        };
        var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
        var geom = f.geometry();
        assert.deepEqual(JSON.parse(geom.toJSONSync()),point);
        var expected_wkb = new Buffer('0104000000020000000101000000000000000000000000000000000000000101000000000000000000f03f000000000000f03f', 'hex');
        assert.deepEqual(geom.toWKB(),expected_wkb);
    });
    
    it('should fail on toJSON due to bad parameters', function() {
        var feature = new mapnik.Feature(1);
        var point = {
         "type": "MultiPoint",
         "coordinates": [[0,0],[1,1]]
        };
        var input = {
            type: "Feature",
            properties: {},
            geometry: point
        };
        var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
        var geom = f.geometry();
        assert.throws(function() { geom.toJSONSync(null); });
        assert.throws(function() { geom.toJSONSync({transform:null}); });
        assert.throws(function() { geom.toJSONSync({transform:{}}); });
        assert.throws(function() { geom.toJSON(null, function(err,json) {}); });
        assert.throws(function() { geom.toJSON({transform:null}, function(err, json) {}); });
        assert.throws(function() { geom.toJSONSync({transform:{}}, function(err, json) {}); });
    });

    it('should throw from emptry geojson from toWKB', function() {
        var src = new mapnik.Projection('+init=epsg:4326');
        var dst = new mapnik.Projection('+init=epsg:3857');
        var trans = new mapnik.ProjTransform(src,dst);
         
        var feature = {
          type: 'Feature',
          properties: { crossing_ref: 'zebra', highway: 'crossing' },
          geometry: {
            type: 'Point',
            coordinates: [ 7.415119300000001, 43.730364300000005 ]
          }
        }
        var transformed = mapnik.Feature.fromJSON(JSON.stringify(feature))
          .geometry().toJSON({transform:trans});
        var s = new mapnik.Feature.fromJSON(transformed);
        var g = s.geometry();
        assert.throws(function() {
            var geom = g.toWKB();
        });
    });
});
