'use strict';

var test = require('tape');
var mapnik = require('../');
var fs = require('fs');

test('should throw with invalid usage', (assert) => {
  assert.throws(function() { mapnik.Image.fromSVGSync(); });
  assert.throws(function() { mapnik.Image.fromSVG(); });
  assert.throws(function() { mapnik.Image.fromSVGBytes(); });
  assert.throws(function() { mapnik.Image.fromSVGBytesSync(1);}, /must provide a buffer argument/);
  assert.throws(function() { mapnik.Image.fromSVGBytesSync({});}, /first argument is invalid, must be a Buffer/);
  assert.throws(function() { mapnik.Image.fromSVGBytes(1, function(err, svg) {});}, /must provide a buffer argument/);
  assert.throws(function() { mapnik.Image.fromSVGBytes({}, function(err, svg) {});}, /first argument is invalid, must be a Buffer/);
  assert.throws(function() {
    mapnik.Image.fromSVGBytesSync(Buffer.from('asdfasdf'));
  }, /SVG error: unable to parse "asdfasdf"/);
  assert.throws(function() {
    mapnik.Image.fromSVGSync('./test/data/SVG_DOES_NOT_EXIST.svg');
  }, /SVG error: unable to open "\.\/test\/data\/SVG_DOES_NOT_EXIST\.svg"/);
  assert.throws(function() {
    mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg', 256);
  }, /optional second arg must be an options object/);
  assert.throws(function() {
    mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', 256);
  }, /last argument must be a callback function/);
  assert.throws(function() {
    mapnik.Image.fromSVGBytes(Buffer.from('asdfasdf'), 256);
  }, /last argument must be a callback function/);
  assert.throws(function() {
    mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', 256, function(err, res) {});
  }, /optional second arg must be an options object/);
  assert.throws(function() {
    mapnik.Image.fromSVGBytes(Buffer.from('asdfasdf'), 256, function(err, res) {});
  }, /optional second arg must be an options object/);
  assert.throws(function() {
    mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg', { scale: 'foo' });
  }, /'scale' must be a number/);
  assert.throws(function() {
    mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', { scale: 'foo' }, function(err, res) {});
  }, /'scale' must be a number/);
  assert.throws(function() {
    mapnik.Image.fromSVGBytes(Buffer.from('asdf'), { scale: 'foo' }, function(err, res) {});
  }, /'scale' must be a number/);
  assert.throws(function() {
    mapnik.Image.fromSVGBytes(undefined, { scale: 1 }, function(err, res) {});
  }, /must provide a buffer argument/);
  assert.throws(function() {
    mapnik.Image.fromSVGBytes(null, { scale: 1 }, function(err, res) {});
  }, /must provide a buffer argument/);
  assert.throws(function() {
    mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg', { scale: -1 });
  }, /'scale' must be a positive non zero number/);
  assert.throws(function() {
    mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', { scale: -1 }, function(err, res) {});
  }, /'scale' must be a positive non zero number/);
  assert.throws(function() {
    mapnik.Image.fromSVGBytes(Buffer.from('asdf'), { scale: -1 }, function(err, res) {});
  }, /'scale' must be a positive non zero number/);
  assert.throws(function() {
    mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.corrupt-svg.svg');
  }, /image created from svg must have a width and height greater than zero/);

  mapnik.Image.fromSVGBytes(Buffer.from('a'), { scale: 1 }, function(err, res) {
    assert.ok(err);
    assert.ok(err.message.match(/SVG error: unable to parse "a"/));
    var svgdata = "<svg width='1000000000000' height='1000000000000'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
    var buffer = Buffer.from(svgdata);
    mapnik.Image.fromSVGBytes(buffer, { scale: 1 }, function(err, img) {
      assert.ok(err);
      assert.ok(err.message.match(/image created from svg must be 2048 pixels or fewer on each side/));
      var svgdata2 = "<svg width='100' height='100'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
      var buffer2 = Buffer.from(svgdata2);
      mapnik.Image.fromSVGBytes(buffer2, { scale: 1000000 }, function(err, img) {
        assert.ok(err);
        assert.ok(err.message.match(/image created from svg must be 2048 pixels or fewer on each side/));
        assert.end();
      });
    });
  });
});


test('blocks allocating a very large image', (assert) => {
  // 65535 is the max width/height in mapnik
  var svgdata = "<svg width='65535' height='65535'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  mapnik.Image.fromSVGBytes(buffer, function(err, img) {
    assert.ok(err);
    assert.ok(err.message.match(/image created from svg must be 2048 pixels or fewer on each side/));
    assert.end();
  });
});

