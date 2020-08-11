"use strict";


var test = require('tape');
var mapnik = require('../');
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'geojson.input'));

test('should throw with invalid usage', (assert) => {
  // geometry cannot be created directly for now
  assert.throws(function() { mapnik.Geometry(); });
  assert.end();
});

test('should access a geometry from a feature', (assert) => {
  var feature = new mapnik.Feature(1);
  var point = {
    "type": "MultiPoint",
    "coordinates": [[0,0],[1,1]]
  };
  var input = {
    type: "Feature",
    properties: {},
    geometry: point
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  assert.equal(geom.type(),mapnik.Geometry.MultiPoint);
  assert.deepEqual(JSON.parse(geom.toJSONSync()),point);
  var expected_wkb = Buffer.from('0104000000020000000101000000000000000000000000000000000000000101000000000000000000f03f000000000000f03f', 'hex');
  assert.deepEqual(geom.toWKB(),expected_wkb);
  assert.end();
});

test('should fail on toJSON due to bad parameters', (assert) => {
  var feature = new mapnik.Feature(1);
  var point = {
    "type": "MultiPoint",
    "coordinates": [[0,0],[1,1]]
  };
  var input = {
    type: "Feature",
    properties: {},
    geometry: point
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  assert.equal(geom.type(),mapnik.Geometry.MultiPoint);
  assert.throws(function() { geom.toJSONSync(null); });
  assert.throws(function() { geom.toJSONSync({transform:null}); });
  assert.throws(function() { geom.toJSONSync({transform:{}}); });
  assert.throws(function() { geom.toJSON(null, function(err,json) {}); });
  assert.throws(function() { geom.toJSON({transform:null}, function(err, json) {}); });
  assert.throws(function() { geom.toJSON({transform:{}}, function(err, json) {}); });
  assert.end();
});

test('should throw if we attempt to create a Feature from a geojson geometry (rather than geojson feature)', (assert) => {
  var geometry = {
    type: 'Point',
    coordinates: [ 7.415119300000001, 43.730364300000005 ]
  };
  // starts throwing, as expected, at Mapnik v3.0.9 (https://github.com/mapnik/node-mapnik/issues/560)
  if (mapnik.versions.mapnik_number >= 300009) {
    assert.throws(function() {
      var transformed = mapnik.Feature.fromJSON(JSON.stringify(geometry));
    });
  }
  assert.end();
});

test('should throw from empty geometry from toWKB', (assert) => {
  var s = new mapnik.Feature(1);
  assert.throws(function() {
    var geom = s.geometry().toWKB();
  });
  assert.end();
});

test('should return a type name for a Point', (assert) => {
  var input = {
    type: 'Feature',
    properties: {},
    geometry: {
      type: 'Point',
      coordinates: [ 10, 15 ]
    }
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  assert.equal(geom.typeName(), 'Point');
  assert.end();
});

test('should return a type name for a LineString', (assert) => {
  var input = {
    type: 'Feature',
    properties: {},
    geometry: {
      type: 'LineString',
      coordinates: [[ 10, 15 ], [ 20, 30 ]]
    }
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  assert.equal(geom.typeName(), 'LineString');
  assert.end();
});

test('should return a type name for a Polygon', (assert) => {
  var input = {
    type: 'Feature',
    properties: {},
    geometry: {
      type: 'Polygon',
      coordinates: [[[ 10, 15 ], [ 20, 30 ], [ 40, 30 ], [ 10, 15 ]]]
    }
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  assert.equal(geom.typeName(), 'Polygon');
  assert.end();
});

test('should return a type name for a MultiPoint', (assert) => {
  var input = {
    type: 'Feature',
    properties: {},
    geometry: {
      type: 'MultiPoint',
      coordinates: [[ 10, 15 ], [ 20, 30 ]]
    }
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  assert.equal(geom.typeName(), 'MultiPoint');
  assert.end();
});

test('should return a type name for a MultiLineString', (assert) => {
  var input = {
    type: 'Feature',
    properties: {},
    geometry: {
      type: 'MultiLineString',
      coordinates: [[[ 10, 15 ], [ 20, 30 ]], [[ 40, 30 ], [ 10, 15 ]]]
    }
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  assert.equal(geom.typeName(), 'MultiLineString');
  assert.end();
});

test('should return a type name for a MultiPolygon', (assert) => {
  var input = {
    type: 'Feature',
    properties: {},
    geometry: {
      type: 'MultiPolygon',
      coordinates: [
        [[[ 10, 15 ], [ 20, 30 ], [ 40, 30 ], [ 10, 15 ]]],
        [[[ 40, 55 ], [ 60, 70 ], [ 80, 70 ], [ 40, 55 ]]]
      ]
    }
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  assert.equal(geom.typeName(), 'MultiPolygon');
  assert.end();
});

test('should return a type name for a GeometryCollection', (assert) => {
  var input = {
    type: 'Feature',
    properties: {},
    geometry: {
      type: 'GeometryCollection',
      geometries: [
        {
          type: 'LineString',
          coordinates: [[ 10, 15 ], [ 20, 30 ]]
        },
        {
          type: 'Point',
          coordinates: [ 10, 15 ]
        }
      ]
    }
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  assert.equal(geom.typeName(), 'GeometryCollection');
  assert.end();
});

test('should return a type name for a broken geometry', (assert) => {
  var input = {
    type: 'Feature',
    properties: {},
    geometry: {
      type: 'Point',
      coordinates: [ 10, 15 ]
    }
  };
  var f = new mapnik.Feature.fromJSON(JSON.stringify(input));
  var geom = f.geometry();
  geom.type = function() { return 777; }
  assert.equal(geom.typeName(), 'Unknown');
  assert.end();
});
