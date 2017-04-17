#!/usr/bin/env node

"use strict";

var exists = require('fs').existsSync || require('path').existsSync;

var fs = require('fs');

var usage = 'usage:';
usage += '\n  mapnik-inspect.js <datasource> (.vector.pbf|.mvt|.shp|.json|.geojson|.osm|.kml|.sqlite|.gml|.vrt|.csv)';
usage += '\n  mapnik-inspect.js <stylesheet> (.xml)';
usage += '\n  mapnik-inspect.js <projection> (.prj)';
usage += '\n  mapnik-inspect.js <zipfile> (.zip)';
usage += '\n  mapnik-inspect.js <svg> (.svg) (will print png image to stdout)';

var obj = process.argv[2];
if (!obj) {
   console.log(usage);
   process.exit(1);
}

if (!exists(obj)) {
    console.log(obj + ' does not exist');
    process.exit(1);
}

var mapnik = require('../');
mapnik.register_default_input_plugins();

var meta = function(ds) {
    console.log('Description -->');
    console.log(ds.describe());
    console.log('extent: ' + ds.extent().toString());
    var fs = ds.featureset();
    var count = 0;
    if (fs) {
        var feat = fs.next();
        while (feat) {
            count++;
            feat = fs.next();
        }
    }
    console.log(count,'features');
};

if (/.shp$/.test(obj)) {
    var opened = new mapnik.Datasource({type: 'shape', file: obj});
    meta(opened);
}
else if ((/.svg$/.test(obj))) {
    mapnik.Image.fromSVGBytes(fs.readFileSync(obj),function(err,img) {
        if (err) {
            console.error(err.message);
            process.exit(1);
        }
        var output="/tmp/mapnik-inspect.png";
        img.save(output);
        console.log("Saved image to "+output);
    });
}
else if ((/.vector.pbf$/.test(obj)) || (/.mvt$/.test(obj))) {
    console.log(mapnik.VectorTile.info(fs.readFileSync(obj)));
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
    var opened = new mapnik.Datasource({type: 'geojson', file: obj, cache_features:false});
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
    map.layers().forEach(function(l) {
        if (l.name == 'place_label') {
            var ds = l.datasource;
            //console.log(l.describe(),console.log(ds.describe()),ds.featureset().next().attributes());
            console.log(ds.featureset().next().attributes());
        }
    });
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
