#!/usr/bin/env node

var path = require('path');

var usage = 'usage:';
usage += '\n  mapnik-inspect.js <datasource> (.shp|.json|.geojson|.kml|.sqlite|.gml|.vrt|.csv)';
usage += '\n  mapnik-inspect.js <stylesheet> (.xml)';
usage += '\n  mapnik-inspect.js <projection> (.prj)';
usage += '\n  mapnik-inspect.js <zipfile> (.zip)';

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

console.log(obj);

if (/.shp$/.test(obj)) {
    var opened = new mapnik.Datasource({type: 'shape', file: obj});
    console.log('Description -->');
    console.log(opened.describe());
    console.log('First feature --> ');
    console.log(opened.features().slice(0,1));
}
else if ((/.json$/.test(obj))
         || (/.geojson$/.test(obj))
         || (/.kml$/.test(obj))
         || (/.sqlite3$/.test(obj))
         || (/.db$/.test(obj))
         || (/.gml$/.test(obj))
         || (/.vrt$/.test(obj))
         || (/.csv$/.test(obj))
         || (/.dbf$/.test(obj))
        ) {
    var opened = new mapnik.Datasource({type: 'ogr', file: obj, 'layer_by_index': 0});
    console.log('Description -->');
    console.log(opened.describe());
    console.log('First feature --> ');
    console.log(opened.features().slice(0,1));
}
else if (/.xml$/.test(obj)) {
    var map = new mapnik.Map(1,1);
    map.loadSync(obj);
    console.log(map.layers());
}
else if (/.prj$/.test(obj)) {
    var srs = require('srs');
    console.log(srs.parse(obj));
}
else if (/.zip$/.test(obj)) {
    var zip = require('zipfile');
    var zf = new zip.ZipFile(obj);
    console.log(zf);
}
else {
    console.log('Error: unsupported file, unable to inspect');
    process.exit(1);
}
