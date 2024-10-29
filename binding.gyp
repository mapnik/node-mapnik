{
  'includes': [ 'common.gypi' ],
  'variables': {
      'ENABLE_GLIBC_WORKAROUND%':'false', # can be overriden by a command line variable because of the % sign
  },
  'targets': [
    {
      'target_name': 'mapnik',
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
        "deps/mapnik-vector-tile/src/vector_tile_compression.cpp",
        "deps/mapnik-vector-tile/src/vector_tile_datasource_pbf.cpp",
        "deps/mapnik-vector-tile/src/vector_tile_featureset_pbf.cpp",
        "deps/mapnik-vector-tile/src/vector_tile_geometry_decoder.cpp",
        "deps/mapnik-vector-tile/src/vector_tile_geometry_encoder_pbf.cpp",
        "deps/mapnik-vector-tile/src/vector_tile_layer.cpp",
        "deps/mapnik-vector-tile/src/vector_tile_processor.cpp",
        "deps/mapnik-vector-tile/src/vector_tile_raster_clipper.cpp",
        "deps/mapnik-vector-tile/src/vector_tile_tile.cpp"
      ],
      'include_dirs': [
        './src',
        "<!@(node -p \"require('node-addon-api').include\")",
        "./deps/geometry/include/",
        "./deps/protozero/include/",
        "./deps/wagyu/include/",
        "./deps/mapnik-vector-tile/src"
      ],
      'defines': [
          'MAPNIK_GIT_REVISION="<!@(mapnik-config --git-describe)"',
          'MAPNIK_VECTOR_TILE_LIBRARY=1',
      ],
      'conditions': [
        ['"<!@(uname -p)"=="x86_64"',{
           'defines' : [ 'SSE_MATH' ]
        }],
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
              "-Wl,-rpath=\\$$ORIGIN/lib"
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
              'MACOSX_DEPLOYMENT_TARGET':'10.15.7',
              'CLANG_CXX_LIBRARY': 'libc++',
              'CLANG_CXX_LANGUAGE_STANDARD':'c++20',
              'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
            }
          },
        ]
      ]
    },
  ]
}
