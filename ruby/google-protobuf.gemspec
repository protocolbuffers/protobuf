Gem::Specification.new do |s|
  s.name        = "google-protobuf"
  s.version     = "3.0.2"
  s.licenses    = ["BSD"]
  s.summary     = "Protocol Buffers"
  s.description = "Protocol Buffers are Google's data interchange format."
  s.homepage    = "https://developers.google.com/protocol-buffers"
  s.authors     = ["Protobuf Authors"]
  s.email       = "protobuf@googlegroups.com"
  s.require_paths = ["lib"]
  s.files       = Dir.glob('lib/**/*.rb')
  if RUBY_PLATFORM == "java"
    s.files     += ["lib/google/protobuf_java.jar"]
  else
    s.files     += Dir.glob('ext/**/*')
    s.extensions= ["ext/google/protobuf_c/extconf.rb"]
    s.add_development_dependency "rake-compiler-dock"
  end
  s.test_files  = ["tests/basic.rb",
                  "tests/stress.rb",
                  "tests/generated_code_test.rb"]
  s.add_development_dependency "rake-compiler"
  s.add_development_dependency "test-unit"
  s.add_development_dependency "rubygems-tasks"
end
