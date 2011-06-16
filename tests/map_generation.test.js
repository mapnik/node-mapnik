var mapnik = require('mapnik');
var assert = require('assert');
var fs = require('fs');
var path = require('path');
var helper = require('./support/helper');

var Map = mapnik.Map;

var base_url = './examples/'; // must end with trailing slash
var style_string = fs.readFileSync(base_url + 'stylesheet.xml', 'utf8');
var map = new Map(600, 400);
map.fromStringSync(style_string, {strict:true,base:base_url});
map.zoom_all();

exports['test map generation'] = function(beforeExit) {
    // no 'new' keyword
    assert.throws(function() { Map('foo'); });

    // invalid args
    assert.throws(function() { new Map(); });
    assert.throws(function() { new Map(1); });
    assert.throws(function() { new Map('foo'); });
    assert.throws(function() { new Map('a', 'b', 'c'); });
    assert.throws(function() { new Map(new Map(1, 1)); });

    var map = new Map(256, 256);
    assert.ok(map instanceof Map);

    // test initial values
    assert.deepEqual(map.extent, [ 0, 0, -1, -1 ]);
};

exports['test synchronous map rendering'] = function(beforeExit) {
    var map = new Map(600, 400);
    assert.ok(map instanceof Map);

    // Test rendering a blank image
    var filename = helper.filename();
    map.renderFileSync(filename);
    assert.ok(path.existsSync(filename));
    assert.equal(helper.md5File(filename), 'ef33223235b26c782736c88933b35331');
};

exports['test asynchronous map rendering'] = function(beforeExit) {
    var completed = false;
    var map = new Map(600, 400);
    assert.ok(map instanceof Map);
    map.zoom_to_box(map.extent);
    var im = new mapnik.Image(map.width,map.height)
    map.render(im, {scale:1}, function(err, image) {
        completed = true;
        assert.ok(!err);
        var buffer = im.encode('png');
        assert.equal(helper.md5(buffer), 'ef33223235b26c782736c88933b35331');
    });

    beforeExit(function() {
        assert.ok(completed);
    });
};

exports['test loading a stylesheet'] = function(beforeExit) {
    var map = new Map(600, 400);
    
    assert.equal(map.width, 600);
    assert.equal(map.height, 400);
    assert.equal(map.srs, '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs');
    assert.equal(map.bufferSize, 0);
    assert.equal(map.maximumExtent, undefined);

    // Test loading a sample world map
    map.loadSync('./examples/stylesheet.xml');

    var layers = map.layers();
    assert.equal(layers.length, 1);
    assert.equal(layers[0].name, 'world');
    assert.equal(layers[0].srs, '+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs +over');
    assert.deepEqual(layers[0].styles, ['style']);
    assert.equal(layers[0].datasource.type, 'shape');
    assert.equal(path.normalize(layers[0].datasource.file), path.normalize(path.join(process.cwd(), 'examples/data/world_merc.shp')));

    // clear styles and layers from previous load to set up for another
    // otherwise layers are duplicated
    map.clear();
    var layers = map.layers();
    assert.equal(layers.length, 0);
};

exports['test rendering with actual data'] = function(beforeExit) {
    var filename = helper.filename();
    map.renderFileSync(filename);
    assert.ok(path.existsSync(filename));
    assert.equal(helper.md5File(filename), 'aaf71787e4d5dcbab3c964192038f465');
};

exports['test map extents'] = function() {
    var expected = [-20037508.3428, -14996604.5082, 20037508.3428, 25078412.1774];
    assert.notStrictEqual(map.extent, expected);

    var expected_precise = [-20037508.342789248,-8317435.060598943,20037508.342789244,18399242.72978672];
    assert.deepEqual(map.extent, expected_precise);
};

exports['test setting map properties'] = function() {
    var map = new Map(600, 400);

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
    
};

exports['test map layers'] = function() {
    var layers = map.layers();
    assert.equal(layers.length, 1);
    assert.equal(layers[0].name, 'world');
    assert.equal(layers[0].srs, '+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs +over');
    assert.deepEqual(layers[0].styles, ['style']);
    assert.equal(layers[0].datasource.type, 'shape');
    assert.equal(path.normalize(layers[0].datasource.file), path.normalize(path.join(process.cwd(), 'examples/data/world_merc.shp')));
};

exports['test map features'] = function() {
    // features
    var features = map.features(0); // for first and only layer
    assert.equal(features.length, 245);
    assert.deepEqual(features[244], {
        AREA: 1638094,
        FIPS: 'RS',
        ISO2: 'RU',
        ISO3: 'RUS',
        LAT: 61.988,
        LON: 96.689,
        NAME: 'Russia',
        POP2005: 143953092,
        REGION: 150,
        SUBREGION: 151,
        UN: 643,
        __id__:245
    });

    // feature slicing, just what you want
    var three_features = map.features(0, 0, 2); // for first and only layer
    assert.deepEqual(three_features, [
        {
            AREA: 44,
            FIPS: 'AC',
            ISO2: 'AG',
            ISO3: 'ATG',
            LAT: 17.078,
            LON: -61.783,
            NAME: 'Antigua and Barbuda',
            POP2005: 83039,
            REGION: 19,
            SUBREGION: 29,
            UN: 28,
            __id__:1
        },
        {
            AREA: 238174,
            FIPS: 'AG',
            ISO2: 'DZ',
            ISO3: 'DZA',
            LAT: 28.163,
            LON: 2.632,
            NAME: 'Algeria',
            POP2005: 32854159,
            REGION: 2,
            SUBREGION: 15,
            UN: 12,
            __id__:2
        },
        {
            AREA: 8260,
            FIPS: 'AJ',
            ISO2: 'AZ',
            ISO3: 'AZE',
            LAT: 40.43,
            LON: 47.395,
            NAME: 'Azerbaijan',
            POP2005: 8352021,
            REGION: 142,
            SUBREGION: 145,
            UN: 31,
            __id__:3
        }
    ]);
};

exports['test map datasource'] = function() {
    // datasource meta data
    var described = map.describe_data();
    assert.deepEqual(described.world.extent, [-20037508.342789248, -8283343.693882697, 20037508.342789244, 18365151.363070473]);
    assert.equal(described.world.type, 'vector');
    assert.equal(described.world.encoding, 'iso-8859-1');
    assert.equal(described.world.fields.FIPS, 'String');

    // get layer by index
    var layer = map.get_layer(0);
    assert.deepEqual(layer.datasource, {});

    // get layer by name
    var layer_same = map.get_layer("world");
    assert.deepEqual(layer_same.datasource, {});

    // but it does have functions
    assert.deepEqual(layer.datasource.describe(), map.describe_data().world);
    assert.deepEqual(layer.datasource.describe(), layer_same.datasource.describe());

    // test fetching one featureset using efficient next() iterator
    var featureset = layer.datasource.featureset();

    // get one feature
    var feature = featureset.next();
    assert.deepEqual(feature, {
        AREA: 44,
        FIPS: 'AC',
        ISO2: 'AG',
        ISO3: 'ATG',
        LAT: 17.078,
        LON: -61.783,
        NAME: 'Antigua and Barbuda',
        POP2005: 83039,
        REGION: 19,
        SUBREGION: 29,
        UN: 28,
        __id__:1
    });

    // loop over all of them to ensure the proper feature count
    var count = 1;
    while (feature = featureset.next()) {
        count++;
    }
    assert.equal(count, 245);

    var options = {
        type: 'shape',
        file: './examples/data/world_merc.shp'
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
    assert.deepEqual(added.datasource, options);
    assert.deepEqual(added.datasource, new mapnik.Datasource(options).parameters());
};
