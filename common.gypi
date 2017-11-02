{
  'target_defaults': {
    'default_configuration': 'Release',
    # the v140 refers to vs2015
    'msbuild_toolset':'v140',
    'msvs_disabled_warnings': [ 4503, 4068,4244,4005,4506,4345,4804,4805,4661 ],
    'cflags_cc' : [
      '-std=c++14',
    ],
    'cflags_cc!': ['-std=gnu++0x','-fno-rtti', '-fno-exceptions'],
    'configurations': {
      'Debug': {
        'defines!': [
          'NDEBUG'
        ],
        'cflags_cc!': [
          '-O3',
          '-Os',
          '-DNDEBUG',
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS!': [
            '-O3',
            '-Os',
            '-DDEBUG'
          ],
          'GCC_OPTIMIZATION_LEVEL': '0',
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES'
        },
        'msvs_settings': {
          'VCCLCompilerTool': {
            'ExceptionHandling': 1, # /EHsc
            'RuntimeTypeInfo': 'true', # /GR
            'RuntimeLibrary': '2', # /MD
            "AdditionalOptions": [
              "/MP", # compile across multiple CPUs
              "/bigobj", #compiling: x86 fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
            ],
          }
        }
      },
      'Release': {
        'defines': [
          'NDEBUG'
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS!': [
            '-Os',
            '-O2'
          ],
          'GCC_OPTIMIZATION_LEVEL': '3',
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'NO',
          'DEAD_CODE_STRIPPING': 'YES',
          'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES'
        },
        'msvs_settings': {
          'VCCLCompilerTool': {
            'ExceptionHandling': 1, # /EHsc
            'RuntimeTypeInfo': 'true', # /GR
            'RuntimeLibrary': '2', # /MD
            "AdditionalOptions": [
              "/MP", # compile across multiple CPUs
              "/bigobj", #compiling: x86 fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
            ],
            "DebugInformationFormat": "3"
          },
          "VCLinkerTool": {
            "GenerateDebugInformation": "true",
            "ProgramDatabaseFile": "$(OutDir)$(TargetName)$(TargetExt).pdb",
          }
        }
      }
    }
  }
}
