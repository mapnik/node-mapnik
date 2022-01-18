"use strict";


var test = require('tape');
var mapnik = require('../');
var path = require('path');
var fs = require('fs');
var os = require('os');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'csv.input'));

test('should throw with invalid usage', (assert) => {
  // no 'new' keyword
  assert.throws(function() { mapnik.Map('foo'); });
  // invalid args
  assert.throws(function() { new mapnik.Map(); });
  assert.throws(function() { new mapnik.Map(1); });
  assert.throws(function() { new mapnik.Map(1,'b'); });
  assert.throws(function() { new mapnik.Map(1,1,1); });
  assert.throws(function() { new mapnik.Map('foo'); });
  assert.throws(function() { new mapnik.Map('a', 'b', 'c'); });
  assert.throws(function() { new mapnik.Map(new mapnik.Map(1, 1)); });
  assert.end();
});

test('should be initialized properly', (assert) => {
  // TODO - more tests
  var map = new mapnik.Map(600, 400);
  assert.ok(map instanceof mapnik.Map);
  assert.end();
});

test('should be initialized properly with projection', (assert) => {
  // TODO - more tests
  var map = new mapnik.Map(25,25,'epsg:3857');
  assert.ok(map instanceof mapnik.Map);
  assert.equal(map.srs, 'epsg:3857');
  assert.end();
});

test('should have settable properties', (assert) => {
  var map = new mapnik.Map(600, 400);

  assert.equal(map.width, 600);
  assert.equal(map.height, 400);
  assert.throws(function() { map.resize(1); });
  assert.throws(function() { map.resize('1','2'); });
  map.resize(256, 256);
  assert.equal(map.width, 256);
  assert.equal(map.height, 256);

  assert.throws(function() { map.width = 'foo'; });
  assert.throws(function() { map.height = 'foo'; });
  map.width = 100;
  map.height = 100;
  assert.equal(map.width, 100);
  assert.equal(map.height, 100);

  // Aspect fix mode
  assert.equal(map.aspect_fix_mode,mapnik.Map.ASPECT_GROW_BBOX);
  // https://github.com/mapnik/mapnik/wiki/Aspect-Fix-Mode
  var world = [-180,-85,180,85];
  map.extent = world;
  assert.throws(function() { map.extent = 'foo'; });
  assert.throws(function() { map.extent = [1,2,3]; });
  // will have been made square
  assert.deepEqual(map.extent,[-180,-180,180,180]);
  // now add a buffer and test bufferedExtent
  assert.throws(function() { map.bufferSize = 'stuff'; });
  map.bufferSize = 10.9;
  assert.equal(map.bufferSize, 10); // Buffer is int so will round down!
  assert.deepEqual(map.bufferedExtent,[-216,-216,216,216]);
  // Check maximum extent
  assert.equal(map.maximumExtent, undefined); // first should be undefined
  map.maximumExtent = [-180,-180,180,180];
  assert.deepEqual(map.maximumExtent,[-180,-180,180,180]);
  // Check backgound
  assert.equal(map.background, undefined);
  assert.throws(function() { map.background = 1; });
  assert.throws(function() { map.background = {} });
  var b = new mapnik.Color('white');
  map.background = b;
  assert.equal(map.background.r, b.r);
  assert.equal(map.background.g, b.g);
  assert.equal(map.background.b, b.b);
  assert.equal(map.background.a, b.a);

  // Complete test coverage
  assert.equal(map.undefined_property, undefined);

  if (mapnik.versions.mapnik_number >= 200300) {
    // now try again after disabling the "fixing"
    assert.throws(function() { map.aspect_fix_mode = 'stuff'; });
    assert.throws(function() { map.aspect_fix_mode = -1; });
    assert.throws(function() { map.aspect_fix_mode = 999; });
    map.aspect_fix_mode = mapnik.Map.ASPECT_RESPECT;
    assert.equal(map.aspect_fix_mode,mapnik.Map.ASPECT_RESPECT);
    map.extent = world;
    assert.deepEqual(map.extent,world);
  }

  assert.equal(map.srs, "epsg:4326");
  assert.throws(function() { map.srs = 100; });
  map.srs = 'epsg:3857';
  assert.equal(map.srs, 'epsg:3857');

  // Test Parameters
  assert.throws(function() { map.parameters = null; });
  map.parameters = { a: 1.2, b: 1, c: 'stuff'};
  assert.equal(map.parameters.a, 1.2);
  assert.equal(map.parameters.b, 1);
  assert.equal(map.parameters.c, 'stuff');
  //map.maximumExtent = map.extent;
  //assert.equal(map.maximumExtent,map.extent)
  assert.end();
});

