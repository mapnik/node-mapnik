var po = org.polymaps;

var map = po.map()
    .container(document.getElementById('map').appendChild(po.svg('svg')))
    .center({lat: 0, lon: 0})
    .zoom(1)
    .zoomRange([1, 20])
    .add(po.drag())
    .add(po.dblclick())
    .add(po.hash());

var tile_url = '/{X}/{Y}/{Z}';
var layer = po.image().url(po.url(tile_url));
map.add(layer);
