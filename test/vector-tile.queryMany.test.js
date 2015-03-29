"use strict";

var mapnik = require('../');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));

describe('mapnik.VectorTile queryMany', function() {
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

    it('vtile.queryMany bad parameters fails', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        assert.throws(function() {
            var manyResults = vtile.queryMany();
        });
        assert.throws(function() {
            var manyResults = vtile.queryMany(null);
        });
        assert.throws(function() {
            var manyResults = vtile.queryMany([1,2], {layer:"layer-name"});
        });
        assert.throws(function() {
            var manyResults = vtile.queryMany([[1,'2']], {layer:"layer-name"});
        });
        assert.throws(function() { vtile.queryMany([[0,0],[0,0],[-40,-40]]);});
        assert.throws(function() { vtile.queryMany([[0,0],[0,0],[-40,-40]], null);});
        assert.throws(function() { vtile.queryMany([[0,0],[0,0],[-40,-40]], {tolerance:null});});
        assert.throws(function() { vtile.queryMany([[0,0],[0,0],[-40,-40]], {fields:null});});
        assert.throws(function() { vtile.queryMany([[0,0],[0,0],[-40,-40]], {layer:null});});
        assert.throws(function() { vtile.queryMany([[0,0],[0,0],[-40,-40]], {layer:''});});
        assert.throws(function() {
            var manyResults = vtile.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1e9,fields:['name'],layer:'donkey'});
        });
        vtile.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1e9,fields:['name'],layer:'donkey'}, function(err, manyResults) {
            assert.throws(function() { if (err) throw err; });
            done();   
        });
    });

    it('vtile.queryMany', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var manyResults = vtile.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1e9,fields:['name'],layer:'layer-name'});
        check(manyResults);
        done();
    });
    
    it('vtile.queryMany with out fields', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var manyResults = vtile.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1e9,layer:'layer-name'});
        check(manyResults);
        done();
    });


    it('vtile.queryMany async', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        vtile.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1e9,fields:['name'],layer:'layer-name'}, function(err, manyResults) {
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
            vtile.queryMany([[0,0],[0,0],[-40,-40]],{tolerance:1e9,fields:['name'],layer:'layer-name'}, function(err, manyResults) {
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

    // Done outside of test so mocha doesn't time the vt load/parse.
    var profile = new mapnik.VectorTile(0,0,0);
    profile.setData(fs.readFileSync(path.join(__dirname, 'data', 'vector_tile', 'tile0.vector.pbf')));
    profile.parse();
    it('vtile.queryMany profile x10 runs', function(done) {
        var remaining = 10;
        function run() {
            profile.queryMany([
                [-107.57812499999999,54.97761367069625],
                [-106.171875,41.50857729743935],
                [-73.125,2.1088986592431382],
                [-68.5546875,7.710991655433229],
                [-63.28125,-9.44906182688142],
                [-65.390625,-15.961329081596634],
                [-58.35937499999999,-21.616579336740593],
                [-64.3359375,-34.30714385628803],
                [-4.21875,17.308687886770034],
                [4.921875,28.92163128242129],
                [20.7421875,25.48295117535531],
                [29.53125,26.43122806450644],
                [11.6015625,50.736455137010665],
                [1.0546875,46.800059446787316],
                [-2.8125,39.63953756436671],
                [-1.40625,52.482780222078205],
                [10.1953125,61.60639637138628],
                [62.22656249999999,47.98992166741417],
                [103.35937499999999,38.272688535980976],
                [138.1640625,36.03133177633187],
                [131.484375,-19.642587534013032],
                [112.8515625,-3.162455530237848],
                [102.65625,17.308687886770034],
                [95.625,20.3034175184893],
                [75.234375,23.88583769986199],
                [66.09375,27.68352808378776],
                [56.25,33.137551192346145],
                [44.29687499999999,26.43122806450644],
                [28.4765625,12.897489183755905],
                [6.6796875,9.102096738726456],
                [24.609375,-3.864254615721396],
                [18.984375,-14.264383087562637],
                [17.9296875,-24.206889622398023],
                [26.71875,-29.84064389983441]
            ],{tolerance:1000,fields:['AREA'],layer:'world'}, function(err, manyResults) {
                assert.ifError(err);
                assert.equal(manyResults.hits.length, 34);
                assert.equal(manyResults.features.length, 33);
                if (!--remaining) {
                    done();
                } else {
                    run();
                }
            });
        }
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

describe('mapnik.VectorTile queryMany (distance <= tolerance)', function() {
    it('LineString - no features', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify({
          "type": "FeatureCollection",
          "features": [{
            "type": "Feature",
            "geometry": {
              "type": "LineString",
              "coordinates": [ [-180,85], [180,-85] ]
            },
            "properties": { "name": "A" }
          }]
        }),"data");
        assert.equal(vtile.queryMany([[175,80]],{tolerance:1,fields:['name'],layer:'data'}).hits.length,0);
        done();
    });
    it('MultiPoint - no features', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify({
          "type": "FeatureCollection",
          "features": [{
            "type": "Feature",
            "geometry": {
              "type": "MultiPoint",
              "coordinates": [ [-180,85], [180,-85] ]
            },
            "properties": { "name": "A" }
          }]
        }),"data");
        assert.equal(vtile.queryMany([[175,80]],{tolerance:1,fields:['name'],layer:'data'}).hits.length,0);
        done();
    });
    // why does this fail (?)
    it.skip('Polygon - no features', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify({
          "type": "FeatureCollection",
          "features": [{
            "type": "Feature",
            "geometry": {
              "type": "Polygon",
              "coordinates": [ [-180,85], [180,-85], [-180,-85], [-180,85] ]
            },
            "properties": { "name": "A" }
          }]
        }),"data");
        assert.equal(vtile.queryMany([[175,80]],{tolerance:1,fields:['name'],layer:'data'}).hits.length,0);
        done();
    });
});

