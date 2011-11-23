
{
  'variables': {
      'node_mapnik_sources': [
          "src/node_mapnik.cpp",
          "src/mapnik_map.cpp",
          "src/mapnik_color.cpp",
          "src/mapnik_geometry.cpp",
          "src/mapnik_feature.cpp",
          "src/mapnik_image.cpp",
          "src/mapnik_image_view.cpp",
          "src/mapnik_grid.cpp",
          "src/mapnik_grid_view.cpp",
          "src/mapnik_js_datasource.cpp",
          "src/mapnik_memory_datasource.cpp",
          "src/mapnik_palette.cpp",
          "src/mapnik_projection.cpp",
          "src/mapnik_layer.cpp",
          "src/mapnik_datasource.cpp",
          "src/mapnik_featureset.cpp"
      ]
  },
  'targets': [
    {
      'target_name': '_mapnik',
      'product_name': '_mapnik',
      'type': 'loadable_module',
      'product_prefix': '',
      'product_extension':'node',
      'sources': [
        '<@(node_mapnik_sources)',
      ],
      'defines': [
        'PLATFORM="<(OS)"',
      ],
      'conditions': [
        [ 'OS=="mac"', {
          'libraries': [
            '-lmapnik2',
            '-undefined dynamic_lookup'
          ],
          'include_dirs': [
             'src/',
             '/usr/local/include/node',
             '/opt/boost-48/include',
             '/usr/local/Cellar/icu4c/4.8.1.1/include',
             '/usr/X11/include/freetype2',
             '/usr/X11/include',
             '/usr/include',
          ],
          'defines': [
            '_LARGEFILE_SOURCE',
            '_FILE_OFFSET_BITS=64',
          ],
        }],
        [ 'OS=="win"', {
          'defines': [
            'PLATFORM="win32"',
          ],
          'libraries': [ 'node.lib' ],
          'include_dirs': [
             'c:\\dev2\\node\\src',
             'c:\\dev2\\node\\deps\\uv\\include',
             'c:\\dev2\\node\\deps\\v8\\include',
             'c:\\dev2\\freetype',
             'c:\\dev2\\icu',
             'c:\\dev2\\boost',
          ],
          'defines': [
            'PLATFORM="<(OS)"',
            '_LARGEFILE_SOURCE',
            '_FILE_OFFSET_BITS=64',
            #'WIN32',
            #'_WINDOWS',
            #'_USRDLL',
            #'BUILDING_NODE_EXTENSION'
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalLibraryDirectories': [
                'c:\\dev2\\node\\Debug',
              ],
          },
        },
      }],
      ]
    },
  ],
}