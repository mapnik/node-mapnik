
var fs = require('fs');
var mapnik = require('mapnik');
var path = require('path');
var merc = require('../utils/sphericalmercator').proj4;

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
var map = new mapnik.Map(256, 256);
map.fromStringSync(s);

// go get some arbitrary data that we can stream
var shp = path.join(__dirname, '../data/world_merc');

var ds = new mapnik.Datasource({
    type: 'shape',
    file: shp
});

// get the featureset that exposes lazy next() iterator
var featureset = ds.featureset();

var mem_datasource = new mapnik.MemoryDatasource(
    {'extent': '-20037508.342789,-8283343.693883,20037508.342789,18365151.363070'}
    );

// build up memory datasource
while ((feat = featureset.next(true))) {
    var e = feat.extent();
    // center longitude of polygon bbox
    var x = (e[0] + e[2]) / 2;
    // center latitude of polygon bbox
    var y = (e[1] + e[3]) / 2;
    var attr = feat.attributes();
    mem_datasource.add({ 'x' : x,
                         'y' : y,
                         'properties' : { 'feat_id': feat.id(), 'NAME': attr.NAME, 'POP2005': attr.POP2005 }
                       });
}

var options = {
    extent: '-20037508.342789,-8283343.693883,20037508.342789,18365151.363070'
};

// contruct a mapnik layer dynamically
var l = new mapnik.Layer('test');
l.srs = map.srs;
l.styles = ['points'];

// add our custom datasource
l.datasource = mem_datasource;

// add this layer to the map
map.add_layer(l);

// zoom to the extent of the new layer (pulled from options since otherwise we cannot know)
map.zoomAll();

// render it! You should see a bunch of red and blue points reprenting
map.renderFileSync('memory_points.png');

var options = {
    layer: 0,
    fields: ['POP2005', 'NAME', 'feat_id']
    };

var grid = new mapnik.Grid(map.width, map.height, {key: 'feat_id'});
map.render(grid, options, function(err, grid) {
    if (err) throw err;
    fs.writeFileSync('memory_points.json', JSON.stringify(grid.encodeSync('utf', {resolution: 4})));
});

console.log('rendered to memory_points.png and memory_points.json');
