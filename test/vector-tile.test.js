var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var mercator = new(require('sphericalmercator'));
var existsSync = require('fs').existsSync || require('path').existsSync;
var overwrite_expected_data = false;

var trunc_6 = function(key, val) {
    return val.toFixed ? Number(val.toFixed(6)) : val;
}

function deepEqualTrunc(json1,json2) {
    return assert.deepEqual(JSON.stringify(json1,trunc_6),JSON.stringify(json2,trunc_6));
}

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));

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

    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.VectorTile(); });

        // invalid args
        assert.throws(function() { new mapnik.VectorTile(); });
        assert.throws(function() { new mapnik.VectorTile(1); });
        assert.throws(function() { new mapnik.VectorTile('foo'); });
        assert.throws(function() { new mapnik.VectorTile('a', 'b', 'c'); });
    });

    it('should be initialized properly', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.ok(vtile instanceof mapnik.VectorTile);
        assert.equal(vtile.width(), 256);
        assert.equal(vtile.height(), 256);
        assert.equal(vtile.painted(), false);
        assert.equal(vtile.getData().toString(),"");
        assert.equal(vtile.isSolid(), "");
        vtile.isSolid(function(err, solid, key) {
            if (err) throw err;
            assert.equal(solid, true);
            assert.equal(key, "");
            done();
        });
    });

    it('should be able to setData/parse (sync)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        // tile1 represents a "solid" vector tile with one layer
        // that only encodes a single feature with a single path with
        // a polygon box resulting from clipping a chunk out of 
        // a larger polygon fully outside the rendered/clipping extent
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data);
        vtile.parse();
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.isSolid(), "world");
        vtile.isSolid(function(err, solid, key) {
            if (err) throw err;
            assert.equal(solid, true);
            assert.equal(key, "world");
            done();
        });
    });

    it('should error out if we pass invalid data to setData/addData', function(done) {
        var vtile = new mapnik.VectorTile(0,0,0);
        assert.throws(function() { vtile.setData('foo'); }); // first arg must be a buffer object
        assert.throws(function() { vtile.setData('foo',function(){}); }); // first arg must be a buffer object
        assert.throws(function() { vtile.setData(new Buffer('foo'));vtile.parse(); });
        assert.throws(function() { vtile.setData(new Buffer(0)); }); // empty buffer is not valid
        assert.throws(function() { vtile.addData(new Buffer(0)); }); // empty buffer is not valid
        vtile.setData(new Buffer('foo'),function(err,success) {
            if (err) throw err;
            vtile.parse(function(err) {
                assert.ok(err);
                done();
            });
        })
    });

    it('should be able to setData/parse (async)', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        vtile.setData(data, function(err) {
            if (err) throw err;
            vtile.parse(function(err) {
                if (err) throw err;
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
            vtile.parse()
            assert.deepEqual(vtile.names(), ["world"]);
            done();
        });
    });


    it('should be able to get tile info as JSON', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        vtile.parse();
        assert.deepEqual(vtile.names(),['world'])
        var expected = [{"name":"world","extent":4096,"version":2,"features":[{"id":207,"type":3,"geometry":[9,0,0,26,0,8190,8190,0,0,8189,15],"properties":{"AREA":915896,"FIPS":"US","ISO2":"US","ISO3":"USA","LAT":39.622,"LON":-98.606,"NAME":"United States","POP2005":299846449,"REGION":19,"SUBREGION":21,"UN":840}}]}];
        assert.deepEqual(vtile.toJSON(),expected)
        done();
    });

    it('should be able to get tile info as various flavors of GeoJSON', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        vtile.parse();
        var expected_geojson = {"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Polygon","coordinates":[[[-101.25,39.36827914916011],[-101.25,38.82272471585834],[-100.54704666137694,38.82272471585834],[-100.54704666137694,39.36827914916011],[-101.25,39.36827914916011]]]},"properties":{"AREA":915896,"FIPS":"US","ISO2":"US","ISO3":"USA","LAT":39.622,"LON":-98.606,"NAME":"United States","POP2005":299846449,"REGION":19,"SUBREGION":21,"UN":840}}]};
        var expected_copy = JSON.parse(JSON.stringify(expected_geojson));
        expected_geojson.name = "world";
        deepEqualTrunc(vtile.toGeoJSON(0),expected_geojson)
        deepEqualTrunc(vtile.toGeoJSON(0),vtile.toGeoJSON('world'))
        deepEqualTrunc(vtile.toGeoJSON('__all__'),expected_copy)
        deepEqualTrunc(vtile.toGeoJSON('__array__'),[expected_geojson])
        done();
    });

    it('should be able to get and set data', function(done) {
        var vtile = new mapnik.VectorTile(9,112,195);
        vtile.setData(new Buffer(_data,"hex"));
        vtile.parse();
        var vtile2 = new mapnik.VectorTile(9,112,195);
        vtile2.setData(vtile.getData());
        vtile2.parse();
        assert.deepEqual(vtile.names(),vtile2.names())
        assert.deepEqual(vtile.toJSON(),vtile2.toJSON())
        assert.deepEqual(vtile2.toJSON(),_vtile.toJSON())
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
        vtile.parse();
        assert.equal(vtile.getData().length,_length);
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.isSolid(), "world");
        var feature_count = vtile.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        vtile.clear(function(err) {
            if (err) throw err;
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
        vtile.parse();
        assert.equal(vtile.getData().length,_length*2);
        assert.equal(vtile.painted(), true);
        assert.deepEqual(vtile.names(), ["world","world"]);
        assert.equal(vtile.isSolid(), "world-world");
        var feature_count = vtile.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        vtile.clear(function(err) {
            if (err) throw err;
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

    it('should render expected results', function(done) {
        var data = fs.readFileSync("./test/data/vector_tile/tile3.vector.pbf");
        var vtile = new mapnik.VectorTile(5,28,12);
        vtile.setData(data);
        vtile.parse();
        assert.equal(vtile.getData().length,544);
        assert.equal(vtile.painted(), true);
        assert.equal(vtile.isSolid(), false);
        var map = new mapnik.Map(vtile.width(),vtile.height());
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        vtile.render(map, new mapnik.Image(256,256), function(err,image) {
            if (err) throw err;
            var expected = './test/data/vector_tile/tile3.expected.png';
            var actual = './test/data/vector_tile/tile3.actual.png';
            if (!existsSync(expected)) {
                image.save(expected, 'png32');
            }
            image.save(actual, 'png32');
            var e = fs.readFileSync(expected);
            var a = fs.readFileSync(actual);
            if (mapnik.versions.mapnik_number >= 300000) {
                assert.ok(Math.abs(e.length - a.length) < 100);
            } else {
                assert.equal(e.length,a.length)
            }
            done();
        });
    });

    it('should render a vector_tile of the whole world', function(done) {
        var vtile = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        //var png = map.renderSync('png', new mapnik.Image(256, 256), {});
        //fs.writeFileSync('./test/data/vector_tile/tile0.expected.png', png);

        map.render(vtile, {}, function(err, vtile) {
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
            var actual = './test/data/vector_tile/tile0.actual.png';
            var expected = './test/data/vector_tile/tile0.expected.png';
            if (!existsSync(expected)) {
                vtile_image.save(expected, 'png32');
            }
            vtile_image.save(actual, 'png32');
            var a = fs.readFileSync(actual);
            var e = fs.readFileSync(expected)
            assert.ok(Math.abs(e.length - a.length) < 100);
            done();
        });
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
            var actual = './test/data/vector_tile/tile0-b.actual.png';
            var expected = './test/data/vector_tile/tile0-b.expected.png';
            if (!existsSync(expected)) {
                vtile_image.save(expected, 'png32');
            }
            vtile_image.save(actual, 'png32');
            var a = fs.readFileSync(actual);
            var e = fs.readFileSync(expected)
            assert.ok(Math.abs(e.length - a.length) < 100);
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
            var utf = vtile_image.encodeSync('utf');
            var expected_file = './test/data/vector_tile/tile0.expected.grid.json';
            var actual_file = './test/data/vector_tile/tile0.actual.grid.json';
            if (!existsSync(expected_file)) {
                fs.writeFileSync(expected_file,JSON.stringify(utf,null,1));
            }
            fs.writeFileSync(actual_file,JSON.stringify(utf,null,1));
            var expected = JSON.parse(fs.readFileSync(expected_file));
            assert.deepEqual(utf,expected)
            done();
        });
    });

    it('should be able to query features from vector tile', function(done) {
        var data = fs.readFileSync("./test/data/vector_tile/tile3.vector.pbf");
        var vtile = new mapnik.VectorTile(5,28,12);
        vtile.setData(data);
        vtile.parse();
        var features = vtile.query(139.6142578125,37.17782559332976,{tolerance:0});
        assert.equal(features.length,1);
        assert.equal(JSON.parse(features[0].toJSON()).properties.NAME,'Japan');
        assert.equal(features[0].id(),89);
        // tolerance only applies to points and lines currently in mapnik::hit_test
        var features = vtile.query(142.3388671875,39.52099229357195,{tolerance:100000000000000});
        assert.equal(features.length,0);
        // restrict to single layer
        // first query one that does not exist
        var features = vtile.query(139.6142578125,37.17782559332976,{tolerance:0,layer:'doesnotexist'});
        assert.equal(features.length,0);
        // query one that does exist
        var features = vtile.query(139.6142578125,37.17782559332976,{tolerance:0,layer:vtile.names()[0]});
        assert.equal(features.length,1);
        assert.equal(features[0].id(),89);
        // ensure querying clipped polygons works
        var pbf = require('fs').readFileSync('./test/data/vector_tile/6.20.34.pbf');
        var vt = new mapnik.VectorTile(6, 20, 34);
        vt.setData(pbf,function(err) {
            if (err) throw err;
            vt.parse();
            var json = vt.toJSON();
            assert.equal(2, json[0].features.length);
            assert.equal('Brazil', json[0].features[0].properties.name);
            assert.equal('Bolivia', json[0].features[1].properties.name);
            var results = vt.query(-64.27521952641217,-16.28853953000943,{tolerance:10})
            assert.equal(1, results.length);
            var feat_json = JSON.parse(results[0].toJSON());
            assert.equal('Bolivia',feat_json.properties.name);
            assert.equal(86,feat_json.id);
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
            var actual = './test/data/vector_tile/tile0-c.actual.png';
            var expected = './test/data/vector_tile/tile0-c.expected.png';
            if (!existsSync(expected)) {
                vtile_image.save(expected, 'png32');
            }
            vtile_image.save(actual, 'png32');
            var a = fs.readFileSync(actual);
            var e = fs.readFileSync(expected)
            assert.ok(Math.abs(e.length - a.length) < 100);
            done();
        });
    });
});
