# This file describes to Cocoapods how to integrate the Objective-C runtime into a dependent
# project.
# Despite this file being specific to Objective-C, it needs to be on the root of the repository.
# Otherwise, Cocoapods gives trouble like not picking up the license file correctly, or not letting
# dependent projects use the :git notation to refer to the library.
Pod::Spec.new do |s|
  s.name     = 'Protobuf'
  s.version  = '3.3.2'
  s.summary  = 'Protocol Buffers v.3 runtime library for Objective-C.'
  s.homepage = 'https://github.com/google/protobuf'
  s.license  = '3-Clause BSD License'
  s.authors  = { 'The Protocol Buffers contributors' => 'protobuf@googlegroups.com' }
  s.cocoapods_version = '>= 1.0'

  s.source = { :git => 'https://github.com/google/protobuf.git',
               :tag => "v#{s.version}" }

  s.source_files = 'objectivec/*.{h,m}',
                   'objectivec/google/protobuf/Any.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Api.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Duration.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Empty.pbobjc.{h,m}',
                   'objectivec/google/protobuf/FieldMask.pbobjc.{h,m}',
                   'objectivec/google/protobuf/SourceContext.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Struct.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Timestamp.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Type.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Wrappers.pbobjc.{h,m}'
  # The following would cause duplicate symbol definitions. GPBProtocolBuffers is expected to be
  # left out, as it's an umbrella implementation file.
  s.exclude_files = 'objectivec/GPBProtocolBuffers.m'

  # Set a CPP symbol so the code knows to use framework imports.
  s.user_target_xcconfig = { 'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS=1' }
  s.pod_target_xcconfig = { 'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS=1' }

  s.ios.deployment_target = '7.0'
  s.osx.deployment_target = '10.9'
  s.tvos.deployment_target = '9.0'
  s.watchos.deployment_target = '2.0'
  s.requires_arc = false
end