test('should support scale methods', (assert) => {
  var map = new mapnik.Map(4000, 4000);
  map.zoomToBox([0,0,1,1]);
  assert.equal(map.scale(), 0.00025);
  assert.equal(map.scaleDenominator(), 99392.40249399428);
  assert.end();
});

test('should fail to load a stylesheet async', (assert) => {
  var map = new mapnik.Map(600, 400);
  assert.equal(map.width, 600);
  assert.equal(map.height, 400);
  assert.equal(map.srs, 'epsg:4326');
  assert.equal(map.bufferSize, 0);
  assert.equal(map.maximumExtent, undefined);
  map.load('./test/stylesheet.xml', {base: '/DOESNOTEXIST' }, function(err, result_map) {
    assert.ok(err);
    assert.equal(result_map, undefined);
    assert.end();
  });
});

test('should load a stylesheet async', (assert) => {
  var map = new mapnik.Map(600, 400);

  assert.equal(map.width, 600);
  assert.equal(map.height, 400);
  assert.equal(map.srs, 'epsg:4326');
  assert.equal(map.bufferSize, 0);
  assert.equal(map.maximumExtent, undefined);

  // Assert bad ways to loadSync
  assert.throws(function() { map.load(); });
  assert.throws(function() { map.load(1); });
  assert.throws(function() { map.load('string',{},3); });
  assert.throws(function() { map.load(1, function(err, result_map) {}); });
  assert.throws(function() { map.load('./test/stylesheet.xml', null, function(err, result_map) {}); });
  assert.throws(function() { map.load('./test/stylesheet.xml', {strict: 12 }, function(err, result_map) {}); });
  assert.throws(function() { map.load('./test/stylesheet.xml', {base: 12 }, function(err, result_map) {}); });

  // Test loading a sample world map
  map.load('./test/stylesheet.xml', function(err, result_map) {

    var cloned = result_map.clone();
    assert.equal(result_map.toXML(),cloned.toXML());
    cloned.srs = 'foo';
    assert.notEqual(result_map.toXML(),cloned.toXML());

    var layers = result_map.layers();
    assert.equal(layers.length, 1);
    assert.equal(layers[0].name, 'world');
    assert.equal(layers[0].srs, 'epsg:3857');
    assert.deepEqual(layers[0].styles, ['style']);
    assert.equal(layers[0].datasource.type, 'vector');
    assert.equal(layers[0].datasource.parameters().type, 'shape');
    assert.equal(path.normalize(layers[0].datasource.parameters().file), path.normalize(path.join(process.cwd(), './test/data/world_merc.shp')));

    // clear styles and layers from previous load to set up for another
    // otherwise layers are duplicated
    result_map.clear();
    var layers2 = result_map.layers();
    assert.equal(layers2.length, 0);
    assert.end();
  });
});

