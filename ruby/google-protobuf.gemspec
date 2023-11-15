Gem::Specification.new do |s|
  s.name        = "google-protobuf"
  s.version     = "3.25.1"
  git_tag       = "v#{s.version.to_s.sub('.rc.', '-rc')}" # Converts X.Y.Z.rc.N to vX.Y.Z-rcN, used for the git tag
  s.licenses    = ["BSD-3-Clause"]
  s.summary     = "Protocol Buffers"
  s.description = "Protocol Buffers are Google's data interchange format."
  s.homepage    = "https://developers.google.com/protocol-buffers"
  s.authors     = ["Protobuf Authors"]
  s.email       = "protobuf@googlegroups.com"
  s.metadata    = { "source_code_uri" => "https://github.com/protocolbuffers/protobuf/tree/#{git_tag}/ruby" }
  s.require_paths = ["lib"]
  s.files       = Dir.glob('lib/**/*.{rb,rake}')
  if RUBY_PLATFORM == "java"
    s.platform  = "java"
    s.files     += ["lib/google/protobuf_java.jar"] +
      Dir.glob('ext/**/*').reject do |file|
        File.basename(file) =~ /^((convert|defs|map|repeated_field)\.[ch]|
                                   BUILD\.bazel|extconf\.rb|wrap_memcpy\.c)$/x
      end
    s.extensions = ["ext/google/protobuf_c/Rakefile"]
    s.add_dependency "ffi", "~>1"
    s.add_dependency "ffi-compiler", "~>1"
  else
    s.files     += Dir.glob('ext/**/*').reject do |file|
      File.basename(file) =~ /^(BUILD\.bazel)$/
    end
    s.extensions = %w[
      ext/google/protobuf_c/extconf.rb
      ext/google/protobuf_c/Rakefile
    ]
    s.add_development_dependency "rake-compiler-dock", "= 1.2.1"
  end
  s.required_ruby_version = '>= 2.7'
  s.add_development_dependency "rake", "~> 13"
  s.add_development_dependency "ffi", "~>1"
  s.add_development_dependency "ffi-compiler", "~>1"
  s.add_development_dependency "rake-compiler", "~> 1.1.0"
  s.add_development_dependency "test-unit", '~> 3.0', '>= 3.0.9'
end