test('customized the max image size to block', (assert) => {
  // 65535 is the max width/height in mapnik
  var svgdata = "<svg width='65535' height='65535'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  mapnik.Image.fromSVGBytes(buffer, {max_size:2049}, function(err, img) {
    assert.ok(err);
    assert.ok(err.message.match(/image created from svg must be 2049 pixels or fewer on each side/));
    assert.end();
  });
});

test('max image size blocks dimension*scale', (assert) => {
  // 65535 is the max width/height in mapnik
  var svgdata = "<svg width='5000' height='5000'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  mapnik.Image.fromSVGBytes(buffer, {scale: 2, max_size:10000-1}, function(err, img) {
    assert.ok(err);
    assert.ok(err.message.match(/image created from svg must be 9999 pixels or fewer on each side/));
    assert.end();
  });
});

test('should err with async file w/o width or height', (assert) => {
  mapnik.Image.fromSVG('./test/data/vector_tile/tile0.corrupt-svg.svg', function(err, svg) {
    assert.ok(err);
    assert.ok(err.message.match(/image created from svg must have a width and height greater than zero/));
    assert.equal(svg, undefined);
    assert.end();
  });
});

test('should err with async file w/o width or height as Bytes', (assert) => {
  var svgdata = "<svg width='0' height='0'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  mapnik.Image.fromSVGBytes(buffer, function(err, img) {
    assert.ok(err);
    assert.ok(err.message.match(/image created from svg must have a width and height greater than zero/));
    assert.equal(img, undefined);
    assert.end();
  });
});

test('should err with async invalid buffer', (assert) => {
  mapnik.Image.fromSVGBytes(Buffer.from('asdfasdf'), function(err, svg) {
    assert.ok(err);
    assert.ok(err.message.match(/SVG error: unable to parse "asdfasdf"/));
    assert.equal(svg, undefined);
    assert.end();
  });
});

test('should err with async non-existent file', (assert) => {
  mapnik.Image.fromSVG('./test/data/SVG_DOES_NOT_EXIST.svg', function(err, svg) {
    assert.ok(err);
    assert.ok(err.message.match(/SVG error: unable to open "\.\/test\/data\/SVG_DOES_NOT_EXIST\.svg"/));
    assert.equal(svg, undefined);
    assert.end();
  });
});

test('should not error with async in non-strict mode on svg with validation and parse errors', (assert) => {
  mapnik.Image.fromSVG('./test/data/vector_tile/errors.svg', {strict:false}, function(err, svg) {
    assert.ok(!err);
    assert.ok(svg);
    assert.end();
  });
});

test('should error with async in strict mode on svg with validation and parse errors', (assert) => {
  mapnik.Image.fromSVG('./test/data/vector_tile/errors.svg', {strict:true}, function(err, svg) {
    assert.ok(err);
    assert.ok(err.message.match(/SVG parse error:/));
    assert.equal(svg, undefined);
    assert.end();
  });
});

test('should not error on svg with unhandled elements in non-strict mode', (assert) => {
  var svgdata = "<svg width='100' height='100'><g id='a'><text></text><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  var img = mapnik.Image.fromSVGBytesSync(buffer, {strict:false});
  assert.ok(img);
  assert.ok(img instanceof mapnik.Image);
  assert.equal(img.width(), 100);
  assert.equal(img.height(), 100);
  var length = img.encodeSync("png").length;
  img.encode('png', (err, img) => {
    assert.equal(img.length, length);
    assert.end();
  });
});


//  test the mapnik core known unsupported elements: https://github.com/mapnik/mapnik/blob/634928fcbe780e8a5a355ddb3cd075ce2450adb4/src/svg/svg_parser.cpp#L106-L114
test('should error on svg with unhandled elements in strict mode', (assert) => {
  var svgdata = "<svg width='100' height='100'><g id='a'><unsupported></unsupported><text></text><symbol></symbol><image></image><marker></marker><view></view><switch></switch><a></a><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  var error = false;
  try {
    var img = mapnik.Image.fromSVGBytesSync(buffer,{strict:true});
    assert.ok(!img);
  } catch (err) {
    error = err;
  }
  assert.ok(error);
  assert.ok(error.message.match(/<text> element is not supported/));
  assert.ok(error.message.match(/<switch> element is not supported/));
  assert.ok(error.message.match(/<symbol> element is not supported/));
  assert.ok(error.message.match(/<marker> element is not supported/));
  assert.ok(error.message.match(/<view> element is not supported/));
  assert.ok(error.message.match(/<a> element is not supported/));
  assert.ok(error.message.match(/<image> element is not supported/));
  assert.end();
});

