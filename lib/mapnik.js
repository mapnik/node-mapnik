var path = require('path'),
    _settings = require('./mapnik_settings'),
    mapnik = require('./_mapnik.node');

exports = module.exports = mapnik;
exports.settings = _settings;


if (_settings.paths.fonts) {
    // TODO - make async and warn if not successful
    mapnik.register_fonts(_settings.paths.fonts);
}

if (_settings.paths.input_plugins) {
    // TODO - make async and warn if not successful
    mapnik.register_datasources(_settings.paths.input_plugins);
}

// TODO - move into mapnik core - http://trac.mapnik.org/ticket/602
exports.register_system_fonts = function() {
    var num_faces = mapnik.fonts().length;
    var dirs = [];

    /*
      node process.version is based on tools/wafadmin/Utils.py
      which pulls from python's sys.platform (uname)
      this info taken from Tools/ccroot.py

    platforms = {
    '__linux__'   : 'linux',
    '__GNU__'     : 'hurd',
    '__FreeBSD__' : 'freebsd',
    '__NetBSD__'  : 'netbsd',
    '__OpenBSD__' : 'openbsd',
    '__sun'       : 'sunos',
    '__hpux'      : 'hpux',
    '__sgi'       : 'irix',
    '_AIX'        : 'aix',
    '__CYGWIN__'  : 'cygwin',
    '__MSYS__'    : 'msys',
    '_UWIN'       : 'uwin',
    '_WIN64'      : 'win32',
    '_WIN32'      : 'win32'
    }

    */

    // linux, bsd systems or solaris/opensolaris
    if (process.platform == 'linux' ||
       (process.platform.indexOf('bsd') != -1) ||
       process.platform.indexOf('bsd') == 'sunos') {
        dirs.push('/usr/share/fonts/truetype/');
        dirs.push('/usr/local/share/fonts/truetype/');
        dirs.push(path.join(process.env.HOME, '.fonts'));
    }

    else if (process.platform == 'darwin') {
        dirs.push('/Library/Fonts');
        dirs.push('/System/Library/Fonts');
        if (path.existsSync('/usr/X11R6/lib/X11/fonts/')) {
            // if xcode's X11 is installed
            dirs.push('/usr/X11R6/lib/X11/fonts/');
        }
        dirs.push(path.join(process.env.HOME, 'Library/Fonts'));
    }

    else if (process.platform == 'win32') {
       dirs.push('C:\Windows\Fonts');
    }

    dirs.forEach(function(p) {
        if (path.existsSync(p)) {
            mapnik.register_fonts(p, {recurse: true})
        }
    });

    if (mapnik.fonts().length == num_faces) {
       return false;
    }
    else {
       return true;
    }
};

exports.register_system_fonts();
