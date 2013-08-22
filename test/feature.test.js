var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

describe('mapnik.Feature ', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { mapnik.Feature(); });
        // invalid args
        assert.throws(function() { new mapnik.Feature(); });
        assert.throws(function() { new mapnik.Feature(1, 4, 5); });
        assert.throws(function() { new mapnik.Feature('foo'); });
    });

    it('should match known features', function() {
        var options = {
            type: 'shape',
            file: './test/data/world_merc.shp'
        };

        var ds = new mapnik.Datasource(options);
        // get one feature
        var featureset = ds.featureset();
        var feature = featureset.next();
        assert.deepEqual(feature.attributes(), {
            AREA: 44,
            FIPS: 'AC',
            ISO2: 'AG',
            ISO3: 'ATG',
            LAT: 17.078,
            LON: -61.783,
            NAME: 'Antigua and Barbuda',
            POP2005: 83039,
            REGION: 19,
            SUBREGION: 29,
            UN: 28
        });

        // loop over all of them to ensure the proper feature count
        var count = 1;
        while ((feature = featureset.next())) {
            count++;
        }
        assert.equal(count, 245);
    });

    it('should report null values as js null',function() {
       var extent = '-180,-60,180,60';
       var ds = new mapnik.MemoryDatasource({'extent': extent});
       var feat = {x:0,y:0,properties: {feat_id:1,null_val:null,name:"name"}};
       ds.add(feat);
       var actual_feat = ds.features()[0];
       assert.ok(actual_feat.null_val === null);
       assert.equal(actual_feat.feat_id,1);
       assert.equal(actual_feat.__id__,1);
       assert.equal(actual_feat.name,'name');
    });
});
