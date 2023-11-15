# This file describes to Cocoapods how to integrate the Objective-C runtime into a dependent
# project.
# Despite this file being specific to Objective-C, it needs to be on the root of the repository.
# Otherwise, Cocoapods gives trouble like not picking up the license file correctly, or not letting
# dependent projects use the :git notation to refer to the library.
Pod::Spec.new do |s|
  s.name     = 'Protobuf'
  s.version  = '3.25.1'
  s.summary  = 'Protocol Buffers v.3 runtime library for Objective-C.'
  s.homepage = 'https://github.com/protocolbuffers/protobuf'
  s.license  = 'BSD-3-Clause'
  s.authors  = { 'The Protocol Buffers contributors' => 'protobuf@googlegroups.com' }
  s.cocoapods_version = '>= 1.0'

  s.source = { :git => 'https://github.com/protocolbuffers/protobuf.git',
               :tag => "v#{s.version}" }

  s.source_files = 'objectivec/*.{h,m}',
                   'objectivec/google/protobuf/Any.pbobjc.h',
                   'objectivec/google/protobuf/Api.pbobjc.h',
                   'objectivec/google/protobuf/Duration.pbobjc.h',
                   'objectivec/google/protobuf/Empty.pbobjc.h',
                   'objectivec/google/protobuf/FieldMask.pbobjc.h',
                   'objectivec/google/protobuf/SourceContext.pbobjc.h',
                   'objectivec/google/protobuf/Struct.pbobjc.h',
                   'objectivec/google/protobuf/Timestamp.pbobjc.h',
                   'objectivec/google/protobuf/Type.pbobjc.h',
                   'objectivec/google/protobuf/Wrappers.pbobjc.h'
  # The following would cause duplicate symbol definitions. GPBProtocolBuffers is expected to be
  # left out, as it's an umbrella implementation file.
  s.exclude_files = 'objectivec/GPBProtocolBuffers.m'

  # Set a CPP symbol so the code knows to use framework imports.
  s.user_target_xcconfig = { 'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS=1' }
  s.pod_target_xcconfig = { 'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS=1' }

  s.ios.deployment_target = '10.0'
  s.osx.deployment_target = '10.12'
  s.tvos.deployment_target = '12.0'
  s.watchos.deployment_target = '6.0'
  s.requires_arc = false

  # The unittest need the generate sources from the testing related .proto
  # files. So to add a `test_spec`, there would also need to be something like a
  # `script_phases` to generate them, but there would also need to be a way to
  # ensure `protoc` had be built. Another option would be to move to a model
  # where the needed files are always generated and checked in. Neither of these
  # seem that great at the moment, so the tests have *not* been wired into here
  # at this time.
end
