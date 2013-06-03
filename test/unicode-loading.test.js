var mapnik = require('mapnik');
var assert = require('assert');
var path = require('path');
var fs = require('fs');
var existsSync = require('fs').existsSync || require('path').existsSync;

var map_pre = '\n<Map>\n  <Layer name="test">\n    <Datasource>'
var map_param = '\n      <Parameter name="{{{key}}}">{{{value}}}</Parameter>'
var map_post = '\n    </Datasource>\n  </Layer>\n</Map>'

describe('Handling unicode paths, filenames, and data', function(){

    // beware: folder storage can get messed up
    // https://github.com/mapnik/node-mapnik/issues/142

    it('register font file with unicode directory and name', function(){
        var filepath = './test/data/dir-区县级行政区划/你好_DejaVuSansMono-BoldOblique.ttf';
        assert.ok(existsSync(filepath));
        mapnik.register_fonts(filepath);
        assert.deepEqual(mapnik.fontFiles()['DejaVu Sans Mono Bold Oblique'],filepath);
    });

    it('render a map with unicode markers', function(done){
        var filepath = './test/data/ünicode_symbols.xml';
        assert.ok(existsSync(filepath));
        var svg = './test/data/dir-区县级行政区划/你好-ellipses.svg';
        assert.ok(existsSync(svg));
        var map = new mapnik.Map(256,256);
        map.load(filepath,function(err,map) {
            if (err) throw err;
            var im = new mapnik.Image(256,256);
            map.zoomAll();
            map.render(im,function(err,im) {
                assert.ok(im);
                //im.save('test.png')
                done();
            });
        });
    });

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

    it('open csv file with abs path and unicode name in XML', function(){
        var filepath = path.join(path.dirname(__dirname),'test/data/avlee-区县级行政区划.csv');
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

    it('open csv file with unicode directory name in XML', function(){
        var filepath = './test/data/dir-区县级行政区划/points.csv';
        assert.ok(existsSync(filepath));
        var map_string = map_pre;
        map_string += map_param.replace('{{{key}}}','type').replace('{{{value}}}','csv');
        map_string += map_param.replace('{{{key}}}','file').replace('{{{value}}}',filepath);
        map_string += map_post;
        var map = new mapnik.Map(256,256);
        map.fromStringSync(map_string,{base:path.dirname(__dirname)})
        var xml_path = '/tmp/mapnik-tmp-map-load'+'区县级行政区划' +'.xml';
		fs.writeFileSync(xml_path,map_string,'utf-8')
		assert.ok(existsSync(xml_path));
        map.loadSync(xml_path,{base:path.dirname(__dirname)})
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