"use strict";

var mapnik = require('../');
var assert = require('assert');
var path = require('path');
var fs = require('fs');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));

describe('mapnik.VectorTile query polygon', function() {
    var vtile;
    before(function(done) {
        var data = fs.readFileSync(path.resolve(__dirname + "/data/vector_tile/tile3.mvt"));
        vtile = new mapnik.VectorTile(5,28,12);
        vtile.setData(data);
        done();
    });

    it('query fails due to bad parameters', function(done) {
        assert.throws(function() { vtile.query(); });
        assert.throws(function() { vtile.query(1); });
        assert.throws(function() { vtile.query(1,'2'); });
        assert.throws(function() { vtile.query(1,2,null); });
        assert.throws(function() { vtile.query(1,2,{tolerance:null}); });
        assert.throws(function() { vtile.query(1,2,{layer:null}); });
        done();
    });

    it('should fail when querying an invalid .mvt', function(done) {
        var badTile = fs.readFileSync(path.resolve(__dirname + '/data/vector_tile/invalid_v2_tile_bad_geom.mvt'));
        var invalidTile = new mapnik.VectorTile(0,0,0);
        invalidTile.setData(badTile); // bad geometry doesn't fail setData validation
        assert.throws(function() { invalidTile.query(1,1); });

        // test async query throws an error
        invalidTile.query(1,1, function(err) {
            assert.throws(function() { if(err) throw err; });
            done();
        });
    });

    it('should return nothing when querying an image layer', function(done) {
        var vtile2 = new mapnik.VectorTile(0,0,0, {tile_size:256});
        var im = new mapnik.Image(256,256);
        vtile2.addImage(im, 'foo');
        assert.deepEqual(vtile2.query(0,0), []);
        vtile2.query(0,0, function(err, features) {
            assert.ifError(err);
            assert.deepEqual(features, []);
            done();
        });
    });

    it('query polygon', function(done) {
        check(vtile.query(139.6142578125,37.17782559332976,{tolerance:0}));
        vtile.query(139.6142578125,37.17782559332976,{tolerance:0}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,2);
            assert.equal(JSON.parse(features[0].toJSON()).properties.NAME,'Japan');
            assert.equal(features[0].id(),89);
            assert.equal(features[0].geometry().type(),mapnik.Geometry.Polygon);
            assert.equal(features[0].distance,0);
            assert.equal(features[0].x_hit,0);
            assert.equal(features[0].y_hit,0);
            assert.equal(features[0].layer,'world');
            assert.equal(JSON.parse(features[1].toJSON()).properties.NAME,'Japan');
            assert.equal(features[1].id(),89);
            assert.equal(features[1].geometry().type(),mapnik.Geometry.Polygon);
            assert.equal(features[1].distance,0);
            assert.equal(features[1].layer,'world2');
        }
    });

    it('query polygon + tolerance (noop)', function(done) {
        // tolerance only applies to points and lines currently in mapnik::hit_test
        check(vtile.query(142.3388671875,39.52099229357195,{tolerance:100000000000000}));
        vtile.query(142.3388671875,39.52099229357195,{tolerance:100000000000000}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,0);
        }
    });
    // restrict to single layer
    // first query one that does not exist
    it('query polygon + layer (doesnotexist)', function(done) {
        check(vtile.query(139.6142578125,37.17782559332976,{tolerance:0,layer:'doesnotexist'}));
        vtile.query(139.6142578125,37.17782559332976,{tolerance:0,layer:'doesnotexist'}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,0);
        }
    });
    // query one that does exist
    it('query polygon + layer (world)', function(done) {
        check(vtile.query(139.6142578125,37.17782559332976,{tolerance:0,layer:vtile.names()[0]}));
        vtile.query(139.6142578125,37.17782559332976,{tolerance:0,layer:vtile.names()[0]}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,1);
            assert.equal(features[0].id(),89);
            assert.equal(features[0].geometry().type(),mapnik.Geometry.Polygon);
            assert.equal(features[0].distance,0);
            assert.equal(features[0].layer,'world');
        }
    });
});

