var po = org.polymaps;

var map = po.map()
    .container(document.getElementById('map').appendChild(po.svg('svg')))
    .center({lat: 0, lon: 0})
    .zoom(1)
    .zoomRange([1, 40])
    .add(po.interact())
    .add(po.hash());

// mapquest's mapnik tiles
map.add(po.image()
    .url(po.url('http://otile{S}.mqcdn.com/tiles/1.0.0/osm/'
    + '{Z}/{X}/{Y}.png')
    .hosts(['1', '2', '3', '4'])));

// local tile server
map.add(po.image()
    .url(po.url('http://localhost:8000/?z={Z}&x={X}&y={Y}')));
