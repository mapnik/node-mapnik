var mapnik = require('mapnik'),
    fs = require('fs'),
    path = require('path');

describe('test memory datasource', function() {
    var options = {};
    var ds = new mapnik.MemoryDatasource(options);

    it('should be creatable', function() {
        ds.parameters().should.be.ok;
        ds.parameters().should.eql(options);
    });

    for (var x = -80; x <= 80; x += 10) {
        for (var y = -80; y <= 80; y += 10) {
            var f = {
                x: x, y: y,
                properties: {
                    val: x
                }
            };
            if (y >= 0) {
                f.properties.y = y;
            }
            ds.add(f);
        }
    }

    it('should report length and be an object', function() {
        ds.features().should.have.lengthOf(289);
        ds.statistics().should.be.an.instanceof(Object);
    });

    it('should have statistics properties', function() {
        ds.statistics().val.should.have.property('mean');
        ds.statistics().val.should.have.property('min');
        ds.statistics().val.should.have.property('max');

        ds.statistics().y.should.have.property('mean');
        ds.statistics().y.should.have.property('min');
        ds.statistics().y.should.have.property('max');
    });

    it('should record correct statistics', function() {
        ds.statistics().val.mean.should.equal(0);
        ds.statistics().val.min.should.equal(-80);
        ds.statistics().val.max.should.equal(80);

        ds.statistics().y.mean.should.equal(40);
        ds.statistics().y.min.should.equal(0);
        ds.statistics().y.max.should.equal(80);
    });
});
