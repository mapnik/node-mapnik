var mapnik = require('mapnik');
var assert = require('assert');
var path = require('path');
var fs = require('fs');
var existsSync = require('fs').existsSync || require('path').existsSync;

var map_pre = '\n<Map>\n  <Layer name="test">\n    <Datasource>'
var map_param = '\n      <Parameter name="{{{key}}}">{{{value}}}</Parameter>'
var map_post = '\n    </Datasource>\n  </Layer>\n</Map>'

describe('Handling unicode paths, filenames, and data', function(){

    /* folder storage in git is messed up - comment for now */
    // https://github.com/mapnik/node-mapnik/issues/142
    /*
    it('open csv from folder with unicode', function(){
        var filepath = './test/data/Clément/foo.csv';
        assert.ok(existsSync(filepath));
        var ds = new mapnik.Datasource({type:'csv',file:filepath});
        assert.ok(ds); 
    });

    it('open shape from folder with unicode', function(){
        var filepath = './test/data/Clément/long_lat.shp';
        assert.ok(existsSync(filepath));
        var ds = new mapnik.Datasource({type:'shape',file:filepath});
        assert.ok(ds); 
    });
    */

    it('open csv file with unicode name', function(){
        var filepath = './test/data/你好_points.csv';
        assert.ok(existsSync(filepath));
        var ds = new mapnik.Datasource({type:'csv',file:filepath});
        assert.ok(ds); 
    });

    it('open csv file with unicode name in XML', function(){
        var filepath = './test/data/你好_points.csv';
        assert.ok(existsSync(filepath));
        var map_string = map_pre;
        map_string += map_param.replace('{{{key}}}','type').replace('{{{value}}}','csv');
        map_string += map_param.replace('{{{key}}}','file').replace('{{{value}}}',filepath);
        map_string += map_post;
        var map = new mapnik.Map(256,256);
        map.fromStringSync(map_string,{base:path.dirname(__dirname)})
        fs.writeFileSync('/tmp/mapnik-tmp-map-load.xml',map_string,'utf-8')
        map.loadSync('/tmp/mapnik-tmp-map-load.xml',{base:path.dirname(__dirname)})
        assert.ok(true);
    });

    it('open shape file with unicode name', function(){
        var filepath = './test/data/你好_points.shp';
        assert.ok(existsSync(filepath));
        var ds = new mapnik.Datasource({type:'shape',file:filepath});
        assert.ok(ds); 
    });

    it('open shape file with ogr and unicode name', function(){
        var filepath = './test/data/你好_points.shp';
        assert.ok(existsSync(filepath));
        var ds = new mapnik.Datasource({type:'ogr',file:filepath, layer_by_index:0});
        assert.ok(ds);
    });

	it('open json with unicode name', function(){
        var filepath = './test/data/你好_points.json';
        assert.ok(existsSync(filepath));
        var ds = new mapnik.Datasource({type:'geojson',file:filepath});
        assert.ok(ds);
    });

	it('open sqlite with unicode name', function(){
        var filepath = './test/data/你好_points.sqlite';
        assert.ok(existsSync(filepath));
        var ds = new mapnik.Datasource({type:'sqlite',file:filepath,use_spatial_index:false,table_by_index:0});
        assert.ok(ds);
    });

});