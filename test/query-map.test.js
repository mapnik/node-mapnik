var mapnik = require('../');
var assert = require('assert');

describe('mapnik.queryPoint', function() {
    it('should throw with invalid usage', function() {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        assert.throws(function() { map.queryPoint(); });
        assert.throws(function() { map.queryPoint(0, 0, 0); });
        assert.throws(function() { map.queryPoint(0, 0, {},0); });
        assert.throws(function() { map.queryPoint(0, 0, 0, 0); });
    });

    it('should return a feature if geo coords are used', function(done) {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        map.queryPoint(-12957605.0331, 5518141.9452, {layer: 0}, function(err, results) {
            assert.equal(results.length, 1);
            var result = results[0];
            assert.equal(result.layer, 'world');
            var fs = result.featureset;
            assert.ok(fs);
            var feat = fs.next();
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
            assert.deepEqual(feat.attributes(), expected);
            // no more features
            assert.ok(!fs.next());
            done();
        });
    });

    it('should return a feature if screen coords are used', function(done) {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        map.queryMapPoint(55, 130, {layer: 0}, function(err, results) {
            assert.equal(results.length, 1);
            var result = results[0];
            assert.equal(result.layer, 'world');
            var fs = result.featureset;
            assert.ok(fs);
            var feat = fs.next();
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
            assert.deepEqual(feat.attributes(), expected);
            // no more features
            assert.ok(!fs.next());
            done();
        });
    });

    it('should return a feature if multiple layers are queried', function(done) {
        var map = new mapnik.Map(256, 256);
        map.loadSync('./test/stylesheet.xml');
        map.zoomAll();
        map.queryPoint(-12957605.0331, 5518141.9452, {/*will query all layers*/}, function(err, results) {
            assert.equal(results.length, 1);
            var result = results[0];
            assert.equal(result.layer, 'world');
            var fs = result.featureset;
            assert.ok(fs);
            var feat = fs.next();
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
            assert.deepEqual(feat.attributes(), expected);
            // no more features
            assert.ok(!fs.next());
            done();
        });
    });

    it('should return a feature if multiple layers are queried', function(done) {
        var map = new mapnik.Map(256, 256);
        var layer = new mapnik.Layer('world');
        layer.srs = map.srs;
        var options = {
            type: 'shape',
            file: './test/data/world_merc.shp'
        };
        layer.datasource = new mapnik.Datasource(options);
        map.add_layer(layer);
        var layer2 = new mapnik.Layer('world2');
        layer2.srs = map.srs;
        layer2.datasource = new mapnik.Datasource(options);
        map.add_layer(layer2);
        map.zoomAll();
        map.queryPoint(-12957605.0331, 5518141.9452, {/*will query all layers*/}, function(err, results) {
            assert.equal(results.length, 2);

            // first layer hit
            var result = results[0];
            assert.equal(result.layer, 'world');
            var fs = result.featureset;
            assert.ok(fs);
            var feat = fs.next();
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
            assert.deepEqual(feat.attributes(), expected);
            // no more features
            assert.ok(!fs.next());

            // second layer hit
            result = results[1];
            assert.equal(result.layer, 'world2');
            fs = result.featureset;
            assert.ok(fs);
            feat = fs.next();
            assert.deepEqual(feat.attributes(), expected);
            // no more features
            assert.ok(!fs.next());
            done();
        });
    });
});
