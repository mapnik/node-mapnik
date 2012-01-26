var mapnik = require('mapnik'),
    fs = require('fs'),
    path = require('path');

describe('test memory datasource', function() {
    var options = {};
    var ds = new mapnik.MemoryDatasource(options);
    ds.parameters().should.be.ok;
    ds.parameters().should.eql(options);

    for (var x = -80; x < 80; x += 10) {
        for (var y = -80; y < 80; y += 10) {
            ds.add({
                x: x, y: y,
                properties: {
                    val: x
                }
            });
        }
    }

    ds.features().should.have.lengthOf(256);
    ds.statistics().should.be.an.instanceof(Object);
    console.log(ds.statistics());
});
