"use strict";

var test = require('tape');
var mapnik = require('../');
var path = require('path');
var fs = require('fs');
var existsSync = require('fs').existsSync || require('path').existsSync;

var map_pre = '\n<Map>\n  <Layer name="test">\n    <Datasource>';
var map_param = '\n      <Parameter name="{{{key}}}">{{{value}}}</Parameter>';
var map_post = '\n    </Datasource>\n  </Layer>\n</Map>';

mapnik.register_default_input_plugins();

var available_ds = mapnik.datasources();

function xmlWithFont(font) {
  var val = '<Map><Style name="text"><Rule>';
  val += '<TextSymbolizer size="12" face-name="' + font + '"><![CDATA[[name]]]></TextSymbolizer>';
  val += '</Rule></Style>';
  val += '<Layer name="text" srs="epsg:4326">';
  val += '<StyleName>text</StyleName>';
  val += '<Datasource>'
  val += '<Parameter name="type">csv</Parameter>';
  val += '<Parameter name="inline">\n';
  val += 'x,y,name\n';
  val += '2,2.5,blake\n';
  val += '3,2.5,joe\n';
  val += '</Parameter></Datasource></Layer></Map>';
  return val;
}


// beware: folder storage can get messed up
// https://github.com/mapnik/node-mapnik/issues/142

test('register font file with unicode directory and name', (assert) => {
  var filepath = './test/data/dir-区县级行政区划/你好_DejaVuSansMono-BoldOblique.ttf';
  assert.ok(existsSync(filepath));
  assert.throws(function() { mapnik.register_fonts(); });
  assert.throws(function() { mapnik.register_fonts(filepath, null); });
  assert.throws(function() { mapnik.register_fonts(filepath, {recurse:null}); });
  assert.equal(mapnik.register_fonts('./fooooooo.ttf'), false);
  assert.equal(mapnik.register_fonts(filepath), true);
  assert.deepEqual(mapnik.fontFiles()['DejaVu Sans Mono Bold Oblique'],filepath);

  var map = new mapnik.Map(256, 256);
  map.fromStringSync(xmlWithFont('DejaVu Sans Mono Bold Oblique'));

  map.zoomAll();
  var im = new mapnik.Image(map.width, map.height);
  map.render(im, function(err, im) {
    if (err) throw err;
    assert.equal(mapnik.memoryFonts().length, 1)
    assert.end();
  });
});

test('render a map with unicode markers', (assert) => {
  if (available_ds.indexOf('csv') == -1) {
    console.log('skipping due to lack of csv plugin');
    return assert.end();
  }
  var filepath = './test/data/ünicode_symbols.xml';
  assert.ok(existsSync(filepath));
  var svg = './test/data/dir-区县级行政区划/你好-ellipses.svg';
  assert.ok(existsSync(svg));
  var map = new mapnik.Map(256,256);
  map.load(filepath,function(err,map) {
    if (err) throw err;
    var im = new mapnik.Image(256,256);
    map.zoomAll();
    map.render(im,function(err,im) {
      assert.ok(im);
      //im.save('test.png')
      assert.end();
    });
  });
});

test('open csv file with unicode name', (assert) => {
  if (available_ds.indexOf('csv') == -1) {
    console.log('skipping due to lack of csv plugin');
    return assert.end();
  }
  var filepath = './test/data/你好_points.csv';
  assert.ok(existsSync(filepath));
  var ds = new mapnik.Datasource({type:'csv',file:filepath});
  assert.ok(ds);
  assert.end();
});

test('open csv file with unicode name in XML', (assert) => {
  if (available_ds.indexOf('csv') == -1) {
    console.log('skipping due to lack of csv plugin');
    return assert.end();
  }
  var filepath = './test/data/你好_points.csv';
  assert.ok(existsSync(filepath));
  var map_string = map_pre;
  map_string += map_param.replace('{{{key}}}','type').replace('{{{value}}}','csv');
  map_string += map_param.replace('{{{key}}}','file').replace('{{{value}}}',filepath);
  map_string += map_post;
  var map = new mapnik.Map(256,256);
  map.fromStringSync(map_string,{base:path.dirname(__dirname)});
  fs.writeFileSync('./test/tmp/mapnik-tmp-map-load.xml',map_string,'utf-8');
  map.loadSync('./test/tmp/mapnik-tmp-map-load.xml',{base:path.dirname(__dirname)});
  assert.ok(true);
  assert.end();
});

