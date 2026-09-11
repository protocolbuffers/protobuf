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

  def test_gemspec_source_code_uri_tag_pattern
    # Ruby gem versions carry a "4." major prefix (e.g. 4.37.0) while repository
    # tags follow protoc versions (e.g. v37.0 or v37.0-rc1).
    to_git_tag = ->(ver) { "v#{ver.to_s.sub(/^4\./, '').sub('.rc.', '-rc')}" }

    assert_equal "v37.0", to_git_tag.call("4.37.0")
    assert_equal "v36.1", to_git_tag.call("4.36.1")
    assert_equal "v36.0", to_git_tag.call("4.36.0")
    assert_equal "v36.0-rc1", to_git_tag.call("4.36.0.rc.1")
    assert_equal "v36.0-rc2", to_git_tag.call("4.36.0.rc.2")
  end

  def test_gemspec_source_code_uri
    gemspec_candidates = [
      "ruby/google-protobuf.gemspec",
      "google-protobuf.gemspec",
      File.expand_path("../../google-protobuf.gemspec", __FILE__)
    ]
    gemspec = gemspec_candidates.find { |f| File.file?(f) }
    return unless gemspec

    content = File.read(gemspec)
    assert_no_match(/"v#\{s\.version\.to_s\.sub\('\.rc\.', '-rc'\)\}"/, content)
    assert_match(/git_tag\s*=\s*"v#\{s\.version\.to_s\.sub\(\/\^4\\\.\/, ''\)\.sub\('\.rc\.', '-rc'\)\}"/, content)
  end

end
