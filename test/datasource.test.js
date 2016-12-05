"use strict";

var mapnik = require('../');
var assert = require('assert');
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'ogr.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'gdal.input'));

describe('mapnik.Datasource', function() {
    
    it('should throw with invalid usage', function() {
        assert.throws(function() { mapnik.Datasource('foo'); });
        assert.throws(function() { mapnik.Datasource({ 'foo': 1 }); });
        assert.throws(function() { mapnik.Datasource({ 'type': 'foo' }); });
        assert.throws(function() { mapnik.Datasource({ 'type': 'shape' }); });

        assert.throws(function() { new mapnik.Datasource('foo'); },
            /Must provide an object, eg \{type: 'shape', file : 'world.shp'\}/);

        assert.throws(function() { new mapnik.Datasource(); });
        assert.throws(function() { new mapnik.Datasource({ 'foo': 1 }); });

        assert.throws(function() { new mapnik.Datasource({ 'type': 'foo' }); });

        assert.throws(function() { new mapnik.Datasource({ 'type': 'shape' }); },
            /Shape Plugin: missing <file> parameter/);
    });

    it('should validate with known shapefile - ogr', function() {
        var options = {
            type: 'ogr',
            file: './test/data/world_merc.shp',
            layer: 'world_merc'
        };

        var ds = new mapnik.Datasource(options);
        assert.ok(ds);
        assert.deepEqual(ds.parameters(), options);

        var features = [];
        var featureset = ds.featureset();
        var feature;
        while ((feature = featureset.next())) {
            features.push(feature);
        }

        assert.equal(features.length, 245);
        assert.deepEqual(features[244].attributes(), {
            AREA: 1638094,
            FIPS: 'RS',
            ISO2: 'RU',
            ISO3: 'RUS',
            LAT: 61.988,
            LON: 96.689,
            NAME: 'Russia',
            POP2005: 143953092,
            REGION: 150,
            SUBREGION: 151,
            UN: 643
        });

        var expected = {
            type: 'vector',
            extent: [
                -20037508.342789248,
                -8283343.693882697,
                20037508.342789244,
                18365151.363070473
            ],
            encoding: 'utf-8',
            fields: {
                FIPS: 'String',
                ISO2: 'String',
                ISO3: 'String',
                UN: 'Number',
                NAME: 'String',
                AREA: 'Number',
                POP2005: 'Number',
                REGION: 'Number',
                SUBREGION: 'Number',
                LON: 'Number',
                LAT: 'Number'
            },
            geometry_type: 'polygon',
            proj4:'+proj=merc +lon_0=0 +lat_ts=0 +x_0=0 +y_0=0 +datum=WGS84 +units=m +no_defs'
        };
        var actual = ds.describe();
        assert.equal(actual.proj4, expected.proj4);
        assert.deepEqual(actual.type, expected.type);
        assert.deepEqual(actual.encoding, expected.encoding);
        assert.deepEqual(actual.fields, expected.fields);
        assert.deepEqual(actual.geometry_type, expected.geometry_type);

        assert.deepEqual(ds.extent(), expected.extent);

        assert.deepEqual(ds.fields(), expected.fields);
    });

    it('should validate with known shapefile', function() {
        var options = {
            type: 'shape',
            file: './test/data/world_merc.shp'
        };

        var ds = new mapnik.Datasource(options);
        assert.ok(ds);
        assert.deepEqual(ds.parameters(), options);

        var features = [];
        var featureset = ds.featureset();
        var feature;
        while ((feature = featureset.next())) {
            features.push(feature);
        }

        assert.equal(features.length, 245);
        assert.deepEqual(features[244].attributes(), {
            AREA: 1638094,
            FIPS: 'RS',
            ISO2: 'RU',
            ISO3: 'RUS',
            LAT: 61.988,
            LON: 96.689,
            NAME: 'Russia',
            POP2005: 143953092,
            REGION: 150,
            SUBREGION: 151,
            UN: 643
        });

        var expected = {
            type: 'vector',
            extent: [
                -20037508.342789248,
                -8283343.693882697,
                20037508.342789244,
                18365151.363070473
            ],
            encoding: 'utf-8',
            fields: {
                FIPS: 'String',
                ISO2: 'String',
                ISO3: 'String',
                UN: 'Number',
                NAME: 'String',
                AREA: 'Number',
                POP2005: 'Number',
                REGION: 'Number',
                SUBREGION: 'Number',
                LON: 'Number',
                LAT: 'Number'
            },
            geometry_type: 'polygon'
        };
        var actual = ds.describe();
        assert.deepEqual(actual.type, expected.type);
        assert.deepEqual(actual.encoding, expected.encoding);
        assert.deepEqual(actual.fields, expected.fields);
        assert.deepEqual(actual.geometry_type, expected.geometry_type);

        assert.deepEqual(ds.extent(), expected.extent);

        assert.deepEqual(ds.fields(), expected.fields);
    });
    
    it('test invalid use of memory datasource', function() {
        var ds = new mapnik.MemoryDatasource({'extent': '-180,-90,180,90'});
        assert.throws(function() { ds.add(); });
        assert.throws(function() { ds.add(null); });
        assert.throws(function() { ds.add({}, null); });
        assert.throws(function() { ds.add({'wkt': '1234'}); });
        assert.equal(false, ds.add({}));
    });

    it('test empty memory datasource', function() {
        var ds = new mapnik.MemoryDatasource({'extent': '-180,-90,180,90'});
        var empty_fs = ds.featureset();
        assert.equal(typeof(empty_fs),'undefined');
        assert.equal(empty_fs, null);
    });

    it('test empty geojson datasource', function() {
        var input = {
          "type": "Feature",
          "properties": {
            "something": []
          },
          "geometry": {
            "type": "Point",
            "coordinates": [ 1, 1 ]
          }
        };
        var ds = new mapnik.Datasource({ type:'geojson', inline: JSON.stringify(input) });
        var fs = ds.featureset()
        var feat = fs.next();
        var feature = JSON.parse(feat.toJSON());
        // pass invalid extent to filter all features out
        // resulting in empty featureset that should be returned
        // as a null object
        var empty_fs = ds.featureset({extent:[-1,-1,0,0]});
        assert.equal(typeof(empty_fs),'undefined');
        assert.equal(empty_fs, null);
    });

    it('test empty geojson datasource due to invalid json string', function() {
        var input = "{ \"type\": \"FeatureCollection\", \"features\": [{ \"oofda\" } ] }";
        // from string will fail to parse
        assert.throws(function() { new mapnik.Datasource({ type:'geojson', inline: inline, cache_features: false }); });
        assert.throws(function() { new mapnik.Datasource({ type:'geojson', inline: fs.readFileSync('./test/data/parse.error.json').toString(), cache_features: false }); });
    });

    it('test empty geojson datasource due to invalid json file', function() {
        var ds = new mapnik.Datasource({ type:'geojson', file: './test/data/parse.error.json', cache_features: false });
        var empty_fs = ds.featureset();
        assert.equal(typeof(empty_fs),'undefined');
        assert.equal(empty_fs, null);
    });

    it('test valid use of memory datasource', function() {
        var ds = new mapnik.MemoryDatasource({'extent': '-180,-90,180,90'});
        assert.equal(true, ds.add({ 'x': 0, 'y': 0 }));
        assert.equal(true, ds.add({ 'x': 0.23432, 'y': 0.234234 }));
        assert.equal(true, ds.add({ 'x': 1, 'y': 1 , 'properties': {'a':'b', 'c':1, 'd':0.23 }}));
        var expected_describe = { 
            type: 'vector',
            encoding: 'utf-8',
            fields: {},
            geometry_type: 'collection' 
        };
        assert.deepEqual(expected_describe, ds.describe());
        // Currently descriptors can not be added to memory datasource so will always be empty object
        assert.deepEqual({},ds.fields());
    });

    it('should validate with raster', function() {
        var options = {
            type: 'gdal',
            file: './test/data/images/sat_image.tif'
        };

        var ds = new mapnik.Datasource(options);
        assert.ok(ds);
        assert.deepEqual(ds.parameters(), options);
        var describe = ds.describe();
        var expected = {  type: 'raster',
                          encoding: 'utf-8',
                          fields: { nodata: 'Number' },
                          geometry_type: 'raster'
                       };
        assert.deepEqual(expected,describe);

        // Test that if added to layer, can get datasource back
        var layer = new mapnik.Layer('foo', '+init=epsg:4326');
        layer.datasource = ds;
        var ds2 = layer.datasource;
        assert.ok(ds2);
        assert.deepEqual(ds2.parameters(), options);
    });
});
