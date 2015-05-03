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
    system("mvn clean")
  end

  task :compile do
    system("mvn package")
  end
else
  Rake::ExtensionTask.new("protobuf_c", spec) do |ext|
    ext.ext_dir = "ext/google/protobuf_c"
    ext.lib_dir = "lib/google"
  end
end

Gem::PackageTask.new(spec) do |pkg|
end

Rake::TestTask.new(:test => :build) do |t|
  t.test_files = FileList["tests/*.rb"]
end

task :build => [:clean, :compile]
task :default => [:build]

# vim:sw=2:et
