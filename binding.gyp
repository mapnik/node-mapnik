{
  'includes': [ 'common.gypi' ],
  'variables': {
      'ENABLE_GLIBC_WORKAROUND%':'false', # can be overriden by a command line variable because of the % sign
      'enable_sse%':'true'
  },
  'targets': [
    {
      'target_name': '<(module_name)',
      'product_dir': '<(module_path)',
      'sources': [
        "src/mapnik_logger.cpp",
        "src/node_mapnik.cpp",
        "src/blend.cpp",
        "src/mapnik_map.cpp",
        "src/mapnik_map_load.cpp",
        "src/mapnik_map_from_string.cpp",
        "src/mapnik_map_render.cpp",
        "src/mapnik_map_query_point.cpp",
        "src/mapnik_color.cpp",
        "src/mapnik_geometry.cpp",
        "src/mapnik_feature.cpp",
        "src/mapnik_image.cpp",
        "src/mapnik_image_encode.cpp",
        "src/mapnik_image_open.cpp",
        "src/mapnik_image_fill.cpp",
        "src/mapnik_image_save.cpp",
        "src/mapnik_image_from_bytes.cpp",
        "src/mapnik_image_from_svg.cpp",
        "src/mapnik_image_solid.cpp",
        "src/mapnik_image_multiply.cpp",
        "src/mapnik_image_clear.cpp",
        "src/mapnik_image_copy.cpp",
        "src/mapnik_image_resize.cpp",
        "src/mapnik_image_compositing.cpp",
        "src/mapnik_image_filter.cpp",
        "src/mapnik_image_view.cpp",
        "src/mapnik_grid.cpp",
        "src/mapnik_grid_view.cpp",
        "src/mapnik_palette.cpp",
        "src/mapnik_projection.cpp",
        "src/mapnik_layer.cpp",
        "src/mapnik_datasource.cpp",
        "src/mapnik_featureset.cpp",
        "src/mapnik_expression.cpp",
        "src/mapnik_cairo_surface.cpp",
        "src/mapnik_vector_tile.cpp",
        "src/mapnik_vector_tile_data.cpp",
        "src/mapnik_vector_tile_query.cpp",
        "src/mapnik_vector_tile_json.cpp",
        "src/mapnik_vector_tile_info.cpp",
        "src/mapnik_vector_tile_simple_valid.cpp",
        "src/mapnik_vector_tile_render.cpp",
        "src/mapnik_vector_tile_clear.cpp",
        "src/mapnik_vector_tile_image.cpp",
        "src/mapnik_vector_tile_composite.cpp",
        "node_modules/mapnik-vector-tile/src/vector_tile_compression.cpp",
        "node_modules/mapnik-vector-tile/src/vector_tile_datasource_pbf.cpp",
        "node_modules/mapnik-vector-tile/src/vector_tile_featureset_pbf.cpp",
        "node_modules/mapnik-vector-tile/src/vector_tile_geometry_decoder.cpp",
        "node_modules/mapnik-vector-tile/src/vector_tile_geometry_encoder_pbf.cpp",
        "node_modules/mapnik-vector-tile/src/vector_tile_layer.cpp",
        "node_modules/mapnik-vector-tile/src/vector_tile_processor.cpp",
        "node_modules/mapnik-vector-tile/src/vector_tile_raster_clipper.cpp",
        "node_modules/mapnik-vector-tile/src/vector_tile_tile.cpp"
      ],
      'include_dirs': [
        './mason_packages/.link/include/',
        './mason_packages/.link/include/freetype2',
        './mason_packages/.link/include/cairo',
        './mason_packages/.link/include/mapnik',
        './src',
        "<!@(node -p \"require('node-addon-api').include\")",
        # TODO: move these to mason packages once we have a minimal windows client for mason (@springmeyer)
        # https://github.com/mapbox/mason/issues/396
        "./deps/geometry/include/",
        "./deps/protozero/include/",
        "./deps/wagyu/include/",
        "<!(node -e \"require('mapnik-vector-tile')\")"
      ],
      'defines': [
          'MAPNIK_GIT_REVISION="<!@(mapnik-config --git-describe)"',
          'MAPNIK_VECTOR_TILE_LIBRARY=1',
      ],
      'conditions': [
        ['ENABLE_GLIBC_WORKAROUND != "false"', {
            'sources': [
              "src/glibc_workaround.cpp"
            ]
        }],
        ['OS=="win"',
          {
            'include_dirs':[
              '<!@(mapnik-config --includes)',
              '<!@(mapnik-config --dep-includes)'
            ],
            'defines': ['NOMINMAX','<!@(mapnik-config --defines)'],
            'defines!': ["_HAS_EXCEPTIONS=0"],
            'libraries': [
              '<!@(mapnik-config --libs)',
              'mapnik-wkt.lib',
              'mapnik-json.lib',
              '<!@(mapnik-config --dep-libs)',
            ],
            'msvs_disabled_warnings': [ 4244,4005,4506,4345,4804,4805 ],
            'msvs_settings': {
              'VCLinkerTool': {
                'AdditionalLibraryDirectories': [
                  '<!@(mapnik-config --ldflags)'
                ],
              },
            }
          },
          {
            'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
            'cflags_cc' : [
              '<!@(mapnik-config --cflags)',
            ],
            'libraries':[
              '<!@(mapnik-config --libs)',
              '-lmapnik-wkt',
              '-lmapnik-json',
              '<!@(mapnik-config --ldflags)',
              '<!@(mapnik-config --dep-libs)'
            ],
            'ldflags': [
              '-Wl,-z,now',
              "-Wl,-z,origin",
              "-Wl,-rpath=\$$ORIGIN/lib"
            ],
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS':[
                '<!@(mapnik-config --cflags)',
              ],
              'OTHER_CFLAGS':[
                '<!@(mapnik-config --cflags)'
              ],
              'OTHER_LDFLAGS':[
                '-Wl,-bind_at_load'
              ],
              'GCC_ENABLE_CPP_RTTI': 'YES',
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
              'MACOSX_DEPLOYMENT_TARGET':'10.8',
              'CLANG_CXX_LIBRARY': 'libc++',
              'CLANG_CXX_LANGUAGE_STANDARD':'c++14',
              'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
            }
          },
        ],
        ['enable_sse == "true"', {
          'defines' : [ 'SSE_MATH' ]
        }]
      ]
    },
    {
      'target_name': 'action_after_build',
      'type': 'none',
      'dependencies': [ '<(module_name)' ],
      'hard_dependency': 1,
      'conditions': [
        ['OS!="win"',
          {
            'actions': [
              {
                'action_name': 'postinstall',
                'inputs': ['./scripts/postinstall.sh'],
                'outputs': ['./lib/binding/mapnik'],
                'action': ['./scripts/postinstall.sh']
              }
            ]
          }
        ]
      ]
    },
  ]
}
