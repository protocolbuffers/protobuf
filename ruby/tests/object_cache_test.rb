require 'google/protobuf'
require 'test/unit'

class ObjectCacheTest < Test::Unit::TestCase
  def test_correct_implementation_for_platform
    if Gem::Version.new(RUBY_VERSION) >= Gem::Version.new('2.7.0') and Google::Protobuf::SIZEOF_LONG >= Google::Protobuf::SIZEOF_VALUE
      assert_instance_of Google::Protobuf::ObjectCache, Google::Protobuf::OBJECT_CACHE
    else
      assert_instance_of Google::Protobuf::LegacyObjectCache, Google::Protobuf::OBJECT_CACHE
    end
  end

  def test_try_add_returns_existing_value
    cache = Google::Protobuf::OBJECT_CACHE.class.new
    key = 0xdeadbeefdeadbeef
    first_value = 42
    second_value = 43
    assert_same first_value, cache.try_add(key, first_value)
    assert_same first_value, cache.get(key)
    assert_same first_value, cache.try_add(key, second_value)
    assert_same first_value, cache.get(key)
  end

  def test_multithreaded_access
    cache = Google::Protobuf::OBJECT_CACHE.class.new
    key = 0xdeadbeefdeadbeef
    threads = []

    100.times do |i|
      threads[i] = Thread.new {
        Thread.current["result"] = cache.try_add(key, i)
      }
    end

    results = {}

    threads.each_with_index {|t, i | t.join; results[i] = t["result"] }
    assert_equal 100, results.size
    assert_equal 1, results.values.uniq.size
  end
end