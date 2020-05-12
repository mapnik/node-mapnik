"use strict";

var test = require('tape');
var mapnik = require('../');
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'ogr.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'gdal.input'));


let compare_proj_string = (str1, str2) => {
  let a1 = str1.split(" ").sort();
  let a2 = str2.split(" ").sort();
  if (a1.length != a1.length)
    return false;
  for (let i = 0; i < a1.length; ++i) {
    if (a1[i] != a2[i]) return false;
  }
  return true;
};

test('should throw with invalid usage', (assert) => {
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
  assert.end();
});

test('should validate with known shapefile - ogr', (assert) => {
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
  assert.ok(compare_proj_string(actual.proj4, expected.proj4));
  assert.deepEqual(actual.type, expected.type);
  assert.deepEqual(actual.encoding, expected.encoding);
  assert.deepEqual(actual.fields, expected.fields);
  assert.deepEqual(actual.geometry_type, expected.geometry_type);

  assert.deepEqual(ds.extent(), expected.extent);

  assert.deepEqual(ds.fields(), expected.fields);
  assert.end();
});

test('should validate with known shapefile', (assert) => {
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
  assert.end();
});

test('test empty geojson datasource', (assert) => {
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
  assert.equal(typeof(empty_fs),'object');
  assert.equal(empty_fs, null);
  assert.end();
});

test('test empty geojson datasource due to invalid json string', (assert) => {
  var input = "{ \"type\": \"FeatureCollection\", \"features\": [{ \"oofda\" } ] }";
  // from string will fail to parse
  assert.throws(function() { new mapnik.Datasource({ type:'geojson', inline: inline, cache_features: false }); });
  assert.throws(function() { new mapnik.Datasource({ type:'geojson', inline: fs.readFileSync('./test/data/parse.error.json').toString(), cache_features: false }); });
  assert.end();
});

test('test empty geojson datasource due to invalid json file', (assert) => {
  assert.throws(function() { new mapnik.Datasource({ type:'geojson', file: './test/data/parse.error.json', cache_features: false }); });
  assert.end()
});

test('should validate with raster', (assert) => {
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
  assert.end();
});
