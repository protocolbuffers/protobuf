Gem::Specification.new do |s|
  s.name        = "google-protobuf"
  s.version     = "3.0.0.alpha.5.0"
  s.licenses    = ["BSD"]
  s.summary     = "Protocol Buffers"
  s.description = "Protocol Buffers are Google's data interchange format."
  s.homepage    = "https://developers.google.com/protocol-buffers"
  s.authors     = ["Protobuf Authors"]
  s.email       = "protobuf@googlegroups.com"
  s.require_paths = ["lib"]
  s.files       = `git ls-files -z`.split("\x0").find_all{|f| f =~ /lib\/.+\.rb/}
  unless RUBY_PLATFORM == "java"
    s.files     += `git ls-files "*.c" "*.h" extconf.rb Makefile`.split
    s.extensions= ["ext/google/protobuf_c/extconf.rb"]
  else
    s.files     += ["lib/google/protobuf_java.jar"]
  end
  s.test_files  = ["tests/basic.rb",
                  "tests/stress.rb",
                  "tests/generated_code_test.rb"]
  s.add_development_dependency "rake-compiler"
  s.add_development_dependency "test-unit"
  s.add_development_dependency "rubygems-tasks"
end
