
/*
Example of streaming features into Mapnik using a
javascript callback that leverages experimental javascript
datasource support
*/

/*
NOTE - maps using mapnik.JSDatasource can only be rendered with
mapnik.renderSync() or mapnik.renderFileSync() as the javascript
callback only works if the rendering happens in the main thread.

If you want async rendering using mapnik.render() then use the
mapnik.MemoryDatasource instead of mapnik.JSDatasource.
*/

var mapnik = require('mapnik');
var sys = require('fs');
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
map.fromStringSync(s, {strict: true, base: '.'});

// go get some arbitrary data that we can stream
var shp = path.join(__dirname, '../data/world_merc');

var ds = new mapnik.Datasource({
    type: 'shape',
    file: shp
});

// get the featureset that exposes lazy next() iterator
var featureset = ds.featureset();

// now construct a callback based javascript datasource
var options = {
    // right now 'extent' is required
    // world in merc
    extent: '-20037508.342789,-8283343.693883,20037508.342789,18365151.363070'
    // world in long lat
    //extent: '-180,-90,180,90',
};


// Define a callback to pass to the javascript datasource
// this will be called repeatedly as mapnik renders
// Rendering will continue until the function
// no longer returns a valid object of x,y,properties

// WARNING - this API will change!

var feat;
var next = function() {
    while ((feat = featureset.next(true))) {
        // center longitude of polygon bbox
        var e = feat.extent();
        var x = (e[0] + e[2]) / 2;
        // center latitude of polygon bbox
        var y = (e[1] + e[3]) / 2;
        var attr = feat.attributes();
        return { 'x' : x,
                 'y' : y,
                 'properties' : { 'NAME': attr.NAME, 'POP2005': attr.POP2005 }
               };
    }
};

// create the special datasource
var ds = new mapnik.JSDatasource(options, next);

// contruct a mapnik layer dynamically
var l = new mapnik.Layer('test');
l.srs = map.srs;
l.styles = ['points'];

// add our custom datasource
l.datasource = ds;

// add this layer to the map
map.add_layer(l);

// zoom to the extent of the new layer (pulled from options since otherwise we cannot know)
map.zoomAll();

// render it! You should see a bunch of red and blue points reprenting
map.renderFileSync('js_points.png');

console.log('rendered to js_points.png!');
