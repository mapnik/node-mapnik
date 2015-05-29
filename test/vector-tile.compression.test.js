"use strict";

var zlib = require('zlib');
var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var mercator = new(require('sphericalmercator'))();
var existsSync = require('fs').existsSync || require('path').existsSync;
var overwrite_expected_data = false;
//var SegfaultHandler = require('segfault-handler');
//SegfaultHandler.registerHandler();

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));

var trunc_6 = function(key, val) {
    return val.toFixed ? Number(val.toFixed(6)) : val;
};

function deepEqualTrunc(json1,json2) {
    return assert.deepEqual(JSON.stringify(json1,trunc_6),JSON.stringify(json2,trunc_6));
}

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'gdal.input'));

describe('mapnik.VectorTile compression', function() {

    it('should be able to create a vector tile from geojson', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "Point",
                "coordinates": [
                  -122,
                  48
                ]
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");

        assert.equal(vtile.getData().length, 58);
        assert.equal(vtile.getData({ compression: 'gzip'}).length,76);
        assert.equal(zlib.gunzipSync(vtile.getData({ compression: 'gzip'})).length,58);

        done();
    });

});
