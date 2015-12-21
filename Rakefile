require "rubygems"
require "rubygems/package_task"
require "rake/extensiontask" unless RUBY_PLATFORM == "java"
require "rake/testtask"

spec = Gem::Specification.load("google-protobuf.gemspec")

if RUBY_PLATFORM == "java"
  if `which mvn` == ''
    raise ArgumentError, "maven needs to be installed"
  end
  task :clean do
    system("mvn -f ruby/pom.xml clean")
  end

  task :compile do
    system("mvn -f ruby/pom.xml package")
  end
else
  Rake::ExtensionTask.new("protobuf_c", spec) do |ext|
    ext.ext_dir = "ruby/ext/google/protobuf_c"
    ext.lib_dir = "ruby/lib/google"
  end
end

Gem::PackageTask.new(spec) do |pkg|
end

task :protoc do
  raise "autogen.sh failed" unless system './autogen.sh'
  raise "configure failed" unless system './configure'
  raise "make failed" unless system 'make'
end

Rake::TestTask.new(:test => :build) do |t|
  t.test_files = FileList["ruby/tests/*.rb"]
end

task :build => [:clean, :compile, :protoc]
task :default => [:build]

# vim:sw=2:et
