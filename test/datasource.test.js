var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

describe('mapnik.Datasource', function() {
    it('should throw with invalid usage', function() {
        assert.throws(function() { mapnik.Datasource('foo'); });
        assert.throws(function() { mapnik.Datasource({ 'foo': 1 }); });
        assert.throws(function() { mapnik.Datasource({ 'type': 'foo' }); });
        assert.throws(function() { mapnik.Datasource({ 'type': 'shape' }); });

        assert.throws(function() { new mapnik.Datasource('foo'); },
            /Must provide an object, eg \{type: 'shape', file : 'world.shp'\}/);

        assert.throws(function() { new mapnik.Datasource({ 'foo': 1 }); });

        assert.throws(function() { new mapnik.Datasource({ 'type': 'foo' }); });

        assert.throws(function() { new mapnik.Datasource({ 'type': 'shape' }); },
            /Shape Plugin: missing <file> parameter/);
    });

    it('should validate with known shapefile', function() {
        var options = {
            type: 'shape',
            file: './test/data/world_merc.shp'
        };

        var ds = new mapnik.Datasource(options);
        assert.ok(ds);
        assert.deepEqual(ds.parameters(), options);

        var features = ds.features();
        assert.equal(features.length, 245);
        assert.deepEqual(features[244], {
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
            UN: 643,
            __id__: 245
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
    });

    it('should validate with known geojson', function() {
        // same datasource but from json file (originally converted with ogr2ogr)
        var options = {
            type: 'ogr',
            file: './test/data/world_merc.json',
            layer_by_index: 0
        };

        var ds = new mapnik.Datasource(options);
        assert.ok(ds);
        assert.deepEqual(ds.parameters(), options);

        var features = ds.features();
        assert.equal(features.length, 245);
        assert.deepEqual(features[244], {
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
            UN: 643,
            __id__: 245
        });

        var expected = {
            type: 'vector',
            extent: [
                -20037508.342789,
                -8283343.693883,
                20037508.342789,
                18365151.36307
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
    });
});
