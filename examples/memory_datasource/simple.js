#!/usr/bin/env node

var fs = require('fs');
var mapnik = require('mapnik');
var path = require('path');
var merc = '+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +no_defs +over';

// map with just a style
// eventually the api will support adding styles in javascript
var s = '<Map srs="' + merc + '">';
s += '<Style name="points">';
s += ' <Rule>';
s += '  <Filter>[POP2005]&gt;200000000</Filter>';
s += '  <MarkersSymbolizer marker-type="ellipse" fill="red" width="10" allow-overlap="true" placement="point"/>';
s += ' </Rule>';
s += ' <Rule>';
s += '  <Filter>[POP2005]&gt;20000000</Filter>';
s += '  <MarkersSymbolizer marker-type="ellipse" fill="green" width="7" allow-overlap="true" placement="point"/>';
s += ' </Rule>';
s += ' <Rule>';
s += '  <ElseFilter />';
s += '  <MarkersSymbolizer marker-type="ellipse" fill="blue" width="5" allow-overlap="true" placement="point"/>';
s += ' </Rule>';
s += '</Style>';
s += '</Map>';

// create map object
var map = new mapnik.Map(256,256);
map.from_string(s,'.');

// go get some arbitrary data that we can stream
var shp = path.join(__dirname,'../data/world_merc');

var ds = new mapnik.Datasource({
    type: 'shape',
    file: shp
});

// get the featureset that exposes lazy next() iterator
var featureset = ds.featureset();

var mem_datasource = new mapnik.MemoryDatasource(
    {'extent':'-20037508.342789,-8283343.693883,20037508.342789,18365151.363070'}
    )

// build up memory datasource
while (feat = featureset.next(true)) {
    // center longitude of polygon bbox
    var x = (feat._extent[0]+feat._extent[2])/2;
    // center latitude of polygon bbox
    var y = (feat._extent[1]+feat._extent[3])/2;
    mem_datasource.add({ 'x'          : x,
                         'y'          : y,
                         'properties' : { 'feat_id':feat.__id__, 'NAME':feat.NAME,'POP2005':feat.POP2005 }
                       });
}

var options = {
    extent: '-20037508.342789,-8283343.693883,20037508.342789,18365151.363070',
};

// contruct a mapnik layer dynamically
var l = new mapnik.Layer('test');
l.srs = map.srs;
l.styles = ["points"];

// add our custom datasource
l.datasource = mem_datasource;

// add this layer to the map
map.add_layer(l);

// zoom to the extent of the new layer (pulled from options since otherwise we cannot know)
map.zoom_all();

// render it! You should see a bunch of red and blue points reprenting
map.render_to_file('memory_points.png');

if (mapnik.supports.grid) {
    var options = { resolution:4,
                    key:'feat_id',
                    fields: ['POP2005','NAME','feat_id']
                  };
    map.render_grid(0,options,function(err, data) {
                          if (err) throw err;
                          fs.writeFileSync('memory_points.json',JSON.stringify(data));
                          });
} else {
    map._render_grid(
        0,
        4,
        'feat_id',
        true,
        ['POP2005','NAME','feat_id'], function(err, data) {
            if (err) throw err;
            fs.writeFileSync('memory_points.json',JSON.stringify(data));
        });
}
console.log('rendered to memory_points.png and memory_points.json' );