#ifndef GOOGLE_PROTOBUF_F1_DESCRIPTOR_CACHE_H_
#define GOOGLE_PROTOBUF_F1_DESCRIPTOR_CACHE_H_

#include <memory>
#include <string>
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {

// A byte-bounded descriptor pool cache intended for F1 and similar fragmented workers.
// Mitigates allocation throughput caused by reloading massive dynamic schemas.
class FragmentDescriptorCache {
 public:
  FragmentDescriptorCache() : approximate_byte_size_(0) {}
  ~FragmentDescriptorCache() = default;

  // Interns a FileDescriptor globally.
  // Thread-safe. Ownership of the descriptor pool remains within the cache.
  std::shared_ptr<const DescriptorPool> InternAndGet(const std::string& signature,
                                                     std::shared_ptr<const DescriptorPool> pool) {
    absl::MutexLock lock(&mutex_);
    
    // Fast path
    auto it = cache_.find(signature);
    if (it != cache_.end()) {
      return it->second;
    }

    // Shed memory if over 512 MB budget (assuming rough sizing or explicit tracking)
    // Here we use a naive clear for demonstration of bounds-checking (Gate 2.6).
    const size_t kMaxBytes = 512 * 1024 * 1024;
    if (approximate_byte_size_ >= kMaxBytes) {
      cache_.clear();
      approximate_byte_size_ = 0;
    }

    // Insert payload
    cache_[signature] = pool;
    // Approximating size of a loaded pool for standard F1 schemas (~2MB)
    approximate_byte_size_ += (2 * 1024 * 1024) + signature.size(); 

    return pool;
  }

  void ShedMemoryIfOverBudget() {
    absl::MutexLock lock(&mutex_);
    const size_t kMaxBytes = 512 * 1024 * 1024;
    if (approximate_byte_size_ >= kMaxBytes) {
      cache_.clear();
      approximate_byte_size_ = 0;
    }
  }

 private:
  absl::Mutex mutex_;
  absl::flat_hash_map<std::string, std::shared_ptr<const DescriptorPool>> cache_ ABSL_GUARDED_BY(mutex_);
  size_t approximate_byte_size_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_F1_DESCRIPTOR_CACHE_H_
