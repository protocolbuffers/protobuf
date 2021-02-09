Gem::Specification.new do |s|
  s.name        = "google-protobuf"
  s.version     = "3.14.0"
  git_tag       = "v#{s.version.to_s.sub('.rc.', '-rc')}" # Converts X.Y.Z.rc.N to vX.Y.Z-rcN, used for the git tag
  s.licenses    = ["BSD-3-Clause"]
  s.summary     = "Protocol Buffers"
  s.description = "Protocol Buffers are Google's data interchange format."
  s.homepage    = "https://developers.google.com/protocol-buffers"
  s.authors     = ["Protobuf Authors"]
  s.email       = "protobuf@googlegroups.com"
  s.metadata    = { "source_code_uri" => "https://github.com/protocolbuffers/protobuf/tree/#{git_tag}/ruby" }
  s.require_paths = ["lib"]
  s.files       = Dir.glob('lib/**/*.rb')
  if RUBY_PLATFORM == "java"
    s.platform  = "java"
    s.files     += ["lib/google/protobuf_java.jar"]
  else
    s.files     += Dir.glob('ext/**/*')
    s.extensions= ["ext/google/protobuf_c/extconf.rb"]
    s.add_development_dependency "rake-compiler-dock", ">= 1.0.1", "< 2.0"
  end
  s.test_files  = ["tests/basic.rb",
                  "tests/stress.rb",
                  "tests/generated_code_test.rb"]
  s.required_ruby_version = '>= 2.3'
  s.add_development_dependency "rake-compiler", "~> 1.1.0"
  s.add_development_dependency "test-unit", '~> 3.0', '>= 3.0.9'
  s.add_development_dependency "rubygems-tasks", "~> 0.2.4"
end
