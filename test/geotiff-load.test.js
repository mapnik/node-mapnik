"use strict";

var mapnik = require('../');
var assert = require('assert');
var path = require('path');

describe('reading GeoTIFF in threads', function() {
    // puts unnatural, odd, and intentionally racey load on opening geotiff
    it.skip('should be able to open geotiff various ways without crashing', function(done) {
        mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'gdal.input'));
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.load('./test/data/vector_tile/raster_layer.xml',{},function(err,_map) { if (err) throw err; assert.ok(_map); });
        map.render(vtile,{},function(err,vtile) {
            if (err) throw err;
            assert.ok(vtile);
        });
        var map2 = new mapnik.Map(256, 256);
        map2.load('./test/data/vector_tile/raster_layer.xml',{},function(err,_map) { if (err) throw err; assert.ok(_map); });
        var map3 = new mapnik.Map(256, 256);
        map3.load('./test/data/vector_tile/raster_layer.xml',{},function(err,_map) { if (err) throw err; assert.ok(_map); });
        map3.render(vtile,{},function(err,vtile) {
            if (err) throw err;
            assert.ok(vtile);
            done();
        });
    });
});
