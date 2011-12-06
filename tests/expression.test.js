var mapnik = require('mapnik');
var assert = require('assert');

exports['test expression'] = function(beforeExit) {
    // no 'new' keyword
    assert.throws(function() { mapnik.Expression(); });

    // invalid args
    assert.throws(function() { new mapnik.Expression(); });
    assert.throws(function() { new mapnik.Expression(1); });

    // valid expression strings
    var expr = new mapnik.Expression('[ATTR]');
    var expr = new mapnik.Expression('[ATTR]+2');
    var expr = new mapnik.Expression('[ATTR]/2');

    var expr = new mapnik.Expression('[ATTR1]/[ATTR2]');
    assert.equal(expr.toString(),'[ATTR1]/[ATTR2]');
    
    var expr = new mapnik.Expression('\'literal\'');
    assert.equal(expr.toString(),"'literal'");
};


exports['test expression evaluation'] = function(beforeExit) {

    var expr = new mapnik.Expression("[attr]='value'");
    var feature = new mapnik.Feature(0);
    feature.addAttributes({'attr':'value'});
    assert.equal(expr.evaluate(feature),true);
    assert.equal(expr.evaluate(feature).toString(),'true');
    
};
