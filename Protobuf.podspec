# This file describes to Cocoapods how to integrate the Objective-C runtime into a dependent
# project.
# Despite this file being specific to Objective-C, it needs to be on the root of the repository.
# Otherwise, Cocoapods gives trouble like not picking up the license file correctly, or not letting
# dependent projects use the :git notation to refer to the library.
Pod::Spec.new do |s|
  s.name     = 'Protobuf'
  s.version  = '3.19.4'
  s.summary  = 'Protocol Buffers v.3 runtime library for Objective-C.'
  s.homepage = 'https://github.com/protocolbuffers/protobuf'
  s.license  = '3-Clause BSD License'
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

  s.ios.deployment_target = '9.0'
  s.osx.deployment_target = '10.9'
  s.tvos.deployment_target = '9.0'
  s.watchos.deployment_target = '2.0'
  s.requires_arc = false
end
