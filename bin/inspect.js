#!/usr/bin/env node

var sys = require('fs');
var path = require('path');

var usage = 'usage: inspect.js <datasource>';

var ds = process.ARGV[2];
if (!ds) {
   console.log(usage);
   process.exit(1);
}

if (!path.existsSync(ds)) {
    console.log(ds + ' does not exist');
    process.exit(1);
}

var mapnik = require('mapnik');

if (/.shp$/.test(ds)) {
    var opened = new mapnik.Datasource({type: 'shape', file: ds});
    console.log(opened.describe());
}
else if ((/.json$/.test(ds)) || (/.geojson$/.test(ds))) {
    var opened = new mapnik.Datasource({type: 'ogr', file: ds, 'layer_by_index': 0});
    console.log(opened.describe());
}
else {
    console.log('only currently supports shapefiles and geojson');
    process.exit(1);
}