test('open csv file with abs path and unicode name in XML', (assert) => {
  if (available_ds.indexOf('csv') == -1) {
    console.log('skipping due to lack of csv plugin');
    return assert.end();
  }
  var filepath = path.join(path.dirname(__dirname),'test/data/avlee-区县级行政区划.csv');
  assert.ok(existsSync(filepath));
  var map_string = map_pre;
  map_string += map_param.replace('{{{key}}}','type').replace('{{{value}}}','csv');
  map_string += map_param.replace('{{{key}}}','file').replace('{{{value}}}',filepath);
  map_string += map_post;
  var map = new mapnik.Map(256,256);
  map.fromStringSync(map_string,{base:path.dirname(__dirname)});
  fs.writeFileSync('./test/tmp/mapnik-tmp-map-load.xml',map_string,'utf-8');
  map.loadSync('./test/tmp/mapnik-tmp-map-load.xml',{base:path.dirname(__dirname)});
  assert.ok(true);
  assert.end();
});

test('open csv file with unicode directory name in XML', (assert) => {
  if (available_ds.indexOf('csv') == -1) {
    console.log('skipping due to lack of csv plugin');
    return assert.end();
  }
  var filepath = './test/data/dir-区县级行政区划/points.csv';
  assert.ok(existsSync(filepath));
  var map_string = map_pre;
  map_string += map_param.replace('{{{key}}}','type').replace('{{{value}}}','csv');
  map_string += map_param.replace('{{{key}}}','file').replace('{{{value}}}',filepath);
  map_string += map_post;
  var map = new mapnik.Map(256,256);
  map.fromStringSync(map_string,{base:path.dirname(__dirname)});
  var xml_path = './test/tmp/mapnik-tmp-map-load'+'区县级行政区划' +'.xml';
  fs.writeFileSync(xml_path,map_string,'utf-8');
  assert.ok(existsSync(xml_path));
  map.loadSync(xml_path,{base:path.dirname(__dirname)});
  assert.ok(true);
  assert.end();
});

test('open shape file with unicode name', (assert) => {
  if (available_ds.indexOf('shape') == -1) {
    console.log('skipping due to lack of shape plugin');
    return assert.end();
  }
  var filepath = './test/data/你好_points.shp';
  assert.ok(existsSync(filepath));
  var ds = new mapnik.Datasource({type:'shape',file:filepath});
  assert.ok(ds);
  assert.end();
});

test('open shape file with ogr and unicode name', (assert) => {
  if (available_ds.indexOf('ogr') == -1) {
    console.log('skipping due to lack of ogr plugin');
    return assert.end();
  }
  var filepath = './test/data/你好_points.shp';
  assert.ok(existsSync(filepath));
  var ds = new mapnik.Datasource({type:'ogr',file:filepath, layer_by_index:0});
  assert.ok(ds);
  assert.end();
});

test('open json with unicode name', (assert) => {
  if (available_ds.indexOf('geojson') == -1) {
    console.log('skipping due to lack of geojson plugin');
    return assert.end();
  }
  var filepath = './test/data/你好_points.geojson';
  assert.ok(existsSync(filepath));
  var ds = new mapnik.Datasource({type:'geojson',file:filepath});
  assert.ok(ds);
  assert.end();
});

test('open sqlite with unicode name', (assert) => {
  if (available_ds.indexOf('sqlite') == -1) {
    console.log('skipping due to lack of sqlite plugin');
    return assert.end();
  }
  var filepath = './test/data/你好_points.sqlite';
  assert.ok(existsSync(filepath));
  var ds = new mapnik.Datasource({type:'sqlite',file:filepath,use_spatial_index:false,table_by_index:0});
  assert.ok(ds);
  assert.end();
});
