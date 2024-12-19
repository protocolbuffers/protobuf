#include "third_party/tcmalloc/experiment.h"
#include "third_party/tcmalloc/experiment_config.h"

namespace google {
namespace protobuf {
namespace internal {

// We provide this function because protobuf can't always depend directly on
// TCMalloc experiment framework. This will be linked into binaries that use
// TCMalloc and we can define a weak version of this function that always
// returns false for cases in which TCMalloc is not used.
bool IsLazyExtensionsAblationExperimentActive() {
  return tcmalloc::IsExperimentActive(
      tcmalloc::Experiment::PROTOBUF_LAZY_EXTENSION_ABLATION);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
