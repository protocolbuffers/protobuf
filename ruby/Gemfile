source 'https://rubygems.org'

gem "bigdecimal"

# FFI is required for JRuby platforms, but optional elsewhere
if defined? $JRUBY_VERSION
  gem "ffi", "~>1", platforms: %i[jruby]
  gem "ffi-compiler", "~>1", platforms: %i[jruby]
else
  group :development do
    gem "ffi", "~>1"
    gem "ffi-compiler", "~>1"
  end
end

group :test do
  gem "test-unit", '~> 3.0', '>= 3.0.9'
end

group :development do
  gem "rake", ">= 13"
  gem "rake-compiler", "~> 1.2"
  gem "rake-compiler-dock", "~> 1.9"
end
