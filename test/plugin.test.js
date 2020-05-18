"use strict";

var test = require('tape');
var mapnik = require('../');
var path = require('path');


test('test registering of datasource', (assert) => {
  // Should register fine the first time
  // ensure shape input is registered
  mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
  // should not register again
  var b = mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
  assert.equal(false, b);

  var ds_list = mapnik.datasources();
  var contains = false;
  for (var i = 0; i < ds_list.length; i++) {
    if (ds_list[i] === 'shape') contains = true;
  }
  assert.equal(contains, true);
  assert.end();
});

test('test registering of datasource - other naming', (assert) => {
  // Should register fine the first time
  // ensure shape input is registered
  mapnik.registerDatasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
  // should not register again
  var b = mapnik.registerDatasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
  assert.equal(false, b);

  var ds_list = mapnik.datasources();
  var contains = false;
  for (var i = 0; i < ds_list.length; i++) {
    if (ds_list[i] === 'shape') contains = true;
  }
  assert.equal(contains, true);
  assert.end();
});


test('should fail to register plugin', (assert) => {

  assert.throws(function() {
    mapnik.register_datasource();
  });
  assert.throws(function() {
    mapnik.register_datasource(1);
  });
  assert.throws(function() {
    mapnik.register_datasource('ab', 'c');
  });

  assert.throws(function() {
    mapnik.register_datasources();
  });
  assert.throws(function() {
    mapnik.register_datasources(1);
  });
  assert.throws(function() {
    mapnik.register_datasources('ab', 'c');
  });
  assert.end();
});

test('test registering of multiple datasources', (assert) => {
  // Should register fine the first time
  // ensure shape input is registered
  mapnik.register_datasources(mapnik.settings.paths.input_plugins);
  // should not register again
  var b = mapnik.register_datasources(mapnik.settings.paths.input_plugins);
  assert.equal(false, b);
  assert.end();
});

test('test registering of multiple datasources - alternate naming', (assert) => {
  // Should register fine the first time
  // ensure shape input is registered
  mapnik.registerDatasources(mapnik.settings.paths.input_plugins);
  // should not register again
  var b = mapnik.registerDatasources(mapnik.settings.paths.input_plugins);
  assert.equal(false, b);
  assert.end();
});
