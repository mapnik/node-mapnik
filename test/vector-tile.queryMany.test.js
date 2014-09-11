var mapnik = require('../');
var assert = require('assert');
var path = require('path');

describe('mapnik.VectorTile queryMany', function() {

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
        var manyResults = vtile2.queryMany([[0,0],[0,0]],{tolerance:1,fields:['name'],layer:'layer-name'});
        assert.equal(manyResults.features.length,2);
        assert.equal(manyResults.features[1].id(),1);
        assert.ok(manyResults.hits['0'][0].distance < 0.00000001);
        assert.equal(manyResults.features[1].layer, 'layer-name');
        assert.deepEqual(manyResults.features[1].attributes(), { name: 'geojson data' });
        assert.equal(manyResults.features[1].id(),1);
        assert.ok(manyResults.hits['1'][0].distance < 0.00000001);
        assert.equal(manyResults.features[1].layer, 'layer-name');
        assert.deepEqual(manyResults.features[1].attributes(), { name: 'geojson data' });
        done();
    });

});
