"use strict";

var test = require('tape');
var mapnik = require('../');
var exists = require('fs').existsSync || require('path').existsSync;
var path = require('path');

mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'gdal.input'));

test('should render to a file', (assert) => {
  var map = new mapnik.Map(600, 400);
  var filename = './test/tmp/renderFile.png';
  map.renderFile(filename, {scale:1, scale_denominator:0, buffer_size:0, variables:{pizza:'pie'}}, function(error) {
    assert.ok(!error);
    assert.ok(exists(filename));
    assert.end();
  });
});

test('should fail to render two things at once', (assert) => {
  var map = new mapnik.Map(256, 256);
  map.loadSync('./test/stylesheet.xml');
  map.zoomAll();
  var im = new mapnik.Image(map.width, map.height);
  assert.throws(function() {
    map.render(im, function(err, result) {});
    map.render(im, function(err, result) {});
  });
  assert.end();
});

test('should fail to render two things at once with sync', (assert) => {
  var map = new mapnik.Map(256, 256);
  map.loadSync('./test/stylesheet.xml');
  map.zoomAll();
  var im = new mapnik.Image(map.width, map.height);
  assert.throws(function() {
    map.render(im, function(err, result) {});
    map.renderSync(im);
  });
  assert.end();
});

test('should fail to renderFile two things at once', (assert) => {
  var map = new mapnik.Map(256, 256);
  map.loadSync('./test/stylesheet.xml');
  map.zoomAll();
  assert.throws(function() {
    map.renderFile('./test/tmp/foo1.png', function(err, result) {});
    map.renderFile('./test/tmp/foo2.png', function(err, result) {});
  });
  assert.end();
});

test('should fail to renderFile two things at once with Sync', (assert) => {
  var map = new mapnik.Map(256, 256);
  map.loadSync('./test/stylesheet.xml');
  map.zoomAll();
  assert.throws(function() {
    map.renderFile('./test/tmp/foo1.png', function(err, result) {});
    map.renderFileSync('./test/tmp/foo2.png');
  });
  assert.end();
});

test('should fail to render to an image gray8', (assert) => {
  var map = new mapnik.Map(256, 256);
  map.load('./test/stylesheet.xml', function(err,map) {
    if (err) throw err;
    map.zoomAll();
    var im = new mapnik.Image(map.width, map.height, {type:mapnik.imageType.gray8});
    assert.throws(function() { map.render(); } );
    assert.throws(function() { map.render(im, null); } );
    assert.throws(function() { map.render(null, function(err, im) {}); } );
    assert.throws(function() { map.render({}, function(err, im) {}); } );
    assert.throws(function() { map.render(im, null, function(err, im) {}); } );
    assert.throws(function() { map.render(im, {buffer_size:null}, function(err, im) {}); } );
    assert.throws(function() { map.render(im, {scale:null}, function(err, im) {}); } );
    assert.throws(function() { map.render(im, {scale_denominator:null}, function(err, im) {}); } );
    assert.throws(function() { map.render(im, {offset_x:null}, function(err, im) {}); } );
    assert.throws(function() { map.render(im, {offset_y:null}, function(err, im) {}); } );
    assert.throws(function() { map.render(im, {variables:null}, function(err, im) {}); } );
    map.render(im, function(err, im) {
      assert.throws(function() { if (err) throw err; });
      assert.end();
    });
  });
});

test('should fail to renderFile', (assert) => {
  var map = new mapnik.Map(256, 256);
  map.load('./test/stylesheet.xml', function(err,map) {
    if (err) throw err;
    map.zoomAll();
    map.srs = '+init=pizza';
    var im = 'test/data/tmp';
    assert.throws(function() { map.renderFile(); } );
    assert.throws(function() { map.renderFile(im, null); } );
    assert.throws(function() { map.renderFile(null, function(err, im) {}); } );
    assert.throws(function() { map.renderFile({}, function(err, im) {}); } );
    assert.throws(function() { map.renderFile(im, null, function(err, im) {}); } );
    assert.throws(function() { map.renderFile(im, {buffer_size:null}, function(err, im) {}); } );
    assert.throws(function() { map.renderFile(im, {scale:null}, function(err, im) {}); } );
    assert.throws(function() { map.renderFile(im, {scale_denominator:null}, function(err, im) {}); } );
    assert.throws(function() { map.renderFile(im, {format:null}, function(err, im) {}); } );
    assert.throws(function() { map.renderFile(im, {format:''}, function(err, im) {}); } );
    assert.throws(function() { map.renderFile(im, {variables:null}, function(err, im) {}); } );
    assert.throws(function() { map.renderFile(im, {palette:null}, function(err, im) {}); } );
    assert.throws(function() { map.renderFile(im, {palette:{}}, function(err, im) {}); } );
    map.renderFile(im, function(err, im) {
      assert.throws(function() { if (err) throw err; });
      assert.end();
    });
  });
});

test('should render to an image', (assert) => {
  var map = new mapnik.Map(256, 256);
  map.load('./test/stylesheet.xml', function(err,map) {
    if (err) throw err;
    map.zoomAll();
    var im = new mapnik.Image(map.width, map.height);
    map.render(im, {variables: {pizza:'pie'}}, function(err, im) {
      im.encode('png', {variables: {pizza:'pie'}}, function(err,buffer) {
        assert.ok(buffer);
        var string = im.toString();
        assert.ok(string);
        assert.end();
      });
    });
  });
});

test('should render to an image', (assert) => {
  var map = new mapnik.Map(256, 256);
  map.load('./test/stylesheet.xml', function(err,map) {
    if (err) throw err;
    map.zoomAll();
    var im = new mapnik.Image(map.width, map.height);
    map.render(im, {variables: {pizza:'pie'}}, function(err, im) {
      im.encode('png', {variables: {pizza:'pie'}}, function(err,buffer) {
        assert.ok(buffer);
        var string = im.toString();
        assert.ok(string);
        assert.end();
      });
    });
  });
});

test('should render to an image - raster', (assert) => {
  var map = new mapnik.Map(100, 100);
  map.load('./test/raster.xml', function(err,map) {
    if (err) throw err;
    map.zoomAll();
    var im = new mapnik.Image(map.width, map.height);
    var im2 = new mapnik.Image.open('./test/data/images/sat_image-expected-100x100-near.png');
    map.render(im, {variables: {pizza:'pie'}}, function(err, im) {
      assert.equal(0, im.compare(im2));
      im.encode('png', {variables: {pizza:'pie'}}, function(err,buffer) {
        assert.ok(buffer);
        var string = im.toString();
        assert.ok(string);
        assert.end();
      });
    });
  });
});
