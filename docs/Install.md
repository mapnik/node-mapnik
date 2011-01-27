
# Node-Mapnik Installation

## Install Mapnik

  First you need the Mapnik library installed.
  
  The build scripts are set up to use Mapnik2 >= r2401,
  which is current Mapnik trunk as of Nov 17th, 2010.

  Note: you can also customize the 'wscript' file (python code)
  to build against Mapnik 0.7.2.

## Requirements

  Mapnik compiled against libtool (ltdl) >= 2.x.x
  Mapnik trunk > 2397
  Node >= 0.2.4


## More info

  General details about Mapnik2 at:
  
  http://trac.mapnik.org/wiki/Mapnik2
  
  General Mapnik instructions are available at:
  
  http://trac.mapnik.org/wiki/MapnikInstallation
  

## mapnik-config

  Once Mapnik trunk is installed you should have the `mapnik-config`
  program on your path.
  
  This will allow the `wscript` build script to find and link to Mapnik.


## Install node:
  
    $ git clone git://github.com/ry/node.git
    $ cd node
    $ ./configure && make && make install


## Install node-mapnik

  Then install node-mapnik:
 
    $ git clone git://github.com/mapnik/node-mapnik.git
    $ cd node-mapnik
    $ node-waf configure build install
    
  Or use the make wrappers:
  
    ./configure
    make && sudo make install

  If you want to use a custom prefix do:
  
    $ node-waf --prefix=/opt/mapnik configure build install
  
  or:

    $ ./configure --prefix=/opt/mapnik make && sudo make install
  
  
## Alternative install with npm (http://npmjs.org/):

  Install npm:
  
    $ curl http://npmjs.org/install.sh | sh
  
  Install node-mapnik:
    
    $ npm install mapnik


Any build failures, please post to https://github.com/mapnik/node-mapnik/issues.