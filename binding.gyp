{
  'target_defaults': {
      'default_configuration': 'Release',
      'configurations': {
          'Debug': {
              'cflags_cc!': ['-O3', '-DNDEBUG'],
              'xcode_settings': {
                'OTHER_CPLUSPLUSFLAGS!':['-O3', '-DNDEBUG']
              },
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': 1,
            }
          }
          },
        'Release': {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': 1,
            },
     	  'VCLinkerTool': {
              'AdditionalOptions': [
                # https://github.com/mapnik/node-mapnik/issues/74
                '/FORCE:MULTIPLE'
              ],
              'AdditionalLibraryDirectories': [
                 #http://stackoverflow.com/questions/757418/should-i-compile-with-md-or-mt
				 '<!@(mapnik-config --dep-libs)'
              ],
            },
          }
        }
      },
      'include_dirs': [
          './src'
      ],
      'conditions': [
        ['OS=="win"', {
		   'include_dirs':['<!@(mapnik-config --includes)'],
		   'defines': ['<!@(mapnik-config --defines)'],
		   'libraries': ['<!@(mapnik-config --libs)'],
		   'msvs_disabled_warnings': [ 4244,4005,4506,4345,4804 ],
		}],
		['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="netbsd" or OS=="mac"', {
          'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
          'cflags_cc' : ['<!@(mapnik-config --cflags)'],
          'libraries':[
            '<!@(mapnik-config --libs)', # will bring in -lmapnik and the -L to point to it
          ]
        }],
        ['OS=="linux"', {
          'libraries':[
            '-licuuc',
            '-lboost_regex',
            # if the above are not enough, link all libs
            # mapnik uses by uncommenting the next line
            #'<!@(mapnik-config --ldflags --dep-libs)'
          ]
        }],
      ]
  },
  'targets': [
    {
      'target_name': '_mapnik',
      'sources': ["src/node_mapnik.cpp",
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
                   "src/mapnik_proj_transform.cpp",
                   "src/mapnik_layer.cpp",
                   "src/mapnik_datasource.cpp",
                   "src/mapnik_featureset.cpp",
                   "src/mapnik_expression.cpp",
                   "src/mapnik_query.cpp"
      ],
      # this has to be per target to correctly
      # override node-gyp defaults
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '<!@(mapnik-config --cflags)'
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
      },
    }
  ]
}
