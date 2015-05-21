Pod::Spec.new do |s|
  s.name     = 'Protobuf'
  s.version  = '3.0.0'
  s.summary  = 'Protocol Buffers v.3 runtime library for Objective-C.'
  s.homepage = 'https://github.com/google/protobuf'
  s.license  = 'New BSD'
  s.authors  = { 'The Protocol Buffers contributors' => 'protobuf@googlegroups.com' }

  s.source_files = 'objectivec/*.{h,m}', 'objectivec/google/protobuf/*.pbobjc.h', 'objectivec/google/protobuf/Descriptor.pbobjc.m'
  # The following is a .m umbrella file, and would cause duplicate symbol
  # definitions:
  s.exclude_files = 'objectivec/GPBProtocolBuffers.m'
  # The .m's of the proto Well-Known-Types under google/protobuf are #imported
  # by GPBWellKnownTypes.m. So we can't compile them (duplicate symbols), but we
  # need them available for the importing:
  s.preserve_paths = 'objectivec/google/protobuf/*.pbobjc.m'
  s.header_mappings_dir = 'objectivec'

  s.ios.deployment_target = '6.0'
  s.osx.deployment_target = '10.8'
  s.requires_arc = false
end
