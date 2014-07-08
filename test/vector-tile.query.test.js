var mapnik = require('../');
var assert = require('assert');
var path = require('path');

describe('mapnik.VectorTile query', function() {
   
    it('vtile.query should return distance attribute on feature representing shortest distance from point to line', function(done) {
        mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'ogr.input'));
        var vtile = new mapnik.VectorTile(0,0,0);
        var coords = [
          [0,0],
          [0.1,0.1],
          [20,20],
          [20.1,20.1]
        ];
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "LineString",
                "coordinates": coords
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var features = vtile.query(0,0,{tolerance:1});
        assert.equal(features.length,1);
        assert.equal(features[0].id(),1);
        assert.ok(features[0].distance < 0.00000001);
        assert.equal(features[0].layer,'layer-name');

        var vtile2 = new mapnik.VectorTile(0,0,0);
        var geojson2 = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "LineString",
                "coordinates": coords.reverse()
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile2.addGeoJSON(JSON.stringify(geojson2),"layer-name");
        var features2 = vtile2.query(0,0,{tolerance:1});
        assert.equal(features2.length,1);
        assert.equal(features2[0].id(),1);
        assert.ok(features2[0].distance < 0.00000001);
        assert.equal(features2[0].layer,'layer-name');
        done();
    });

    it('vtile.query should return distance attribute on feature representing shortest distance from point to multiline', function(done) {
        mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'ogr.input'));
        var vtile = new mapnik.VectorTile(0,0,0);
        var coords = [
        [
          [0.1,0.1],
          [20,20],
          [20.1,20.1]
        ],
        [
          [0.3,0.3],
          [25,25],
          [20.1,20.1],
          [1,1]
        ]
        ];
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "MultiLineString",
                "coordinates": coords
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var features = vtile.query(1,1,{tolerance:1});
        assert.equal(features.length,1);
        assert.equal(features[0].id(),1);
        assert.ok(features[0].distance < 100);
        assert.equal(features[0].layer,'layer-name');
        done();
    });


    it('vtile.query should return distance attribute on feature representing shortest distance from point to multipoint', function(done) {
        mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'ogr.input'));
        var vtile = new mapnik.VectorTile(0,0,0);
        var coords = [
          [0.1,0.1],
          [20,20],
          [20.1,20.1],
          [0,0]
        ];
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "MultiPoint",
                "coordinates": coords
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var features = vtile.query(0,0,{tolerance:1});
        assert.equal(features.length,1);
        assert.equal(features[0].id(),1);
        assert.ok(features[0].distance < 0.00000001);
        assert.equal(features[0].layer,'layer-name');
        done();
    });

    it('vtile.queryMany', function(done) {
        mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'ogr.input'));
        var vtile = new mapnik.VectorTile(0,0,0);
        var coords = [
          [0,0],
          [0.1,0.1],
          [20,20],
          [20.1,20.1]
        ];
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "LineString",
                "coordinates": coords
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var features = vtile.query(0,0,{tolerance:1});
        assert.equal(features.length,1);
        assert.equal(features[0].id(),1);
        assert.ok(features[0].distance < 0.00000001);
        assert.equal(features[0].layer,'layer-name');

        var vtile2 = new mapnik.VectorTile(0,0,0);
        var geojson2 = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "LineString",
                "coordinates": coords.reverse()
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile2.addGeoJSON(JSON.stringify(geojson2),"layer-name");
        var manyResults = vtile2.queryMany([[0,0],[0,0]],{layer:'layer-name'});
        assert.equal(manyResults.length,2);
        for (var i = 0; i < 2; i++) {
            assert.equal(manyResults[i].length,1);
            assert.equal(manyResults[i][0].id(),1);
            assert.equal(manyResults[i][0].id(),1);
            assert.ok(manyResults[i][0].distance < 0.00000001);
            assert.equal(manyResults[i][0].layer, 'layer-name');
            assert.deepEqual(manyResults[i][0].attributes(), { name: 'geojson data' });
        }
        done();
    });

});
