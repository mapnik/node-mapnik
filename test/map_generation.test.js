"use strict";

var test = require('tape');
var mapnik = require('../');
var path = require('path');
var exists = require('fs').existsSync || require('path').existsSync;

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));


test('should render async (blank)', (assert) => {
  var map = new mapnik.Map(600, 400);
  assert.ok(map instanceof mapnik.Map);
  map.extent = map.extent;
  var im = new mapnik.Image(map.width, map.height);
  map.render(im, {scale: 1, buffer_size: 1}, function(err, image) {
    assert.ok(image);
    assert.ok(!err);
    var buffer = im.encodeSync('png');
    assert.ok(buffer);
    assert.end();
  });
});

test('should render async (real data)', (assert) => {
  var filename = './test/tmp/renderFile2.png';
  var map = new mapnik.Map(600, 400);
  map.loadSync('./test/stylesheet.xml');
  map.zoomAll();
  map.renderFile(filename, function(error) {
    assert.ok(!error);
    assert.ok(exists(filename));
    assert.end();
  });
});

test('should render async to file (png)', (assert) => {
  var filename = './test/tmp/renderFile2.png';
  var map = new mapnik.Map(600, 400);
  map.loadSync('./test/stylesheet.xml');
  map.zoomAll();
  map.renderFile(filename, function(error) {
    assert.ok(!error);
    assert.ok(exists(filename));
    assert.end();
  });
});

test('should render async to file (cairo format)', (assert) => {
  if (mapnik.supports.cairo) {
    var filename = './test/tmp/renderFile2.pdf';
    var map = new mapnik.Map(600, 400);
    map.loadSync('./test/stylesheet.xml');
    map.zoomAll();
    map.renderFile(filename, { format: 'pdf' }, function(error) {
      assert.ok(!error);
      assert.ok(exists(filename));
      assert.end();
    });
  } else { assert.end(); }
});

test('should render async to file (guessing format)', (assert) => {
  var filename = './test/tmp/renderFile.jpg';
  var map = new mapnik.Map(600, 400);
  map.loadSync('./test/stylesheet.xml');
  map.zoomAll();
  map.renderFile(filename, function(error) {
    assert.ok(!error);
    assert.ok(exists(filename));
    assert.end();
  });
});

test('should render async and throw with invalid format', (assert) => {
  var filename = './test/tmp/renderFile2.pdf';
  var map = new mapnik.Map(600, 400);
  map.loadSync('./test/stylesheet.xml');
  map.zoomAll();
  try {
    map.renderFile(filename, null, function(error) {  assert.ok(error); });
  } catch (ex) {
    assert.ok(ex);
    assert.end();
  }
});
