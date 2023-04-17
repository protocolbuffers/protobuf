// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#include "google/protobuf/compiler/rust/message.h"

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/accessors/accessors.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {
void MessageStructFields(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit(R"rs(
        msg: $NonNull$<u8>,
      )rs");
      return;

    case Kernel::kUpb:
      msg.Emit(R"rs(
        msg: $NonNull$<u8>,
        arena: *mut $pb$::Arena,
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageNew(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit(R"rs(
        Self { msg: unsafe { __rust_proto_thunk__$pkg_Msg$__new() } }
      )rs");
      return;

    case Kernel::kUpb:
      msg.Emit(R"rs(
        let arena = unsafe { $pb$::Arena::new() };
        let msg = unsafe { $pkg_Msg$_new(arena) };
        $Msg$ { msg, arena }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageSerialize(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit(R"rs(
        unsafe { __rust_proto_thunk__$pkg_Msg$__serialize(self.msg) }
      )rs");
      return;

    case Kernel::kUpb:
      msg.Emit(R"rs(
        let arena = unsafe { $pb$::__runtime::upb_Arena_New() };
        let mut len = 0;
        unsafe {
          let data = $pkg_Msg$_serialize(self.msg, arena, &mut len);
          $pb$::SerializedData::from_raw_parts(arena, data, len)
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageDeserialize(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit(R"rs(
        let success = unsafe {
          let data = $pb$::SerializedData::from_raw_parts(
            $NonNull$::new(data.as_ptr() as *mut _).unwrap(),
            data.len(),
          );

          __rust_proto_thunk__$pkg_Msg$__deserialize(self.msg, data)
        };
        success.then_some(()).ok_or($pb$::ParseError)
      )rs");
      return;

    case Kernel::kUpb:
      msg.Emit(R"rs(
        let _ = data;
        $std$::unimplemented!()
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageExterns(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit(R"rs(
          fn __rust_proto_thunk__$pkg_Msg$__new() -> $NonNull$<u8>;
          fn __rust_proto_thunk__$pkg_Msg$__serialize(raw_msg: $NonNull$<u8>)
            -> $pb$::SerializedData;
          fn __rust_proto_thunk__$pkg_Msg$__deserialize(
            raw_msg: $NonNull$<u8>,
            data: $pb$::SerializedData,
          ) -> bool;
      )rs");
      return;

    case Kernel::kUpb:
      msg.Emit(R"rs(
          fn $pkg_Msg$_new(arena: *mut $pb$::Arena) -> $NonNull$<u8>;
          fn $pkg_Msg$_serialize(
            msg: $NonNull$<u8>,
            arena: *mut $pb$::Arena,
            len: &mut usize) -> $NonNull$<u8>;
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}
}  // namespace

MessageGenerator::MessageGenerator(Context<Descriptor> msg) {
  accessors_.resize(msg.desc().field_count());
  for (int i = 0; i < msg.desc().field_count(); ++i) {
    auto field = msg.WithDesc(msg.desc().field(i));
    accessors_[i] = AccessorGenerator::For(field);
    if (accessors_[i] == nullptr) {
      ABSL_LOG(WARNING) << "unsupported field: " << field.desc().full_name();
    }
  }
}

void MessageGenerator::GenerateRs(Context<Descriptor> msg) {
  msg.Emit(
      {
          {"Msg", msg.desc().name()},
          {"pkg_Msg", GetUnderscoreDelimitedFullName(msg)},
          {"Msg.fields", [&] { MessageStructFields(msg); }},
          {"Msg::new", [&] { MessageNew(msg); }},
          {"Msg::serialize", [&] { MessageSerialize(msg); }},
          {"Msg::deserialize", [&] { MessageDeserialize(msg); }},
          {"Msg_externs", [&] { MessageExterns(msg); }},
          {"accessor_fns",
           [&] {
             for (int i = 0; i < msg.desc().field_count(); ++i) {
               auto& gen = accessors_[i];
               auto field = msg.WithDesc(*msg.desc().field(i));
               msg.Emit({{"comment", FieldInfoComment(field)}}, R"rs(
                 // $comment$
               )rs");

               if (gen == nullptr) {
                 msg.Emit({{"field", field.desc().full_name()}}, R"rs(
                  // Unsupported! :(
                 )rs");
                 msg.printer().PrintRaw("\n");
                 continue;
               }

               gen->GenerateMsgImpl(field);
               msg.printer().PrintRaw("\n");
             }
           }},
          {"accessor_externs",
           [&] {
             for (int i = 0; i < msg.desc().field_count(); ++i) {
               auto& gen = accessors_[i];
               if (gen == nullptr) continue;

               gen->GenerateExternC(msg.WithDesc(*msg.desc().field(i)));
               msg.printer().PrintRaw("\n");
             }
           }},
      },
      R"rs(
        #[allow(non_camel_case_types)]
        pub struct $Msg$ {
          $Msg.fields$
        }

        impl $Msg$ {
          pub fn new() -> Self {
            $Msg::new$
          }

          pub fn serialize(&self) -> $pb$::SerializedData {
            $Msg::serialize$
          }
          pub fn deserialize(&mut self, data: &[u8]) -> Result<(), $pb$::ParseError> {
            $Msg::deserialize$
          }

          $accessor_fns$
        }  // impl $Msg$

        extern "C" {
          $Msg_externs$

          $accessor_externs$
        }  // extern "C" for $Msg$
      )rs");

  if (msg.is_cpp()) {
    msg.printer().PrintRaw("\n");
    msg.Emit({{"Msg", msg.desc().name()}}, R"rs(
      impl $Msg$ {
        pub fn __unstable_wrap_cpp_grant_permission_to_break(msg: $NonNull$<u8>) -> Self {
          Self { msg }
        }
        pub fn __unstable_cpp_repr_grant_permission_to_break(&mut self) -> $NonNull$<u8> {
          self.msg
        }
      }
    )rs");
  }
}

// Generates code for a particular message in `.pb.thunk.cc`.
void MessageGenerator::GenerateThunkCc(Context<Descriptor> msg) {
  ABSL_CHECK(msg.is_cpp());

  msg.Emit(
      {
          {"abi", "\"C\""},  // Workaround for syntax highlight bug in VSCode.
          {"Msg", msg.desc().name()},
          {"pkg_Msg", GetUnderscoreDelimitedFullName(msg)},
          {"namespace", cpp::Namespace(&msg.desc())},
          {"accessor_thunks",
           [&] {
             for (int i = 0; i < msg.desc().field_count(); ++i) {
               auto& gen = accessors_[i];
               if (gen == nullptr) continue;

               gen->GenerateThunkCc(msg.WithDesc(*msg.desc().field(i)));
             }
           }},
      },
      R"cc(
        extern $abi$ {
          void* __rust_proto_thunk__$pkg_Msg$__new() {
            return new $namespace$::$Msg$();
          }

          google::protobuf::rust_internal::SerializedData
          __rust_proto_thunk__$pkg_Msg$__serialize($namespace$::$Msg$ * msg) {
            return google::protobuf::rust_internal::SerializeMsg(msg);
          }

          bool __rust_proto_thunk__$pkg_Msg$__deserialize(
              $namespace$::$Msg$ * msg,
              google::protobuf::rust_internal::SerializedData data) {
            return msg->ParseFromArray(data.data, data.len);
          }

          $accessor_thunks$
        }  // extern $abi$
      )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
