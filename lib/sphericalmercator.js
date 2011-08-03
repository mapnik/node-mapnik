var mapnik = require('mapnik');

var proj4 = '+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs +over';
var mercator = new mapnik.Projection(proj4);

/**
 * SphericalMercator constructor: precaches calculations
 * for fast tile lookups
 */
function SphericalMercator() {
    var size = 256;
    this.Bc = [];
    this.Cc = [];
    this.zc = [];
    this.Ac = [];
    this.DEG_TO_RAD = Math.PI / 180;
    this.RAD_TO_DEG = 180 / Math.PI;
    this.size = 256;
    this.levels = 18;
    this.proj4 = proj4;
    for (var d = 0; d < this.levels; d++) {
        this.Bc.push(size / 360);
        this.Cc.push(size / (2 * Math.PI));
        this.zc.push(size / 2);
        this.Ac.push(size);
        size *= 2;
    }
}

/**
 * Get the max of the first two numbers and the min of that and the third
 *
 * @param {Number} a the first number.
 * @param {Number} b the second number.
 * @param {Number} c the third number.
 * @return {Number}
 */
SphericalMercator.prototype.minmax = function(a, b, c) {
    return Math.min(Math.max(a, b), c);
};

/**
 * Convert lat lon to screen pixel value
 *
 * @param {Array} px [lat lon] array of geographic coordinates.
 * @param {Number} zoom number of the zoom level.
 */
SphericalMercator.prototype.ll_to_px = function(ll, zoom) {
    var d = this.zc[zoom];
    var f = this.minmax(Math.sin(this.DEG_TO_RAD * ll[1]), -0.9999, 0.9999);
    var x = Math.round(d + ll[0] * this.Bc[zoom]);
    var y = Math.round(d + 0.5 * Math.log((1 + f) / (1 - f)) * (-this.Cc[zoom]));
    return [x, y];
};

/**
 * Convert screen pixel value to lat lon
 *
 * @param {Array} px [x y] array of geographic coordinates.
 * @param {Number} zoom number of the zoom level.
 */
SphericalMercator.prototype.px_to_ll = function(px, zoom) {
    var zoom_denom = this.zc[zoom];
    var g = (px[1] - zoom_denom) / (-this.Cc[zoom]);
    var lat = (px[0] - zoom_denom) / this.Bc[zoom];
    var lon = this.RAD_TO_DEG * (2 * Math.atan(Math.exp(g)) - 0.5 * Math.PI);
    return [lat, lon];
};

/**
 * Convert tile xyz value to Mapnik envelope
 *
 * @param {Number} x latitude number.
 * @param {Number} y longitude number.
 * @param {Number} zoom zoom.
 * @param {Boolean} tms_style whether to compute a tms tile.
 * @return Object Mapnik envelope.
 */
SphericalMercator.prototype.xyz_to_envelope = function(x, y, zoom, TMS_SCHEME) {
    if (TMS_SCHEME) {
        y = (Math.pow(2, zoom) - 1) - y;
    }
    var ll = [x * this.size, (y + 1) * this.size];
    var ur = [(x + 1) * this.size, y * this.size];
    var bbox = this.px_to_ll(ll, zoom).concat(this.px_to_ll(ur, zoom));
    return mercator.forward(bbox);
};

module.exports = new SphericalMercator();

