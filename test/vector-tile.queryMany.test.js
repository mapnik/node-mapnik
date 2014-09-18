var mapnik = require('../');
var assert = require('assert');
var path = require('path');

describe('mapnik.VectorTile queryMany', function() {
    mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'ogr.input'));
    var geojson = {
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
        // This is an invalid polygon and results in a distance of -1
        // from path_to_point_distance().
        {
          "type": "Feature",
          "geometry": {
            "type": "Polygon",
            "coordinates": [[
                [-20.2,-20.2],
                [-20.1,-20.1],
                [-20.3,-20.3],
                [-20.4,-20.4],
                [-20.2,-20.2]
            ]]
          },
          "properties": {
            "name": "C"
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

    it('vtile.queryMany', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var manyResults = vtile.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1,fields:['name'],layer:'layer-name'});
        check(manyResults);
        done();
    });

    it('vtile.queryMany async', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        vtile.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1,fields:['name'],layer:'layer-name'}, function(err, manyResults) {
            assert.ifError(err);
            check(manyResults);
            done();
        });
    });

    it('vtile.queryMany concurrent x4', function(done) {
        var remaining = 4;
        function run() {
            var vtile = new mapnik.VectorTile(0,0,0);
            vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
            vtile.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1,fields:['name'],layer:'layer-name'}, function(err, manyResults) {
                assert.ifError(err);
                check(manyResults);
                if (!--remaining) done();
            });
        }
        run();
        run();
        run();
        run();
    });

    function check(manyResults) {
        assert.equal(Array.isArray(manyResults.hits), true);
        assert.equal(manyResults.hits.length, 3);

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
        assert.equal(Math.round(manyResults.hits[2][0].distance), 514);
        assert.equal(Math.round(manyResults.hits[2][1].distance), 6595805);
        assert.equal(manyResults.features[manyResults.hits[2][0].feature_id].attributes().name, 'B');
        assert.equal(manyResults.features[manyResults.hits[2][1].feature_id].attributes().name, 'A');

        assert.equal(Array.isArray(manyResults.features), true);
        assert.equal(manyResults.features.length,2);

        assert.equal(manyResults.features[0].id(),1);
        assert.equal(manyResults.features[0].layer, 'layer-name');
        assert.deepEqual(manyResults.features[0].attributes(), { name: 'A' });

        assert.equal(manyResults.features[1].id(),3);
        assert.equal(manyResults.features[1].layer, 'layer-name');
        assert.deepEqual(manyResults.features[1].attributes(), { name: 'B' });
    }
});
