var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var inflate = require('zlib').inflate;
var mercator = new(require('sphericalmercator'));

var overwrite_expected_data = false;

describe('mapnik.VectorTile ', function() {
    // generate test data
    var _dt;
    var _data;
    var _length;
    before(function(done) {
        if (overwrite_expected_data) {
            var map = new mapnik.Map(256, 256);
            map.loadSync('./test/data/vector_tile/layers.xml');
            var dt = new mapnik.VectorTile(9,112,195);
            _dt = dt;
            map.extent = [-11271098.442818949,4696291.017841229,-11192826.925854929,4774562.534805249];
            map.render(dt,{},function(err,dt) {
                if (err) throw err;
                fs.writeFileSync('./test/data/vector_tile/tile1.vector.pbf',dt.getData());
                _data = dt.getData().toString("hex");
                _length = dt.getData().length;
                var map2 = new mapnik.Map(256,256);
                map2.loadSync('./test/data/vector_tile/layers.xml');
                var dt2 = new mapnik.VectorTile(5,28,12);
                var bbox = mercator.bbox(28, 12, 5, false, '900913');
                map2.extent = bbox;
                map2.render(dt2,{}, function(err,dt2) {
                    if (err) throw err;
                    fs.writeFileSync("./test/data/vector_tile/tile3.vector.pbf",dt2.getData());
                    done();
                });
            });
        } else {
            _dt = new mapnik.VectorTile(9,112,195);
            _dt.setData(fs.readFileSync('./test/data/vector_tile/tile1.vector.pbf'));
            _data = _dt.getData().toString("hex");
            _length = _dt.getData().length;
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
        var dt = new mapnik.VectorTile(0,0,0);
        assert.ok(dt instanceof mapnik.VectorTile);
        assert.equal(dt.width(), 256);
        assert.equal(dt.height(), 256);
        assert.equal(dt.painted(), false);
        assert.equal(dt.getData().toString(),"");
        assert.equal(dt.isSolid(), "");
        dt.isSolid(function(err, solid, key) {
            if (err) throw err;
            assert.equal(solid, true);
            assert.equal(key, "");
            done();
        });
    });

    it('should be able to set data (sync)', function(done) {
        var dt = new mapnik.VectorTile(9,112,195);
        // tile1 represents a "solid" data tile with one layer
        // that only encodes a single feature with a single path with
        // a polygon box resulting from clipping a chunk out of 
        // a larger polygon fully outside the rendered/clipping extent
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        dt.setData(data);

        assert.equal(dt.painted(), true);
        assert.equal(dt.isSolid(), "world");
        dt.isSolid(function(err, solid, key) {
            if (err) throw err;
            assert.equal(solid, true);
            assert.equal(key, "world");
            done();
        });
    });

    it('should error out if we pass invalid data to setData', function(done) {
        var dt = new mapnik.VectorTile(0,0,0);
        assert.throws(function() { dt.setData('foo'); }); // first arg must be a buffer object
        assert.throws(function() { dt.setData('foo',function(){}); }); // first arg must be a buffer object
        assert.throws(function() { dt.setData(new Buffer('foo')) });
        dt.setData(new Buffer('foo'),function(err,success) {
            assert.ok(err);
            done();
        })
    });

    it('should be able to set data (async)', function(done) {
        var dt = new mapnik.VectorTile(9,112,195);
        var data = fs.readFileSync("./test/data/vector_tile/tile1.vector.pbf");
        dt.setData(data, function(err) {
            if (err) throw err;
            assert.equal(dt.painted(), true);
            assert.equal(dt.isSolid(), "world");
            dt.isSolid(function(err, solid, key) {
                if (err) throw err;
                assert.equal(solid, true);
                assert.equal(key, "world");
                done();
            });
        });
    });

    it('should be able to get tile info as JSON', function(done) {
        var dt = new mapnik.VectorTile(9,112,195);
        dt.setData(new Buffer(_data,"hex"));
        assert.deepEqual(dt.names(),['world'])
        var expected = [{"name":"world","extent":4096,"version":2,"features":[{"id":207,"type":3,"geometry":[9,0,0,26,0,8190,8190,0,0,8189,15],"properties":{"AREA":915896,"FIPS":"US","ISO2":"US","ISO3":"USA","LAT":39.622,"LON":-98.606,"NAME":"United States","POP2005":299846449,"REGION":19,"SUBREGION":21,"UN":840}}]}];
        assert.deepEqual(dt.toJSON(),expected)
        done();
    });

    it('should be able to get tile info as various flavors of GeoJSON', function(done) {
        var dt = new mapnik.VectorTile(9,112,195);
        dt.setData(new Buffer(_data,"hex"));
        var expected_geojson = {"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Polygon","coordinates":[[[-101.25,39.36827914916011],[-101.25,38.82272471585834],[-100.54704666137694,38.82272471585834],[-100.54704666137694,39.36827914916011],[-101.25,39.36827914916011]]]},"properties":{"AREA":915896,"FIPS":"US","ISO2":"US","ISO3":"USA","LAT":39.622,"LON":-98.606,"NAME":"United States","POP2005":299846449,"REGION":19,"SUBREGION":21,"UN":840}}]};
        var expected_copy = JSON.parse(JSON.stringify(expected_geojson));
        expected_geojson.name = "world";
        assert.deepEqual(dt.toGeoJSON(0),expected_geojson)
        assert.deepEqual(dt.toGeoJSON(0),dt.toGeoJSON('world'))
        assert.deepEqual(dt.toGeoJSON('__all__'),expected_copy)
        assert.deepEqual(dt.toGeoJSON('__array__'),[expected_geojson])
        done();
    });

    it('should be able to get reduce the precision of GeoJSON coordinates', function(done) {
        var dt = new mapnik.VectorTile(9,112,195);
        dt.setData(new Buffer(_data,"hex"));
        var expected_geojson = {"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Polygon","coordinates":[[[-101.25,39.36827914916011],[-101.25,38.82272471585834],[-100.54704666137694,38.82272471585834],[-100.54704666137694,39.36827914916011],[-101.25,39.36827914916011]]]},"properties":{"AREA":915896,"FIPS":"US","ISO2":"US","ISO3":"USA","LAT":39.622,"LON":-98.606,"NAME":"United States","POP2005":299846449,"REGION":19,"SUBREGION":21,"UN":840}}],"name":"world"};
        var trunc = function(key, val) {
            return val.toFixed ? Number(val.toFixed(2)) : val;
        }
        var expected_string = JSON.stringify(expected_geojson,trunc);
        assert.deepEqual(JSON.stringify(dt.toGeoJSON(0),trunc),expected_string)
        done();
    });

    it('should be able to get and set data', function(done) {
        var dt = new mapnik.VectorTile(9,112,195);
        dt.setData(new Buffer(_data,"hex"));
        var dt2 = new mapnik.VectorTile(9,112,195);
        dt2.setData(dt.getData());
        assert.deepEqual(dt.names(),dt2.names())
        assert.deepEqual(dt.toJSON(),dt2.toJSON())
        assert.deepEqual(dt2.toJSON(),_dt.toJSON())
        done();
    });

    it('should be able to get virtual datasource and features', function(done) {
        var dt = new mapnik.VectorTile(9,112,195);
        dt.setData(new Buffer(_data,"hex"));
        assert.equal(dt.getData().length,_length);
        var ds = dt.toJSON();
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
        var dt = new mapnik.VectorTile(9,112,195);
        dt.setData(new Buffer(_data,"hex"));
        assert.equal(dt.getData().length,_length);
        assert.equal(dt.painted(), true);
        assert.equal(dt.isSolid(), "world");
        var feature_count = dt.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        dt.clear();
        assert.equal(dt.getData().length,0);
        assert.deepEqual(dt.toJSON(), {});
        assert.equal(dt.painted(), false);
        assert.equal(dt.isSolid(), false);
        done();
    });

    it('should be able to clear data (async)', function(done) {
        var dt = new mapnik.VectorTile(9,112,195);
        dt.setData(new Buffer(_data,"hex"));
        assert.equal(dt.getData().length,_length);
        assert.equal(dt.painted(), true);
        assert.equal(dt.isSolid(), "world");
        var feature_count = dt.toJSON()[0].features.length;
        assert.equal(feature_count, 1);
        dt.clear(function(err) {
            if (err) throw err;
            assert.equal(dt.getData().length,0);
            assert.deepEqual(dt.toJSON(), {});
            assert.equal(dt.painted(), false);
            assert.equal(dt.isSolid(), false);
            done();
        });
    });


    it('should detect as solid a tile with two "box" layers', function(done) {
        var dt = new mapnik.VectorTile(9,112,195);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/data/vector_tile/layers.xml');
        map.extent = [-11271098.442818949,4696291.017841229,-11192826.925854929,4774562.534805249];
        map.render(dt,{},function(err,dt) {
            // this tile duplicates the layer that is present in tile1.vector.pbf
            fs.writeFileSync('./test/data/vector_tile/tile2.vector.pbf',dt.getData());
            _data = dt.getData().toString("hex");
            var dt2 = new mapnik.VectorTile(9,112,195);
            var raw_data = fs.readFileSync("./test/data/vector_tile/tile2.vector.pbf");
            dt2.setData(raw_data);
            assert.equal(dt.painted(), true);
            assert.equal(dt.isSolid(), "world-world2");
            assert.equal(dt.getData().toString("hex"),raw_data.toString("hex"));
            dt.isSolid(function(err, solid, key) {
                if (err) throw err;
                assert.equal(solid, true);
                assert.equal(key, "world-world2");
                done();
            });
        });
    });

    it('should render expected results', function(done) {
        var data = fs.readFileSync("./test/data/vector_tile/tile3.vector.pbf");
        var dt = new mapnik.VectorTile(5,28,12);
        dt.setData(data);
        assert.equal(dt.getData().length,544);
        assert.equal(dt.painted(), true);
        assert.equal(dt.isSolid(), false);
        var map = new mapnik.Map(dt.width(),dt.height());
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];
        dt.render(map, new mapnik.Image(256,256), function(err,image) {
            if (err) throw err;
            image.save('./test/data/vector_tile/tile3.actual.png');
            var e = fs.readFileSync('./test/data/vector_tile/tile3.expected.png');
            var a = fs.readFileSync('./test/data/vector_tile/tile3.actual.png');
            assert.equal(e.length,a.length)
            done();
        });
    });

    it('should render a vector_tile of the whole world', function(done) {
        var dt = new mapnik.VectorTile(0, 0, 0);
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        var png = map.renderSync('png', new mapnik.Image(256, 256), {});
        fs.writeFileSync('./test/data/vector_tile/tile0.expected.png', png);

        map.render(dt, {}, function(err, dt) {
            if (err) throw err;
            assert.equal(dt.isSolid(), false);
            fs.writeFileSync('./test/data/vector_tile/tile0.vector.pbf', dt.getData());
            done();
        });
    });

    it('should read back the data tile and render an image with it', function(done) {
        var dt = new mapnik.VectorTile(0, 0, 0);
        dt.setData(fs.readFileSync('./test/data/vector_tile/tile0.vector.pbf'));
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        assert.equal(dt.isSolid(), false);

        dt.render(map, new mapnik.Image(256, 256), function(err, dt_image) {
            if (err) throw err;
            dt_image.save('./test/data/vector_tile/tile0.actual.png');
            done();
        });
    });

    it('should read back the data tile and render a grid with it', function(done) {
        var dt = new mapnik.VectorTile(0, 0, 0);
        dt.setData(fs.readFileSync('./test/data/vector_tile/tile0.vector.pbf'));
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.extent = [-20037508.34, -20037508.34, 20037508.34, 20037508.34];

        assert.equal(dt.isSolid(), false);

        dt.render(map, new mapnik.Grid(256, 256), {layer:0}, function(err, dt_image) {
            if (err) throw err;
            var utf = dt_image.encodeSync('utf');
            //fs.writeFileSync('./test/data/vector_tile/tile0.actual.grid.json',JSON.stringify(utf));
            var expected = JSON.parse(fs.readFileSync('./test/data/vector_tile/tile0.actual.grid.json'));
            assert.deepEqual(utf,expected)
            done();
        });
    });

});
