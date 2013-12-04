var mapnik = require('../');
var assert = require('assert');
var fs = require('fs');

describe('Convert to GeoJSON', function() {
    it('should convert shapefile', function(done) {
        if (process.versions.node.split('.')[1] !== '6') {
            var ds = new mapnik.Datasource({type:'shape',file:'test/data/world_merc.shp'});
            var featureset = ds.featureset()
            var geojson = {
              "type": "FeatureCollection",
              "features": [
              ]
            }
            var feat = featureset.next();
            while (feat) {
                geojson.features.push(JSON.parse(feat.toJSON()));
                feat = featureset.next();
            }
            var actual = './test/tmp/world_merc.converted.geojson';
            var expected = './test/data/world_merc.converted.geojson';
            fs.writeFileSync(actual,JSON.stringify(geojson,null,2));
            if (mapnik.versions.mapnik_number >= 200300) {
                assert.equal(fs.readFileSync(actual).length,fs.readFileSync(expected).length)
            }
        }
        done();
    });
});
