"use strict";

var mapnik = require('../');
var assert = require('assert');
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));

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
    });


});