test('should load a stylesheet sync', (assert) => {
  var map = new mapnik.Map(600, 400);

  assert.equal(map.width, 600);
  assert.equal(map.height, 400);
  assert.equal(map.srs, 'epsg:4326');
  assert.equal(map.bufferSize, 0);
  assert.equal(map.maximumExtent, undefined);

  // Assert bad ways to loadSync
  assert.throws(function() { map.loadSync(); });
  assert.throws(function() { map.loadSync(1); });
  assert.throws(function() { map.loadSync('./test/stylesheet.xml', {}, 2); });
  assert.throws(function() { map.loadSync('./test/stylesheet.xml', null); });
  assert.throws(function() { map.loadSync('./test/stylesheet.xml', {strict: 12 }); });
  assert.throws(function() { map.loadSync('./test/stylesheet.xml', {base: 12 }); });
  assert.throws(function() { map.loadSync('./test/stylesheet.xml', {base: '/DOESNOTEXIST' }); });
  // replace map with the new one
  map = new mapnik.Map(600,400);
  // Test loading a sample world map
  map.loadSync('./test/stylesheet.xml');

  var cloned = map.clone();
  assert.equal(map.toXML(),cloned.toXML());
  cloned.srs = 'foo';
  assert.notEqual(map.toXML(),cloned.toXML());

  var layers = map.layers();
  assert.equal(layers.length, 1);
  assert.equal(layers[0].name, 'world');
  assert.equal(layers[0].srs, 'epsg:3857');
  assert.deepEqual(layers[0].styles, ['style']);
  assert.equal(layers[0].datasource.type, 'vector');
  assert.equal(layers[0].datasource.parameters().type, 'shape');
  assert.equal(path.normalize(layers[0].datasource.parameters().file), path.normalize(path.join(process.cwd(), './test/data/world_merc.shp')));

  // clear styles and layers from previous load to set up for another
  // otherwise layers are duplicated
  map.clear();
  var layers2 = map.layers();
  assert.equal(layers2.length, 0);
  assert.end();
});

test('should load fromString sync', (assert) => {
  var map = new mapnik.Map(4, 4);
  var s = '<Map>';
  s += '<Style name="points">';
  s += ' <Rule>';
  s += '  <PointSymbolizer />';
  s += ' </Rule>';
  s += '</Style>';
  s += '</Map>';

  assert.equal(map.fromStringSync(s, {strict:false, base:''}),undefined);
  assert.end();
});

test('should not load fromString Sync', (assert) => {
        var map = new mapnik.Map(4, 4);
        var s = '<xMap>';
        s += '<Style name="points">';
        s += ' <Rule>';
        s += '  <PointSymbolizer />';
        s += ' </Rule>';
        s += '</Style>';
        s += '</aMap>';

        // Test some bad parameter passings
        assert.throws(function() { map.fromStringSync(); });
        assert.throws(function() { map.fromStringSync(1); });
        assert.throws(function() { map.fromStringSync(s, null); });
        assert.throws(function() { map.fromStringSync(s, {strict:null}); });
        assert.throws(function() { map.fromStringSync(s, {base:null}); });
        // Test good parameters, bad string
        assert.throws(function() { map.fromStringSync(s, {strict:false, base:''}); });
  assert.end();
});

test('should not load fromString Async - bad string', (assert) => {
  var map = new mapnik.Map(4, 4);
  var s = '<xMap>';
  s += '<Style name="points">';
  s += ' <Rule>';
  s += '  <PointSymbolizer />';
  s += ' </Rule>';
  s += '</Style>';
  s += '</aMap>';

  map.fromString(s, {strict:false, base:''}, function(err, result_map) {
    assert.ok(err);
    assert.equal(result_map, undefined);
    assert.end();
  });
});

test('should load fromString Async', (assert) => {
  var map = new mapnik.Map(4, 4);
  var s = '<Map>';
  s += '<Style name="points">';
  s += ' <Rule>';
  s += '  <PointSymbolizer />';
  s += ' </Rule>';
  s += '</Style>';
  s += '</Map>';

  // Test passing bad parameters
  assert.throws(function() { map.fromString(); });
  assert.throws(function() { map.fromString(s, 1); });
  assert.throws(function() { map.fromString(12, function(err, result_map) {}); });
  assert.throws(function() { map.fromString(s, null, function(err, result_map) {});  });
  assert.throws(function() { map.fromString(s, {strict:12}, function(err, result_map) {}); });
  assert.throws(function() { map.fromString(s, {base:12}, function(err, result_map) {}); });
  map.fromString(s, {strict:false, base:''}, function(err, result_map) {
    assert.equal(err, null);
    assert.equal(result_map.width, 4);
    assert.equal(result_map.height, 4);
    assert.end();
  });
});

test('cloned map should be safely independent of other maps', (assert) => {
  var map2;

  function localized() {
    var map = new mapnik.Map(600, 400);
    map.loadSync('./test/data/roads.xml');
    map2 = map.clone();
    map.clear();
    var layers2 = map.layers();
    assert.equal(layers2.length, 0);
    map.clear();
  }
  localized();
  var layers2 = map2.layers();
  map2.toXML();
  map2.toXML();
  map2.toXML();
  map2.toXML();
  assert.equal(layers2.length, 1);
  assert.end();
});

