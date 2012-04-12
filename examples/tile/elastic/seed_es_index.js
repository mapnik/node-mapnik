
var mapnik = require('mapnik');
var path = require('path');

// port of elastic search
var port = 9200;
// "index"/"type"
var url = '/geo/project/';

var client = require('http').createClient(port, 'localhost');

var headers = {
    'charset': 'UTF-8',
    'Content-Type': 'application/json'
};

// go get some arbitrary data
var shp = './examples/data/world_merc.shp';

var ds = new mapnik.Datasource({
    type: 'shape',
    file: shp
});


// get the featureset that exposes lazy next() iterator
var featureset = ds.featureset();

var feat;
var idx = 1;
while (feat = featureset.next(true)) {
    // center longitude of polygon bbox
    var x = (feat._extent[0] + feat._extent[2]) / 2;
    // center latitude of polygon bbox
    var y = (feat._extent[1] + feat._extent[3]) / 2;
    var request = client.request('POST', url + idx, headers);
    idx++;
    var put = {
        'project' : {
            'location' : {
                'lat' : y,
                'lon' : x
            },
            'name': feat.NAME,
            'pop2005': feat.POP2005
        }
    };
    request.write(JSON.stringify(put), 'utf8');
    request.on('response', function(response) {
        var body = '';
        response.on('data', function(data) {
            body += data;
        });
        response.on('end', function() {
            console.log(JSON.parse(body));
        });
    });
    request.end();
}
