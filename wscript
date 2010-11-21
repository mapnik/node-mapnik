import Options
from os import unlink, symlink, popen, uname, environ
from os.path import exists
from shutil import copy2 as copy
from subprocess import call

TARGET = '_mapnik'
TARGET_FILE = '%s.node' % TARGET
built = 'build/default/%s' % TARGET_FILE
dest = 'mapnik/%s' % TARGET_FILE
settings = 'mapnik/settings.js'

# only works with Mapnik2/trunk..
AUTOCONFIGURE = True

# this goes into a settings.js file beside the C++ _mapnik.node
settings_template = """
module.exports.paths = {
    'fonts': '%s',
    'input_plugins': '%s',
};
"""

def write_mapnik_settings(fonts='',input_plugins=''):
    open(settings,'w').write(settings_template % (fonts,input_plugins))

def set_options(opt):
    opt.tool_options("compiler_cxx")
    #opt.add_option('-D', '--debug', action='store_true', default=False, dest='debug')

def configure(conf):
    conf.check_tool("compiler_cxx")
    conf.check_tool("node_addon")
    settings_dict = {}

    if AUTOCONFIGURE:
        # attempt to use clang++ if available instead of g++
        if call(['clang++','--version']) == 0:
            environ['CXX'] = 'clang++'

        # Note, working 'mapnik-config' is only available with mapnik >= r2378
        print 'NOTICE: searching for "mapnik-config" program, requires mapnik >= r2378'
        mapnik_config = conf.find_program('mapnik-config', var='MAPNIK_CONFIG', mandatory=True)
        # todo - check return value of popen other we can end up with
        # return of 'Usage: mapnik-config [OPTION]'
        mapnik_libdir = popen("%s --libs" % mapnik_config).readline().strip().split(' ')[:2]
        conf.env.append_value("LINKFLAGS_MAPNIK", mapnik_libdir)
        conf.env.append_value("LIB_MAPNIK", "mapnik2")
        mapnik_includedir = popen("%s --cflags" % mapnik_config).readline().strip()
        conf.env.append_value("CXXFLAGS_MAPNIK", mapnik_includedir.split(' '))
        
        # settings for fonts and input plugins
        settings_dict['input_plugins'] = popen("%s --input-plugins" % mapnik_config).readline().strip()
        settings_dict['fonts'] = popen("%s --fonts" % mapnik_config).readline().strip()
        
    else:
        
        prefix = "/usr/local"
        
        import platform
        if platform.dist()[0] in ('Ubuntu','debian'):
            LIBDIR_SCHEMA='lib'
        elif platform.uname()[4] == 'x86_64' and platform.system() == 'Linux':
            LIBDIR_SCHEMA='lib64' 
        elif platform.uname()[4] == 'ppc64':
            LIBDIR_SCHEMA='lib64'
        else:
            LIBDIR_SCHEMA='lib'

        MAPNIK2 = False
        # manual configure of Mapnik 0.7.x, as only trunk provides a mapnik-config
        
        # mapnik 0.7.x
        if MAPNIK2:
            # libmapnik2
            conf.env.append_value("LIB_MAPNIK", "mapnik2")
        else:
            conf.env.append_value("LIB_MAPNIK", "mapnik")
        
        # add path of mapnik lib
        conf.env.append_value("LIBPATH_MAPNIK", "%s/%s" % (prefix,LIBDIR_SCHEMA))
        ldflags = []
        if platform.uname()[0] == "Darwin":
            ldflags.append('-L/usr/X11/lib/')
        #libs = ['-lboost_thread','-lboost_regex','-libpng12','-libjpeg']
        #if uname()[0] == 'Darwin':
        #   pass #libs.append('-L/usr/X11/lib/','-libicuuc')
        if ldflags:
            conf.env.append_value("LDFLAGS", ldflags)
        
        # add path of mapnik headers
        conf.env.append_value("CPPPATH_MAPNIK", "%s/include" % prefix)
        
        # add paths for freetype, boost, icu, as needed
        cxxflags = popen("freetype-config --cflags").readline().strip().split(' ')
        cxxflags.append('-I/usr/include')
        cxxflags.append('-I/usr/local/include')
        conf.env.append_value("CXXFLAGS_MAPNIK", cxxflags)

        # settings for fonts and input plugins
        if MAPNIK2:
            settings_dict['input_plugins'] = "%s/%s/mapnik2/input" % (prefix,LIBDIR_SCHEMA)
            settings_dict['fonts'] = "%s/%s/mapnik2/fonts" % (prefix,LIBDIR_SCHEMA)
        else:
            settings_dict['input_plugins'] = "%s/lib/mapnik/input" % prefix
            settings_dict['fonts'] = "%s/lib/mapnik/fonts" % prefix

    write_mapnik_settings(**settings_dict)

def build(bld):
    obj = bld.new_task_gen("cxx", "shlib", "node_addon", install_path=None)
    obj.cxxflags = ["-DNDEBUG", "-O3", "-Wall", "-DBOOST_SPIRIT_THREADSAFE", "-DMAPNIK_THREADSAFE","-ansi","-finline-functions","-Wno-inline"]
    obj.target = TARGET
    obj.source = "src/%s.cc" % TARGET
    obj.uselib = "MAPNIK"
    bld.install_files('${PREFIX}/lib/node/mapnik/', 'mapnik/*')

#def install(args,**kwargs):
#    import pdb;pdb.set_trace()
    
def shutdown():
    if Options.commands['clean']:
        if exists(TARGET): unlink(TARGET)
    if Options.commands['clean']:
        if exists(dest):
            unlink(dest)
    else:
        if exists(built):
            copy(built,dest)
