"use strict";

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var mercator = new(require('@mapbox/sphericalmercator'))();
var existsSync = require('fs').existsSync || require('path').existsSync;
var overwrite_expected_data = false;
var zlib = require('zlib');
var boost_version = mapnik.versions.boost.split('.');

var hasBoostSimple = false;
if (boost_version[0] > 1 || (boost_version[0] == 1 && boost_version[1] >= 58))
{
    hasBoostSimple = true;
}

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));

var trunc_6 = function(key, val) {
    return val.toFixed ? Number(val.toFixed(6)) : val;
};

function deepEqualTrunc(json1,json2) {
    return assert.deepEqual(JSON.stringify(json1,trunc_6),JSON.stringify(json2,trunc_6));
}

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'gdal.input'));

describe('mapnik.VectorTile ', function() {
    // generate test data
    var _vtile;
    var _data;
    var _length;
    before(function(done) {
        if (overwrite_expected_data) {
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/data/vector_tile/layers.xml');
            var vtile = new mapnik.VectorTile(9,112,195);
            _vtile = vtile;
            map.extent = [-11271098.442818949,4696291.017841229,-11192826.925854929,4774562.534805249];
            map.render(vtile,{},function(err,vtile) {
                if (err) throw err;
                fs.writeFileSync('./test/data/vector_tile/tile1.vector.pbf',vtile.getData());
                fs.writeFileSync('./test/data/vector_tile/tile1.vector.pbf.gz',vtile.getData({compress:"gzip"}));
                _data = vtile.getData().toString("hex");
                _length = vtile.getData().length;
                var map2 = new mapnik.Map(256,256);
                map2.loadSync('./test/data/vector_tile/layers.xml');
                var vtile2 = new mapnik.VectorTile(5,28,12);
                var bbox = mercator.bbox(28, 12, 5, false, '900913');
                map2.extent = bbox;
                map2.render(vtile2,{}, function(err,vtile2) {
                    if (err) throw err;
                    fs.writeFileSync("./test/data/vector_tile/tile3.mvt",vtile2.getData());
                    done();
                });
            });
        } else {
            _vtile = new mapnik.VectorTile(9,112,195);
            _vtile.setData(fs.readFileSync('./test/data/vector_tile/tile1.vector.pbf'));
            _data = _vtile.getData().toString("hex");
            _length = _vtile.getData().length;
            done();
        }
    });

    if (hasBoostSimple) {
        it('should fail when bad parameters are passed to reportGeometrySimplicity', function() {
            var vtile = new mapnik.VectorTile(0,0,0);
            assert.throws(function() { vtile.reportGeometrySimplicity(null); });
        });

        // this is not recommend, doing this purely to ensure we have test coverage
        // it('successfully creates a not_simple_feature with a non-simple geometry', function(done) {
        //     var vtile = new mapnik.VectorTile(0,0,0);
        //     var gj_str = fs.readFileSync('./test/data/bowtie.geojson').toString('utf8');
        //     vtile.addGeoJSON(gj_str, 'not-simple', {strictly_simple: false, fill_type: mapnik.polygonFillType.evenOdd});
        //     var simple = vtile.reportGeometrySimplicity();
        //     // some sort of assert
        //     done();
        // });
    } else {
        it('should fail when bad parameters are passed to reportGeometrySimplicity', function() {});
    }

    if (hasBoostSimple) {
        it('empty tile should be simple', function (done) {
            var vtile = new mapnik.VectorTile(0,0,0);
            assert.equal(vtile.reportGeometrySimplicitySync(), 0);
            assert.equal(vtile.reportGeometrySimplicity(), 0);
            vtile.reportGeometrySimplicity(function(err, simple) {
                if (err) throw err;
                assert.equal(simple, 0);
                done();
            });
        });
    } else {
        it.skip('empty tile should be simple', function () {});
    }

    if (hasBoostSimple) {
        it('should fail when bad parameters are passed to reportGeometryValidity', function() {
            var vtile = new mapnik.VectorTile(0,0,0);
            assert.throws(function() { vtile.reportGeometryValidity(null); });
            assert.throws(function() { vtile.reportGeometryValidity(1); });
            assert.throws(function() { vtile.reportGeometryValidity({split_multi_features:null}); });
            assert.throws(function() { vtile.reportGeometryValidity({lat_lon:null}); });
            assert.throws(function() { vtile.reportGeometryValidity({web_merc:null}); });
            assert.throws(function() { vtile.reportGeometryValidity({}, null); });
            assert.throws(function() { vtile.reportGeometryValiditySync(null); });
            assert.throws(function() { vtile.reportGeometryValiditySync(1); });
            assert.throws(function() { vtile.reportGeometryValiditySync({split_multi_features:null}); });
            assert.throws(function() { vtile.reportGeometryValiditySync({lat_lon:null}); });
            assert.throws(function() { vtile.reportGeometryValiditySync({web_merc:null}); });
            assert.throws(function() { vtile.reportGeometryValidity(null, function() {}); });
            assert.throws(function() { vtile.reportGeometryValidity(1, function() {}); });
            assert.throws(function() { vtile.reportGeometryValidity({split_multi_features:null}, function() {}); });
            assert.throws(function() { vtile.reportGeometryValidity({lat_lon:null}, function() {}); });
            assert.throws(function() { vtile.reportGeometryValidity({web_merc:null}, function() {}); });
        });
    } else {
        it.skip('should fail when bad parameters are passed to reportGeometryValidity', function() {});
    }

    if (hasBoostSimple) {
        it('empty tile should be valid', function (done) {
            var vtile = new mapnik.VectorTile(0,0,0);
            assert.equal(vtile.reportGeometryValiditySync(), 0);
            assert.equal(vtile.reportGeometryValidity(), 0);
            vtile.reportGeometryValidity(function(err, valid) {
                if (err) throw err;
                assert.equal(valid, 0);
                done();
            });
        });
    } else {
        it.skip('empty tile should be valid', function (done) {});
    }

    it('should fail when adding bad parameters to add geoJSON', function() {
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
        var geo_str = JSON.stringify(geojson);
        assert.throws(function() { vtile.addGeoJSON('asdf', 'layer-a'); });
        assert.throws(function() { vtile.addGeoJSON(); });
        assert.throws(function() { vtile.addGeoJSON(geo_str); });
        assert.throws(function() { vtile.addGeoJSON(1, "layer"); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, 1); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", null); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", {area_threshold:null}); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", {area_threshold:-1}); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", {strictly_simple:null}); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", {multi_polygon_union:null}); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", {fill_type:null}); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", {fill_type:99}); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", {process_all_rings:null}); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", {simplify_distance:null}); });
        assert.throws(function() { vtile.addGeoJSON(geo_str, "layer", {simplify_distance:-0.5}); });
    });

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
        assert.equal(vtile.getData().length,58);
        var out = JSON.parse(vtile.toGeoJSON(0));
        assert.equal(out.type,'FeatureCollection');
        assert.equal(out.features.length,1);
        var coords = out.features[0].geometry.coordinates;
        assert.ok(Math.abs(coords[0] - geojson.features[0].geometry.coordinates[0]) < 0.3);
        assert.ok(Math.abs(coords[1] - geojson.features[0].geometry.coordinates[1]) < 0.3);
        assert.equal(out.features[0].properties.name,'geojson data');
        assert.equal(vtile.toGeoJSON(0),vtile.toGeoJSONSync(0));
        vtile.toGeoJSON(0,function(err,json_string) {
            var out2 = JSON.parse(json_string);
            assert.equal(out2.type,'FeatureCollection');
            assert.equal(out2.features.length,1);
            var coords = out2.features[0].geometry.coordinates;
            assert.ok(Math.abs(coords[0] - geojson.features[0].geometry.coordinates[0]) < 0.3);
            assert.ok(Math.abs(coords[1] - geojson.features[0].geometry.coordinates[1]) < 0.3);
            assert.equal(out2.features[0].properties.name,'geojson data');
            done();
        });
    });

    it('should be able to create a vector tile from geojson - with simplification', function(done) {
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
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name", {simplify_distance:1.0,path_multiplier:16,area_threshold:0.1,strictly_simple:false} );
        assert.equal(vtile.getData().length,58);
        var out = JSON.parse(vtile.toGeoJSON(0));
        assert.equal(out.type,'FeatureCollection');
        assert.equal(out.features.length,1);
        var coords = out.features[0].geometry.coordinates;
        assert.ok(Math.abs(coords[0] - geojson.features[0].geometry.coordinates[0]) < 0.3);
        assert.ok(Math.abs(coords[1] - geojson.features[0].geometry.coordinates[1]) < 0.3);
        assert.equal(out.features[0].properties.name,'geojson data');
        assert.equal(vtile.toGeoJSON(0),vtile.toGeoJSONSync(0));
        vtile.toGeoJSON(0,function(err,json_string) {
            var out2 = JSON.parse(json_string);
            assert.equal(out2.type,'FeatureCollection');
            assert.equal(out2.features.length,1);
            var coords = out2.features[0].geometry.coordinates;
            assert.ok(Math.abs(coords[0] - geojson.features[0].geometry.coordinates[0]) < 0.3);
            assert.ok(Math.abs(coords[1] - geojson.features[0].geometry.coordinates[1]) < 0.3);
            assert.equal(out2.features[0].properties.name,'geojson data');
            done();
        });
    });

    it('should be able to export point with toJSON decode_geometry', function(done) {
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
        var actual = vtile.toJSON({decode_geometry:true});
        var expected = [ {
            name: 'layer-name',
            extent: 4096,
            version: 2,
            features: [{
                geometry: [660,1424],
                geometry_type: "Point",
                id: 1,
                properties: {
                  name: "geojson data"
                },
                type: 1
              }]
        } ];
        assert.deepEqual(actual, expected);
        done();
    });

    it('should be able to export multipoint with toJSON decode_geometry', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "MultiPoint",
                "coordinates": [[
                  -122,
                  48
                ],
                [
                  -121,
                  48
                ]]
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var actual = vtile.toJSON({decode_geometry:true});
        var expected = [ {
            name: 'layer-name',
            extent: 4096,
            version: 2,
            features: [{
                geometry: [[660,1424],[671,1424]],
                geometry_type: "MultiPoint",
                id: 1,
                properties: {
                  name: "geojson data"
                },
                type: 1
              }]
        } ];
        assert.deepEqual(actual, expected);
        done();
    });

    it('should be able to export line-string with toJSON decode_geometry', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "LineString",
                "coordinates": [[
                  -122,
                  48
                ],
                [
                  -121,
                  48
                ]]
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var actual = vtile.toJSON({decode_geometry:true});
        var expected = [ {
            name: 'layer-name',
            extent: 4096,
            version: 2,
            features: [{
                geometry: [[660,1424],[671,1424]],
                geometry_type: "LineString",
                id: 1,
                properties: {
                  name: "geojson data"
                },
                type: 2
              }]
        } ];
        assert.deepEqual(actual, expected);
        done();
    });

    it('should be able to export multi-line-string with toJSON decode_geometry', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "MultiLineString",
                "coordinates": [[[
                      -122,
                      48
                    ],
                    [
                      -121,
                      48
                    ]],[[
                      -122,
                      49
                    ],
                    [
                      -121,
                      49

                ]]]
              },
              "properties": {
                "name": "geojson data",
                "bool": true // test for boolean coverage
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        var actual = vtile.toJSON({decode_geometry:true});
        var expected = [ {
            name: 'layer-name',
            extent: 4096,
            version: 2,
            features: [{
                geometry: [[[660,1424],[671,1424]],[[660,1407],[671,1407]]],
                geometry_type: "MultiLineString",
                id: 1,
                properties: {
                  name: "geojson data",
                  "bool": true
                },
                type: 2
              }]
        } ];
        assert.deepEqual(actual, expected);
        done();
    });

    it('should throw when x, y, or z are negative', function(done) {
      assert.throws(function() { new mapnik.VectorTile(0,0,-1); });
      assert.throws(function() { new mapnik.VectorTile(0,-1,0); });
      assert.throws(function() { new mapnik.VectorTile(-1,0,0); });
      done();
    });

    it('should be able to create a vector tile from geojson - multipoint', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "MultiPoint",
                "coordinates": [[
                  -122,
                  48
                ],
                [
                  -121,
                  48
                ]]
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name");
        assert.equal(vtile.getData().length,60);
        var out = JSON.parse(vtile.toGeoJSON(0));
        assert.equal(out.type,'FeatureCollection');
        assert.equal(out.features.length,1);
        var coords = out.features[0].geometry.coordinates;
        assert.ok(Math.abs(coords[0][0] - geojson.features[0].geometry.coordinates[0][0]) < 0.3);
        assert.ok(Math.abs(coords[0][1] - geojson.features[0].geometry.coordinates[0][1]) < 0.3);
        assert.ok(Math.abs(coords[1][0] - geojson.features[0].geometry.coordinates[1][0]) < 0.3);
        assert.ok(Math.abs(coords[1][1] - geojson.features[0].geometry.coordinates[1][1]) < 0.3);
        assert.equal(out.features[0].properties.name,'geojson data');
        assert.equal(vtile.toGeoJSON(0),vtile.toGeoJSONSync(0));
        vtile.toGeoJSON(0,function(err,json_string) {
            var out2 = JSON.parse(json_string);
            assert.equal(out2.type,'FeatureCollection');
            assert.equal(out2.features.length,1);
            var coords = out2.features[0].geometry.coordinates;
            assert.ok(Math.abs(coords[0][0] - geojson.features[0].geometry.coordinates[0][0]) < 0.3);
            assert.ok(Math.abs(coords[0][1] - geojson.features[0].geometry.coordinates[0][1]) < 0.3);
            assert.ok(Math.abs(coords[1][0] - geojson.features[0].geometry.coordinates[1][0]) < 0.3);
            assert.ok(Math.abs(coords[1][1] - geojson.features[0].geometry.coordinates[1][1]) < 0.3);
            assert.equal(out2.features[0].properties.name,'geojson data');
            done();
        });
    });

    it('should be able to create a vector tile from geojson - multipoint - with simplification', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var geojson = {
          "type": "FeatureCollection",
          "features": [
            {
              "type": "Feature",
              "geometry": {
                "type": "MultiPoint",
                "coordinates": [[
                  -122,
                  48
                ],
                [
                  -121,
                  48
                ]]
              },
              "properties": {
                "name": "geojson data"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson),"layer-name", {simplify_distance:1.0});
        assert.equal(vtile.getData().length,60);
        var out = JSON.parse(vtile.toGeoJSON(0));
        assert.equal(out.type,'FeatureCollection');
        assert.equal(out.features.length,1);
        var coords = out.features[0].geometry.coordinates;
        assert.ok(Math.abs(coords[0][0] - geojson.features[0].geometry.coordinates[0][0]) < 0.3);
        assert.ok(Math.abs(coords[0][1] - geojson.features[0].geometry.coordinates[0][1]) < 0.3);
        assert.ok(Math.abs(coords[1][0] - geojson.features[0].geometry.coordinates[1][0]) < 0.3);
        assert.ok(Math.abs(coords[1][1] - geojson.features[0].geometry.coordinates[1][1]) < 0.3);
        assert.equal(out.features[0].properties.name,'geojson data');
        assert.equal(vtile.toGeoJSON(0),vtile.toGeoJSONSync(0));
        vtile.toGeoJSON(0,function(err,json_string) {
            var out2 = JSON.parse(json_string);
            assert.equal(out2.type,'FeatureCollection');
            assert.equal(out2.features.length,1);
            var coords = out2.features[0].geometry.coordinates;
            assert.ok(Math.abs(coords[0][0] - geojson.features[0].geometry.coordinates[0][0]) < 0.3);
            assert.ok(Math.abs(coords[0][1] - geojson.features[0].geometry.coordinates[0][1]) < 0.3);
            assert.ok(Math.abs(coords[1][0] - geojson.features[0].geometry.coordinates[1][0]) < 0.3);
            assert.ok(Math.abs(coords[1][1] - geojson.features[0].geometry.coordinates[1][1]) < 0.3);
            assert.equal(out2.features[0].properties.name,'geojson data');
            done();
        });
    });

    it('should be able to create a vector tile from multiple geojson files', function(done) {
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
        var geojson2 = {
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
                "name": "geojson data2"
              }
            }
          ]
        };
        vtile.addGeoJSON(JSON.stringify(geojson2),"layer-name2");
        assert.equal(vtile.getData().length,118);
        // test flattened to single geojson
        var json_out = vtile.toGeoJSONSync('__all__');
        var out = JSON.parse(json_out);
        assert.equal(out.type,'FeatureCollection');
        assert.equal(out.features.length,2);
        var coords = out.features[0].geometry.coordinates;
        assert.ok(Math.abs(coords[0] - geojson.features[0].geometry.coordinates[0]) < 0.3);
        assert.ok(Math.abs(coords[1] - geojson.features[0].geometry.coordinates[1]) < 0.3);
        var coords2 = out.features[1].geometry.coordinates;
        assert.ok(Math.abs(coords2[0] - geojson2.features[0].geometry.coordinates[0]) < 0.3);
        assert.ok(Math.abs(coords2[1] - geojson2.features[0].geometry.coordinates[1]) < 0.3);
        assert.equal(out.features[0].properties.name,'geojson data');
        assert.equal(out.features[1].properties.name,'geojson data2');
        // not passing callback trigger sync method
        assert.equal(vtile.toGeoJSON('__all__'),json_out);
        // Test that we can pull back the different layers by name
        var json_one = vtile.toGeoJSON('layer-name');
        var json_two = vtile.toGeoJSON('layer-name2');
        assert(json_one.length > 0);
        assert(json_two.length > 0);
        // Test for an invalid name throws
        assert.throws(function() { vtile.toGeoJSON('foo'); });
        // test array containing each geojson
        var json_array = vtile.toGeoJSONSync('__array__');
        var out2 = JSON.parse(json_array);
        assert.equal(out2.length,2);
        var coords3 = out2[0].features[0].geometry.coordinates;
        assert.ok(Math.abs(coords3[0] - geojson.features[0].geometry.coordinates[0]) < 0.3);
        assert.ok(Math.abs(coords3[1] - geojson.features[0].geometry.coordinates[1]) < 0.3);
        // not passing callback trigger sync method
        assert.equal(vtile.toGeoJSON('__array__'),json_array);
        // ensure async method has same result, but chain together callbacks
        // since we need to call done() at the end
        vtile.toGeoJSON('__all__',function(err,json_string) {
            assert.deepEqual(json_string,json_out);
            vtile.toGeoJSON('__array__',function(err,json_string) {
                assert.deepEqual(json_string,json_array);
                vtile.toGeoJSON('layer-name', function(err, json_string) {
                  assert(json_string.length > 0);
                  done();
                });
            });
        });
    });

    it('toGeoJSON should fail with invalid useage', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.throws(function() { vtile.toGeoJSONSync(); });
        assert.throws(function() { vtile.toGeoJSONSync(0); });
        assert.throws(function() { vtile.toGeoJSONSync(-1); });
        assert.throws(function() { vtile.toGeoJSONSync('foo'); });
        assert.throws(function() { vtile.toGeoJSONSync(null); });
        assert.throws(function() { vtile.toGeoJSON(); });
        assert.throws(function() { vtile.toGeoJSON(0, function(err, jstr) {}) });
        assert.throws(function() { vtile.toGeoJSON(-1, function(err, jstr) {}) });
        assert.throws(function() { vtile.toGeoJSON('foo', function(err, jstr) {}) });
        assert.throws(function() { vtile.toGeoJSON(null, function(err, jstr) {}) });

        // Error message change after adding a feature.
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

        // Test failures again
        assert.throws(function() { vtile.toGeoJSONSync(); });
        assert.throws(function() { vtile.toGeoJSONSync(1); });
        assert.throws(function() { vtile.toGeoJSONSync(-1); });
        assert.throws(function() { vtile.toGeoJSONSync('foo'); });
        assert.throws(function() { vtile.toGeoJSONSync(null); });
        assert.throws(function() { vtile.toGeoJSON(); });
        assert.throws(function() { vtile.toGeoJSON(1, function(err, jstr) {}) });
        assert.throws(function() { vtile.toGeoJSON(-1, function(err, jstr) {}) });
        assert.throws(function() { vtile.toGeoJSON('foo', function(err, jstr) {}) });
        assert.throws(function() { vtile.toGeoJSON(null, function(err, jstr) {}) });

        done();
    });

    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.VectorTile(); });

        // invalid args
        assert.throws(function() { new mapnik.VectorTile(); });
        assert.throws(function() { new mapnik.VectorTile(1); });
        assert.throws(function() { new mapnik.VectorTile('foo'); });
        assert.throws(function() { new mapnik.VectorTile('a', 'b', 'c'); });
        assert.throws(function() { new mapnik.VectorTile(0,0,0,null); });
        assert.throws(function() { new mapnik.VectorTile(0,0,0,{tile_size:null}); });
        assert.throws(function() { new mapnik.VectorTile(0,0,0,{buffer_size:null}); });

        // invalid range
        assert.throws(function() { new mapnik.VectorTile(-1,0,0); });
        assert.throws(function() { new mapnik.VectorTile(1,-1,0); });
        assert.throws(function() { new mapnik.VectorTile(1,0,-1); });
        assert.throws(function() { new mapnik.VectorTile(1,0,2); });
        assert.throws(function() { new mapnik.VectorTile(1,2,0); });
        assert.throws(function() { new mapnik.VectorTile(4,16,0); });
        assert.throws(function() { new mapnik.VectorTile(4,0,16); });

    });

    it('should not throw when using higher zoom levels', function() {
        assert.doesNotThrow(function() { new mapnik.VectorTile(28,0,0); });
        assert.doesNotThrow(function() { new mapnik.VectorTile(35,0,0); });
        assert.doesNotThrow(function() { new mapnik.VectorTile(45,0,0); });
    });

    it('should be initialized properly', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.ok(vtile instanceof mapnik.VectorTile);
        assert.equal(vtile.tileSize, 4096);
        assert.equal(vtile.bufferSize, 128);
        assert.equal(vtile.painted(), false);
        assert.equal(vtile.getData().toString(),"");
        if (hasBoostSimple) {
            assert.equal(vtile.reportGeometrySimplicity().length, 0);
            assert.equal(vtile.reportGeometryValidity().length, 0);
        }
        assert.equal(vtile.empty(), true);
    });

    it('should accept optional tileSize', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0,{tile_size:512});
        assert.equal(vtile.tileSize, 512);
        assert.throws(function() { new mapnik.VectorTile(0,0,0,{tile_size:null});});
        assert.throws(function() { new mapnik.VectorTile(0,0,0,{tile_size:0});});
        assert.throws(function() { new mapnik.VectorTile(0,0,0,{tile_size:-256});});
        assert.throws(function() { vtile.tileSize = 0; });
        assert.throws(function() { vtile.tileSize = -256; });
        assert.throws(function() { vtile.tileSize = 'asdf'; });
        assert.equal(vtile.tileSize, 512);
        vtile.tileSize = 256;
        assert.equal(vtile.tileSize, 256);
        done();
    });

    it('should accept optional bufferSize', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0,{buffer_size:18});
        assert.equal(vtile.bufferSize, 18);
        assert.throws(function() { new mapnik.VectorTile(0,0,0,{buffer_size:null});});
        assert.throws(function() { new mapnik.VectorTile(0,0,0,{buffer_size:-2048});});
        assert.throws(function() { vtile.bufferSize = 'asdf'; });
        assert.throws(function() { vtile.bufferSize = -2048; }); // buffer size greater then tilesize supports
        assert.equal(vtile.bufferSize, 18);
        vtile.bufferSize = 512;
        assert.equal(vtile.bufferSize, 512);
        vtile.bufferSize = -2047;
        assert.equal(vtile.bufferSize, -2047);
        done();
    });

    it('should be able to addData in reasonable time', function(done) {
        var vtile = new mapnik.VectorTile(3,2,3);
        // tile1 represents a "solid" vector tile with one layer
        // that only encodes a single feature with a single path with
        // a polygon box resulting from clipping a chunk out of
        // a larger polygon fully outside the rendered/clipping extent
        var data = fs.readFileSync("./test/data/vector_tile/3.2.3.mbs4.mvt");
        vtile.addData(data, function(err) {
            assert.equal(vtile.empty(), false);
            assert.equal(vtile.painted(), true);
            assert.equal(vtile.empty(), false);
            done();
        });
    });

    it('should be able to setData/parse (sync)', function() {
        var vtile = new mapnik.VectorTile(9,112,195);
        // tile1 represents a "solid" vector tile with one layer
        // that only encodes a single feature with a single path with
        // a polygon box resulting from clipping a chunk out of
        // a larger polygon fully outside the rendered/clipping extent
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        assert.equal(vtile.empty(), false);
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.empty(), false);
        if (hasBoostSimple) {
            assert.equal(vtile.reportGeometrySimplicity().length, 0);
            assert.equal(vtile.reportGeometryValidity().length, 0);
        }
    });

    it('should fail to getData due to bad input', function() {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        assert.throws(
            function() { vtile.getDataSync(null); }, null,
            "first arg must be a options object"
        );
        assert.throws(
            function() { vtile.getData(null, function(err, out) {}); }, null,
            "first arg must be a options object"
        );
        assert.throws(
            function() { vtile.getDataSync({compression:null}); }, null,
            "option 'compression' must be a string, either 'gzip', or 'none' (default)"
        );
        assert.throws(
            function() { vtile.getData({compression:null}, function(err,out) {}); }, null,
            "option 'compression' must be a string, either 'gzip', or 'none' (default)"
        );
        assert.throws(
            function() { vtile.getDataSync({release:null}); }, null,
            "option 'release' must be a boolean"
        );
        assert.throws(
            function() { vtile.getData({release:null}, function(err,out) {}); }, null,
            "option 'release' must be a boolean"
        );
        assert.throws(
            function() { vtile.getDataSync({level:null}); }, null,
            "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive"
        );
        assert.throws(
            function() { vtile.getData({level:null}, function(err,out) {}); }, null,
            "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive"
        );
        assert.throws(
            function() { vtile.getDataSync({level:99}); }, null,
            "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive"
        );
        assert.throws(
            function() { vtile.getData({level:99}, function(err,out) {}); }, null,
            "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive"
        );
        assert.throws(
            function() { vtile.getDataSync({level:-1}); }, null,
            "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive"
        );
        assert.throws(
            function() { vtile.getData({level:-1}, function(err,out) {}); }, null,
            "option 'level' must be an integer between 0 (no compression) and 9 (best compression) inclusive"
        );
        assert.throws(
            function() { vtile.getDataSync({strategy:null}); }, null,
            "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT"
        );
        assert.throws(
            function() { vtile.getData({strategy:null}, function(err,out) {}); }, null,
            "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT"
        );
        assert.throws(
            function() { vtile.getDataSync({strategy:'FOO'}); }, null,
            "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT"
        );
        assert.throws(
            function() { vtile.getData({strategy:'FOO'}, function(err,out) {}); }, null,
            "option 'strategy' must be one of the following strings: FILTERED, HUFFMAN_ONLY, RLE, FIXED, DEFAULT"
        );
    });

    it('should create empty buffer with getData with no data provided.', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var uncompressed = vtile.getData();
        assert.equal(uncompressed.length, 0);
        vtile.getData(function(err, uncompressed2) {
            assert.equal(uncompressed2.length, 0);
            done();
        });
    });

    it('should return the correct bufferedExtent', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var extent = vtile.bufferedExtent();
        var expected = [-11273544.427724076, 4693845.032936104, -11190380.940949803, 4777008.519710373];
        // typically not different, but rounding can cause different values
        // so we assert each value's difference is nominal
        assert(Math.abs(extent[0] - expected[0]) < 1e-8);
        assert(Math.abs(extent[1] - expected[1]) < 1e-8);
        assert(Math.abs(extent[2] - expected[2]) < 1e-8);
        assert(Math.abs(extent[3] - expected[3]) < 1e-8);
        done();
    });

    it('should return the correct extent', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var extent = vtile.extent();
        var expected = [-11271098.44281895, 4696291.017841229, -11192826.925854929, 4774562.534805248];
        // typically not different, but rounding can cause different values
        // so we assert each value's difference is nominal
        assert(Math.abs(extent[0] - expected[0]) < 1e-8);
        assert(Math.abs(extent[1] - expected[1]) < 1e-8);
        assert(Math.abs(extent[2] - expected[2]) < 1e-8);
        assert(Math.abs(extent[3] - expected[3]) < 1e-8);
        done();
    });

    it('only should throw if you try to set x, y, and z with bad input', function() {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.throws(function() { vtile.x = null; });
        assert.throws(function() { vtile.x = '1'; });
        assert.throws(function() { vtile.x = -1; });
        assert.throws(function() { vtile.y = null; });
        assert.throws(function() { vtile.y = '1'; });
        assert.throws(function() { vtile.y = -1; });
        assert.throws(function() { vtile.z = null; });
        assert.throws(function() { vtile.z = '1'; });
        assert.throws(function() { vtile.z = -1; });
    });

    it('should be able to change tile coordinates and it change the extent', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var extent = vtile.extent();
        var expected = [-11271098.442818949, 4696291.017841229, -11192826.925854929, 4774562.534805249];
        // typically not different, but rounding can cause different values
        // so we assert each value's difference is nominal
        assert(Math.abs(extent[0] - expected[0]) < 1e-8);
        assert(Math.abs(extent[1] - expected[1]) < 1e-8);
        assert(Math.abs(extent[2] - expected[2]) < 1e-8);
        assert(Math.abs(extent[3] - expected[3]) < 1e-8);
        assert.equal(vtile.z, 9);
        assert.equal(vtile.x, 112);
        assert.equal(vtile.y, 195);
        vtile.x = 0;
        assert.equal(vtile.x, 0);
        extent = vtile.extent();
        var expected_x = [-20037508.342789244, 4696291.017841229, -19959236.82582522, 4774562.534805249];
        assert(Math.abs(extent[0] - expected_x[0]) < 1e-8);
        assert(Math.abs(extent[1] - expected_x[1]) < 1e-8);
        assert(Math.abs(extent[2] - expected_x[2]) < 1e-8);
        assert(Math.abs(extent[3] - expected_x[3]) < 1e-8);
        vtile.y = 0;
        assert.equal(vtile.y, 0);
        extent = vtile.extent();
        var expected_y = [-20037508.342789244, 19959236.82582522, -19959236.82582522, 20037508.342789244];
        assert(Math.abs(extent[0] - expected_y[0]) < 1e-8);
        assert(Math.abs(extent[1] - expected_y[1]) < 1e-8);
        assert(Math.abs(extent[2] - expected_y[2]) < 1e-8);
        assert(Math.abs(extent[3] - expected_y[3]) < 1e-8);
        vtile.z = 0;
        assert.equal(vtile.z, 0);
        extent = vtile.extent();
        var expected_z = [-20037508.342789244, -20037508.342789244, 20037508.342789244, 20037508.342789244];
        assert(Math.abs(extent[0] - expected_z[0]) < 1e-8);
        assert(Math.abs(extent[1] - expected_z[1]) < 1e-8);
        assert(Math.abs(extent[2] - expected_z[2]) < 1e-8);
        assert(Math.abs(extent[3] - expected_z[3]) < 1e-8);
        done();
    });

    it('should be able to getData with a RLE', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        var uncompressed = vtile.getData();
        var gzip = zlib.createGzip({
            strategy: zlib.Z_RLE
        }), buffers=[], nread=0;

        gzip.on('error', function(err) {
            gzip.removeAllListeners();
            gzip=null;
        });

        gzip.on('data', function(chunk) {
            buffers.push(chunk);
            nread += chunk.length;
        });

        gzip.on('end', function() {
            var buffer;
            switch (buffers.length) {
                case 0: // no data.  return empty buffer
                    buffer = new Buffer(0);
                    break;
                case 1: // only one chunk of data.  return it.
                    buffer = buffers[0];
                    break;
                default: // concatenate the chunks of data into a single buffer.
                    buffer = new Buffer(nread);
                    var n = 0;
                    buffers.forEach(function(b) {
                        var l = b.length;
                        b.copy(buffer, n, 0, l);
                        n += l;
                    });
                    break;
            }

            gzip.removeAllListeners();
            gzip=null;
            var compressed = vtile.getData({compression:'gzip', strategy:'RLE'});
            // Substring used to remove gzip header
            assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
            vtile.getData({compression:'gzip', strategy:'RLE'}, function (err, compressed) {
                // Substring used to remove gzip header
                assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
                done();
            });
        });

        gzip.write(uncompressed);
        gzip.end();
    });

    it('should be able to getData with a FILTERED', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        var uncompressed = vtile.getData();
        var gzip = zlib.createGzip({
            strategy: zlib.Z_FILTERED
        }), buffers=[], nread=0;

        gzip.on('error', function(err) {
            gzip.removeAllListeners();
            gzip=null;
        });

        gzip.on('data', function(chunk) {
            buffers.push(chunk);
            nread += chunk.length;
        });

        gzip.on('end', function() {
            var buffer;
            switch (buffers.length) {
                case 0: // no data.  return empty buffer
                    buffer = new Buffer(0);
                    break;
                case 1: // only one chunk of data.  return it.
                    buffer = buffers[0];
                    break;
                default: // concatenate the chunks of data into a single buffer.
                    buffer = new Buffer(nread);
                    var n = 0;
                    buffers.forEach(function(b) {
                        var l = b.length;
                        b.copy(buffer, n, 0, l);
                        n += l;
                    });
                    break;
            }

            gzip.removeAllListeners();
            gzip=null;
            var compressed = vtile.getData({compression:'gzip', strategy:'FILTERED'});
            // Substring used to remove gzip header
            assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
            vtile.getData({compression:'gzip', strategy:'FILTERED'}, function (err, compressed) {
                // Substring used to remove gzip header
                assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
                done();
            });
        });

        gzip.write(uncompressed);
        gzip.end();
    });

    it('should be able to getData with a HUFFMAN_ONLY', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        var uncompressed = vtile.getData();
        var gzip = zlib.createGzip({
            strategy: zlib.Z_HUFFMAN_ONLY
        }), buffers=[], nread=0;

        gzip.on('error', function(err) {
            gzip.removeAllListeners();
            gzip=null;
        });

        gzip.on('data', function(chunk) {
            buffers.push(chunk);
            nread += chunk.length;
        });

        gzip.on('end', function() {
            var buffer;
            switch (buffers.length) {
                case 0: // no data.  return empty buffer
                    buffer = new Buffer(0);
                    break;
                case 1: // only one chunk of data.  return it.
                    buffer = buffers[0];
                    break;
                default: // concatenate the chunks of data into a single buffer.
                    buffer = new Buffer(nread);
                    var n = 0;
                    buffers.forEach(function(b) {
                        var l = b.length;
                        b.copy(buffer, n, 0, l);
                        n += l;
                    });
                    break;
            }

            gzip.removeAllListeners();
            gzip=null;
            var compressed = vtile.getData({compression:'gzip', strategy:'HUFFMAN_ONLY'});
            // Substring used to remove gzip header
            assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
            vtile.getData({compression:'gzip', strategy:'HUFFMAN_ONLY'}, function (err, compressed) {
                // Substring used to remove gzip header
                assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
                done();
            });
        });

        gzip.write(uncompressed);
        gzip.end();
    });

    it('should be able to getData with a FIXED', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        var uncompressed = vtile.getData();
        var gzip = zlib.createGzip({
            strategy: zlib.Z_FIXED
        }), buffers=[], nread=0;

        gzip.on('error', function(err) {
            gzip.removeAllListeners();
            gzip=null;
        });

        gzip.on('data', function(chunk) {
            buffers.push(chunk);
            nread += chunk.length;
        });

        gzip.on('end', function() {
            var buffer;
            switch (buffers.length) {
                case 0: // no data.  return empty buffer
                    buffer = new Buffer(0);
                    break;
                case 1: // only one chunk of data.  return it.
                    buffer = buffers[0];
                    break;
                default: // concatenate the chunks of data into a single buffer.
                    buffer = new Buffer(nread);
                    var n = 0;
                    buffers.forEach(function(b) {
                        var l = b.length;
                        b.copy(buffer, n, 0, l);
                        n += l;
                    });
                    break;
            }

            gzip.removeAllListeners();
            gzip=null;
            var compressed = vtile.getData({compression:'gzip', strategy:'FIXED'});
            // Substring used to remove gzip header
            assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
            vtile.getData({compression:'gzip', strategy:'FIXED'}, function (err, compressed) {
                // Substring is used to remove gzip header
                assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
                done();
            });
        });

        gzip.write(uncompressed);
        gzip.end();
    });

    it('should be able to getData with a DEFAULT_STRATEGY', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        var uncompressed = vtile.getData();
        var gzip = zlib.createGzip({
            strategy: zlib.Z_DEFAULT_STRATEGY
        }), buffers=[], nread=0;

        gzip.on('error', function(err) {
            gzip.removeAllListeners();
            gzip=null;
        });

        gzip.on('data', function(chunk) {
            buffers.push(chunk);
            nread += chunk.length;
        });

        gzip.on('end', function() {
            var buffer;
            switch (buffers.length) {
                case 0: // no data.  return empty buffer
                    buffer = new Buffer(0);
                    break;
                case 1: // only one chunk of data.  return it.
                    buffer = buffers[0];
                    break;
                default: // concatenate the chunks of data into a single buffer.
                    buffer = new Buffer(nread);
                    var n = 0;
                    buffers.forEach(function(b) {
                        var l = b.length;
                        b.copy(buffer, n, 0, l);
                        n += l;
                    });
                    break;
            }

            gzip.removeAllListeners();
            gzip=null;
            var compressed = vtile.getData({compression:'gzip', strategy:'DEFAULT'});
            // Substring used to remove gzip header
            assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
            vtile.getData({compression:'gzip', strategy:'DEFAULT'}, function (err, compressed) {
                // Substring used to remove gzip header
                assert.equal(buffer.toString('hex').substring(20), compressed.toString('hex').substring(20));
                done();
            });
        });

        gzip.write(uncompressed);
        gzip.end();
    });

    it('should be able to getData with release', function(done) {
        var vtile1 = new mapnik.VectorTile(9,112,195);
        var vtile2 = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile1.setData(data);
        vtile2.setData(data);
        assert.equal(vtile1.empty(), false);
        assert.equal(data, vtile1.getData().toString());
        var data1 = vtile1.getData({release:true}).toString();
        assert.equal(data, data1);
        assert.equal(vtile1.empty(), true);
        assert.equal(vtile2.empty(), false);
        vtile2.getData({release:true}, function(err, buffer) {
            if (err) throw err;
            assert.equal(data, buffer.toString());
            assert.equal(vtile2.empty(), true);
            done();
        });
    });

    it('should be able to getData with release and compression', function(done) {
        var vtile1 = new mapnik.VectorTile(9,112,195);
        var vtile2 = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile1.setData(data);
        vtile2.setData(data);
        assert.equal(vtile1.empty(), false);
        var data_c = vtile1.getData({compression:'gzip'}).toString();
        var data1 = vtile1.getData({release:true, compression:'gzip'}).toString();
        assert.equal(data_c, data1);
        assert.equal(vtile1.empty(), true);
        assert.equal(vtile2.empty(), false);
        vtile2.getData({release:true, compression:'gzip'}, function(err, buffer) {
            if (err) throw err;
            assert.equal(data_c, buffer.toString());
            assert.equal(vtile2.empty(), true);
            done();
        });
    });

    it('should be able to setData/parse gzip compressed (sync)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf.gz");
        vtile.setData(data);
        vtile.getData({compression:'gzip',level:9,strategy:'RLE'},function(err,compressed) {
            if (err) throw err;
            assert.equal(compressed[0],0x1F);
            assert.equal(compressed[1],0x8B);
            assert.equal(compressed[2],8);
            assert.equal(compressed.length,vtile.getData({compression:'gzip',level:9,strategy:'RLE'}).length);
            var uncompressed = vtile.getData();
            var default_compressed = vtile.getData({compression:'gzip'});
            // ensure all the default options match the default node zlib options
            zlib.gzip(uncompressed,function(err,node_compressed) {
                assert.equal(default_compressed.length,node_compressed.length);
            });
            var vtile2 = new mapnik.VectorTile(9,112,195);
            vtile2.setData(compressed);
            assert.equal(vtile.getData().length,vtile2.getData().length);
            assert.equal(vtile.empty(), false);
            assert.equal(vtile2.empty(), false);
            assert.equal(vtile.painted(), true);
            assert.equal(vtile.empty(), false);
            assert.equal(vtile2.painted(), true);
            assert.equal(vtile2.empty(), false);
            done();
        });
    });

    it('setData should error on bogus gzip data', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var fake_gzip = new Buffer(30);
        fake_gzip.fill(0);
        fake_gzip[0] = 0x1F;
        fake_gzip[1] = 0x8B;
        assert.throws(function() { vtile.setData(fake_gzip); });
        vtile.setData(fake_gzip,function(err) {
            assert.ok(err);
            done();
        });
    });

    it('setData should error on bogus zlib data', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var fake_zlib = new Buffer(30);
        fake_zlib.fill(0);
        fake_zlib[0] = 0x78;
        fake_zlib[1] = 0x9C;
        assert.throws(function() { vtile.setData(fake_zlib); });
        vtile.setData(fake_zlib,function(err) {
            assert.ok(err);
            done();
        });
    });

    it('info should throw with invalid use', function() {
        assert.throws(function() { mapnik.VetorTile.info(); });
        assert.throws(function() { mapnik.VetorTile.info(null); });
        assert.throws(function() { mapnik.VetorTile.info({}); });
    });

    it('info should show error on bogus gzip data', function() {
        var fake_gzip = new Buffer(30);
        fake_gzip.fill(0);
        fake_gzip[0] = 0x1F;
        fake_gzip[1] = 0x8B;
        var out = mapnik.VectorTile.info(fake_gzip);
        assert(out.errors);
        assert.equal(out.layers.length, 0);
        assert.equal(out.tile_errors.length, 1);
        assert.equal(out.tile_errors[0], 'Buffer is not encoded as a valid PBF');
    });

    it('info should show error on bogus zlib data', function() {
        var fake_zlib = new Buffer(30);
        fake_zlib.fill(0);
        fake_zlib[0] = 0x78;
        fake_zlib[1] = 0x9C;
        var out = mapnik.VectorTile.info(fake_zlib);
        assert(out.errors);
        assert.equal(out.layers.length, 0);
        assert.equal(out.tile_errors.length, 1);
        assert.equal(out.tile_errors[0], 'Buffer is not encoded as a valid PBF');
    });

    it('info should show problems with invalid v2 tile', function() {
        var badTile = fs.readFileSync(path.resolve(__dirname + '/data/vector_tile/invalid_v2_tile.mvt'));
        var out = mapnik.VectorTile.info(badTile);
        assert(out.errors);
        assert.equal(out.layers.length, 23);
        assert.equal(out.tile_errors, undefined);
        assert.equal(out.layers[0].name, 'landuse');
        assert.equal(out.layers[0].features, 140);
        assert.equal(out.layers[0].point_features, 0);
        assert.equal(out.layers[0].linestring_features, 2);
        assert.equal(out.layers[0].polygon_features, 138);
        assert.equal(out.layers[0].unknown_features, 0);
        assert.equal(out.layers[0].raster_features, 0);
        assert.equal(out.layers[0].version, 2);
        assert.equal(out.layers[0].errors, undefined);
        assert.equal(out.layers[4].name, 'barrier_line');
        assert.equal(out.layers[4].features, 0);
        assert.equal(out.layers[4].point_features, 0);
        assert.equal(out.layers[4].linestring_features, 0);
        assert.equal(out.layers[4].polygon_features, 0);
        assert.equal(out.layers[4].unknown_features, 0);
        assert.equal(out.layers[4].raster_features, 0);
        assert.equal(out.layers[4].version, 2);
        assert.equal(out.layers[4].errors.length, 1);
        assert.equal(out.layers[4].errors[0], 'Vector Tile Layer message has no features');
    });

    it('should not have errors if we pass a valid vector tile to info', function() {
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf.gz");
        var out = mapnik.VectorTile.info(data);
        assert.equal(out.errors, false);
        assert.equal(out.layers.length, 2);
    });

    it('should error out if we pass invalid data to setData - 1', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        assert.throws(function() { vtile.setData('foo'); }); // first arg must be a buffer object
        assert.throws(function() { vtile.setDataSync({}); }); // first arg must be a buffer object
        assert.throws(function() { vtile.setData('foo',function(){}); }); // first arg must be a buffer object
        assert.throws(function() { vtile.setData({},function(){}); }); // first arg must be a buffer object
        assert.throws(function() { vtile.setData({},function(){}); }); // first arg must be a buffer object
        assert.throws(function() { vtile.setData(new Buffer('foo'), null); });
        assert.throws(function() { vtile.setData(new Buffer(0)); }); // empty buffer is not valid
        vtile.setData(new Buffer('foo'),function(err) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should error out if we validate tile with empty layers and features to setData - 1', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        var tile_with_emptyness = fs.readFileSync(path.resolve(__dirname + '/data/vector_tile/invalid_v2_tile.mvt'));
        assert.throws(function() { vtile.setData(tile_with_emptyness,{validate:true}); });
        // tile will be partially parsed (when not upgrading) before validation error is throw
        // so we check that not all the layers are added
        assert.equal(vtile.names().length,3);
        // only fails with validation
        var vtile3 = new mapnik.VectorTile(0,0,0);
        vtile3.setData(tile_with_emptyness,{validate:false});
        assert.equal(vtile3.names().length,23);
        done();
    });

    it('should error out if we pass invalid data to setData - 2', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        vtile.setData(new Buffer('0'),function(err) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should error out if we pass invalid data to setData - 3', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        vtile.setData(new Buffer('0B1234', 'hex'),function(err) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should error out if we pass invalid data to setData - 4', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        // fails due to invalid tag
        assert.throws(function() { vtile.setData(new Buffer('120774657374696e67', 'hex')); });
        assert.equal(vtile.empty(), true);
        // fails due to invalid tag
        assert.throws(function() { vtile.setData(new Buffer('089601', 'hex')); });
        assert.equal(vtile.empty(), true);
        assert.throws(function() {
            vtile.setData(new Buffer('0896818181818181818181818181818181818181', 'hex'));
        });
        assert.throws(function() {
            vtile.setData(new Buffer('0896818181818181818181818181818181818181', 'hex'));
        });
        assert.throws(function() {
            vtile.setData(new Buffer('090123456789012345', 'hex'));
        });
        assert.throws(function() {
            vtile.setData(new Buffer('0D01234567', 'hex'));
        });
        assert.throws(function() {
            vtile.setData(new Buffer('0D0123456', 'hex')); // 32 should fail because missing a byte
        });
        assert.throws(function() {
            vtile.setData(new Buffer('0D0123456', 'hex')); // 32 should fail because missing a byte
        });
    });

    it('should be empty if we pass empty buffer to setData', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        assert.equal(vtile.empty(), true);
        vtile.setData(new Buffer(0),function(err) {
            assert.throws(function() { if (err) throw err; });
            assert.deepEqual(vtile.names(), []);
            assert.equal(vtile.empty(), true);
            done();
        });
    });

    it('should not return empty and will have layer name', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.setData(new Buffer('1A0C0A0A6C617965722D6E616D65', 'hex'));
        assert.deepEqual(vtile.names(), ['layer-name']);
        assert.equal(vtile.empty(), false);
    });

    it('should return empty and have no layer name when upgraded', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.setData(new Buffer('1A0C0A0A6C617965722D6E616D65', 'hex'),{upgrade:true});
        // a layer with only a name "layer-name" and no features
        // during v1 to v2 conversion this will be dropped
        // note: this tile also lacks a version, but mapnik-vt defaults to assuming v1
        assert.deepEqual(vtile.names(), []);
        assert.equal(vtile.empty(), true);
    });

    it('should error out if we pass invalid data to addData', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        assert.throws(function() { vtile.addData(null); }); // empty buffer is not valid
        assert.throws(function() { vtile.addData({}); }); // empty buffer is not valid
        assert.throws(function() { vtile.addData(new Buffer(0)); }); // empty buffer is not valid
        assert.throws(function() { vtile.addData(new Buffer('foo')); });
        assert.throws(function() { vtile.addData(new Buffer(0), 'not a function'); }); // last item must be a function
        assert.throws(function() { vtile.addDataSync(null); }); // empty buffer is not valid
        assert.throws(function() { vtile.addDataSync({}); }); // empty buffer is not valid
        assert.throws(function() { vtile.addDataSync(new Buffer(0)); }); // empty buffer is not valid
        assert.throws(function() { vtile.addDataSync(new Buffer('foo')); });
        assert.throws(function() { vtile.addData(null, function(err) {}); });
        assert.throws(function() { vtile.addData({}, function(err) {}); });

        vtile.addData(new Buffer(0), function(err) {
            assert.throws(function() { if (err) throw err; });
            vtile.addData(new Buffer('foo'), function(err) {
                assert.throws(function() { if (err) throw err; });
                done();
            });
        });
    });

    it('should error out if we pass invalid data to setData', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        assert.throws(function() { vtile.setData(null); }); // empty buffer is not valid
        assert.throws(function() { vtile.setData({}); }); // empty buffer is not valid
        assert.throws(function() { vtile.setData(new Buffer(0)); }); // empty buffer is not valid
        assert.throws(function() { vtile.setData(new Buffer('foo')); });
        assert.throws(function() { vtile.setDataSync(null); }); // empty buffer is not valid
        assert.throws(function() { vtile.setDataSync({}); }); // empty buffer is not valid
        assert.throws(function() { vtile.setDataSync(new Buffer(0)); }); // empty buffer is not valid
        assert.throws(function() { vtile.setDataSync(new Buffer('foo')); });
        assert.throws(function() { vtile.setData(null, function(err) {}); });
        assert.throws(function() { vtile.setData({}, function(err) {}); });
        vtile.setData(new Buffer(0), function(err) {
            assert.throws(function() { if (err) throw err; });
            vtile.setData(new Buffer('foo'), function(err) {
                assert.throws(function() { if (err) throw err; });
                done();
            });
        });
    });

    it('should fail to do clear', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.throws(function() { vtile.clear(null); });
    });

    it('should fail to addData/setData due to bad data', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.throws(function() { vtile.addData(new Buffer('foo')); });
        assert.throws(function() { vtile.setData(new Buffer('foo')); });
    });

    it('should be able to addData sync', function() {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.equal(vtile.empty(), true);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.addDataSync(data);
        assert.deepEqual(vtile.names(), ["world", "world2"]);
        assert.equal(vtile.empty(), false);
        assert.equal(vtile.painted(), true);
    });

    it('should be able to setData sync', function() {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.equal(vtile.empty(), true);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setDataSync(data);
        assert.deepEqual(vtile.names(), ["world", "world2"]);
        assert.equal(vtile.empty(), false);
        assert.equal(vtile.painted(), true);
    });

    it('should be able to addData (async)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.equal(vtile.empty(), true);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.addData(data, function(err) {
            if (err) throw err;
            assert.deepEqual(vtile.names(), ["world", "world2"]);
            assert.equal(vtile.empty(), false);
            assert.equal(vtile.painted(), true);
            done();
        });
    });

    it('should be able to setData (async)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.equal(vtile.empty(), true);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data, function(err) {
            if (err) throw err;
            assert.deepEqual(vtile.names(), ["world", "world2"]);
            assert.equal(vtile.empty(), false);
            assert.equal(vtile.painted(), true);
            done();
        });
    });

    it('should be able to extract one layer', function() {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.equal(vtile.empty(), true);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        assert.deepEqual(vtile.names(), ["world", "world2"]);
        var vtile2 = vtile.layer('world');
        assert.deepEqual(vtile2.names(), ["world"]);
        var first = vtile.toGeoJSON('world');
        var second = vtile2.toGeoJSON('world');
        assert.equal(first, second);
    });

    it('should fail to extract one layer', function() {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.equal(vtile.empty(), true);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        assert.deepEqual(vtile.names(), ["world", "world2"]);
        assert.throws(function() { var vtile2 = vtile.layer(); });
        assert.throws(function() { var vtile2 = vtile.layer({}); });
        assert.throws(function() { var vtile2 = vtile.layer('fish'); });
    });

    it('should be able to addData gzip compressed (async)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.equal(vtile.empty(), true);
        assert.equal(vtile.painted(), false);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf.gz");
        vtile.addData(data, function(err) {
            if (err) throw err;
            assert.deepEqual(vtile.names(), ["world", "world2"]);
            assert.equal(vtile.empty(), false);
            assert.equal(vtile.painted(), true);
            done();
        });
    });

    it('should be able to setData gzip compressed (async)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.equal(vtile.empty(), true);
        assert.equal(vtile.painted(), false);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf.gz");
        vtile.setData(data, function(err) {
            if (err) throw err;
            assert.deepEqual(vtile.names(), ["world", "world2"]);
            assert.equal(vtile.empty(), false);
            assert.equal(vtile.painted(), true);
            done();
        });
    });

    it('should be able to get layer names without parsing', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data, function(err) {
            if (err) throw err;
            assert.deepEqual(vtile.names(), ["world","world2"]);
            assert.equal(vtile.empty(), false);
            done();
        });
    });


    it('should be able to get tile info as JSON', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        assert.deepEqual(vtile.names(),['world','world2']);
        var expected = [
            {
                "name":"world",
                "extent":4096,
                "version":2,
                "features":[
                    {
                        "id":207,
                        "type":3,
                        "geometry":[9,8448,255,26,0,8704,8703,0,0,8703,15],
                        "properties":{
                            "AREA":915896,
                            "FIPS":"US",
                            "ISO2":"US",
                            "ISO3":"USA",
                            "LAT":39.622,
                            "LON":-98.606,
                            "NAME":"United States",
                            "POP2005":299846449,
                            "REGION":19,
                            "SUBREGION":21,
                            "UN":840
                        }
                    }
                ]
            },
            {
                "name":"world2",
                "extent":4096,
                "version":2,
                "features":[
                    {
                        "id":207,
                        "type":3,
                        "geometry":[9,8448,255,26,0,8704,8703,0,0,8703,15],
                        "properties":{
                            "AREA":915896,
                            "FIPS":"US",
                            "ISO2":"US",
                            "ISO3":"USA",
                            "LAT":39.622,
                            "LON":-98.606,
                            "NAME":"United States",
                            "POP2005":299846449,
                            "REGION":19,
                            "SUBREGION":21,
                            "UN":840
                        }
                    }
                ]
            }
        ];
        assert.deepEqual(vtile.toJSON(),expected);
        done();
    });

    it('should be able to get tile info as JSON with decoded geometry', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        assert.deepEqual(vtile.names(),['world','world2']);
        var expected = [
            {
                "name":"world",
                "extent":4096,
                "version":2,
                "features":[
                    {
                        "id":207,
                        "type":3,
                        "geometry_type":"Polygon",
                        "geometry":[
                            [[4224,-128],[4224,4224],[-128,4224],[-128,-128],[4224,-128]]
                        ],
                        "properties":{
                            "AREA":915896,
                            "FIPS":"US",
                            "ISO2":"US",
                            "ISO3":"USA",
                            "LAT":39.622,
                            "LON":-98.606,
                            "NAME":"United States",
                            "POP2005":299846449,
                            "REGION":19,
                            "SUBREGION":21,
                            "UN":840
                        }
                    }
                ]
            },
            {
                "name":"world2",
                "extent":4096,
                "version":2,
                "features":[
                    {
                        "id":207,
                        "type":3,
                        "geometry_type":"Polygon",
                        "geometry":[
                            [[4224,-128],[4224,4224],[-128,4224],[-128,-128],[4224,-128]]
                        ],
                        "properties":{
                            "AREA":915896,
                            "FIPS":"US",
                            "ISO2":"US",
                            "ISO3":"USA",
                            "LAT":39.622,
                            "LON":-98.606,
                            "NAME":"United States",
                            "POP2005":299846449,
                            "REGION":19,
                            "SUBREGION":21,
                            "UN":840
                        }
                    }
                ]
            }
        ];
        assert.deepEqual(vtile.toJSON({decode_geometry:true}),expected);
        done();
    });

    it('should be able to get tile info as various flavors of GeoJSON', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        var expected_geojson = {
            "type":"FeatureCollection",
            "features":[
                {
                    "type":"Feature",
                    "geometry": {
                        "type":"Polygon",
                        "coordinates":[
                            [
                                [ -100.52490234375, 39.3852638109977 ],
                                [ -101.27197265625, 39.3852638109977 ],
                                [ -101.27197265625, 38.8054702231775 ],
                                [ -100.52490234375, 38.8054702231775 ],
                                [ -100.52490234375, 39.3852638109977 ],
                            ]
                        ]
                    },
                    "properties": {
                        "AREA":915896,
                        "FIPS":"US",
                        "ISO2":"US",
                        "ISO3":"USA",
                        "LAT":39.622,
                        "LON":-98.606,
                        "NAME":"United States",
                        "POP2005":299846449,
                        "REGION":19,
                        "SUBREGION":21,
                        "UN":840
                    }
                }
            ]
        };
        var expected_copy = JSON.parse(JSON.stringify(expected_geojson));
        expected_geojson.name = "world";
        var actual = JSON.parse(vtile.toGeoJSON(0));
        assert.equal(actual.type,expected_geojson.type);
        assert.equal(actual.name,expected_geojson.name);
        assert.equal(actual.features.length,1);
        assert.equal(actual.features[0].properties.length,expected_geojson.features[0].properties.length);
        assert.equal(actual.features[0].properties.NAME,expected_geojson.features[0].properties.NAME);
        deepEqualTrunc(actual.features[0].geometry,expected_geojson.features[0].geometry);
        deepEqualTrunc(JSON.parse(vtile.toGeoJSON(0)),JSON.parse(vtile.toGeoJSON('world')));
        actual = JSON.parse(vtile.toGeoJSON('__all__'));
        assert.equal(actual.type,expected_copy.type);
        assert.equal(actual.name,expected_copy.name);
        assert.equal(actual.features.length,2);
        assert.equal(actual.features[0].properties.length,expected_copy.features[0].properties.length);
        assert.equal(actual.features[1].properties.length,expected_copy.features[0].properties.length);
        assert.equal(actual.features[0].properties.NAME,expected_copy.features[0].properties.NAME);
        assert.equal(actual.features[1].properties.NAME,expected_copy.features[0].properties.NAME);
        deepEqualTrunc(actual.features[0].geometry,expected_copy.features[0].geometry);
        deepEqualTrunc(actual.features[1].geometry,expected_copy.features[0].geometry);
        var json_array = JSON.parse(vtile.toGeoJSON('__array__'));
        assert.equal(json_array.length,2);
        actual = json_array[0];
        assert.equal(actual.type,expected_geojson.type);
        assert.equal(actual.name,expected_geojson.name);
        assert.equal(actual.features.length,expected_geojson.features.length);
        assert.equal(actual.features[0].properties.length,expected_geojson.features[0].properties.length);
        assert.equal(actual.features[0].properties.NAME,expected_geojson.features[0].properties.NAME);
        deepEqualTrunc(actual.features[0].geometry,expected_geojson.features[0].geometry);

        //
        done();
    });

    it('should fail to parse toJSON due to bad input', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.throws(function() { vtile.toJSON(null) });
        assert.throws(function() { vtile.toJSON({decode_geometry:null}) });
    });

    it('should be able to get and set data (sync)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        var vtile2 = new mapnik.VectorTile(9,112,195);
        vtile2.setData(vtile.getData());
        assert.deepEqual(vtile.names(),vtile2.names());
        assert.deepEqual(vtile.toJSON(),vtile2.toJSON());
        assert.deepEqual(vtile2.toJSON(),_vtile.toJSON());
        done();
    });

    it('should be able to get and set data (async)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"),function(err) {
            if (err) throw err;
            var vtile2 = new mapnik.VectorTile(9,112,195);
            vtile.getData(function(err,data) {
                if (err) throw err;
                vtile2.setData(data);
                assert.deepEqual(vtile.names(),vtile2.names());
                assert.deepEqual(vtile.toJSON(),vtile2.toJSON());
                assert.deepEqual(vtile2.toJSON(),_vtile.toJSON());
                done();
            });
        });
    });

    it('should be able to get virtual datasource and features', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        assert.equal(vtile.getData().length,_length);
        var ds = vtile.toJSON();
        assert.equal(ds.length,2);
        assert.equal(ds[0].name,"world");
        assert.equal(ds[0].extent,4096);
        assert.equal(ds[0].version,2);
        assert.equal(ds[0].features.length,1);
        assert.equal(ds[0].features[0].id,207);
        assert.equal(ds[0].features[0].type,3);
        var expected = { AREA: 915896,
                          FIPS: 'US',
                          ISO2: 'US',
                          ISO3: 'USA',
                          LAT: 39.622,
                          LON: -98.606,
                          NAME: 'United States',
                          POP2005: 299846449,
                          REGION: 19,
                          SUBREGION: 21,
                          UN: 840 };
        assert.deepEqual(ds[0].features[0].properties,expected);
        assert.equal(ds[1].name,"world2");
        assert.equal(ds[1].extent,4096);
        assert.equal(ds[1].version,2);
        assert.equal(ds[1].features.length,1);
        assert.equal(ds[1].features[0].id,207);
        assert.equal(ds[1].features[0].type,3);
        var expected = { AREA: 915896,
                          FIPS: 'US',
                          ISO2: 'US',
                          ISO3: 'USA',
                          LAT: 39.622,
                          LON: -98.606,
                          NAME: 'United States',
                          POP2005: 299846449,
                          REGION: 19,
                          SUBREGION: 21,
                          UN: 840 };
        assert.deepEqual(ds[1].features[0].properties,expected);
        done();
    });

    it('should be able to clear data (sync)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        assert.equal(vtile.getData().length,_length);
        assert.equal(vtile.painted(), true);
        var feature_count = vtile.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        vtile.clear();
        assert.equal(vtile.getData().length,0);
        // Node 12 returns []
//         assert.deepEqual(vtile2.toJSON(), {});
        assert.equal(vtile.painted(), false);

        // call synchronous method directly
        var vtile2 = new mapnik.VectorTile(9,112,195);
        vtile2.setData(new Buffer(_data,"hex"));
        assert.equal(vtile2.getData().length,_length);
        assert.equal(vtile2.painted(), true);
        var feature_count2 = vtile2.toJSON()[0].features.length;
        assert.equal(feature_count2, 1);
        vtile2.clearSync();
        assert.equal(vtile2.getData().length,0);
        // Node 12 returns []
//         assert.deepEqual(vtile2.toJSON(), {});
        assert.equal(vtile2.painted(), false);
        done();
    });

    it('should be able to clear data (async)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(), ["world","world2"]);
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(), ["world","world2"]);
        assert.equal(vtile.getData().length,_length);
        assert.equal(vtile.painted(), true);
        var feature_count = vtile.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        vtile.clear(function(err) {
            if (err) throw err;
            assert.equal(vtile.empty(), true);
            assert.deepEqual(vtile.names(), []);
            assert.equal(vtile.getData().length,0);
            // Node 12 returns []
//             assert.deepEqual(vtile.toJSON(), {});
            assert.equal(vtile.painted(), false);
            done();
        });
    });

    it('should be able to add data', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        assert.equal(vtile.empty(), false);
        assert.equal(vtile.painted(), true);
        assert.deepEqual(vtile.names(), ["world","world2"]);
        // Setting data again should still result in the same data.
        vtile.setData(new Buffer(_data,"hex"));
        assert.equal(vtile.empty(), false);
        assert.equal(vtile.painted(), true);
        assert.deepEqual(vtile.names(), ["world","world2"]);
        // Adding same data again should not result in new data.
        vtile.addData(new Buffer(_data,"hex"));
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(), ["world","world2"]);
        assert.equal(vtile.getData().length,_length);
        assert.equal(vtile.painted(), true);
        var feature_count = vtile.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        vtile.clear(function(err) {
            if (err) throw err;
            assert.equal(vtile.empty(), true);
            assert.deepEqual(vtile.names(), []);
            assert.equal(vtile.getData().length,0);
        // Node 12 returns []
//         assert.deepEqual(vtile2.toJSON(), {});
            assert.equal(vtile.painted(), false);
            done();
        });
    });


    it('should detect as solid a tile with two "box" layers', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/layers.xml');
        //map.extent = [-11271098.442818949,4696291.017841229,-11192826.925854929,4774562.534805249];
        map.render(vtile,{},function(err,vtile) {
            // this tile duplicates the layer that is present in tile1.vector.pbf
            fs.writeFileSync('./test/data/vector_tile/tile2.mvt',vtile.getData());
            _data = vtile.getData().toString("hex");
            var vtile2 = new mapnik.VectorTile(9,112,195);
            var raw_data = fs.readFileSync("./test/data/vector_tile/tile2.mvt");
            vtile2.setData(raw_data);
            assert.equal(vtile.empty(), false);
            assert.equal(vtile.painted(), true);
            assert.deepEqual(vtile.paintedLayers(), ['world','world2']);
            assert.equal(vtile.getData().toString("hex"),raw_data.toString("hex"));
            done();
        });
    });

    it('should render an empty vector', function(done) {
        var vtile = new mapnik.VectorTile(9,9,9);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/layers.xml');
        map.render(vtile,{},function(err,vtile) {
            assert.deepEqual(vtile.emptyLayers(), ['world', 'world2']);
            assert.equal(vtile.empty(), true);
            done();
        });
    });

    it('should fail to render due to bad arguments passed', function(done) {
        var data = fs.readFileSync("./test/data/vector_tile/tile3.mvt");
        var vtile = new mapnik.VectorTile(5,28,12);
        vtile.setData(data);
        var im = new mapnik.Image(256,256);
        var im_g = new mapnik.Image(256,256,{ type: mapnik.imageType.gray8 });
        var im_c = new mapnik.CairoSurface('SVG',256, 256);
        var map = new mapnik.Map(vtile.tileSize,vtile.tileSize);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        assert.throws(function() { vtile.render(); });
        assert.throws(function() { vtile.render({}); });
        assert.throws(function() { vtile.render(map); });
        assert.throws(function() { vtile.render(map, {}); });
        assert.throws(function() { vtile.render(map, im); });
        assert.throws(function() { vtile.render(map, im, null, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, {}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:null, y:0, z:0}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:0, y:null, z:0}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:0, y:0, z:null}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {y:0, z:0}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:0, z:0}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:0, y:0}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:-1, y:0, z:0}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:0, y:-1, z:0}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:0, y:0, z:-1}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:1, y:0, z:0}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:0, y:1, z:0}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:0, y:2, z:1}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {x:2, y:0, z:1}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {buffer_size:null}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {scale:null}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {scale_denominator:null}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im, {variables:null}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im_c, {renderer:null}, function(e,i) {}); });
        assert.throws(function() { vtile.render(map, im_c, {renderer:'foo'}, function(e,i) {}); });
        if (mapnik.supports.grid)
        {
            var grid = new mapnik.Grid(256, 256);
            var map2 = new mapnik.Map(vtile.tileSize,vtile.tileSize);
            assert.throws(function() { vtile.render(map, grid, {}, function(e,i) {}); });
            assert.throws(function() { vtile.render(map, grid, function(e,i) {}); });
            assert.throws(function() { vtile.render(map, grid, {layer:null}, function(e,i) {}); });
            assert.throws(function() { vtile.render(map, grid, {layer:1}, function(e,i) {}); });
            assert.throws(function() { vtile.render(map2, grid, {layer:0}, function(e,i) {}); });
            assert.throws(function() { vtile.render(map, grid, {layer:'world-false'}, function(e,i) {}); });
            assert.throws(function() { vtile.render(map, grid, {layer:'world', fields:null}, function(e,i) {}); });
        }
        assert.throws(function() { vtile.render(function(e,i) {}); });
        vtile.render(map, im_g, function(err,output) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should render expected results', function(done) {
        var data = fs.readFileSync("./test/data/vector_tile/tile3.mvt");
        var vtile = new mapnik.VectorTile(5,28,12);
        vtile.setData(data);
        assert.equal(vtile.getData().length,987);
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.empty(), false);
        var map = new mapnik.Map(vtile.tileSize,vtile.tileSize);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        vtile.render(map, new mapnik.Image(256,256), function(err,image) {
            if (err) throw err;
            var expected = './test/data/vector_tile/tile3.expected.png';
            var actual = './test/data/vector_tile/tile3.actual.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                image.save(expected, 'png32');
            }
            image.save(actual, 'png32');
            var e = fs.readFileSync(expected);
            var a = fs.readFileSync(actual);
            if (mapnik.versions.mapnik_number >= 300000) {
                assert.ok(Math.abs(e.length - a.length) < 100);
            } else {
                assert.equal(e.length,a.length);
            }
            done();
        });
    });

    it('should render an image with a large amount of overzooming', function(done) {
        var data = fs.readFileSync("./test/data/images/14_2788_6533.webp");
        var vtile = new mapnik.VectorTile(14,2788,6533);
        vtile.addImageBufferSync(data, '_image');
        var map = new mapnik.Map(256,256);
        map.loadSync('./test/data/large_overzoom.xml');
        var opts = { z: 24, x: 2855279, y: 6690105, scale: 1, buffer_size: 256 };
        var img = new mapnik.Image(256,256);
        vtile.render(map, img, opts, function(err, image) {
            if (err) throw err;
            var expected = './test/data/images/large_overzoom.expected.png';
            var actual = './test/data/images/large_overzoom.actual.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                image.save(expected, 'png32');
            }
            image.save(actual, 'png32');
            var e = fs.readFileSync(expected);
            var a = fs.readFileSync(actual);
            if (mapnik.versions.mapnik_number >= 300000) {
                assert.ok(Math.abs(e.length - a.length) < 100);
            } else {
                assert.equal(e.length,a.length);
            }

            done();
        });

    });

    it('should render expected results - with objectional arguments', function(done) {
        var data = fs.readFileSync("./test/data/vector_tile/tile3.mvt");
        var vtile = new mapnik.VectorTile(5,28,12);
        vtile.setData(data);
        assert.equal(vtile.getData().length,987);
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.empty(), false);
        var map = new mapnik.Map(vtile.tileSize,vtile.tileSize);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        vtile.render(map,
                     new mapnik.Image(256,256),
                     {scale:1.2, scale_denominator:1.5, variables:{pizza:'pie'}},
                     function(err,image) {
            if (err) throw err;
            var expected = './test/data/vector_tile/tile3.expected.png';
            var actual = './test/data/vector_tile/tile3.actual.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                image.save(expected, 'png32');
            }
            image.save(actual, 'png32');
            var e = fs.readFileSync(expected);
            var a = fs.readFileSync(actual);
            if (mapnik.versions.mapnik_number >= 300000) {
                assert.ok(Math.abs(e.length - a.length) < 100);
            } else {
                assert.equal(e.length,a.length);
            }
            done();
        });
    });

    it('should fail to render due to bad parameters', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        map.loadSync('./test/stylesheet.xml');
        map.srs = '+init=PIZZA';
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        assert.throws(function() { map.render(vtile, {image_scaling:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {image_scaling:'foo'}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {image_format:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {area_threshold:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {area_threshold:-0.1}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {strictly_simple:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {multi_polygon_union:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {fill_type:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {fill_type:99}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {process_all_rings:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {simplify_distance:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {simplify_distance:-0.5}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {variables:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {threading_mode:99}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {threading_mode:null}, function(err, vtile) {}); });
        map.render(vtile, {}, function(err, vtile) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should fail to render two vector tiles at once', function() {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        assert.throws(function() {
            map.render(vtile, {}, function(err, vtile) {});
            map.render(vtile, {}, function(err, vtile) {});
        });

    });

    it('should render a vector_tile of the whole world', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                assert.equal(vtile.reportGeometrySimplicity().length, 0);
                assert.equal(vtile.reportGeometryValidity().length, 25); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile0.mvt';
            var actual = './test/data/vector_tile/tile0.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON({decode_geometry:true})) == JSON.stringify(vt2.toJSON({decode_geometry:true})), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world with threading auto', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/map.xml');
        map.srs = "+init=epsg:3857";
        map.extent = vtile.extent();
        // until bug is fixed in std::future do not run this test on osx
        if (process.platform == 'darwin') {
            var opts = { threading_mode: mapnik.threadingMode.deferred };
        } else {
            var opts = { threading_mode: mapnik.threadingMode.auto };
        }

        map.render(vtile, opts, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                // commented since this is not stable across boost versions
                //assert.equal(vtile.reportGeometrySimplicity().length, 1); // seems simple, boost bug?
                assert.equal(vtile.reportGeometryValidity().length, 25); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile_threading_auto.mvt';
            var actual = './test/data/vector_tile/tile_threading_auto.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            assert.deepEqual(vtile.names(), ['world','nz']);
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON({decode_geometry:true})) == JSON.stringify(vt2.toJSON({decode_geometry:true})), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world with threading async', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/map.xml');
        map.srs = "+init=epsg:3857";
        map.extent = vtile.extent();
        // until bug is fixed in std::future do not run this test on osx
        if (process.platform == 'darwin') {
            var opts = { threading_mode: mapnik.threadingMode.deferred };
        } else {
            var opts = { threading_mode: mapnik.threadingMode.async };
        }

        map.render(vtile, opts, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                // commented since this is not stable across boost versions
                //assert.equal(vtile.reportGeometrySimplicity().length, 1); // geometry is simple, boost bug?
                assert.equal(vtile.reportGeometryValidity().length, 25); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile_threading_async.mvt';
            var actual = './test/data/vector_tile/tile_threading_async.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            assert.deepEqual(vtile.names(), ['world','nz']);
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON({decode_geometry:true})) == JSON.stringify(vt2.toJSON({decode_geometry:true})), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - area threshold applied', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, area_threshold:0.8}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                assert.equal(vtile.reportGeometrySimplicity().length, 0);
                assert.equal(vtile.reportGeometryValidity().length, 24); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile0-area_threshold.mvt';
            var actual = './test/data/vector_tile/tile0-area_threshold.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - strictly simple applied', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, strictly_simple:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 25); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile0-strictly_simple.mvt';
            var actual = './test/data/vector_tile/tile0-strictly_simple.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - strictly simple false', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, strictly_simple:false}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                var validityReport2 = vtile.reportGeometryValidity({split_multi_features:true});
                var validityReport3 = vtile.reportGeometryValidity({lat_lon:true});
                var validityReport4 = vtile.reportGeometryValidity({web_merc:true});
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 25); // Dataset not expected to be OGC valid
                assert.equal(validityReport2.length, 0); // Dataset not expected to be OGC valid
                assert.equal(validityReport3.length, 26); // Dataset not expected to be OGC valid
                assert.equal(validityReport4.length, 25); // Dataset not expected to be OGC valid
            }
            var expected = './test/data/vector_tile/tile0-strictly_simple_false.mvt';
            var actual = './test/data/vector_tile/tile0-strictly_simple_false.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - simplify_distance applied', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, simplify_distance:4}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 33); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile0-simplify_distance.mvt';
            var expected_nosse = './test/data/vector_tile/tile0-simplify_distance.nosse.mvt';
            var actual = './test/data/vector_tile/tile0-simplify_distance.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            if (!existsSync(expected_nosse)) {
                fs.writeFileSync(expected_nosse, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            var expected_nosse_data = fs.readFileSync(expected_nosse);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(expected_nosse_data);
            var vt3 = new mapnik.VectorTile(0,0,0);
            vt3.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt3.toJSON()) ||
                         JSON.stringify(vt2.toJSON()) == JSON.stringify(vt3.toJSON()), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - strictly simple and simplify distance applied', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, strictly_simple:true, simplify_distance:4}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 33); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile0-simple_and_distance.mvt';
            var expected_nosse = './test/data/vector_tile/tile0-simple_and_distance.nosse.mvt'
            var actual = './test/data/vector_tile/tile0-simple_and_distance.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            if (!existsSync(expected_nosse)) {
                fs.writeFileSync(expected_nosse, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            var expected_nosse_data = fs.readFileSync(expected_nosse);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(expected_nosse_data);
            var vt3 = new mapnik.VectorTile(0,0,0);
            vt3.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt3.toJSON()) ||
                         JSON.stringify(vt2.toJSON()) == JSON.stringify(vt3.toJSON()), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - multi_polygon_union set to false', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, multi_polygon_union:false}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                var validityReport2 = vtile.reportGeometryValidity({split_multi_features:true});
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 25); // Dataset not expected to be OGC valid
                assert.equal(validityReport2.length, 0);
            }
            var expected = './test/data/vector_tile/tile0-mpu-false.mvt';
            var actual = './test/data/vector_tile/tile0-mpu-false.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - multi_polygon_union set to true', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, multi_polygon_union:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile0-mpu-true.mvt';
            var actual = './test/data/vector_tile/tile0-mpu-true.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - fill_type set to evenOdd', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, fill_type:mapnik.polygonFillType.evenOdd}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 25); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile0-evenOdd.mvt';
            var actual = './test/data/vector_tile/tile0-evenOdd.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - fill_type set to nonZero', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, fill_type:mapnik.polygonFillType.nonZero}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 25); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile0-nonZero.mvt';
            var actual = './test/data/vector_tile/tile0-nonZero.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('should render a vector_tile of the whole world - process_all_rings', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        map.render(vtile, {variables:{pizza:'pie'}, process_all_rings:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 25); // Dataset not expected to be OGC valid
                assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            }
            var expected = './test/data/vector_tile/tile0-process-all-mp-rings.mvt';
            var actual = './test/data/vector_tile/tile0-process-all-mp-rings.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('should read back the vector tile and render an image with it', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.mvt'));
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        vtile.render(map, new mapnik.Image(256, 256), function(err, vtile_image) {
            if (err) throw err;
            var actual = './test/data/vector_tile/tile0.actual.png';
            vtile_image.save(actual, 'png32');
            var expected = './test/data/vector_tile/tile0.expected.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                vtile_image.save(expected, 'png32');
            }
            assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
            done();
        });
    });

    it('should read back the vector tile and render a native svg with it', function(done) {
        if (mapnik.supports.svg) {
            var vtile = new mapnik.VectorTile(0, 0, 0, {tile_size: 256});
            vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.mvt'));
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/stylesheet.xml');
            map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
            var surface2 = new mapnik.CairoSurface('svg',vtile.tileSize,vtile.tileSize);
            vtile.render(map, surface2, {renderer:'svg'}, function(err,surface2) {
                if (err) throw err;
                var actual_svg2 = './test/data/vector_tile/tile0.actual-svg.svg';
                var expected_svg2 = './test/data/vector_tile/tile0.expected-svg.svg';
                if (!existsSync(expected_svg2) || process.env.UPDATE) {
                    fs.writeFileSync(expected_svg2,surface2.getData());
                }
                fs.writeFileSync(actual_svg2,surface2.getData());
                var diff = Math.abs(fs.readFileSync(actual_svg2,'utf8').replace(/\r/g, '').length - fs.readFileSync(expected_svg2,'utf8').replace(/\r/g, '').length);
                assert.ok(diff < 50,"svg diff "+diff+" not less that 50");
                done();
            });
        } else {
            done();
        }
    });

    it('should read back the vector tile and render a cairo svg with it', function(done) {
        if (mapnik.supports.cairo) {
            var vtile = new mapnik.VectorTile(0, 0, 0, {tile_size:256});
            vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.mvt'));
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/stylesheet.xml');
            map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

            var surface = new mapnik.CairoSurface('svg',vtile.tileSize,vtile.tileSize);
            vtile.render(map, surface, {renderer:'cairo'}, function(err,surface) {
                if (err) throw err;
                var actual_svg = './test/data/vector_tile/tile0.actual-cairo.svg';
                var expected_svg = './test/data/vector_tile/tile0.expected-cairo.svg';
                if (!existsSync(expected_svg) || process.env.UPDATE) {
                    fs.writeFileSync(expected_svg,surface.getData());
                }
                fs.writeFileSync(actual_svg,surface.getData(),'utf-8');
                var diff = Math.abs(fs.readFileSync(actual_svg,'utf8').replace(/\r/g, '').length - fs.readFileSync(expected_svg,'utf8').replace(/\r/g, '').length);
                assert.ok(diff < 50,"svg diff "+diff+" not less that 50");
                done();
            });
        } else {
            done();
        }
    });

    it('should read back the vector tile and render an image with it using negative buffer', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.mvt'));
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        vtile.render(map, new mapnik.Image(256, 256), {buffer_size:-64}, function(err, vtile_image) {
            if (err) throw err;
            var actual = './test/data/vector_tile/tile0-b.actual.png';
            var expected = './test/data/vector_tile/tile0-b.expected.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                vtile_image.save(expected, 'png32');
            }
            vtile_image.save(actual, 'png32');
            assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
            done();
        });
    });

    if (mapnik.supports.grid) {
        it('should read back the vector tile and render a grid with it', function(done) {
            var vtile = new mapnik.VectorTile(0, 0, 0);
            vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.mvt'));
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/stylesheet.xml');
            map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

            vtile.render(map, new mapnik.Grid(256, 256), {layer:0}, function(err, vtile_image) {
                if (err) throw err;
                var utf = vtile_image.encodeSync();
                var expected_file = './test/data/vector_tile/tile0.expected.grid.json';
                var actual_file = './test/data/vector_tile/tile0.actual.grid.json';
                if (!existsSync(expected_file) || process.env.UPDATE) {
                    fs.writeFileSync(expected_file,JSON.stringify(utf,null,1));
                }
                fs.writeFileSync(actual_file,JSON.stringify(utf,null,1));
                var expected = JSON.parse(fs.readFileSync(expected_file));
                assert.deepEqual(utf,expected);
                done();
            });
        });
    } else {
      it.skip('should read back the vector tile and render a grid with it', function() { });
    }

    if (mapnik.supports.grid) {
        it('should read back the vector tile and render a grid with it - layer name and fields', function(done) {
            var vtile = new mapnik.VectorTile(0, 0, 0);
            vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.mvt'));
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/stylesheet.xml');
            map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

            vtile.render(map, new mapnik.Grid(256, 256, {key:'AREA'}), {layer:'world', fields:['NAME','REGION','NOTREAL', '__id__']}, function(err, vtile_image) {
                if (err) throw err;
                var utf = vtile_image.encodeSync();
                var expected_file = './test/data/vector_tile/tile0-fields.expected.grid.json';
                var actual_file = './test/data/vector_tile/tile0-fields.actual.grid.json';
                if (!existsSync(expected_file) || process.env.UPDATE) {
                    fs.writeFileSync(expected_file,JSON.stringify(utf,null,1));
                }
                fs.writeFileSync(actual_file,JSON.stringify(utf,null,1));
                var expected = JSON.parse(fs.readFileSync(expected_file));
                assert.deepEqual(utf,expected);
                done();
            });
        });
    } else {
      it.skip('should read back the vector tile and render a grid with it - layer name and fields', function() { });
    }

    it('should read back the vector tile and render an image with markers', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.mvt'));
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/markers.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        vtile.render(map, new mapnik.Image(256, 256), {buffer_size:-64}, function(err, vtile_image) {
            if (err) throw err;
            var actual = './test/data/vector_tile/tile0-c.actual.png';
            var expected = './test/data/vector_tile/tile0-c.expected.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                vtile_image.save(expected, 'png32');
            }
            vtile_image.save(actual, 'png32');
            // TODO - visual difference in master vs 2.3.x due to https://github.com/mapnik/mapnik/commit/ecc5acbdb953e172fcc652b55ed19b8b581e2146
            assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
            done();
        });
    });

    it('should be able to resample and encode (render) a geotiff into vector tile', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0, {tile_size:256, buffer_size:0});
        // first we render a geotiff into an image tile
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/raster_layer.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        map.render(vtile,{image_scaling:"bilinear",image_format:"jpeg"},function(err,vtile) {
            if (err) throw err;
            // now this vtile contains a 256/256 image
            // now render out with fancy styling
            var map2 = new mapnik.Map(256, 256);
            // TODO - minor bug, needs https://github.com/mapbox/mapnik-vector-tile/commit/f7b62c28a26d08e63d5665e8019ec4443f8701a1
            //assert.equal(vtile.painted(), true);
            assert.equal(vtile.empty(), false);
            map2.loadSync('./test/data/vector_tile/raster_style.xml');
            vtile.render(map2, new mapnik.Image(256, 256), {z:2,x:0,y:0,buffer_size:256}, function(err, vtile_image) {
                if (err) throw err;
                var actual = './test/data/vector_tile/tile-raster.actual.jpg';
                var expected = './test/data/vector_tile/tile-raster.expected.jpg';
                if (!existsSync(expected) || process.env.UPDATE) {
                    vtile_image.save(expected, 'jpeg80');
                }
                var diff = vtile_image.compare(new mapnik.Image.open(expected));
                if (diff > 0) {
                    vtile_image.save(actual, 'jpeg80');
                }
                assert.equal(0,diff);
                done();
            });
        });
    });

    it('should fail to addImage due to bad input', function(done) {
        var vtile = new mapnik.VectorTile(1, 0, 0, {tile_size:64});
        var img = new mapnik.Image(64,64);
        assert.throws(function() { vtile.addImage(); });
        assert.throws(function() { vtile.addImage(null); });
        assert.throws(function() { vtile.addImage('asdf'); });
        assert.throws(function() { vtile.addImage({}); });
        assert.throws(function() { vtile.addImage(img); });
        assert.throws(function() { vtile.addImage(img, 12); });
        assert.throws(function() { vtile.addImage({}, 'asdf'); });
        assert.throws(function() { vtile.addImage({}, 'asdf', function(err) {}); });
        assert.throws(function() { vtile.addImage({}, 'asdf', function(err) {}); });
        assert.throws(function() { vtile.addImage(img, 12, function(err) {}); });
        assert.throws(function() { vtile.addImage('not an object', 'waka', function(err) {}); });
        assert.throws(function() { vtile.addImage(new mapnik.Image(0,0), 'waka', function(err) {}); });
        assert.throws(function() { vtile.addImage(img, 'hoorah', null); });
        assert.throws(function() { vtile.addImageSync(); });
        assert.throws(function() { vtile.addImageSync(null); });
        assert.throws(function() { vtile.addImageSync('asdf'); });
        assert.throws(function() { vtile.addImageSync({}); });
        assert.throws(function() { vtile.addImageSync(img); });
        assert.throws(function() { vtile.addImageSync(img, 12); });
        assert.throws(function() { vtile.addImageSync({}, 'asdf'); });
        assert.throws(function() { vtile.addImageSync(img, 'optionsarenotanobject', null); });
        // invalid image type captured in try/catch sync
        assert.throws(function() { vtile.addImageSync(img, 'waka', {image_format: 'asdf'}); });
        // invalid image type captures error in try/catch
        vtile.addImage(img, 'waka', {image_format: 'asdf'}, function(err) {
            assert.throws(function() { if (err) throw err; });
            done();
        });
    });

    it('should fail with invalid options object for addImage & addImageSync', function(done) {
      var vtile = new mapnik.VectorTile(1, 0, 0);
      var im = new mapnik.Image(256,256);
      assert.throws(function() { vtile.addImage(im, 'waka', 'not an object', function(err) {}); });
      assert.throws(function() { vtile.addImage(im, 'waka', {image_scaling: 10}, function(err) {}); });
      assert.throws(function() { vtile.addImage(im, 'waka', {image_scaling: 'wordsarehard'}, function(err) {}); });
      assert.throws(function() { vtile.addImage(im, 'waka', {image_format: 45}, function(err) {}); });
      assert.throws(function() { vtile.addImageSync(im, 'waka', {image_scaling: 10}); });
      assert.throws(function() { vtile.addImageSync(im, 'waka', {image_scaling: 'wordsarehard'}); });
      assert.throws(function() { vtile.addImageSync(im, 'waka', {image_format: 45}); });
      done();
    });

    it('should fail to addImageBuffer due to bad input', function(done) {
        var vtile = new mapnik.VectorTile(1, 0, 0);
        assert.throws(function() { vtile.addImageBuffer(); });
        assert.throws(function() { vtile.addImageBuffer(null); });
        assert.throws(function() { vtile.addImageBuffer('asdf'); });
        assert.throws(function() { vtile.addImageBuffer({}); });
        assert.throws(function() { vtile.addImageBuffer(new Buffer('foo')); });
        assert.throws(function() { vtile.addImageBuffer(new Buffer('foo'), 12); });
        assert.throws(function() { vtile.addImageBuffer({}, 'asdf'); });
        assert.throws(function() { vtile.addImageBuffer(new Buffer(0), 'waka', {}); });
        assert.throws(function() { vtile.addImageBuffer({}, 'asdf', function(err) {}); });
        assert.throws(function() { vtile.addImageBuffer({}, 'asdf', function(err) {}); });
        assert.throws(function() { vtile.addImageBuffer(new Buffer('foo'), 12, function(err) {}); });
        assert.throws(function() { vtile.addImageBuffer('not a buffer', 'bar', function(err) {}); });
        assert.throws(function() { vtile.addImageBufferSync(); });
        assert.throws(function() { vtile.addImageBufferSync(null); });
        assert.throws(function() { vtile.addImageBufferSync('asdf'); });
        assert.throws(function() { vtile.addImageBufferSync({}); });
        assert.throws(function() { vtile.addImageBufferSync(new Buffer(0), 'waka'); });
        assert.throws(function() { vtile.addImageBufferSync(new Buffer('foo')); });
        assert.throws(function() { vtile.addImageBufferSync(new Buffer('foo'), 12); });
        assert.throws(function() { vtile.addImageBufferSync({}, 'asdf', function(err) {}); });
        assert.throws(function() { vtile.addImageBufferSync({}, 'asdf', function(err) {}); });
        done();
    });

    it('should be able to put an Image object into a vector tile layer', function(done) {
        var vtile = new mapnik.VectorTile(1, 0, 0, {tile_size:256});
        var image_buffer = fs.readFileSync('./test/data/vector_tile/cloudless_1_0_0.jpg');
        var im = new mapnik.Image.fromBytesSync(image_buffer);
        // push image into a named vtile layer
        vtile.addImageSync(im,'raster', {image_format:'jpeg', image_scaling: 'bilinear'});
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(),['raster']);
        var json_obj = vtile.toJSON();
        assert.equal(json_obj[0].name,'raster');
        assert.equal(json_obj[0].features[0].raster.length, 12983);
        // now render out with fancy styling
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/raster_style.xml');
        vtile.render(map, new mapnik.Image(256, 256), {buffer_size:256}, function(err, vtile_image) {
            if (err) throw err;
            var actual = './test/data/vector_tile/tile-raster2.actual.png';
            vtile_image.save(actual, 'png32');
            var expected = './test/data/vector_tile/tile-raster2.expected.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                vtile_image.save(expected, 'png32');
            }
            assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
            done();
        });
    });

    it('should fail if image object x or y are zero pixels', function(done) {
      var vtile = new mapnik.VectorTile(1, 0, 0, {tile_size:256});
      var im_empty = new mapnik.Image(0,0);
      var im_empty_y = new mapnik.Image(300,0);
      var im_empty_x = new mapnik.Image(0,300);
      assert.throws(function() { vtile.addImageSync(im_empty, 'nothing to see here'); });
      assert.throws(function() { vtile.addImageSync(im_empty_y, 'x is the best'); });
      assert.throws(function() { vtile.addImageSync(im_empty_y, 'y wins'); });
      done();
    });

    it('should be able to put an Image object into a vector tile layer async', function(done) {
        var vtile = new mapnik.VectorTile(1, 0, 0, {tile_size:256});
        var image_buffer = fs.readFileSync('./test/data/vector_tile/cloudless_1_0_0.jpg');
        var im = new mapnik.Image.fromBytesSync(image_buffer);
        // push image into a named vtile layer
        vtile.addImage(im,'raster', {image_format:'jpeg', image_scaling: 'bilinear'}, function (err) {
            assert.equal(vtile.painted(), true);
            assert.equal(vtile.empty(), false);
            assert.deepEqual(vtile.names(),['raster']);
            var json_obj = vtile.toJSON();
            assert.equal(json_obj[0].name,'raster');
            assert.equal(json_obj[0].features[0].raster.length, 12983);
            // now render out with fancy styling
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/data/vector_tile/raster_style.xml');
            vtile.render(map, new mapnik.Image(256, 256), {buffer_size:256}, function(err, vtile_image) {
                if (err) throw err;
                var actual = './test/data/vector_tile/tile-raster3.actual.png';
                vtile_image.save(actual, 'png32');
                var expected = './test/data/vector_tile/tile-raster3.expected.png';
                if (!existsSync(expected) || process.env.UPDATE) {
                    vtile_image.save(expected, 'png32');
                }
                assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
                done();
            });
        });
    });

    it('should be able to push an image tile directly into a vector tile layer without decoding', function(done) {
        var vtile = new mapnik.VectorTile(1, 0, 0, {tile_size:256});
        var image_buffer = fs.readFileSync('./test/data/vector_tile/cloudless_1_0_0.jpg');
        // push image into a named vtile layer
        vtile.addImageBufferSync(image_buffer,'raster');
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(),['raster']);
        var json_obj = vtile.toJSON();
        assert.equal(json_obj[0].name,'raster');
        assert.equal(json_obj[0].features[0].raster.length, 12146);
        // now render out with fancy styling
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/raster_style.xml');
        vtile.render(map, new mapnik.Image(256, 256), {buffer_size:256}, function(err, vtile_image) {
            if (err) throw err;
            var actual = './test/data/vector_tile/tile-raster4.actual.png';
            vtile_image.save(actual, 'png32');
            var expected = './test/data/vector_tile/tile-raster4.expected.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                vtile_image.save(expected, 'png32');
            }
            assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
            done();
        });
    });

    it('should be able to push an image tile directly into a vector tile layer without decoding -- async', function(done) {
        var vtile = new mapnik.VectorTile(1, 0, 0, {tile_size:256});
        var image_buffer = fs.readFileSync('./test/data/vector_tile/cloudless_1_0_0.jpg');
        // push image into a named vtile layer
        vtile.addImageBuffer(image_buffer,'raster', function(err) {
            assert.equal(vtile.painted(), true);
            assert.equal(vtile.empty(), false);
            assert.deepEqual(vtile.names(),['raster']);
            var json_obj = vtile.toJSON();
            assert.equal(json_obj[0].name,'raster');
            assert.equal(json_obj[0].features[0].raster.length, 12146);
            // now render out with fancy styling
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/data/vector_tile/raster_style.xml');
            vtile.render(map, new mapnik.Image(256, 256), {buffer_size:256}, function(err, vtile_image) {
                if (err) throw err;
                var actual = './test/data/vector_tile/tile-raster5.actual.png';
                vtile_image.save(actual, 'png32');
                var expected = './test/data/vector_tile/tile-raster5.expected.png';
                if (!existsSync(expected) || process.env.UPDATE) {
                    vtile_image.save(expected, 'png32');
                }
                assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
                done();
            });
        });
    });

    it('should include image in getData pbf output', function(done) {
        var vtile = new mapnik.VectorTile(1, 0, 0, {tile_size:256});
        var image_buffer = fs.readFileSync('./test/data/vector_tile/cloudless_1_0_0.jpg');
        var im = new mapnik.Image.fromBytesSync(image_buffer);
        // push image into a named vtile layer
        vtile.addImage(im,'raster', {image_format:"jpeg"});
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(),['raster']);
        var json_obj = vtile.toJSON();
        assert.equal(json_obj[0].name,'raster');
        assert.equal(json_obj[0].features[0].raster.length, 12983);
        // getData from the image vtile
        var vtile2 = new mapnik.VectorTile(1, 0, 0, {tile_size:256});
        var vtile_buf = vtile.getData();
        vtile2.setData(vtile_buf);
        var json_obj2 = vtile2.toJSON();
        assert.deepEqual(json_obj, json_obj2);
        done();
    });

    it('should be able to render data->vtile and vtile->image with roughtly the same results', function(done) {
        var x=3;
        var y=2;
        var z=2;
        //var extent = [10018754.171394622,-10018754.17139462,20037508.342789244,9.313225746154785e-10];
        var extent = mercator.bbox(x, y, z, false, '900913');
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/map.xml');
        map.extent = extent;
        // render a png from the map
        var expected_1 = './test/data/vector_tile/nz-1.png';
        var expected_2 = './test/data/vector_tile/nz-1b.png';
        var actual_1 = './test/data/vector_tile/nz-1-actual.png';
        var actual_2 = './test/data/vector_tile/nz-1b-actual.png';
        map.render(new mapnik.Image(256,256),{},function(err,im) {
            if (err) throw err;
            im.save(actual_1, 'png32');
            if (!existsSync(expected_2) || process.env.UPDATE) {
                im.save(expected_1, 'png32');
            }
            assert.equal(0,im.compare(new mapnik.Image.open(expected_1)));
            // render a vtile
            map.render(new mapnik.VectorTile(z,x,y),{},function(err,vtile) {
                if (err) throw err;
                vtile.render(map,new mapnik.Image(256,256),{},function(err, vtile_image) {
                    if (err) throw err;
                    vtile_image.save(actual_2, 'png32');
                    if (!existsSync(expected_2) || process.env.UPDATE) {
                        vtile_image.save(expected_2, 'png32');
                    }
                    assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected_2)));
                    done();
                });
            });
        });
    });

    it('toGeoJSON should not drop geometries outside tile extent', function(done) {
        var vt = new mapnik.VectorTile(10,131,242);
        vt.setData(fs.readFileSync('./test/data/v4-10_131_242.mvt'));
        vt.toJSON().forEach(function(layer) {
            assert.equal(layer.features.length, JSON.parse(vt.toGeoJSONSync(layer.name)).features.length);
        })
        done();
    });

    var multi_polygon_with_degerate_exterior_ring = {
        "type": "FeatureCollection",
        "features": [
          {
            "type": "Feature",
            "properties": {},
            "geometry": {
              "type": "MultiPolygon",
              "coordinates": [[
                [
                  [
                    -30.937499999999996,
                    2.811371193331128
                  ],
                  [
                    -30.937499999999996,
                    73.42842364106818
                  ]
                ],
                [
                  [
                    -6.328125,
                    23.241346102386135
                  ],
                  [
                    -6.328125,
                    67.60922060496382
                  ],
                  [
                    64.6875,
                    67.60922060496382
                  ],
                  [
                    64.6875,
                    23.241346102386135
                  ],
                  [
                    -6.328125,
                    23.241346102386135
                  ]
                ]
              ]]
            }
          }
        ]
    };

    it('test that degenerate exterior ring causes all rings to be throw out', function() {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.addGeoJSON(JSON.stringify(multi_polygon_with_degerate_exterior_ring), "geojson");
        var expected = [];
        assert.equal(vtile.empty(), true);
        assert.deepEqual(vtile.toJSON({decode_geometry:true}), expected);
    });

    it('test that degenerate exterior ring is skipped when `process_all_rings` is true and remaining polygons are processed', function() {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.addGeoJSON(JSON.stringify(multi_polygon_with_degerate_exterior_ring), "geojson", {process_all_rings:true, fill_type:mapnik.polygonFillType.evenOdd});
        var expected = [{
            "name":"geojson",
            "extent":4096,
            "version":2,
            "features":[
                {
                    "id":1,
                    "type":3,
                    "geometry":[
                        [[1976,1776],[1976,992],[2784,992],[2784,1776],[1976,1776]]
                    ],
                    "geometry_type":"Polygon",
                    "properties":{}
                }
            ]}];
        assert.deepEqual(vtile.toJSON({decode_geometry:true}), expected);
    });

    it('test that polygon with invalid exterior ring results a polygon process_all_rings true', function() {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.addGeoJSON(JSON.stringify({
            "type": "FeatureCollection",
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Polygon",
                        "coordinates": [[[-2, 2], [-2, 2]], [[-1, 1], [1, 1], [1, -1], [-1, -1], [-1, 1]]]
                    },
                    "properties": {}
                }
            ]
        }), "geojson", {process_all_rings:true, fill_type:mapnik.polygonFillType.evenOdd});
        var expected = [{
            "name":"geojson",
            "extent":4096,
            "version":2,
            "features":[
                {
                    "id":1,
                    "type":3,
                    "geometry":[
                        [[2037,2059],[2037,2037],[2059,2037],[2059,2059],[2037,2059]]
                    ],
                    "geometry_type":"Polygon",
                    "properties":{}
                }
            ]}];
        assert.deepEqual(vtile.toJSON({decode_geometry:true}), expected);
    });

    it('test that polygon with invalid exterior ring results in no vector tile with process_all_rings false', function() {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.addGeoJSON(JSON.stringify({
            "type": "FeatureCollection",
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Polygon",
                        "coordinates": [[[-2, 2], [-2, 2]], [[-1, 1], [1, 1], [1, -1], [-1, -1], [-1, 1]]]
                    },
                    "properties": {}
                }
            ]
        }), "geojson", {process_all_rings:false, fill_type:mapnik.polygonFillType.evenOdd});
        var expected = [];
        assert.equal(vtile.empty(), true);
        assert.deepEqual(vtile.toJSON({decode_geometry:true}), expected);
    });

    it('test that overlapping multipolygon results in two polygons in round trip with multipolygon false', function() {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.addGeoJSON(JSON.stringify({
            "type": "FeatureCollection",
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "MultiPolygon",
                        "coordinates": [[[[-2, 2], [2, 2], [2, -2], [-2, -2], [-2, 2]]], [[[-1, 1], [1, 1], [1, -1], [-1, -1], [-1, 1]]]]
                    },
                    "properties": {}
                }
            ]
        }), "geojson", {multi_polygon_union:false});
        var expected = [{
            "name":"geojson",
            "extent":4096,
            "version":2,
            "features":[
                {
                    "id":1,
                    "type":3,
                    "geometry":[
                        [
                            [[2071,2025],[2071,2071],[2025,2071],[2025,2025],[2071,2025]]
                        ],
                        [
                            [[2059,2037],[2059,2059],[2037,2059],[2037,2037],[2059,2037]]
                        ]
                    ],
                    "geometry_type":"MultiPolygon",
                    "properties":{}
                }
            ]}];
        assert.deepEqual(vtile.toJSON({decode_geometry:true}), expected);
    });

    it('test that overlapping multipolygon results in one polygon in round trip with multipolygon true', function() {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.addGeoJSON(JSON.stringify({
            "type": "FeatureCollection",
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "MultiPolygon",
                        "coordinates": [[[[-2, 2], [2, 2], [2, -2], [-2, -2], [-2, 2]]], [[[-1, 1], [1, 1], [1, -1], [-1, -1], [-1, 1]]]]
                    },
                    "properties": {}
                }
            ]
        }), "geojson", {multi_polygon_union:true});
        var expected = [{
            "name":"geojson",
            "extent":4096,
            "version":2,
            "features":[
                {
                    "id":1,
                    "type":3,
                    "geometry":[
                        [[2071,2025],[2071,2071],[2025,2071],[2025,2025],[2071,2025]]
                    ],
                    "geometry_type":"Polygon",
                    "properties":{}
                 }
            ]}];
        assert.deepEqual(vtile.toJSON({decode_geometry:true}), expected);
    });

    it('test that overlapping multipolygon results in one polygon in round trip with multipolygon true and even odd', function() {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.addGeoJSON(JSON.stringify({
            "type": "FeatureCollection",
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "MultiPolygon",
                        "coordinates": [[[[-2, 2], [2, 2], [2, -2], [-2, -2], [-2, 2]]], [[[-1, 1], [1, 1], [1, -1], [-1, -1], [-1, 1]]]]
                    },
                    "properties": {}
                }
            ]
        }), "geojson", {multi_polygon_union:true, fill_type:mapnik.polygonFillType.evenOdd});
        var expected = [{
            "name":"geojson",
            "extent":4096,
            "version":2,
            "features":[
                {
                    "id":1,
                    "type":3,
                    "geometry":[
                        [[2071,2025],[2071,2071],[2025,2071],[2025,2025],[2071,2025]],
                        [[2059,2037],[2037,2037],[2037,2059],[2059,2059],[2059,2037]]
                    ],
                    "geometry_type":"Polygon","properties":{}
                 }
            ]}];
        assert.deepEqual(vtile.toJSON({decode_geometry:true}), expected);
    });

    it('pasted test 1 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted1.xml');
        map.render(vtile, {multi_polygon_union:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted1.mvt';
            var actual = './test/data/vector_tile/pasted/pasted1.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 2 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted2.xml');
        map.render(vtile, {multi_polygon_union:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted2.mvt';
            var actual = './test/data/vector_tile/pasted/pasted2.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 3 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted3.xml');
        map.render(vtile, {multi_polygon_union:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted3.mvt';
            var actual = './test/data/vector_tile/pasted/pasted3.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 4 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted4.xml');
        map.render(vtile, {multi_polygon_union:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted4.mvt';
            var actual = './test/data/vector_tile/pasted/pasted4.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 5 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted5.xml');
        map.render(vtile, {multi_polygon_union:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted5.mvt';
            var actual = './test/data/vector_tile/pasted/pasted5.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 6 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted6.xml');
        map.render(vtile, {multi_polygon_union:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted6.mvt';
            var actual = './test/data/vector_tile/pasted/pasted6.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 7 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted7.xml');
        map.render(vtile, {multi_polygon_union:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted7.mvt';
            var actual = './test/data/vector_tile/pasted/pasted7.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 8 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted8.xml');
        map.render(vtile, {multi_polygon_union:true}, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted8.mvt';
            var actual = './test/data/vector_tile/pasted/pasted8.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(0,0,0);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(0,0,0);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 9 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile1 = new mapnik.VectorTile(0, 0, 0);
        var vtile1_data = fs.readFileSync('./test/data/vector_tile/pasted/pasted9_tile0_0_0_0.mvt.mvt');
        vtile1.setData(vtile1_data);
        var vtile2 = new mapnik.VectorTile(0, 0, 0);
        var vtile2_data = fs.readFileSync('./test/data/vector_tile/pasted/pasted9_tile1_0_0_0.mvt.mvt');
        vtile2.setData(vtile2_data);
        var vtile = new mapnik.VectorTile(1,1,0,{buffer_size: 4096});
        vtile.composite([vtile1, vtile2], function(err) {
            if (err) throw err;
            assert.deepEqual(vtile.names(), ['landuse', 'waterway', 'water', 'road', 'bridge',
                                             'place_label', 'poi_label', 'road_label', 'waterway_label',
                                             'landcover', 'hillshade', 'contour']);
            assert.equal(vtile.reportGeometryValidity().length, 0);
            assert.equal(vtile.reportGeometryValidity({split_multi_features:true}).length, 0);
            // commented since this is not stable across boost versions
            //assert.equal(vtile.reportGeometrySimplicity().length, 6);
            done();
        });
    });

    it('pasted test 10 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(15,19589,17578, {buffer_size:8*16});
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted10.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted10.mvt';
            var actual = './test/data/vector_tile/pasted/pasted10.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(15,19589,17578, {buffer_size:8*16});
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(15,19589,17578, {buffer_size:8*16});
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 11 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(13,4889,4395,{buffer_size:8*16});
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted11.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted11.mvt';
            var actual = './test/data/vector_tile/pasted/pasted11.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(13,4889,4395, {buffer_size:8*16});
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(13,4889,4395, {buffer_size:8*16});
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 12 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(12,2433,2194,{buffer_size:8*16});
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted12.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted12.mvt';
            var actual = './test/data/vector_tile/pasted/pasted12.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(12,2433,2194, {buffer_size:8*16});
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(12,2433,2194, {buffer_size:8*16});
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 13 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(15,19469,17477);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted13.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted13.mvt';
            var actual = './test/data/vector_tile/pasted/pasted13.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(15,19469,17477);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(15,19469,17477);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 14 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(15,19470,17537);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted14.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted14.mvt';
            var actual = './test/data/vector_tile/pasted/pasted14.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(15,19470,17537);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(15,19470,17537);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 15 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(8,154,136);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted15.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted15.mvt';
            var actual = './test/data/vector_tile/pasted/pasted15.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(8,154,136);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(8,154,136);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 16 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(11,1216,1093);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted16.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted16.mvt';
            var actual = './test/data/vector_tile/pasted/pasted16.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(11,1216,1093);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(11,1216,1093);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 17 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(15,19476,17611);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted17.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted17.mvt';
            var actual = './test/data/vector_tile/pasted/pasted17.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(15,19476,17611);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(15,19476,17611);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 18 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(11,288,661);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted18.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted18.mvt';
            var actual = './test/data/vector_tile/pasted/pasted18.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(11,288,661);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(11,288,661);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 19 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(13,1164,2678);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted19.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted19.mvt';
            var actual = './test/data/vector_tile/pasted/pasted19.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(13,1164,2678);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(13,1164,2678);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 20 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(9,72,167);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted20.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted20.mvt';
            var actual = './test/data/vector_tile/pasted/pasted20.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(9,72,167);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(9,72,167);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 21 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(8,37,82);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted21.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted21.mvt';
            var actual = './test/data/vector_tile/pasted/pasted21.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(8,37,82);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(8,37,82);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 22 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(9,72,167);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted22.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted22.mvt';
            var actual = './test/data/vector_tile/pasted/pasted22.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(9,72,167);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(9,72,167);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });

    it('pasted test 23 - testing clipper in mapnik vector tile corrects invalid geometry issues', function(done) {
        var vtile = new mapnik.VectorTile(9,72,165);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/pasted/pasted23.xml');
        map.render(vtile, function(err, vtile) {
            if (err) throw err;
            if (hasBoostSimple) {
                var simplicityReport = vtile.reportGeometrySimplicity();
                var validityReport = vtile.reportGeometryValidity();
                assert.equal(simplicityReport.length, 0);
                assert.equal(validityReport.length, 0);
            }
            assert(!vtile.empty());
            var expected = './test/data/vector_tile/pasted/pasted23.mvt';
            var actual = './test/data/vector_tile/pasted/pasted23.actual.mvt';
            if (!existsSync(expected) || process.env.UPDATE) {
                fs.writeFileSync(expected, vtile.getData());
            }
            var expected_data = fs.readFileSync(expected);
            fs.writeFileSync(actual, vtile.getData());
            var actual_data = fs.readFileSync(actual);
            var vt1 = new mapnik.VectorTile(9,72,165);
            vt1.setData(expected_data);
            var vt2 = new mapnik.VectorTile(9,72,165);
            vt2.setData(actual_data);
            assert.equal(JSON.stringify(vt1.toJSON()) == JSON.stringify(vt2.toJSON()), true);
            done();
        });
    });
});