describe('mapnik.VectorTile query polygon (clipped)', function() {
    var vtile;
    before(function(done) {
        var pbf = require('fs').readFileSync(path.resolve(__dirname + '/data/vector_tile/6.20.34.pbf'));
        vtile = new mapnik.VectorTile(6, 20, 34);
        vtile.setData(pbf);
        done();
    });
    // ensure querying clipped polygons works
    it('query polygon', function(done) {
        var json = vtile.toJSON();
        assert.equal(2, json[0].features.length);
        assert.equal('Brazil', json[0].features[0].properties.name);
        assert.equal('Bolivia', json[0].features[1].properties.name);
        check(vtile.query(-64.27521952641217,-16.28853953000943,{tolerance:10}));
        vtile.query(-64.27521952641217,-16.28853953000943,{tolerance:10}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(1, features.length);
            assert.equal(features[0].distance,0);
            assert.equal(features[0].layer,'data');
            var feat_json = JSON.parse(features[0].toJSON());
            assert.equal('Bolivia',feat_json.properties.name);
            assert.equal(86,feat_json.id);
        }
    });
});

describe('mapnik.VectorTile query point', function() {
    var vtile;
    before(function(done) {
        vtile = new mapnik.VectorTile(0,0,0);
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
                "name": "A"
              }
            },
            {
              "type": "Feature",
              "geometry": {
                "type": "Point",
                "coordinates": [
                  -123,
                  49
                ]
              },
              "properties": {
                "name": "B"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        assert.equal(vtile.empty(), false);
        done();
    });
    it('query point (none)', function(done) {
        check(vtile.query(-120,48,{tolerance:10000}));
        vtile.query(-120,48,{tolerance:10000},function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,0);
        }
    });
    it('query point (A)', function(done) {
        // at z0 we need a large tolerance because of loss of precision in point coords
        // because the points have been rounded to -121.9921875,47.98992166741417
        check(vtile.query(-122,48,{tolerance:10000}));
        vtile.query(-122,48,{tolerance:10000},function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,1);
            assert.equal(features[0].id(),1);
            assert.equal(features[0].geometry().type(),mapnik.Geometry.Point);
            assert.ok(Math.abs(features[0].distance - 1888.66) < 1);
            assert.ok(Math.abs(features[0].x_hit - -121.9921875) < 1);
            assert.ok(Math.abs(features[0].y_hit - 47.98992166741417) < 1);
            assert.equal(features[0].layer,'layer-name');
        }
    });
    it('query point + tolerance (A,B)', function(done) {
        check(vtile.query(-122,48,{tolerance:1e6}));
        vtile.query(-122,48,{tolerance:1e6},function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,2);
            assert.deepEqual(features.map(function(f) { return f.id(); }),[1,2]);
            assert.deepEqual(features.map(function(f) { return f.layer; }),['layer-name','layer-name']);
            assert.deepEqual(features.map(function(f) { return f.attributes().name; }),['A','B']);
            assert.deepEqual(features.map(function(f) { return Math.round(f.distance/1000) * 1000; }),[2000,196000]);
        }
    });
    it('query point + tolerance (B,A)', function(done) {
        check(vtile.query(-123,49,{tolerance:1e6}));
        vtile.query(-123,49,{tolerance:1e6},function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,2);
            assert.deepEqual(features.map(function(f) { return f.id(); }),[2,1]);
            assert.deepEqual(features.map(function(f) { return f.layer; }),['layer-name','layer-name']);
            assert.deepEqual(features.map(function(f) { return f.attributes().name; }),['B','A']);
            assert.deepEqual(features.map(function(f) { return Math.round(f.distance/1000) * 1000; }),[6000,203000]);
        }
    });
});

