var mapnik = require('../');
var assert = require('assert');
var path = require('path');

describe('mapnik.Map', function() {
    it('should throw with invalid usage', function() {
        // no 'new' keyword
        assert.throws(function() { Map('foo'); });

        // invalid args
        assert.throws(function() { new mapnik.Map(); });
        assert.throws(function() { new mapnik.Map(1); });
        assert.throws(function() { new mapnik.Map('foo'); });
        assert.throws(function() { new mapnik.Map('a', 'b', 'c'); });
        assert.throws(function() { new mapnik.Map(new mapnik.Map(1, 1)); });
    });

    it('should be initialized properly', function() {
        // TODO - more tests
        var map = new mapnik.Map(600, 400);
        assert.ok(map instanceof mapnik.Map);
        map.extent = map.extent;
    });

    it('should have settable properties', function() {
        var map = new mapnik.Map(600, 400);

        assert.equal(map.width, 600);
        assert.equal(map.height, 400);
        map.resize(256, 256);
        assert.equal(map.width, 256);
        assert.equal(map.height, 256);

        map.width = 100;
        map.height = 100;
        assert.equal(map.width, 100);
        assert.equal(map.height, 100);

        // TODO - need to expose aspect_fix_mode
        //assert.equal(map.maximumExtent,undefined)
        //map.maximumExtent = map.extent;
        //assert.equal(map.maximumExtent,map.extent)
    });

    it('should load a stylesheet', function() {
        var map = new mapnik.Map(600, 400);

        assert.equal(map.width, 600);
        assert.equal(map.height, 400);
        assert.equal(map.srs, '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs');
        assert.equal(map.bufferSize, 0);
        assert.equal(map.maximumExtent, undefined);

        // Test loading a sample world map
        map.loadSync('./test/stylesheet.xml');

        var layers = map.layers();
        assert.equal(layers.length, 1);
        assert.equal(layers[0].name, 'world');
        assert.equal(layers[0].srs, '+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs +over');
        assert.deepEqual(layers[0].styles, ['style']);
        assert.equal(layers[0].datasource.type, 'vector');
        assert.equal(layers[0].datasource.parameters().type, 'shape');
        assert.equal(path.normalize(layers[0].datasource.parameters().file), path.normalize(path.join(process.cwd(), 'test/data/world_merc.shp')));

        // clear styles and layers from previous load to set up for another
        // otherwise layers are duplicated
        map.clear();
        var layers2 = map.layers();
        assert.equal(layers2.length, 0);
    });

    it('should allow access to layers', function() {
        var map = new mapnik.Map(600, 400);
        map.loadSync('./test/stylesheet.xml');
        var layers = map.layers();
        assert.equal(layers.length, 1);
        assert.equal(layers[0].name, 'world');
        assert.equal(layers[0].srs, '+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs +over');
        assert.deepEqual(layers[0].styles, ['style']);
        assert.equal(layers[0].datasource.type, 'vector');
        assert.equal(layers[0].datasource.parameters().type, 'shape');
        assert.equal(path.normalize(layers[0].datasource.parameters().file), path.normalize(path.join(process.cwd(), 'test/data/world_merc.shp')));

        var layer = map.get_layer(0);
        assert.ok(layer.datasource);

        // get layer by name
        var layer_same = map.get_layer('world');
        assert.ok(layer_same.datasource);

        // compare
        assert.deepEqual(layer.datasource.describe(), layer_same.datasource.describe());

        var options = {
            type: 'shape',
            file: './test/data/world_merc.shp'
        };

        // make a change to layer, ensure it sticks
        layer.name = 'a';
        layer.styles = ['a'];
        layer.srs = '+init=epsg:4326';
        layer.datasource = new mapnik.Datasource(options);

        // check for change, after adding to map
        // adding to map should release original layer
        // as a copy is made when added (I think)
        map.add_layer(layer);
        var added = map.layers()[1];
        // make sure the layer is an identical copy to what is on map
        assert.equal(added.name, layer.name);
        assert.equal(added.srs, layer.srs);
        assert.deepEqual(added.styles, layer.styles);
        assert.deepEqual(added.datasource.parameters(), options);
        assert.deepEqual(added.datasource.parameters(), new mapnik.Datasource(options).parameters());

    });

});
