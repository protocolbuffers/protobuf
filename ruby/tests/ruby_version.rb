#!/usr/bin/ruby

# Test that Kokoro is using the expected version of ruby.

require 'test/unit'

class RubyVersionTest < Test::Unit::TestCase

  def test_ruby_version
    return if RUBY_PLATFORM == "java"
    if not ENV["KOKORO_RUBY_VERSION"]
      STDERR.puts("No kokoro ruby version found, skipping check")
      return
    end

    actual = RUBY_VERSION
    expected = ENV["KOKORO_RUBY_VERSION"].delete_prefix("ruby-")
    assert actual.start_with?(expected), "Version #{actual} found, expecting #{expected}"
  end

  def test_jruby_version
    return if RUBY_PLATFORM != "java"
    if not ENV["KOKORO_RUBY_VERSION"]
      STDERR.puts("No kokoro ruby version found, skipping check")
      return
    end

    expected = ENV["KOKORO_RUBY_VERSION"].delete_prefix("jruby-")
    actual = JRUBY_VERSION
    assert actual.start_with?(expected), "Version #{actual} found, expecting #{expected}"
  end

end