describe('mapnik.VectorTile query line', function() {
    var vtile;
    before(function(done) {
        vtile = new mapnik.VectorTile(0,0,0);
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
            {
              "type": "Feature",
              "geometry": {
                "type": "LineString",
                "coordinates": [
                  [40,40],
                  [40.1,40.1],
                  [60,60],
                  [60.1,60.1]
                ]
              },
              "properties": {
                "name": "B"
              }
            },
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        done();
    });
    it('query line (none)', function(done) {
        check(vtile.query(20.2,20.2,{tolerance:1}));
        vtile.query(20.2,20.2,{tolerance:1}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,0);
        }
    });
    it('query line (A pt 0)', function(done) {
        check(vtile.query(0,0,{tolerance:1}));
        vtile.query(0,0,{tolerance:1}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,1);
            assert.equal(features[0].id(),1);
            assert.equal(features[0].geometry().type(),mapnik.Geometry.LineString);
            assert.ok(features[0].distance < 0.00000001);
            assert.equal(features[0].attributes().name,'A');
            assert.equal(features[0].layer,'layer-name');
        }
    });
    it('query line (A pt 4)', function(done) {
        check(vtile.query(20.1,20.1,{tolerance:1000}));
        vtile.query(20.1,20.1,{tolerance:1000}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,1);
            assert.equal(features[0].id(),1);
            assert.equal(features[0].geometry().type(),mapnik.Geometry.LineString);
            assert.ok(features[0].distance < 1000);
            assert.equal(features[0].attributes().name,'A');
            assert.equal(features[0].layer,'layer-name');
        }
    });
    it('query line + tolerance (A,B)', function(done) {
        check(vtile.query(0,0,{tolerance:1e9}));
        vtile.query(0,0,{tolerance:1e9}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,2);
            assert.deepEqual(features.map(function(f) { return f.id(); }),[1,2]);
            assert.deepEqual(features.map(function(f) { return f.layer; }),['layer-name','layer-name']);
            assert.deepEqual(features.map(function(f) { return f.attributes().name; }),['A','B']);
            assert.deepEqual(features.map(function(f) { return Math.round(f.distance/1000) * 1000; }),[0,6593000]);
        }
    });
    it('query line + tolerance (B,A)', function(done) {
        check(vtile.query(40,40,{tolerance:1e9}));
        vtile.query(40,40,{tolerance:1e9}, function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,2);
            assert.deepEqual(features.map(function(f) { return f.id(); }),[2,1]);
            assert.deepEqual(features.map(function(f) { return f.layer; }),['layer-name','layer-name']);
            assert.deepEqual(features.map(function(f) { return f.attributes().name; }),['B','A']);
            assert.deepEqual(features.map(function(f) { return Math.round(f.distance/1000) * 1000; }),[1000,3396000]);
        }
    });
});

describe('mapnik.VectorTile query multiline', function() {
    var vtile;
    before(function(done) {
        vtile = new mapnik.VectorTile(0,0,0);
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "MultiLineString",
                "coordinates": [
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
                ]
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        done();
    });
    it('query multiline (pt @ 1,1)', function(done) {
        check(vtile.query(1,1,{tolerance:100}));
        vtile.query(1,1,{tolerance:100},function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,1);
            assert.equal(features[0].id(),1);
            assert.equal(features[0].geometry().type(),mapnik.Geometry.MultiLineString);
            assert.ok(features[0].distance < 100);
            assert.equal(features[0].layer,'layer-name');
        }
    });
    it('query multiline (pt @ 25,25)', function(done) {
        check(vtile.query(25,25,{tolerance:10000}));
        vtile.query(25,25,{tolerance:10000},function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,1);
            assert.equal(features[0].id(),1);
            assert.equal(features[0].geometry().type(),mapnik.Geometry.LineString);
            assert.ok(features[0].distance < 10000);
            assert.equal(features[0].layer,'layer-name');
        }
    });
});

describe('mapnik.VectorTile query multipoint', function() {
    var vtile;
    before(function(done) {
        vtile = new mapnik.VectorTile(0,0,0);
        var coords = [
          [0.1,0.1],
          [0.19,0.1],
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
        done();
    });

    it('query multipoint (pt @ 0.1,0.1)', function(done) {
        check(vtile.query(0.1,0.1,{tolerance:10000}));
        vtile.query(0.1,0.1,{tolerance:10000},function(err, features) {
            assert.ifError(err);
            check(features);
            done();
        });
        function check(features) {
            assert.equal(features.length,1);
            assert.equal(features[0].id(),1);
            assert.equal(features[0].geometry().type(),mapnik.Geometry.MultiPoint);
            assert.ok(features[0].distance < 20000);
            assert.equal(features[0].layer,'layer-name');
        }
    });
});

describe('mapnik.VectorTile query (distance <= tolerance)', function() {
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
        }),"layer-name");
        assert.equal(vtile.query(175,80,{tolerance:1}).length,0);
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
              "coordinates": [ [-180,85], [181,-75] ]
            },
            "properties": { "name": "A" }
          }]
        }),"layer-name");
        assert.equal(vtile.query(175,80,{tolerance:1}).length,0);
        done();
    });
    it('Polygon - no features', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify({
          "type": "FeatureCollection",
          "features": [{
            "type": "Feature",
            "geometry": {
              "type": "Polygon",
              "coordinates": [[[-180,85], [180,-85], [-180,-85], [-180,85]]]
            },
            "properties": { "name": "A" }
          }]
        }),"layer-name");
        assert.equal(vtile.query(175,80,{tolerance:1}).length,0);
        done();
    });
});

