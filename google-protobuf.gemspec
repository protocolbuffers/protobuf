Gem::Specification.new do |s|
  s.name        = "google-protobuf"
  s.version     = "3.0.0.alpha.4.0"
  s.licenses    = ["BSD"]
  s.summary     = "Protocol Buffers"
  s.description = "Protocol Buffers are Google's data interchange format."
  s.homepage    = "https://developers.google.com/protocol-buffers"
  s.authors     = ["Protobuf Authors"]
  s.email       = "protobuf@googlegroups.com"
  s.require_paths = ["ruby/lib"]
  s.files       = `git ls-files -z ruby`.split("\x0").find_all{|f| f =~ /lib\/.+\.rb/}
  unless RUBY_PLATFORM == "java"
    s.files     += `git ls-files "ruby/*.c" "ruby/*.h"`.split
    s.extensions= ["ruby/ext/google/protobuf_c/extconf.rb"]
  else
    s.files     += ["ruby/lib/google/protobuf_java.jar"]
  end
  s.files += `git ls-files -z m4 src`.split("\x0")
  s.files += %w( Makefile.am autogen.sh configure.ac ruby/bin/protoc.rb )
  s.test_files  = ["ruby/tests/basic.rb",
                  "ruby/tests/stress.rb",
                  "ruby/tests/generated_code_test.rb"]
  s.bindir      = "ruby/bin"
  s.executables = ["protoc.rb"]
  s.add_development_dependency "rake-compiler"
  s.add_development_dependency "test-unit"
  s.add_development_dependency "rubygems-tasks"
end
