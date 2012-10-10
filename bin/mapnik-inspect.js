#!/usr/bin/env node

var exists = require('fs').existsSync || require('path').existsSync;
var fs = require('fs');

var usage = 'usage:';
usage += '\n  mapnik-inspect.js <datasource> (.shp|.json|.geojson|.osm|.kml|.sqlite|.gml|.vrt|.csv)';
usage += '\n  mapnik-inspect.js <stylesheet> (.xml)';
usage += '\n  mapnik-inspect.js <projection> (.prj)';
usage += '\n  mapnik-inspect.js <zipfile> (.zip)';

var obj = process.argv[2];
if (!obj) {
   console.log(usage);
   process.exit(1);
}

if (!exists(obj)) {
    console.log(obj + ' does not exist');
    process.exit(1);
}

var mapnik = require('mapnik');

console.log(obj);

var meta = function(ds) {
    console.log('Description -->');
    console.log(ds.describe());
    console.log('extent: ' + ds.extent().toString());
    console.log('First feature --> ');
    console.log(ds.features().slice(0, 1));
}

if (/.shp$/.test(obj)) {
    var opened = new mapnik.Datasource({type: 'shape', file: obj});
    meta(opened);
}
else if ((/.csv$/.test(obj)) ||
         (/.tsv$/.test(obj)) || // google refine output .tsv for tab-separated files
         (/.txt$/.test(obj))) {
    var opened = new mapnik.Datasource({type: 'csv', file: obj});
    meta(opened);
}
else if (/.osm$/.test(obj)) {
    var opened = new mapnik.Datasource({type: 'osm', file: obj});
    meta(opened);
}
else if ((/.sqlite$/.test(obj)) ||
         (/.sqlite3$/.test(obj)) ||
         (/.db$/.test(obj))
        ) {
    var opened = new mapnik.Datasource({type: 'sqlite', file: obj, 'table_by_index': 0});
    meta(opened);
}
else if ((/.json$/.test(obj)) ||
         (/.geojson$/.test(obj))) {
    var opened = new mapnik.Datasource({type: 'geojson', file: obj});
    meta(opened);
}
else if ((/.kml$/.test(obj)) ||
         (/.gml$/.test(obj)) ||
         (/.dbf$/.test(obj))
        ) {
    var opened = new mapnik.Datasource({type: 'ogr', file: obj, 'layer_by_index': 0});
    meta(opened);
}
else if ((/.tif$/.test(obj)) ||
         (/.tiff$/.test(obj)) ||
         (/.vrt$/.test(obj)) ||
         (/.geotif$/.test(obj)) ||
         (/.geotiff$/.test(obj))
        ) {
    var opened = new mapnik.Datasource({type: 'gdal', file: obj});
    meta(opened);
}
else if (/.xml$/.test(obj)) {
    var map = new mapnik.Map(1, 1);
    map.loadSync(obj);
    console.log(map.layers());
}
else if (/.prj$/.test(obj)) {
    var srs = require('srs');
    var string = fs.readFileSync(obj).toString();
    var srs_obj = srs.parse(string);
    if (!srs_obj.proj4)
       srs_obj = srs.parse('ESRI::' + string);
    console.log(srs_obj);
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
