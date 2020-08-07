"use strict";

var test = require('tape');
var mapnik = require('../');

var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'csv.input'));

var trunc_6 = function(key, val) {
    return val.toFixed ? Number(val.toFixed(6)) : val;
};

function deepEqualTrunc(json1,json2) {
    var first = JSON.stringify(json1,trunc_6,1);
    var second = JSON.stringify(json2,trunc_6,1);
    if (first !== second) {
        throw new Error("JSON are not equal:\n" + first +  "\n------\n" + second);
    }
}

//describe('mapnik.Feature ', function() {

test('should throw with invalid usage', (assert) => {
  // no 'new' keyword
  assert.throws(function() { mapnik.Feature(); });
  // invalid args
  assert.throws(function() { new mapnik.Feature(); });
  assert.throws(function() { new mapnik.Feature(1, 4, 5); });
  assert.throws(function() { new mapnik.Feature('foo'); });
  assert.end();
});

test('should not be able to construct a Featureset', (assert) => {
  assert.throws(function() { mapnik.Featureset(); });
  assert.throws(function() { new mapnik.Featureset(); });
  assert.end();
});

test('should construct a feature properly', (assert) => {
  var feature = new mapnik.Feature(1);
  assert.ok(feature);
  assert.deepEqual(feature.extent(),[1.7976931348623157e+308,1.7976931348623157e+308,-1.7976931348623157e+308,-1.7976931348623157e+308]);
  assert.throws(function() { mapnik.Feature.fromJSON(); });
  assert.throws(function() { mapnik.Feature.fromJSON(null); });
  assert.throws(function() { mapnik.Feature.fromJSON('foo'); });
  assert.throws(function() {
    var feat = mapnik.Feature.fromJSON('{"type":"Feature","id":1,"geometry":{"type":"Point","coordinates":[0,"b"]},"properties":{"feat_id":1,"name":"name"}}');
  });
  assert.end();
});

test('should match known features', (assert) => {
  var options = {
    type: 'shape',
    file: './test/data/world_merc.shp'
  };

  var ds = new mapnik.Datasource(options);
  // get one feature
  var featureset = ds.featureset();
  var feature = featureset.next();
  assert.deepEqual(feature.attributes(), {
    AREA: 44,
    FIPS: 'AC',
    ISO2: 'AG',
    ISO3: 'ATG',
    LAT: 17.078,
    LON: -61.783,
    NAME: 'Antigua and Barbuda',
    POP2005: 83039,
    REGION: 19,
    SUBREGION: 29,
    UN: 28
  });

  // loop over all of them to ensure the proper feature count
  var count = 1;
  while ((feature = featureset.next())) {
    count++;
  }
  assert.equal(count, 245);
  assert.end();
});

test('should output the same geojson that it read', (assert) => {
  var expected = {
    type: "Feature",
    properties: {},
    geometry: {
      type: 'Polygon',
      coordinates: [[[1,1],[2,1],[2,2],[1,2],[1,1]]]
    }
  };
  var ds = new mapnik.Datasource({type:'csv', 'inline': "geojson\n'" + JSON.stringify(expected.geometry) + "'"});
  var describe = ds.describe();
  var expected_describe = { type: 'vector',
                            encoding: 'utf-8',
                            fields: {},
                            geometry_type: 'polygon' };
  assert.deepEqual(expected_describe,describe);
  var f = ds.featureset().next();
  var feature = JSON.parse(f.toJSON());

  assert.equal(expected.type, feature.type);
  assert.deepEqual(expected.properties, feature.properties);
  assert.equal(expected.geometry.type, feature.geometry.type);
  assert.deepEqual(expected.geometry.coordinates, feature.geometry.coordinates);
  assert.end();
});

