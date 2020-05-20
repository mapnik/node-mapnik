"use strict";

var test = require('tape');
var mapnik = require('../');
var fs = require('fs');

var path = require('path');
mapnik.register_datasource(path.join(mapnik.settings.paths.input_plugins,'shape.input'));

test('should convert shapefile', (assert) => {
  if (process.versions.node.split('.')[1] !== '6') {
    var ds = new mapnik.Datasource({type:'shape',file:'test/data/world_merc.shp'});
    var featureset = ds.featureset();
    var geojson = {
      "type": "FeatureCollection",
      "features": [
      ]
    };
    var feat = featureset.next();
    while (feat) {
      geojson.features.push(JSON.parse(feat.toJSON()));
      feat = featureset.next();
    }
    var actual = './test/tmp/world_merc.converted.geojson';
    var expected = './test/data/world_merc.converted.geojson';
    if (!fs.existsSync(expected) || process.env.UPDATE ) {
      fs.writeFileSync(expected,JSON.stringify(geojson,null,2));
    }
    fs.writeFileSync(actual,JSON.stringify(geojson,null,2));
    assert.ok(Math.abs(fs.readFileSync(actual).length-fs.readFileSync(expected).length) < 3000);
  }
  assert.end();
});
