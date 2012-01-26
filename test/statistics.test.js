var mapnik = require('mapnik'),
    fs = require('fs'),
    path = require('path');

describe('test memory datasource', function() {
    var options = {};
    var ds = new mapnik.MemoryDatasource(options);
    ds.parameters().should.be.ok;
    ds.parameters().should.eql(options);

    for (var x = -80; x <= 80; x += 10) {
        for (var y = -80; y <= 80; y += 10) {
            ds.add({
                x: x, y: y,
                properties: {
                    val: x
                }
            });
        }
    }

    ds.features().should.have.lengthOf(289);
    ds.statistics().should.be.an.instanceof(Object);
    ds.statistics().val.should.have.property('mean');
    ds.statistics().val.should.have.property('min');
    ds.statistics().val.should.have.property('max');
    ds.statistics().val.mean.should.equal(0);
    ds.statistics().val.min.should.equal(-80);
    ds.statistics().val.max.should.equal(80);
    console.log(ds.statistics());
});
