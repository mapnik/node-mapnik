{
  'includes': [ 'common.gypi' ],
  'variables': {
      'runtime_link%':'shared',
  },
  'targets': [
    {
      'target_name': '_mapnik',
      'sources': [
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
          "src/mapnik_proj_transform.cpp",
          "src/mapnik_layer.cpp",
          "src/mapnik_datasource.cpp",
          "src/mapnik_featureset.cpp",
          "src/mapnik_expression.cpp",
          "src/mapnik_query.cpp",
      ],
      'include_dirs': [
          './src'
      ],
      'conditions': [
        ['OS=="win"', {
            'include_dirs':[
                '<!@(mapnik-config --includes)',
                '<!@(mapnik-config --dep-includes)'
              ],
            'defines': ['<!@(mapnik-config --defines)'],
            'libraries': [
                '<!@(mapnik-config --libs)',
                '<!@(mapnik-config --dep-libs)'
            ],
            'msvs_disabled_warnings': [ 4244,4005,4506,4345,4804 ],
            'msvs_settings': {
            'VCCLCompilerTool': {
                # note: not respected, need to change in C:\Users\mapnik\.node-gyp\*\common.gypi
                'ExceptionHandling': 1,
                'RuntimeTypeInfo':'true',
                'RuntimeLibrary': '2'  # /MD http://stackoverflow.com/questions/757418/should-i-compile-with-md-or-mt
            },
            'VCLinkerTool': {
                'AdditionalOptions': [
                    # https://github.com/mapnik/node-mapnik/issues/74
                    '/FORCE:MULTIPLE'
                ],
                'AdditionalLibraryDirectories': [
                    '<!@(mapnik-config --ldflags)'
                ],
            },
          }
        },{
            'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
            'cflags_cc' : [
                '<!@(mapnik-config --cflags)'
            ],
            'libraries':[
                '<!@(mapnik-config --libs)'
            ],
            'conditions': [
              ['runtime_link == "static"', {
                  'libraries': [
                    '<!@(mapnik-config --ldflags)',
                    '<!@(mapnik-config --dep-libs)'
                   ]
              }]
            ],
        }]
      ],
      # this has to be per target to correctly
      # override node-gyp defaults
      'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS':[
              '<!@(mapnik-config --cflags)'
          ],
          'OTHER_CFLAGS':[
              '<!@(mapnik-config --cflags)'
          ],
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
      },
      'msvs_settings': {
          'VCCLCompilerTool': {
              'ExceptionHandling': 1,
              'RuntimeTypeInfo':'true',
              'RuntimeLibrary': '3'  # /MDd
          }
      }
    },
    {
      'target_name': 'action_after_build',
      'type': 'none',
      'dependencies': [ '_mapnik' ],
      'actions': [
          {
            'action_name': 'generate_setting',
            'inputs': [
              'gen_settings.py'
            ],
            'outputs': [
              'lib/mapnik_settings.js'
            ],
            'action': ['python', 'gen_settings.py']
          },
      ],
      'copies': [
          {
            'files': [ '<(PRODUCT_DIR)/_mapnik.node' ],
            'destination': './lib/'
          }
      ]
    }
  ]
}