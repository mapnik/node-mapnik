import os

settings = 'lib/mapnik_settings.js'

# this goes into a mapnik_settings.js file beside the C++ _mapnik.node
settings_template = """
module.exports.paths = {
    'fonts': %s,
    'input_plugins': %s
};
"""

def write_mapnik_settings(fonts='undefined',input_plugins='undefined'):
    global settings_template
    if '__dirname' in fonts or '__dirname' in input_plugins:
        settings_template = "var path = require('path');\n" + settings_template
    open(settings,'w').write(settings_template % (fonts,input_plugins))


if __name__ == '__main__':
    settings_dict = {}
    
    # settings for fonts and input plugins
    if os.environ.has_key('MAPNIK_INPUT_PLUGINS'):
        settings_dict['input_plugins'] =  os.environ['MAPNIK_INPUT_PLUGINS']
    else:
        settings_dict['input_plugins'] = '\'%s\'' % os.popen("mapnik-config --input-plugins").readline().strip()
    
    if os.environ.has_key('MAPNIK_FONTS'):
        settings_dict['fonts'] =  os.environ['MAPNIK_FONTS']
    else:
        settings_dict['fonts'] = '\'%s\'' % os.popen("mapnik-config --fonts").readline().strip()
    
    write_mapnik_settings(**settings_dict)