test('should save round robin', (assert) => {
  var map = new mapnik.Map(600,400);
  map.loadSync('./test/stylesheet.xml');
  var layers = map.layers();
  // Test bad parameters
  assert.throws(function() { map.save(1); });
  assert.throws(function() { map.save('./test/stylesheet.xml', 2); });
  var tmp_stylesheet = path.join(os.tmpdir(),'stylesheet_copy.xml');
  map.save(tmp_stylesheet);
  var map2 = new mapnik.Map(600,400);
  map2.loadSync(tmp_stylesheet)
  var layers2 = map2.layers();

  assert.equal(layers.length, layers2.length);
  assert.equal(layers[0].name, layers2[0].name);
  assert.equal(layers[0].srs, layers2[0].srs);
  assert.deepEqual(layers[0].styles, layers2[0].styles);
  assert.equal(layers[0].datasource.type, layers2[0].datasource.type);
  assert.equal(layers[0].datasource.parameters().type,layers2[0].datasource.parameters().type);
  assert.equal(layers[0].datasource.type, layers2[0].datasource.type);

  fs.unlink(tmp_stylesheet, function(err) {
    if (err) throw err;
    assert.end();
  });
});

test('should allow access to layers', (assert) => {
  var map = new mapnik.Map(600, 400);
  map.loadSync('./test/stylesheet.xml');
  var layers = map.layers();
  assert.equal(layers.length, 1);
  assert.equal(layers[0].name, 'world');
  assert.equal(layers[0].srs, 'epsg:3857');
  assert.deepEqual(layers[0].styles, ['style']);
  assert.equal(layers[0].datasource.type, 'vector');
  assert.equal(layers[0].datasource.parameters().type, 'shape');
  assert.equal(path.normalize(layers[0].datasource.parameters().file), path.normalize(path.join(process.cwd(), './test/data/world_merc.shp')));

  // Assert that throws when bad things are passed to get_layer
  assert.throws(function() { map.get_layer(); });
  assert.throws(function() { map.get_layer(undefined); });
  assert.throws(function() { map.get_layer(9999); });
  assert.throws(function() { map.get_layer('xasdfad'); });

  var layer = map.get_layer(0);
  assert.ok(layer.datasource);

  // get layer by name
  var layer_same = map.get_layer('world');
  assert.ok(layer_same.datasource);

  // compare
  assert.deepEqual(layer.datasource.describe(), layer_same.datasource.describe());

  var options = {
    type: 'shape',
    file: './test/data/world_merc.shp'
  };

  // make a change to layer, ensure it sticks
  layer.name = 'a';
  layer.styles = ['a'];
  layer.srs = 'epsg:4326';
  layer.datasource = new mapnik.Datasource(options);

  // Assert that add layer throws for bad layers
  assert.throws(function() { map.add_layer(null); });
  assert.throws(function() { map.add_layer({a:'foo'}); });

  // Assert that remove_layer throws for bad layers
  assert.throws(function() { map.remove_layer(); });
  assert.throws(function() { map.remove_layer(0, 0); });
  assert.throws(function() { map.remove_layer(null); });
  assert.throws(function() { map.remove_layer('foo'); });
  assert.throws(function() { map.remove_layer(5); });
  assert.throws(function() { map.remove_layer(-1); });

  // check for change, after adding to map
  // adding to map should release original layer
  // as a copy is made when added (I think)
  map.add_layer(layer);
  var added = map.layers()[1];
  // make sure the layer is an identical copy to what is on map
  assert.equal(added.name, layer.name);
  assert.equal(added.srs, layer.srs);
  assert.deepEqual(added.styles, layer.styles);
  assert.deepEqual(added.datasource.parameters(), options);
  assert.deepEqual(added.datasource.parameters(), new mapnik.Datasource(options).parameters());

  // Assert that remove_layer removes the correct one
  var layersBefore = map.layers();
  map.remove_layer(1);
  var layersAfter = map.layers();
  layersBefore.splice(1, 1);
  assert.deepEqual(layersBefore, layersAfter);
  assert.end();
});
