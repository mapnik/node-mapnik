#!/usr/bin/env node

var path = require('path');

var usage = 'usage:\n  inspect.js <datasource> (.shp|.json)';
usage += '\n  inspect.js <stylesheet> (.xml)';
usage += '\n  inspect.js <projection> (.prj)';

var obj = process.ARGV[2];
if (!obj) {
   console.log(usage);
   process.exit(1);
}

if (!path.existsSync(obj)) {
    console.log(obj + ' does not exist');
    process.exit(1);
}

var mapnik = require('mapnik');

if (/.shp$/.test(obj)) {
    var opened = new mapnik.Datasource({type: 'shape', file: obj});
    console.log(opened.describe());
}
else if ((/.json$/.test(obj)) || (/.geojson$/.test(obj))) {
    var opened = new mapnik.Datasource({type: 'ogr', file: obj, 'layer_by_index': 0});
    console.log(opened.describe());
}
else if (/.xml$/.test(obj)) {
    var map = new mapnik.Map(1,1);
    map.load(obj);
    console.log(map.layers());
}
else if (/.prj$/.test(obj)) {
    var srs = require('srs');
    console.log(srs.parse(obj));
}
else {
    console.log('only currently supports shapefiles and geojson datasources or mapnik xml');
    process.exit(1);
}
