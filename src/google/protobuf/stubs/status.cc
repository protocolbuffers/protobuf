// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include <google/protobuf/stubs/status.h>

#include <ostream>
#include <stdio.h>
#include <string>
#include <utility>

namespace google {
namespace protobuf {
namespace util {
namespace error {
inline string CodeEnumToString(error::Code code) {
  switch (code) {
    case OK:
      return "OK";
    case CANCELLED:
      return "CANCELLED";
    case UNKNOWN:
      return "UNKNOWN";
    case INVALID_ARGUMENT:
      return "INVALID_ARGUMENT";
    case DEADLINE_EXCEEDED:
      return "DEADLINE_EXCEEDED";
    case NOT_FOUND:
      return "NOT_FOUND";
    case ALREADY_EXISTS:
      return "ALREADY_EXISTS";
    case PERMISSION_DENIED:
      return "PERMISSION_DENIED";
    case UNAUTHENTICATED:
      return "UNAUTHENTICATED";
    case RESOURCE_EXHAUSTED:
      return "RESOURCE_EXHAUSTED";
    case FAILED_PRECONDITION:
      return "FAILED_PRECONDITION";
    case ABORTED:
      return "ABORTED";
    case OUT_OF_RANGE:
      return "OUT_OF_RANGE";
    case UNIMPLEMENTED:
      return "UNIMPLEMENTED";
    case INTERNAL:
      return "INTERNAL";
    case UNAVAILABLE:
      return "UNAVAILABLE";
    case DATA_LOSS:
      return "DATA_LOSS";
  }

  // No default clause, clang will abort if a code is missing from
  // above switch.
  return "UNKNOWN";
}
}  // namespace error.

// Representation for global objects.
struct Status::Pod {
  // Structured exactly like Status, but has no constructor so
  // it can be statically initialized
  Rep* rep_;
};

Status::Rep Status::global_reps[3] = {
  { true /* is_global_ */, error::OK, NULL },
  { true /* is_global_ */, error::CANCELLED, NULL },
  { true /* is_global_ */, error::UNKNOWN, NULL },
};

const Status::Pod Status::globals[3] = {
  { &Status::global_reps[0] },
  { &Status::global_reps[1] },
  { &Status::global_reps[2] }
};

const Status& Status::OK = *reinterpret_cast<const Status*>(&globals[0]);
const Status& Status::CANCELLED = *reinterpret_cast<const Status*>(&globals[1]);
const Status& Status::UNKNOWN = *reinterpret_cast<const Status*>(&globals[2]);

Status::Status() : rep_(NewRep(error::OK, "")) {}
Status::Status(error::Code error_code, StringPiece error_message)
    : rep_(NewRep(error_code, "")) {
  if (error_code != error::OK) {
    *rep_->error_message_ = error_message.ToString();
  }
}
Status::Status(const Status& other)
    : rep_(NewRep(other.rep_->error_code_,
                  StringPiece(other.rep_->error_message_ != NULL ?
                              *other.rep_->error_message_ : ""))) {}

Status& Status::operator=(const Status& other) {
  rep_->error_code_ = other.rep_->error_code_;
  if (rep_->error_message_ == NULL) {
    rep_->error_message_ = new string();
  }
  *rep_->error_message_ = other.rep_->error_message_ != NULL ?
      *other.rep_->error_message_ : "";
  return *this;
}

bool Status::operator==(const Status& x) const {
  return rep_ == x.rep_ ||
      (this->error_code() == x.error_code() &&
       this->error_message() == x.error_message());
}

string Status::ToString() const {
  if (rep_->error_code_ == error::OK) {
    return "OK";
  } else {
    if (rep_->error_message_ == NULL || rep_->error_message_->empty()) {
      return error::CodeEnumToString(rep_->error_code_);
    } else {
      return error::CodeEnumToString(rep_->error_code_) + ":" +
          *rep_->error_message_;
    }
  }
}

Status::Rep* Status::NewRep(error::Code error_code, StringPiece error_message) {
  Rep* rep = new Rep;
  rep->is_global_ = false;
  rep->error_code_ = error_code;
  rep->error_message_ = new string(error_message.data(), error_message.size());
  return rep;
}

void Status::DeleteRep(Rep* rep) {
  GOOGLE_DCHECK(!rep->is_global_);

  if (rep->error_message_ != NULL) {
    delete rep->error_message_;
  }
  delete rep;
}

ostream& operator<<(ostream& os, const Status& x) {
  os << x.ToString();
  return os;
}

}  // namespace util
}  // namespace protobuf
}  // namespace google
