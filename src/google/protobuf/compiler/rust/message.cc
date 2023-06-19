// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC. nor the names of its
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
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
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
        //~ rustc incorrectly thinks this field is never read, even though
        //~ it has a destructor!
        #[allow(dead_code)]
        arena: $pbr$::Arena,
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageNew(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit({{"new_thunk", Thunk(msg, "new")}}, R"rs(
        Self { msg: unsafe { $new_thunk$() } }
      )rs");
      return;

    case Kernel::kUpb:
      msg.Emit({{"new_thunk", Thunk(msg, "new")}}, R"rs(
        let arena = $pbr$::Arena::new();
        Self {
          msg: unsafe { $new_thunk$(arena.raw()) },
          arena,
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageSerialize(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit({{"serialize_thunk", Thunk(msg, "serialize")}}, R"rs(
        unsafe { $serialize_thunk$(self.msg) }
      )rs");
      return;

    case Kernel::kUpb:
      msg.Emit({{"serialize_thunk", Thunk(msg, "serialize")}}, R"rs(
        let arena = $pbr$::Arena::new();
        let mut len = 0;
        unsafe {
          let data = $serialize_thunk$(self.msg, arena.raw(), &mut len);
          $pbr$::SerializedData::from_raw_parts(arena, data, len)
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageDeserialize(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit(
          {
              {"deserialize_thunk", Thunk(msg, "deserialize")},
          },
          R"rs(
          let success = unsafe {
            let data = $pbr$::SerializedData::from_raw_parts(
              $NonNull$::new(data.as_ptr() as *mut _).unwrap(),
              data.len(),
            );

            $deserialize_thunk$(self.msg, data)
          };
          success.then_some(()).ok_or($pb$::ParseError)
        )rs");
      return;

    case Kernel::kUpb:
      msg.Emit({{"deserialize_thunk", Thunk(msg, "parse")}}, R"rs(
        let arena = $pbr$::Arena::new();
        let msg = unsafe {
          $NonNull$::<u8>::new(
            $deserialize_thunk$(data.as_ptr(), data.len(), arena.raw())
          )
        };

        match msg {
          None => Err($pb$::ParseError),
          Some(msg) => {
            // This assignment causes self.arena to be dropped and to deallocate
            // any previous message pointed/owned to by self.msg.
            self.arena = arena;
            self.msg = msg;
            Ok(())
          }
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageExterns(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit(
          {
              {"new_thunk", Thunk(msg, "new")},
              {"delete_thunk", Thunk(msg, "delete")},
              {"serialize_thunk", Thunk(msg, "serialize")},
              {"deserialize_thunk", Thunk(msg, "deserialize")},
          },
          R"rs(
          fn $new_thunk$() -> $NonNull$<u8>;
          fn $delete_thunk$(raw_msg: $NonNull$<u8>);
          fn $serialize_thunk$(raw_msg: $NonNull$<u8>) -> $pbr$::SerializedData;
          fn $deserialize_thunk$(raw_msg: $NonNull$<u8>, data: $pbr$::SerializedData) -> bool;
        )rs");
      return;

    case Kernel::kUpb:
      msg.Emit(
          {
              {"new_thunk", Thunk(msg, "new")},
              {"serialize_thunk", Thunk(msg, "serialize")},
              {"deserialize_thunk", Thunk(msg, "parse")},
          },
          R"rs(
          fn $new_thunk$(arena: $pbr$::RawArena) -> $NonNull$<u8>;
          fn $serialize_thunk$(msg: $NonNull$<u8>, arena: $pbr$::RawArena, len: &mut usize) -> $NonNull$<u8>;
          fn $deserialize_thunk$(data: *const u8, size: usize, arena: $pbr$::RawArena) -> *mut u8;
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageDrop(Context<Descriptor> msg) {
  if (msg.is_upb()) {
    // Nothing to do here; drop glue (which will run drop(self.arena)
    // automatically) is sufficient.
    return;
  }

  msg.Emit({{"delete_thunk", Thunk(msg, "delete")}}, R"rs(
    unsafe { $delete_thunk$(self.msg); }
  )rs");
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
  if (msg.desc().map_key() != nullptr) {
    ABSL_LOG(WARNING) << "unsupported map field: " << msg.desc().full_name();
    return;
  }
  msg.Emit(
      {
          {"Msg", msg.desc().name()},
          {"Msg.fields", [&] { MessageStructFields(msg); }},
          {"Msg::new", [&] { MessageNew(msg); }},
          {"Msg::serialize", [&] { MessageSerialize(msg); }},
          {"Msg::deserialize", [&] { MessageDeserialize(msg); }},
          {"Msg::drop", [&] { MessageDrop(msg); }},
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
          {"nested_msgs",
           [&] {
             if (msg.desc().nested_type_count() == 0) {
               return;
             }
             msg.Emit({{"Msg", msg.desc().name()},
                       {"nested_msgs",
                        [&] {
                          for (int i = 0; i < msg.desc().nested_type_count();
                               ++i) {
                            auto nested_msg =
                                msg.WithDesc(msg.desc().nested_type(i));
                            MessageGenerator gen(nested_msg);
                            gen.GenerateRs(nested_msg);
                          }
                        }}},
                      R"rs(
                 pub mod $Msg$_ {
                   $nested_msgs$
                 }  // mod $Msg$_
                )rs");
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

          pub fn serialize(&self) -> $pbr$::SerializedData {
            $Msg::serialize$
          }
          pub fn deserialize(&mut self, data: &[u8]) -> Result<(), $pb$::ParseError> {
            $Msg::deserialize$
          }

          $accessor_fns$
        }  // impl $Msg$

        //~ We implement drop unconditionally, so that `$Msg$: Drop` regardless
        //~ of kernel.
        impl $std$::ops::Drop for $Msg$ {
          fn drop(&mut self) {
            $Msg::drop$
          }
        }

        extern "C" {
          $Msg_externs$

          $accessor_externs$
        }  // extern "C" for $Msg$

        $nested_msgs$
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
void MessageGenerator::GenerateThunksCc(Context<Descriptor> msg) {
  ABSL_CHECK(msg.is_cpp());
  if (msg.desc().map_key() != nullptr) {
    ABSL_LOG(WARNING) << "unsupported map field: " << msg.desc().full_name();
    return;
  }

  msg.Emit(
      {
          {"abi", "\"C\""},  // Workaround for syntax highlight bug in VSCode.
          {"Msg", msg.desc().name()},
          {"QualifiedMsg", cpp::QualifiedClassName(&msg.desc())},
          {"new_thunk", Thunk(msg, "new")},
          {"delete_thunk", Thunk(msg, "delete")},
          {"serialize_thunk", Thunk(msg, "serialize")},
          {"deserialize_thunk", Thunk(msg, "deserialize")},
          {"nested_msg_thunks",
           [&] {
             for (int i = 0; i < msg.desc().nested_type_count(); ++i) {
               Context<Descriptor> nested_msg =
                   msg.WithDesc(msg.desc().nested_type(i));
               MessageGenerator gen(nested_msg);
               gen.GenerateThunksCc(nested_msg);
             }
           }},
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
        //~ $abi$ is a workaround for a syntax highlight bug in VSCode. However,
        //~ that confuses clang-format (it refuses to keep the newline after
        //~ `$abi${`). Disabling clang-format for the block.
        // clang-format off
        extern $abi$ {
        void* $new_thunk$(){return new $QualifiedMsg$(); }
        void $delete_thunk$(void* ptr) { delete static_cast<$QualifiedMsg$*>(ptr); }
        google::protobuf::rust_internal::SerializedData $serialize_thunk$($QualifiedMsg$* msg) {
          return google::protobuf::rust_internal::SerializeMsg(msg);
        }
        bool $deserialize_thunk$($QualifiedMsg$* msg,
                                 google::protobuf::rust_internal::SerializedData data) {
          return msg->ParseFromArray(data.data, data.len);
        }

        $accessor_thunks$
        }  // extern $abi$
        // clang-format on

        $nested_msg_thunks$
      )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
