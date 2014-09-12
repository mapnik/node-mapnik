var mapnik = require('../');
var assert = require('assert');
var path = require('path');

describe('mapnik.VectorTile queryMany', function() {

    it('vtile.queryMany', function(done) {
        mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'ogr.input'));
        var vtile2 = new mapnik.VectorTile(0,0,0);
        var geojson2 = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "LineString",
                "coordinates": [
                    [0,0],
                    [0.1,0.1],
                    [20,20],
                    [20.1,20.1]
                ]
              },
              "properties": {
                "name": "A"
              }
            },
            {
              "type": "Feature",
              "geometry": {
                "type": "LineString",
                "coordinates": [
                    [-40,-40],
                    [-40.1,-40.1],
                    [-60,-60],
                    [-60.1,-60.1]
                ]
              },
              "properties": {
                "name": "B"
              }
            }
          ]
        };
        vtile2.addGeoJSON(JSON.stringify(geojson2),"layer-name");
        var manyResults = vtile2.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1,fields:['name'],layer:'layer-name'});
        assert.equal(Object.keys(manyResults.hits).length, 3);

        assert.equal(manyResults.hits[0].length, 2);
        assert.equal(Math.round(manyResults.hits[0][0].distance), 0);
        assert.equal(manyResults.hits[0][0].feature_id, 0);
        assert.equal(manyResults.hits[0][1].feature_id, 1);
        assert.equal(manyResults.features[manyResults.hits[0][0].feature_id].attributes().name, 'A');
        assert.equal(manyResults.features[manyResults.hits[0][1].feature_id].attributes().name, 'B');

        assert.equal(manyResults.hits[1].length, 2);
        assert.equal(Math.round(manyResults.hits[1][0].distance), 0);
        assert.equal(manyResults.hits[1][0].feature_id, 0);
        assert.equal(manyResults.hits[1][1].feature_id, 1);
        assert.equal(manyResults.features[manyResults.hits[1][0].feature_id].attributes().name, 'A');
        assert.equal(manyResults.features[manyResults.hits[1][1].feature_id].attributes().name, 'B');

        assert.equal(manyResults.hits[2].length, 2);
        assert.equal(Math.round(manyResults.hits[2][0].distance), 6595805);
        assert.equal(Math.round(manyResults.hits[2][1].distance), 514);
        assert.equal(manyResults.features[manyResults.hits[2][0].feature_id].attributes().name, 'A');
        assert.equal(manyResults.features[manyResults.hits[2][1].feature_id].attributes().name, 'B');


        assert.equal(manyResults.features.length,2);

        assert.equal(manyResults.features[0].id(),1);
        assert.equal(manyResults.features[0].layer, 'layer-name');
        assert.deepEqual(manyResults.features[0].attributes(), { name: 'A' });

        assert.equal(manyResults.features[1].id(),2);
        assert.equal(manyResults.features[1].layer, 'layer-name');
        assert.deepEqual(manyResults.features[1].attributes(), { name: 'B' });

        done();
    });

});
