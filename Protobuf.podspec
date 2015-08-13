# This file describes to Cocoapods how to integrate the Objective-C runtime into a dependent
# project.
# Despite this file being specific to Objective-C, it needs to be on the root of the repository.
# Otherwise, Cocoapods gives trouble like not picking up the license file correctly, or not letting
# dependent projects use the :git notation to refer to the library.
Pod::Spec.new do |s|
  s.name     = 'Protobuf'
  s.version  = '3.0.0-alpha-4-pre'
  s.summary  = 'Protocol Buffers v.3 runtime library for Objective-C.'
  s.homepage = 'https://github.com/google/protobuf'
  s.license  = 'New BSD'
  s.authors  = { 'The Protocol Buffers contributors' => 'protobuf@googlegroups.com' }

  s.source_files = 'objectivec/GPBProtocolBuffers.{h,m}'
  # GPBProtocolBuffers.{h,m} are umbrella files. We need Cocoapods to preserve the files imported by
  # them.
  s.preserve_paths = 'objectivec/*.{h,m}',
                     'objectivec/google/protobuf/*.pbobjc.{h,m}'
  s.header_mappings_dir = 'objectivec'

  s.ios.deployment_target = '7.1'
  s.osx.deployment_target = '10.9'
  s.requires_arc = false
end
