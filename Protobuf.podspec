# This file describes to Cocoapods how to integrate the Objective-C runtime into a dependent
# project.
# Despite this file being specific to Objective-C, it needs to be on the root of the repository.
# Otherwise, Cocoapods gives trouble like not picking up the license file correctly, or not letting
# dependent projects use the :git notation to refer to the library.
Pod::Spec.new do |s|
  s.name     = 'Protobuf'
  s.version  = '3.0.0-alpha-4.1'
  s.summary  = 'Protocol Buffers v.3 runtime library for Objective-C.'
  s.homepage = 'https://github.com/google/protobuf'
  s.license  = 'New BSD'
  s.authors  = { 'The Protocol Buffers contributors' => 'protobuf@googlegroups.com' }

  s.source_files = 'objectivec/*.{h,m}',
                   'objectivec/google/protobuf/Any.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Api.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Descriptor.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Duration.pbobjc.h',
                   'objectivec/google/protobuf/Empty.pbobjc.{h,m}',
                   'objectivec/google/protobuf/FieldMask.pbobjc.{h,m}',
                   'objectivec/google/protobuf/SourceContext.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Struct.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Timestamp.pbobjc.h',
                   'objectivec/google/protobuf/Type.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Wrappers.pbobjc.{h,m}'
  # Timestamp.pbobjc.m and Duration.pbobjc.m are #imported by GPBWellKnownTypes.m. So we can't
  # compile them (duplicate symbols), but we need them available for the importing:
  s.preserve_paths = 'objectivec/google/protobuf/Duration.pbobjc.m',
                     'objectivec/google/protobuf/Timestamp.pbobjc.m'
  # The following would cause duplicate symbol definitions. GPBProtocolBuffers is expected to be
  # left out, as it's an umbrella implementation file.
  s.exclude_files = 'objectivec/GPBProtocolBuffers.m'
  s.header_mappings_dir = 'objectivec'

  s.ios.deployment_target = '7.1'
  s.osx.deployment_target = '10.9'
  s.requires_arc = false
end