describe('mapnik.VectorTile query xy single features', function() {
    it('Point', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify({
          "type": "FeatureCollection",
          "features": [{
            "type": "Feature",
            "geometry": {
              "type": "Point",
              "coordinates": [ 0,0 ]
            },
            "properties": { "name": "A" }
          }]
        }),"layer-name");
        var res = vtile.query(0,0,{tolerance:1});
        assert.equal(res[0].x_hit, 0);
        assert.equal(res[0].y_hit, 0);
        assert.equal(res[0].attributes().name, 'A');
        done();
    });

    it('MultiPoint', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify({
          "type": "FeatureCollection",
          "features": [{
            "type": "Feature",
            "geometry": {
              "type": "MultiPoint",
              "coordinates": [[ 0,0 ]]
            },
            "properties": { "name": "A" }
          }]
        }),"layer-name");
        var res = vtile.query(0,0,{tolerance:1});
        assert.equal(res[0].x_hit, 0);
        assert.equal(res[0].y_hit, 0);
        assert.equal(res[0].attributes().name, 'A');
        done();
    });

    it('LineString', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify({
          "type": "FeatureCollection",
          "features": [{
            "type": "Feature",
            "geometry": {
              "type": "LineString",
              "coordinates": [ [ 0,0 ], [100, 100] ]
            },
            "properties": { "name": "A" }
          }]
        }),"layer-name");
        var res = vtile.query(0,0,{tolerance:1});
        assert.equal(res[0].x_hit, 0);
        assert.equal(res[0].y_hit, 0);
        assert.equal(res[0].attributes().name, 'A');
        done();
    });

    it('MultiLineString', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addGeoJSON(JSON.stringify({
          "type": "FeatureCollection",
          "features": [{
            "type": "Feature",
            "geometry": {
              "type": "MultiLineString",
              "coordinates": [ [ [ 0,0 ], [100, 100] ] ]
            },
            "properties": { "name": "A" }
          }]
        }),"layer-name");
        var res = vtile.query(0,0,{tolerance:1});
        assert.equal(res[0].x_hit, 0);
        assert.equal(res[0].y_hit, 0);
        assert.equal(res[0].attributes().name, 'A');
        done();
    });

    // -------
    // | . . |
    // | . . |
    // -------
    it('Multiple Points', function(done) {
        var vtile = new mapnik.VectorTile(14,8192,8192);
        vtile.addGeoJSON(JSON.stringify({
            "type": "FeatureCollection",
            "features": [{
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates": [ 0.005, -0.015 ]
                },
                "properties": { "name": "Point A" }
            },{
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates": [ 0.015,-0.015 ]
                },
                "properties": { "name": "Point B" }
            },{
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates": [ 0.015, -0.005 ]
                },
                "properties": { "name": "Point C" }
            },{
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates": [ 0.005,-0.005 ]
                },
                "properties": { "name": "Point D" }
            }]
        }),"layer-name");
        var res = vtile.query(0.015, -0.005, {tolerance:10000});

        for (var res_it = 0; res_it < res.length; res_it++) {
            res[res_it].x_hit = Math.round(res[res_it].x_hit * 1000000);
            res[res_it].y_hit = Math.round(res[res_it].y_hit * 1000000);
        }
        assert.deepEqual([res[0].x_hit, res[0].y_hit], [ 14999, -5000  ]);
        assert.deepEqual([res[1].x_hit, res[1].y_hit], [ 14999, -14999 ]);
        assert.deepEqual([res[2].x_hit, res[2].y_hit], [ 5000 , -5000  ]);
        assert.deepEqual([res[3].x_hit, res[3].y_hit], [ 5000 , -14999 ]);
        done();
    });
});

