"use strict";

var test = require('tape');
var mapnik = require('../');


test('should throw with invalid usage', (assert) => {
  // no 'new' keyword
  assert.throws(function() { mapnik.Expression(); });
  // invalid args
  assert.throws(function() { new mapnik.Expression(); });
  assert.throws(function() { new mapnik.Expression(1); });
  assert.throws(function() { new mapnik.Expression('[asdfadsa]]'); });
  assert.end();
});

test('should accept complex expressions', (assert) => {
  // valid expression strings
  var expr = new mapnik.Expression('[ATTR]');
  expr = new mapnik.Expression('[ATTR]+2');
  expr = new mapnik.Expression('[ATTR]/2');

  expr = new mapnik.Expression('[ATTR1]/[ATTR2]');
  assert.equal(expr.toString(), '[ATTR1]/[ATTR2]');

  expr = new mapnik.Expression('\'literal\'');
  assert.equal(expr.toString(), "'literal'");
  assert.end();
});

test('should support evaluation to js types', (assert) => {
  var expr = new mapnik.Expression("[attr]='value'");
  var feature = new mapnik.Feature.fromJSON('{"type":"Feature","properties":{"attr":"value"},"geometry":null}');

  // Test bad parameters
  assert.throws(function() { expr.evaluate(); });
  assert.throws(function() { expr.evaluate(null); });
  assert.throws(function() { expr.evaluate(feature, null); });
  assert.throws(function() { expr.evaluate(feature, {variables:null}); });

  assert.equal(expr.evaluate(feature), true);
  assert.equal(expr.evaluate(feature).toString(), 'true');
  assert.end();
});

test('should support evaluation with variables', (assert) => {
  var expr = new mapnik.Expression("[integer]=@integer and [bool]=@bool and [string]=@string and [double]=@double");
  var options = {variables: { 'integer': 22, 'bool': true, 'string': "string", 'double': 1.0001 } };
  var feature = new mapnik.Feature.fromJSON('{"type":"Feature","properties":{"integer":22, "bool": 1, "string": "string", "double":1.0001},"geometry":null}');
  assert.equal(expr.evaluate(feature, options), true);
  assert.equal(expr.evaluate(feature, options).toString(), 'true');
  assert.end();
});
