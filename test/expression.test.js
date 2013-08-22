var mapnik = require('../');
var assert = require('assert');

describe('mapnik.Expression', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Expression(); });
        // invalid args
        assert.throws(function() { new mapnik.Expression(); });
        assert.throws(function() { new mapnik.Expression(1); });
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
        var feature = new mapnik.Feature(0);
        feature.addAttributes({'attr': 'value'});
        assert.equal(expr.evaluate(feature), true);
        assert.equal(expr.evaluate(feature).toString(), 'true');
    });
});
