require 'google/protobuf'
require 'test/unit'

class PlatformTest < Test::Unit::TestCase
  def test_correct_implementation_for_platform
    omit('OBJECT_CACHE not defined') unless defined? Google::Protobuf::OBJECT_CACHE
    if Google::Protobuf::SIZEOF_LONG >= Google::Protobuf::SIZEOF_VALUE and not defined? JRUBY_VERSION
      assert_instance_of Google::Protobuf::ObjectCache, Google::Protobuf::OBJECT_CACHE
    else
      assert_instance_of Google::Protobuf::LegacyObjectCache, Google::Protobuf::OBJECT_CACHE
    end
  end
end

module ObjectCacheTestModule
  def test_try_add_returns_existing_value
    cache = self.create

    keys = %w[k1 k2]
    vals = %w[v1 v2]
    2.times do |i|
      assert_same vals[i], cache.try_add(keys[i], vals[i])
      assert_same vals[i], cache.get(keys[i])
      assert_same vals[i], cache.try_add(keys[i], vals[(i+1)%2])
      assert_same vals[i], cache.get(keys[i])
    end
  end

  def test_multithreaded_access
    cache = self.create

    keys = %w[k0 k1 k2 k3 k4 k5 k6 k7]

    threads = []
    100.times do |i|
      threads[i] = Thread.new {
        Thread.current["result"] = cache.try_add(keys[i % keys.size], i.to_s)
      }
    end

    results = {}
    threads.each_with_index {|t, i| t.join; results[i] = t["result"] }
    assert_equal 100, results.size
    assert_equal 8, results.values.uniq.size
  end
end

class ObjectCacheTest < Test::Unit::TestCase
  def create
    omit('OBJECT_CACHE not defined') unless defined? Google::Protobuf::OBJECT_CACHE
    Google::Protobuf::ObjectCache.new
  end

  include ObjectCacheTestModule
end

class LegacyObjectCacheTest < Test::Unit::TestCase
  def create
    omit('OBJECT_CACHE not defined') unless defined? Google::Protobuf::OBJECT_CACHE
    Google::Protobuf::LegacyObjectCache.new
  end

  include ObjectCacheTestModule
end

