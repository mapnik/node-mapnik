"use strict";

var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var mercator = new(require('sphericalmercator'))();
var existsSync = require('fs').existsSync || require('path').existsSync;
var overwrite_expected_data = false;

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
                _data = vtile.getData().toString("hex");
                _length = vtile.getData().length;
                var map2 = new mapnik.Map(256,256);
                map2.loadSync('./test/data/vector_tile/layers.xml');
                var vtile2 = new mapnik.VectorTile(5,28,12);
                var bbox = mercator.bbox(28, 12, 5, false, '900913');
                map2.extent = bbox;
                map2.render(vtile2,{}, function(err,vtile2) {
                    if (err) throw err;
                    fs.writeFileSync("./test/data/vector_tile/tile3.vector.pbf",vtile2.getData());
                    done();
                });
            });
        } else {
            _vtile = new mapnik.VectorTile(9,112,195);
            _vtile.setData(fs.readFileSync('./test/data/vector_tile/tile1.vector.pbf'));
            _vtile.parse();
            _data = _vtile.getData().toString("hex");
            _length = _vtile.getData().length;
            done();
        }
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
        // test array containing each geojson
        var json_array = vtile.toGeoJSONSync('__array__');
        var out2 = JSON.parse(json_array);
        assert.equal(out2.length,2);
        var coords3 = out2[0].features[0].geometry.coordinates;
        assert.ok(Math.abs(coords3[0] - geojson.features[0].geometry.coordinates[0]) < 0.3);
        assert.ok(Math.abs(coords3[1] - geojson.features[0].geometry.coordinates[1]) < 0.3);
        // not passing callback trigger sync method
        assert.equal(vtile.toGeoJSON('__array__'),json_array);
        // ensure async method has same result
        vtile.toGeoJSON('__all__',function(err,json_string) {
            assert.deepEqual(json_string,json_out);
            vtile.toGeoJSON('__array__',function(err,json_string) {
                assert.deepEqual(json_string,json_array);
                done();
            });
        });
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
        assert.throws(function() { new mapnik.VectorTile(0,0,0,{width:null}); });
        assert.throws(function() { new mapnik.VectorTile(0,0,0,{height:null}); });
    });

    it('should be initialized properly', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.ok(vtile instanceof mapnik.VectorTile);
        assert.equal(vtile.width(), 256);
        assert.equal(vtile.height(), 256);
        assert.equal(vtile.painted(), false);
        assert.equal(vtile.getData().toString(),"");
        assert.equal(vtile.isSolid(), "");
        assert.equal(vtile.empty(), true);
        vtile.isSolid(function(err, solid, key) {
            if (err) throw err;
            assert.equal(solid, true);
            assert.equal(key, "");
            done();
        });
    });

    it('should accept optional width/height', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0,{width:512,height:512});
        assert.equal(vtile.width(), 512);
        assert.equal(vtile.height(), 512);
        done();
    });

    it('should be able to setData/parse (sync)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        // tile1 represents a "solid" vector tile with one layer
        // that only encodes a single feature with a single path with
        // a polygon box resulting from clipping a chunk out of 
        // a larger polygon fully outside the rendered/clipping extent
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        // empty is valid to use before parse() (and after)
        assert.equal(vtile.empty(), false);
        vtile.parse();
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.isSolid(), "world");
        assert.equal(vtile.empty(), false);
        vtile.isSolid(function(err, solid, key) {
            if (err) throw err;
            assert.equal(solid, true);
            assert.equal(key, "world");
            done();
        });
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
        assert.throws(function() { vtile.setData(new Buffer('foo'));vtile.parse(); });
        assert.throws(function() { vtile.setData(new Buffer(0)); }); // empty buffer is not valid
        vtile.setData(new Buffer('foo'),function(err) {
            if (err) throw err;
            assert.throws(function() { vtile.empty(); });
            vtile.parse(function(err) {
                assert.throws(function() { if (err) throw err; });
                done();
            });
        });
    });

    it('should error out if we pass invalid data to setData - 2', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        vtile.setData(new Buffer('0'),function(err) {
            if (err) throw err;
            assert.throws(function() { 
                vtile.names(); 
            }); //  unterminated varint, unexpected end of buffer
            assert.throws(function() { 
                vtile.empty(); 
            }); //  unterminated varint, unexpected end of buffer
            vtile.parse(function(err) {
                assert.throws(function() { if (err) throw err; });
                done();
            });
        });
    });

    it('should error out if we pass invalid data to setData - 3', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        vtile.setData(new Buffer('0B1234', 'hex'),function(err) {
            if (err) throw err;
            assert.throws(function() { 
                vtile.names(); 
            }); // "can not skip unknown type 3"
            assert.throws(function() { 
                vtile.empty(); 
            }); // "can not skip unknown type 3"
            done();
        });
    });
    
    it('should error out if we pass invalid data to setData - 4', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        vtile.setData(new Buffer('120774657374696e67', 'hex')); // should set fine as string 
        vtile.names(); // should not throw and does nothing.
        vtile.setData(new Buffer('089601', 'hex')); // variant should work fine.
        vtile.names(); // should not throw and does nothing.
        assert.throws(function() { 
            vtile.setData(new Buffer('0896818181818181818181818181818181818181', 'hex')); 
            // variant should fail because it is too long.
            vtile.names(); // should throw
        });
        assert.throws(function() { 
            vtile.setData(new Buffer('0896818181818181818181818181818181818181', 'hex')); 
            // variant should fail because it is too long.
            vtile.empty(); // should throw
        });
        vtile.setData(new Buffer('090123456789012345', 'hex')); // 64 should work fine.
        vtile.names(); // should not throw and does nothing.
        assert.equal(vtile.empty(), true);
        vtile.setData(new Buffer('0D01234567', 'hex')); // 32 should work fine.
        vtile.names(); // should not throw and does nothing.
        assert.equal(vtile.empty(), true);
        assert.throws(function() { 
            vtile.setData(new Buffer('0D0123456', 'hex')); // 32 should fail because missing a byte
            vtile.names(); // should throw
        });
        assert.throws(function() { 
            vtile.setData(new Buffer('0D0123456', 'hex')); // 32 should fail because missing a byte
            vtile.empty(); // should throw
        });
    });
    
    it('should error out if we pass invalid data to setData - 5', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        vtile.parse();
        assert.equal(vtile.empty(), true);
        vtile.setData(new Buffer(0),function(err) {
            if (err) throw err;
            vtile.parse(function(err) {
                assert.throws(function() { if (err) throw err; });
                done();
            });
        });
    });

    it('should return empty but have layer name', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        // a layer with only a name "layer-name" and no features
        vtile.setData(new Buffer('1A0C0A0A6C617965722D6E616D65', 'hex'));
        assert.equal(vtile.names()[0], 'layer-name');
        assert.equal(vtile.empty(), true);
    });

    it('should error out if we pass invalid data to addData', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.equal(vtile.empty(), true);
        assert.throws(function() { vtile.addData(null); }); // empty buffer is not valid
        assert.throws(function() { vtile.addData({}); }); // empty buffer is not valid
        assert.throws(function() { vtile.addData(new Buffer(0)); }); // empty buffer is not valid
        assert.throws(function() {
            vtile.addData(new Buffer('foo'));
            vtile.parse();
        });
    });
    
    it('should fail to parse', function() {
        var vtile = new mapnik.VectorTile(0,0,0);
        vtile.addData(new Buffer('foo'));
        vtile.clear();
        assert.throws(function() { vtile.parse(); });
    });

    it('should be able to setData/parse (async)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        assert.equal(vtile.empty(), true);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data, function(err) {
            if (err) throw err;
            // names and empty are valid before parse()
            assert.deepEqual(vtile.names(), ["world"]);
            assert.equal(vtile.empty(), false);
            vtile.parse(function(err) {
                if (err) throw err;
                assert.deepEqual(vtile.names(), ["world"]);
                assert.equal(vtile.empty(), false);
                assert.equal(vtile.painted(), true);
                assert.equal(vtile.isSolid(), "world");
                vtile.isSolid(function(err, solid, key) {
                    if (err) throw err;
                    assert.equal(solid, true);
                    assert.equal(key, "world");
                    done();
                });
            });
        });
    });

    it('should be able to get layer names without parsing', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data, function(err) {
            if (err) throw err;
            assert.deepEqual(vtile.names(), ["world"]);
            assert.equal(vtile.empty(), false);
            vtile.parse();
            assert.deepEqual(vtile.names(), ["world"]);
            assert.equal(vtile.empty(), false);
            done();
        });
    });


    it('should be able to get tile info as JSON', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        vtile.parse();
        assert.deepEqual(vtile.names(),['world']);
        var expected = [{"name":"world","extent":4096,"version":2,"features":[{"id":207,"type":3,"geometry":[9,0,0,26,0,8190,8190,0,0,8189,15],"properties":{"AREA":915896,"FIPS":"US","ISO2":"US","ISO3":"USA","LAT":39.622,"LON":-98.606,"NAME":"United States","POP2005":299846449,"REGION":19,"SUBREGION":21,"UN":840}}]}];
        assert.deepEqual(vtile.toJSON(),expected);
        done();
    });

    it('should be able to get tile info as various flavors of GeoJSON', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        vtile.parse();
        var expected_geojson = {"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Polygon","coordinates":[[[-101.25,39.36827914916011],[-101.25,38.82272471585834],[-100.54704666137694,38.82272471585834],[-100.54704666137694,39.36827914916011],[-101.25,39.36827914916011]]]},"properties":{"AREA":915896,"FIPS":"US","ISO2":"US","ISO3":"USA","LAT":39.622,"LON":-98.606,"NAME":"United States","POP2005":299846449,"REGION":19,"SUBREGION":21,"UN":840}}]};
        var expected_copy = JSON.parse(JSON.stringify(expected_geojson));
        expected_geojson.name = "world";
        var actual = JSON.parse(vtile.toGeoJSON(0));
        assert.equal(actual.type,expected_geojson.type);
        assert.equal(actual.name,expected_geojson.name);
        assert.equal(actual.features.length,expected_geojson.features.length);
        assert.equal(actual.features[0].properties.length,expected_geojson.features[0].properties.length);
        assert.equal(actual.features[0].properties.NAME,expected_geojson.features[0].properties.NAME);
        deepEqualTrunc(actual.features[0].geometry,expected_geojson.features[0].geometry);
        deepEqualTrunc(JSON.parse(vtile.toGeoJSON(0)),JSON.parse(vtile.toGeoJSON('world')));
        actual = JSON.parse(vtile.toGeoJSON('__all__'));
        assert.equal(actual.type,expected_copy.type);
        assert.equal(actual.name,expected_copy.name);
        assert.equal(actual.features.length,expected_copy.features.length);
        assert.equal(actual.features[0].properties.length,expected_copy.features[0].properties.length);
        assert.equal(actual.features[0].properties.NAME,expected_copy.features[0].properties.NAME);
        deepEqualTrunc(actual.features[0].geometry,expected_copy.features[0].geometry);
        var json_array = JSON.parse(vtile.toGeoJSON('__array__'));
        assert.equal(json_array.length,1);
        actual = json_array[0];
        assert.equal(actual.type,expected_geojson.type);
        assert.equal(actual.name,expected_geojson.name);
        assert.equal(actual.features.length,expected_geojson.features.length);
        assert.equal(actual.features[0].properties.length,expected_geojson.features[0].properties.length);
        assert.equal(actual.features[0].properties.NAME,expected_geojson.features[0].properties.NAME);
        deepEqualTrunc(actual.features[0].geometry,expected_geojson.features[0].geometry);
        done();
    });

    it('should be able to get and set data', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        vtile.parse();
        var vtile2 = new mapnik.VectorTile(9,112,195);
        vtile2.setData(vtile.getData());
        vtile2.parse();
        assert.deepEqual(vtile.names(),vtile2.names());
        assert.deepEqual(vtile.toJSON(),vtile2.toJSON());
        assert.deepEqual(vtile2.toJSON(),_vtile.toJSON());
        done();
    });

    it('should be able to get virtual datasource and features', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        vtile.parse();
        assert.equal(vtile.getData().length,_length);
        var ds = vtile.toJSON();
        assert.equal(ds.length,1);
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
        done();
    });

    it('should be able to clear data (sync)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        vtile.parse();
        assert.equal(vtile.getData().length,_length);
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.isSolid(), "world");
        var feature_count = vtile.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        vtile.clear();
        assert.equal(vtile.getData().length,0);
        assert.deepEqual(vtile.toJSON(), {});
        assert.equal(vtile.painted(), false);
        assert.equal(vtile.isSolid(), false);
        done();
    });

    it('should be able to clear data (async)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(), ["world"]);
        vtile.parse();
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(), ["world"]);
        assert.equal(vtile.getData().length,_length);
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.isSolid(), "world");
        var feature_count = vtile.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        vtile.clear(function(err) {
            if (err) throw err;
            assert.equal(vtile.empty(), true);
            assert.deepEqual(vtile.names(), []);
            assert.equal(vtile.getData().length,0);
            assert.deepEqual(vtile.toJSON(), {});
            assert.equal(vtile.painted(), false);
            assert.equal(vtile.isSolid(), false);
            done();
        });
    });

    it('should be able to add data', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        vtile.addData(new Buffer(_data,"hex"));
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(), ["world","world"]);
        vtile.parse();
        assert.equal(vtile.empty(), false);
        assert.deepEqual(vtile.names(), ["world","world"]);
        assert.equal(vtile.getData().length,_length*2);
        assert.equal(vtile.painted(), true);
        assert.deepEqual(vtile.names(), ["world","world"]);
        assert.equal(vtile.isSolid(), "world-world");
        var feature_count = vtile.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        vtile.clear(function(err) {
            if (err) throw err;
            assert.equal(vtile.empty(), true);
            assert.deepEqual(vtile.names(), []);
            assert.equal(vtile.getData().length,0);
            assert.deepEqual(vtile.toJSON(), {});
            assert.equal(vtile.painted(), false);
            assert.equal(vtile.isSolid(), false);
            done();
        });
    });


    it('should detect as solid a tile with two "box" layers', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/layers.xml');
        map.extent = [-11271098.442818949,4696291.017841229,-11192826.925854929,4774562.534805249];
        map.render(vtile,{},function(err,vtile) {
            // this tile duplicates the layer that is present in tile1.vector.pbf
            fs.writeFileSync('./test/data/vector_tile/tile2.vector.pbf',vtile.getData());
            _data = vtile.getData().toString("hex");
            var vtile2 = new mapnik.VectorTile(9,112,195);
            var raw_data = fs.readFileSync("./test/data/vector_tile/tile2.vector.pbf");
            vtile2.setData(raw_data);
            vtile2.parse();
            assert.equal(vtile.empty(), false);
            assert.equal(vtile.painted(), true);
            assert.equal(vtile.isSolid(), "world-world2");
            assert.equal(vtile.getData().toString("hex"),raw_data.toString("hex"));
            vtile.isSolid(function(err, solid, key) {
                if (err) throw err;
                assert.equal(solid, true);
                assert.equal(key, "world-world2");
                done();
            });
        });
    });
    
    it('should render an empty vector', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/layers.xml');
        map.extent = [-1,1,-1,1];
        map.render(vtile,{},function(err,vtile) {
            assert.equal(vtile.empty(), true);
            done();
        });
    });

    // next three testcases cover isSolid edge conditions and can be
    // removed if isSolid is deprecated
    it('should detect as non-solid a tile with intersecting rectangle', function(done) {
        // but whose verticies are all outside the extent
        var vt = new mapnik.VectorTile(13,1337,2825);
        vt.setData(fs.readFileSync('./test/data/vector_tile/13.1337.2825.vector.pbf'));
        assert.equal(vt.empty(),false);
        vt.parse();
        vt.isSolid(function(err, solid, key) {
            if (err) throw err;
            assert.equal(solid, false);
            assert.equal(key, "");
            done();
        });
    });

    it('should detect as non-solid a tile with intersecting rectangle 2', function(done) {
        // but whose verticies are all outside the extent
        var vt = new mapnik.VectorTile(12,771,1608);
        vt.setData(fs.readFileSync('./test/data/vector_tile/12.771.1608.vector.pbf'));
        assert.equal(vt.empty(),false);
        vt.parse();
        vt.isSolid(function(err, solid, key) {
            if (err) throw err;
            assert.equal(solid, false);
            assert.equal(key, "");
            done();
        });
    });

    it('should detect as solid a tile with rectangle that is solid within tile extent', function(done) {
        // however in this case if we respected the buffer there would
        // be gaps in upper left and lower left corners
        // TODO: support buffer in is_solid check?
        var vt = new mapnik.VectorTile(10,196,370);
        vt.setData(fs.readFileSync('./test/data/vector_tile/10.196.370.vector.pbf'));
        assert.equal(vt.empty(),false);
        vt.parse();
        vt.isSolid(function(err, solid, key) {
            if (err) throw err;
            assert.equal(solid, true);
            assert.equal(key, "nps_land_clip_pg-nps_park_polys");
            done();
        });
    });
    
    it('should render expected results', function(done) {
        var data = fs.readFileSync("./test/data/vector_tile/tile3.vector.pbf");
        var vtile = new mapnik.VectorTile(5,28,12);
        vtile.setData(data);
        vtile.parse();
        assert.equal(vtile.getData().length,544);
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.isSolid(), false);
        assert.equal(vtile.empty(), false);
        var map = new mapnik.Map(vtile.width(),vtile.height());
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
        assert.throws(function() { map.render(vtile, {tolerance:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {path_multiplier:null}, function(err, vtile) {}); });
        assert.throws(function() { map.render(vtile, {variables:null}, function(err, vtile) {}); });
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
            assert.equal(vtile.isSolid(), false);
            fs.writeFileSync('./test/data/vector_tile/tile0.vector.pbf', vtile.getData());
            done();
        });
    });

    it('should read back the vector tile and render an image with it', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.vector.pbf'));
        vtile.parse();
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        assert.equal(vtile.isSolid(), false);

        vtile.render(map, new mapnik.Image(256, 256), function(err, vtile_image) {
            if (err) throw err;
            //var actual = './test/data/vector_tile/tile0.actual.png';
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
            var vtile = new mapnik.VectorTile(0, 0, 0);
            vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.vector.pbf'));
            vtile.parse();
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/stylesheet.xml');
            map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
            var surface2 = new mapnik.CairoSurface('svg',vtile.width(),vtile.height());
            vtile.render(map, surface2, {renderer:'svg'}, function(err,surface2) {
                if (err) throw err;
                var actual_svg2 = './test/data/vector_tile/tile0.actual-svg.svg';
                var expected_svg2 = './test/data/vector_tile/tile0.expected-svg.svg';
                if (!existsSync(expected_svg2) || process.env.UPDATE) {
                    fs.writeFileSync(expected_svg2,surface2.getData());
                }
                fs.writeFileSync(actual_svg2,surface2.getData());
                var diff = Math.abs(fs.readFileSync(actual_svg2,'utf8').replace(/\r/g, '').length - fs.readFileSync(expected_svg2,'utf8').replace(/\r/g, '').length);
                assert.ok(diff < 10,"svg diff "+diff+" not less that 10");
                done();
            });
        } else {
            done();
        }
    });

    it('should read back the vector tile and render a cairo svg with it', function(done) {
        if (mapnik.supports.cairo) {
            var vtile = new mapnik.VectorTile(0, 0, 0);
            vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.vector.pbf'));
            vtile.parse();
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/stylesheet.xml');
            map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

            assert.equal(vtile.isSolid(), false);
            var surface = new mapnik.CairoSurface('svg',vtile.width(),vtile.height());
            vtile.render(map, surface, {renderer:'cairo'}, function(err,surface) {
                if (err) throw err;
                var actual_svg = './test/data/vector_tile/tile0.actual-cairo.svg';
                var expected_svg = './test/data/vector_tile/tile0.expected-cairo.svg';
                if (!existsSync(expected_svg) || process.env.UPDATE) {
                    fs.writeFileSync(expected_svg,surface.getData());
                }
                fs.writeFileSync(actual_svg,surface.getData(),'utf-8');
                var diff = Math.abs(fs.readFileSync(actual_svg,'utf8').replace(/\r/g, '').length - fs.readFileSync(expected_svg,'utf8').replace(/\r/g, '').length);
                assert.ok(diff < 20,"svg diff "+diff+" not less that 20");
                done();
            });            
        } else {
            done();
        }
    });

    it('should read back the vector tile and render an image with it using negative buffer', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.vector.pbf'));
        vtile.parse();
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        assert.equal(vtile.isSolid(), false);

        vtile.render(map, new mapnik.Image(256, 256), {buffer_size:-64}, function(err, vtile_image) {
            if (err) throw err;
            //var actual = './test/data/vector_tile/tile0-b.actual.png';
            var expected = './test/data/vector_tile/tile0-b.expected.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                vtile_image.save(expected, 'png32');
            }
            assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
            done();
        });
    });

    it('should read back the vector tile and render a grid with it', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.vector.pbf'));
        vtile.parse();
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        assert.equal(vtile.isSolid(), false);

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

    it('should read back the vector tile and render an image with markers', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        vtile.setData(fs.readFileSync('./test/data/vector_tile/tile0.vector.pbf'));
        vtile.parse();
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/markers.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        assert.equal(vtile.isSolid(), false);

        vtile.render(map, new mapnik.Image(256, 256), {buffer_size:-64}, function(err, vtile_image) {
            if (err) throw err;
            //var actual = './test/data/vector_tile/tile0-c.actual.png';
            var expected = './test/data/vector_tile/tile0-c.expected.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                vtile_image.save(expected, 'png32');
            }
            // TODO - visual difference in master vs 2.3.x due to https://github.com/mapnik/mapnik/commit/ecc5acbdb953e172fcc652b55ed19b8b581e2146
            assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
            done();
        });
    });

    it('should be able to resample and encode (render) a geotiff into vector tile', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
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
            assert.equal(vtile.isSolid(), false);
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
                if (process.platform === 'win32') {
                    // TODO - JPEG colors differ slightly on windows
                    // version difference perhaps?
                    assert.ok(diff < 3000,"jpeg raster diff "+diff+" not less that 3000");
                } else {
                    assert.equal(0,diff);
                }
                done();
            });
        });
    });

    it('should be able to push an image tile directly into a vector tile layer without decoding', function(done) {
        var vtile = new mapnik.VectorTile(1, 0, 0);
        var image_buffer = fs.readFileSync('./test/data/vector_tile/cloudless_1_0_0.jpg');
        // push image into a named vtile layer
        vtile.addImage(image_buffer,'raster');
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.empty(), false);
        assert.equal(vtile.isSolid(), false);
        assert.deepEqual(vtile.names(),['raster']);
        var json_obj = vtile.toJSON();
        assert.equal(json_obj[0].name,'raster');
        assert.equal(json_obj[0].features[0].raster.length,12146);
        // now render out with fancy styling
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/raster_style.xml');
        vtile.render(map, new mapnik.Image(256, 256), {buffer_size:256}, function(err, vtile_image) {
            if (err) throw err;
            //var actual = './test/data/vector_tile/tile-raster2.actual.png';
            var expected = './test/data/vector_tile/tile-raster2.expected.png';
            if (!existsSync(expected) || process.env.UPDATE) {
                vtile_image.save(expected, 'png32');
            }
            //vtile_image.save(actual, 'png32');
            // TODO: NON-visual differences on windows - why?
            if (process.platform === 'win32') {
                assert.ok(vtile_image.compare(new mapnik.Image.open(expected)) < 52);
            } else {
                assert.equal(0,vtile_image.compare(new mapnik.Image.open(expected)));
            }
            done();
        });
    });

    it('should include image in getData pbf output', function(done) {
        var vtile = new mapnik.VectorTile(1, 0, 0);
        var image_buffer = fs.readFileSync('./test/data/vector_tile/cloudless_1_0_0.jpg');
        // push image into a named vtile layer
        vtile.addImage(image_buffer,'raster');
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.empty(), false);
        assert.equal(vtile.isSolid(), false);
        assert.deepEqual(vtile.names(),['raster']);
        var json_obj = vtile.toJSON();
        assert.equal(json_obj[0].name,'raster');
        assert.equal(json_obj[0].features[0].raster.length,12146);
        // getData from the image vtile
        var vtile2 = new mapnik.VectorTile(1, 0, 0);
        vtile2.setData(vtile.getData());
        vtile2.parse();
        var json_obj2 = vtile2.toJSON();
        assert.deepEqual(json_obj, json_obj2);
        done();
    });
});