test('should output the same geojson that it read (point)', (assert) => {
  var expected = {
    type: "Feature",
    properties: {},
    geometry: {
      type: 'Point',
      coordinates: [1,1]
    }
  };
  var ds = new mapnik.Datasource({type:'csv', 'inline': "geojson\n'" + JSON.stringify(expected.geometry) + "'"});
  var describe = ds.describe();
  var expected_describe = { type: 'vector',
                            encoding: 'utf-8',
                            fields: {},
                            geometry_type: 'point' };
  assert.deepEqual(expected_describe,describe);
  var f = ds.featureset().next();
  var feature = JSON.parse(f.toJSON());

  assert.equal(expected.type, feature.type);
  assert.deepEqual(expected.properties, feature.properties);
  assert.equal(expected.geometry.type, feature.geometry.type);
  assert.deepEqual(expected.geometry.coordinates, feature.geometry.coordinates);
  assert.end();
});

test('should output the same geojson that it read (line)', (assert) => {
  var expected = {
    type: "Feature",
    properties: {},
    geometry: {
      type: 'LineString',
      coordinates: [[1,1],[2,2]]
    }
  };
  var ds = new mapnik.Datasource({type:'csv', 'inline': "geojson\n'" + JSON.stringify(expected.geometry) + "'"});
  var describe = ds.describe();
  var expected_describe = { type: 'vector',
                            encoding: 'utf-8',
                            fields: {},
                                  geometry_type: 'linestring' };
  assert.deepEqual(expected_describe,describe);
  var f = ds.featureset().next();
  var feature = JSON.parse(f.toJSON());

  assert.equal(expected.type, feature.type);
  assert.deepEqual(expected.properties, feature.properties);
  assert.equal(expected.geometry.type, feature.geometry.type);
  assert.deepEqual(expected.geometry.coordinates, feature.geometry.coordinates);
  assert.end();
});

test('should be able to create feature from geojson and turn back into geojson', (assert) => {
  var expected = {
    type: "Feature",
    properties: {},
    geometry: {
      type: 'Polygon',
      coordinates: [[[1,1],[2,1],[2,2],[1,2],[1,1]]]
    }
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(expected));
  var feature = JSON.parse(f.toJSON());
  assert.equal(expected.type, feature.type);
  assert.deepEqual(expected.properties, feature.properties);
  assert.equal(expected.geometry.type, feature.geometry.type);
  if (mapnik.versions.mapnik_number >= 200300) {
    assert.deepEqual(expected.geometry.coordinates, feature.geometry.coordinates);
  }
  var geom = f.geometry();
  assert.deepEqual(geom.extent(),[ 1, 1, 2, 2 ]);
  var expected_geom = JSON.stringify(expected.geometry);
  assert.equal(expected_geom,geom.toJSON());
  var source_proj = new mapnik.Projection('+init=epsg:4326');
  var dest_proj = new mapnik.Projection('+init=epsg:3857');
  var trans = new mapnik.ProjTransform(source_proj,dest_proj);
  var transformed = geom.toJSON({transform:trans});
  assert.notEqual(expected_geom,transformed);
  // async toJSON
  geom.toJSON(function(err,json) {
    assert.equal(expected_geom,json);
    geom.toJSON({transform:trans},function(err,json2) {
      assert.equal(transformed,json2);
      assert.end();
    });
  });
});

