Pod::Spec.new do |s|
  s.name     = 'Protobuf-C++'
  s.version  = '3.15.8'
  s.summary  = 'Protocol Buffers v3 runtime library for C++.'
  s.homepage = 'https://github.com/google/protobuf'
  s.license  = '3-Clause BSD License'
  s.authors  = { 'The Protocol Buffers contributors' => 'protobuf@googlegroups.com' }
  s.cocoapods_version = '>= 1.0'

  s.source = { :git => 'https://github.com/google/protobuf.git',
               :tag => "v#{s.version}" }

  s.source_files = 'src/google/protobuf/*.{h,cc,inc}',
                   'src/google/protobuf/stubs/*.{h,cc}',
                   'src/google/protobuf/io/*.{h,cc}',
                   'src/google/protobuf/util/*.{h,cc}',
                   'src/google/protobuf/util/internal/*.{h,cc}'

  # Excluding all the tests in the directories above
  s.exclude_files = 'src/google/**/*_test.{h,cc,inc}',
                    'src/google/**/*_unittest.{h,cc}',
                    'src/google/protobuf/test_util*.{h,cc}',
                    'src/google/protobuf/map_lite_test_util.{h,cc}',
                    'src/google/protobuf/map_test_util*.{h,cc,inc}'

  s.header_mappings_dir = 'src'

  s.ios.deployment_target = '7.0'
  s.osx.deployment_target = '10.9'
  s.tvos.deployment_target = '9.0'
  s.watchos.deployment_target = '2.0'

  s.pod_target_xcconfig = {
    # Do not let src/google/protobuf/stubs/time.h override system API
    'USE_HEADERMAP' => 'NO',
    'ALWAYS_SEARCH_USER_PATHS' => 'NO',

    # Configure tool is not being used for Xcode. When building, assume pthread is supported.
    'GCC_PREPROCESSOR_DEFINITIONS' => '"$(inherited)" "HAVE_PTHREAD=1"',
  }

end
