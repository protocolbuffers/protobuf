
#ifndef UPBC_COMMON_H
#define UPBC_COMMON_H

#include <vector>

#include "absl/strings/substitute.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace upbc {

class Output {
 public:
  Output(google::protobuf::io::ZeroCopyOutputStream* stream)
      : stream_(stream) {}
  ~Output() { stream_->BackUp((int)size_); }

  template <class... Arg>
  void operator()(absl::string_view format, const Arg&... arg) {
    Write(absl::Substitute(format, arg...));
  }

 private:
  void Write(absl::string_view data) {
    while (!data.empty()) {
      RefreshOutput();
      size_t to_write = std::min(data.size(), size_);
      memcpy(ptr_, data.data(), to_write);
      data.remove_prefix(to_write);
      ptr_ += to_write;
      size_ -= to_write;
    }
  }

  void RefreshOutput() {
    while (size_ == 0) {
      void *ptr;
      int size;
      if (!stream_->Next(&ptr, &size)) {
        fprintf(stderr, "upbc: Failed to write to to output\n");
        abort();
      }
      ptr_ = static_cast<char*>(ptr);
      size_ = size;
    }
  }

  google::protobuf::io::ZeroCopyOutputStream* stream_;
  char *ptr_ = nullptr;
  size_t size_ = 0;
};

std::string StripExtension(absl::string_view fname);
std::string ToCIdent(absl::string_view str);
std::string ToPreproc(absl::string_view str);
void EmitFileWarning(const google::protobuf::FileDescriptor* file,
                     Output& output);
std::vector<const google::protobuf::Descriptor*> SortedMessages(
    const google::protobuf::FileDescriptor* file);
std::string MessageInit(const google::protobuf::Descriptor* descriptor);
std::string MessageName(const google::protobuf::Descriptor* descriptor);

}  // namespace upbc

# endif  // UPBC_COMMON_H