test('should be able to reproject geojson feature', (assert) => {
  var merc_poly = {
    "type": "MultiPolygon",
    "coordinates": [
      [
        [
          [
            -6866928.47049373,
            1923670.30196653
          ],
          [
            -6878926.59653091,
            1939845.92736324
          ],
          [
            -6889254.03965028,
            1933083.00247353
          ],
          [
            -6866928.47049373,
            1923670.30196653
          ]
        ]
      ],
      [
        [
          [
            -6871659.99413039,
            1991787.21060401
          ],
          [
            -6887677.75566065,
            2002918.07047496
          ],
          [
            -6885450.92056682,
            1988802.9255689
          ],
          [
            -6871659.99413039,
            1991787.21060401
          ]
        ]
      ]
    ]
  };
  var input = {
    type: "Feature",
    properties: {},
    geometry: merc_poly
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var feature = JSON.parse(f.toJSON());
  assert.equal(input.type, feature.type);
  assert.deepEqual(input.properties, feature.properties);
  assert.equal(input.geometry.type, feature.geometry.type);
  if (mapnik.versions.mapnik_number >= 200300) {
    assert.deepEqual(input.geometry.coordinates, feature.geometry.coordinates);
  }
  var geom = f.geometry();
  var ext = geom.extent();
  var expected_ext = [-6889254.03965028,1923670.30196653,-6866928.47049373,2002918.07047496];
  assert.ok(Math.abs(ext[0] - expected_ext[0]) < 0.001);
  var input_geom = JSON.stringify(input.geometry);
  assert.equal(input_geom,geom.toJSON());
  var dest_proj = new mapnik.Projection('+init=epsg:4326');
  var source_proj = new mapnik.Projection('+init=epsg:3857');
  var trans = new mapnik.ProjTransform(source_proj,dest_proj);
  var expected_transformed = {
    "type": "MultiPolygon",
    "coordinates": [
      [
        [
          [
            -61.686668,
            17.0244410000002
          ],
          [
            -61.7944489999999,
            17.1633300000001
          ],
          [
            -61.887222,
            17.105274
          ],
          [
            -61.686668,
            17.0244410000002
          ]
        ]
      ],
      [
        [
          [
            -61.7291719999999,
            17.608608
          ],
          [
            -61.873062,
            17.7038880000001
          ],
          [
            -61.853058,
            17.5830540000001
          ],
          [
            -61.7291719999999,
            17.608608
          ]
        ]
      ]
    ]
  };
  var transformed = geom.toJSON({transform:trans});
  deepEqualTrunc(expected_transformed,JSON.parse(transformed));
  //assert.equal(expected_transformed,JSON.parse(transformed));
  assert.notEqual(input_geom,transformed);
  // async toJSON
  geom.toJSON(function(err,json) {
    assert.equal(input_geom,json);
    geom.toJSON({transform:trans},function(err,json2) {
      assert.equal(transformed,json2);
      assert.end();
    });
  });
});

test('should output WKT', (assert) => {
  var feature = {
    type: "Feature",
    properties: {},
    geometry: {
      type: 'Polygon',
      coordinates: [[[1,1],[2,1],[2,2],[1,2],[1,1]]]
    }
  };
  var expected = 'POLYGON((1 1,2 1,2 2,1 2,1 1))';
  var ds = new mapnik.Datasource({type:'csv', 'inline': "geojson\n'" + JSON.stringify(feature.geometry) + "'"});
  var f = ds.featureset().next();
  assert.equal(expected, f.geometry().toWKT());
  //assert.equal(expected, f.toWKT());
  assert.end();
});

test('should output WKB', (assert) => {
  var feature = {
    type: "Feature",
    properties: {},
    geometry: {
      type: 'Polygon',
      coordinates: [[[1,1],[2,1],[2,2],[1,2],[1,1]]]
    }
  };
  var expected = Buffer.from('01030000000100000005000000000000000000f03f000000000000f03f0000000000000040000000000000f03f00000000000000400000000000000040000000000000f03f0000000000000040000000000000f03f000000000000f03f', 'hex');
  var ds = new mapnik.Datasource({type:'csv', 'inline': "geojson\n'" + JSON.stringify(feature.geometry) + "'"});
  var f = ds.featureset().next();
  assert.deepEqual(expected, f.geometry().toWKB());
  //assert.deepEqual(expected, f.toWKB());
  assert.end();
});

test('should round trip a geojson property with an array', (assert) => {
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
  assert.equal(input.type, feature.type);
  assert.deepEqual(input.geometry, feature.geometry);
  // expected that the array is stringified after https://github.com/mapnik/mapnik/issues/2678
  input.properties.something = "[]";
  assert.deepEqual(input.properties, feature.properties);
  assert.end();
});
