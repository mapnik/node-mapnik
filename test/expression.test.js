"use strict";

var mapnik = require('../');
var assert = require('assert');

describe('mapnik.Expression', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Expression(); });
        // invalid args
        assert.throws(function() { new mapnik.Expression(); });
        assert.throws(function() { new mapnik.Expression(1); });
        assert.throws(function() { new mapnik.Expression('[asdfadsa]]'); });
    });

    it('should accept complex expressions', function() {
        // valid expression strings
        var expr = new mapnik.Expression('[ATTR]');
        expr = new mapnik.Expression('[ATTR]+2');
        expr = new mapnik.Expression('[ATTR]/2');

        expr = new mapnik.Expression('[ATTR1]/[ATTR2]');
        assert.equal(expr.toString(), '[ATTR1]/[ATTR2]');

        expr = new mapnik.Expression('\'literal\'');
        assert.equal(expr.toString(), "'literal'");
    });

    it('should support evaluation to js types', function() {
        var expr = new mapnik.Expression("[attr]='value'");
        var feature = new mapnik.Feature.fromJSON('{"type":"Feature","properties":{"attr":"value"},"geometry":null}');
        
        // Test bad parameters
        assert.throws(function() { expr.evaluate(); });
        assert.throws(function() { expr.evaluate(null); });
        assert.throws(function() { expr.evaluate(feature, null); });
        assert.throws(function() { expr.evaluate(feature, {variables:null}); });
        
        assert.equal(expr.evaluate(feature), true);
        assert.equal(expr.evaluate(feature).toString(), 'true');
    });

    it('should support evaluation with variables', function() {
        var expr = new mapnik.Expression("[integer]=@integer and [bool]=@bool and [string]=@string and [double]=@double");
        var options = {variables: { 'integer': 22, 'bool': true, 'string': "string", 'double': 1.0001 } };
        var feature = new mapnik.Feature.fromJSON('{"type":"Feature","properties":{"integer":22, "bool": 1, "string": "string", "double":1.0001},"geometry":null}');
        assert.equal(expr.evaluate(feature, options), true);
        assert.equal(expr.evaluate(feature, options).toString(), 'true');
    });
});

