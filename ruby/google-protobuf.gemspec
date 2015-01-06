class << Gem::Specification
  def find_c_source(dir)
    `cd #{dir}; git ls-files "*.c" "*.h" extconf.rb Makefile`.split
    .map{|f| "#{dir}/#{f}"}
  end
end

Gem::Specification.new do |s|
  s.name        = "google-protobuf"
  s.version     = "3.0.0.alpha.1.0"
  s.licenses    = ["BSD"]
  s.summary     = "Protocol Buffers"
  s.description = "Protocol Buffers are Google's data interchange format."
  s.authors     = ["Protobuf Authors"]
  s.email       = "protobuf@googlegroups.com"
  s.require_paths = ["lib"]
  s.extensions  = ["ext/google/protobuf_c/extconf.rb"]
  s.files       = ["lib/google/protobuf.rb"] +
                  # extension C source
                  find_c_source("ext/google/protobuf_c")
  s.test_files = `git ls-files -- tests`.split
end
