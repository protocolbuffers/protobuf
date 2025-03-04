# This file describes to Cocoapods how to integrate the Objective-C runtime into a dependent
# project.
# Despite this file being specific to Objective-C, it needs to be on the root of the repository.
# Otherwise, Cocoapods gives trouble like not picking up the license file correctly, or not letting
# dependent projects use the :git notation to refer to the library.
Pod::Spec.new do |s|
  s.name     = 'Protobuf'
  s.version  = '4.30.1'
  s.summary  = 'Protocol Buffers v.3 runtime library for Objective-C.'
  s.homepage = 'https://github.com/protocolbuffers/protobuf'
  s.license  = 'BSD-3-Clause'
  s.authors  = { 'The Protocol Buffers contributors' => 'protobuf@googlegroups.com' }

  # Ensure developers won't hit CocoaPods/CocoaPods#11402 with the resource
  # bundle for the privacy manifest.
  s.cocoapods_version = '>= 1.12.0'

  s.source = { :git => 'https://github.com/protocolbuffers/protobuf.git',
               :tag => "v#{s.version}" }

  s.source_files = 'objectivec/*.{h,m,swift}'
  # The following would cause duplicate symbol definitions. GPBProtocolBuffers is expected to be
  # left out, as it's an umbrella implementation file.
  s.exclude_files = 'objectivec/GPBProtocolBuffers.m'

  # Now that there is a Swift source file, set a version.
  s.swift_version = '5.0'

  s.resource_bundle = {
    "Protobuf_Privacy" => "PrivacyInfo.xcprivacy"
  }

  # Set a CPP symbol so the code knows to use framework imports.
  s.user_target_xcconfig = { 'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS=1' }
  s.pod_target_xcconfig = { 'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS=1' }

  s.ios.deployment_target = '15.0'
  s.osx.deployment_target = '11.0'
  # The following are best-effort / community supported, and are not covered by
  # our official support policies: https://protobuf.dev/support/version-support/
  s.tvos.deployment_target = '12.0'
  s.watchos.deployment_target = '6.0'
  s.visionos.deployment_target = '1.0'
  s.requires_arc = false

  # The unittest need the generate sources from the testing related .proto
  # files. So to add a `test_spec`, there would also need to be something like a
  # `script_phases` to generate them, but there would also need to be a way to
  # ensure `protoc` had be built. Another option would be to move to a model
  # where the needed files are always generated and checked in. Neither of these
  # seem that great at the moment, so the tests have *not* been wired into here
  # at this time.
end
