"use strict";


var test = require('tape');
var mapnik = require('../');

function cleanXml(xml) {
  return xml.replace('\n','').split(' ').join('');
}

test('should be accessible from map', (assert) => {
  var map = new mapnik.Map(1, 1);
  map.loadSync('./test/support/extra_arbitary_map_parameters.xml');
  var params = map.parameters;
  assert.equal(params.decimal, 0.999);
  assert.equal(params.integer, 10);
  assert.equal(params.key, 'value2');
  assert.equal(params.integer_one, 1);
  assert.equal(params.integer_zero, 0);
  assert.equal(params.boolean_yes, true);
  assert.equal(params.boolean_no, false);
  assert.equal(params.boolean_on, true);
  assert.equal(params.boolean_off, false);
  assert.equal(params.boolean_true, true);
  assert.equal(params.boolean_false, false);
  assert.end();
});

test('should be settable on map', (assert) => {
  var map = new mapnik.Map(1, 1);
  var actual = cleanXml(map.toXML());
  var expected = cleanXml('<?xml version="1.0" encoding="utf-8"?>\n<Map srs="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"/>\n');
  assert.equal(actual, expected);
  map.parameters = {'a': 'b','bool':false, 'num': 12.2, 'num2': 1};
  actual = cleanXml(map.toXML());
  if (mapnik.versions.mapnik_number >= 300000) {
    expected = cleanXml('<?xml version="1.0" encoding="utf-8"?>\n<Map srs="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs">\n    <Parameters>\n        <Parameter name="a">b</Parameter>\n    <Parameter name="bool">false</Parameter>\n    <Parameter name="num">12.2</Parameter>\n    <Parameter name="num2">1</Parameter>\n</Parameters>\n</Map>\n');
  } else {
    expected = cleanXml('<?xml version="1.0" encoding="utf-8"?>\n<Map srs="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs">\n    <Parameters>\n        <Parameter name="a" type="string">b</Parameter>\n    </Parameters>\n</Map>\n');
  }
  assert.equal(actual, expected);
  assert.end();
});
