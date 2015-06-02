Pod::Spec.new do |s|
  s.name     = 'Protobuf'
  s.version  = '3.0.0'
  s.summary  = 'Protocol Buffers v.3 runtime library for Objective-C.'
  s.homepage = 'https://github.com/google/protobuf'
  s.license  = 'New BSD'
  s.authors  = { 'The Protocol Buffers contributors' => 'protobuf@googlegroups.com' }

  s.source_files = 'objectivec/*.{h,m}',
                   'objectivec/google/protobuf/Any.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Descriptor.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Duration.pbobjc.h',
                   'objectivec/google/protobuf/Empty.pbobjc.{h,m}',
                   'objectivec/google/protobuf/FieldMask.pbobjc.{h,m}',
                   'objectivec/google/protobuf/SourceContext.pbobjc.{h,m}',
                   'objectivec/google/protobuf/Timestamp.pbobjc.h',
                   'objectivec/google/protobuf/Wrappers.pbobjc.{h,m}'
  # The following would cause duplicate symbol definitions. GPBProtocolBuffers is expected to be
  # left out, as it's an umbrella implementation file. For Api, Struct and Type, see issue #449.
  s.exclude_files = 'objectivec/GPBProtocolBuffers.m',
                    'objectivec/google/protobuf/Api.pbobjc.{h,m}',
                    'objectivec/google/protobuf/Struct.pbobjc.{h,m}',
                    'objectivec/google/protobuf/Type.pbobjc.{h,m}'
  # Timestamp.pbobjc.m and Duration.pbobjc.m are #imported by GPBWellKnownTypes.m. So we can't
  # compile them (duplicate symbols), but we need them available for the importing:
  s.preserve_paths = 'objectivec/google/protobuf/Timestamp.pbobjc.m',
                     'objectivec/google/protobuf/Duration.pbobjc.m'
  s.header_mappings_dir = 'objectivec'

  s.ios.deployment_target = '6.0'
  s.osx.deployment_target = '10.8'
  s.requires_arc = false
end