test('#fromSVGSync load from SVG file', (assert) => {
  var img = mapnik.Image.fromSVGSync('./test/data/vector_tile/tile0.expected-svg.svg');
  assert.ok(img);
  assert.ok(img instanceof mapnik.Image);
  assert.equal(img.width(), 256);
  assert.equal(img.height(), 256);
  var length = img.encodeSync('png32').length;
  img.encode('png32',(err, image)=>{
    assert.equal(image.length, length);
    assert.end();
  });
});

test('#fromSVGSync load from SVG file - 2', (assert) => {
  var img = mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg');
  assert.ok(img);
  assert.ok(img instanceof mapnik.Image);
  assert.equal(img.width(), 256);
  assert.equal(img.height(), 256);
  var length = img.encodeSync('png32').length;
  img.encode('png32',(err, image)=>{
    assert.equal(image.length, length);
    assert.end();
  });
});


test('#fromSVG load from SVG file', (assert) => {
  mapnik.Image.fromSVG('./test/data/vector_tile/tile0.expected-svg.svg', function(err, img) {
    assert.ok(img); assert.ok(img instanceof mapnik.Image);
    assert.equal(img.width(), 256);
    assert.equal(img.height(), 256);
    var length = img.encodeSync('png32').length;
    assert.equal(img.premultiplied(), false);
    img.encode('png32',(err, image)=>{
      assert.equal(image.length, length);
      assert.end();
    });

  });
});

test('#fromSVGBytesSync load from SVG buffer', (assert) => {
  var svgdata = "<svg width='100' height='100'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  var img = mapnik.Image.fromSVGBytesSync(buffer);
  assert.ok(img);
  assert.ok(img instanceof mapnik.Image);
  assert.equal(img.width(), 100);
  assert.equal(img.height(), 100);
  var length = img.encodeSync('png').length;
  img.encode('png',(err, image)=>{
    assert.equal(image.length, length);
    assert.end();
  });
});

test('#fromSVGBytesSync load from SVG buffer - 2', (assert) => {
  var svgdata = "<svg width='100' height='100'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  var img = mapnik.Image.fromSVGBytes(buffer);
  assert.ok(img);
  assert.ok(img instanceof mapnik.Image);
  assert.equal(img.width(), 100);
  assert.equal(img.height(), 100);
  var length = img.encodeSync('png').length;
  img.encode('png',(err, image)=>{
    assert.equal(image.length, length);
    assert.end();
  });
});

test('#fromSVGBytes load from SVG buffer', (assert) => {
  var svgdata = "<svg width='100' height='100'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  mapnik.Image.fromSVGBytes(buffer, function(err, img) {
    assert.ok(img);
    assert.ok(img instanceof mapnik.Image);
    assert.equal(img.width(), 100);
    assert.equal(img.height(), 100);
    var length = img.encodeSync('png').length;
    assert.equal(img.premultiplied(), false);
    img.encode('png',(err, image)=>{
      assert.equal(image.length, length);
      assert.end();
    });
  });
});

test('svg scaling', (assert) =>  {
  var svgdata = "<svg width='100' height='100'><g id='a'><ellipse fill='#FFFFFF' stroke='#000000' stroke-width='4' cx='50' cy='50' rx='25' ry='25'/></g></svg>";
  var buffer = Buffer.from(svgdata);
  var img = mapnik.Image.fromSVGBytesSync(buffer, { scale: 0.5});
  assert.ok(img);
  assert.ok(img instanceof mapnik.Image);
  assert.equal(img.width(), 50);
  assert.equal(img.height(), 50);
  assert.equal(img.encodeSync("png").length, 616);
  assert.equal(img.premultiplied(), false);
  assert.end();
});

test('svg viewBox', (assert) => {
  var svgdata = '<svg xmlns="http://www.w3.org/2000/svg" width="1em" height="1em" viewBox="0 0 24 24"/>';
  // "em" units are not supported, use viewBox to determine viewport
  var buffer = Buffer.from(svgdata);
  var img = mapnik.Image.fromSVGBytesSync(buffer, {scale: 1.0});
  assert.ok(img);
  assert.ok(img instanceof mapnik.Image);
  assert.equal(img.width(), 24);
  assert.equal(img.height(), 24);
  assert.end();
});
