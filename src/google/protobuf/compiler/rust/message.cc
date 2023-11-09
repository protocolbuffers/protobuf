// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/message.h"

#include <algorithm>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/accessors/accessors.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/oneof.h"
#include "google/protobuf/descriptor.h"
#include "third_party/upb/upb_generator/mangle.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {

void MessageNew(Context<Descriptor> msg) {
  switch (msg.opts().kernel) {
    case Kernel::kCpp:
      msg.Emit({{"new_thunk", Thunk(msg, "new")}}, R"rs(
        Self { inner: $pbr$::MessageInner { msg: unsafe { $new_thunk$() } } }
      )rs");
      return;

    case Kernel::kUpb:
      msg.Emit({{"new_thunk", Thunk(msg, "new")}}, R"rs(
        let arena = $pbr$::Arena::new();
        Self {
          inner: $pbr$::MessageInner {
            msg: unsafe { $new_thunk$(arena.raw()) },
            arena,
          }
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
        unsafe { $serialize_thunk$(self.inner.msg) }
      )rs");
      return;

    case Kernel::kUpb:
      msg.Emit({{"serialize_thunk", Thunk(msg, "serialize")}}, R"rs(
        let arena = $pbr$::Arena::new();
        let mut len = 0;
        unsafe {
          let data = $serialize_thunk$(self.inner.msg, arena.raw(), &mut len);
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

            $deserialize_thunk$(self.inner.msg, data)
          };
          success.then_some(()).ok_or($pb$::ParseError)
        )rs");
      return;

    case Kernel::kUpb:
      msg.Emit({{"deserialize_thunk", Thunk(msg, "parse")}}, R"rs(
        let arena = $pbr$::Arena::new();
        let msg = unsafe {
          $deserialize_thunk$(data.as_ptr(), data.len(), arena.raw())
        };

        match msg {
          None => Err($pb$::ParseError),
          Some(msg) => {
            // This assignment causes self.arena to be dropped and to deallocate
            // any previous message pointed/owned to by self.inner.msg.
            self.inner.arena = arena;
            self.inner.msg = msg;
            Ok(())
          }
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

// Copied from upb:
// http://google3/third_party/upb/upb_generator/common.cc;l=63;rcl=580661752.
static std::string MinitableName(Context<Descriptor> msg) {
  return upb::generator::MessageInit(msg.desc().full_name());
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
              {"copy_from_thunk", Thunk(msg, "copy_from")},
              {"repeated_len_thunk", Thunk(msg, "repeated_len")},
              {"repeated_get_thunk", Thunk(msg, "repeated_get")},
              {"repeated_add_thunk", Thunk(msg, "repeated_add")},
              {"repeated_set_thunk", Thunk(msg, "repeated_set")},
              {"repeated_clear_thunk", Thunk(msg, "repeated_clear")},
          },
          R"rs(
          fn $new_thunk$() -> $pbi$::RawMessage;
          fn $delete_thunk$(raw_msg: $pbi$::RawMessage);
          fn $serialize_thunk$(raw_msg: $pbi$::RawMessage) -> $pbr$::SerializedData;
          fn $deserialize_thunk$(raw_msg: $pbi$::RawMessage, data: $pbr$::SerializedData) -> bool;
          fn $copy_from_thunk$(dst: $pbi$::RawMessage, src: $pbi$::RawMessage);
          fn $repeated_len_thunk$(raw: $pbi$::RawRepeatedField) -> usize;
          fn $repeated_add_thunk$(raw: $pbi$::RawRepeatedField) -> $pbi$::RawMessage;
          fn $repeated_get_thunk$(raw: $pbi$::RawRepeatedField, index: usize) -> $pbi$::RawMessage;
          fn $repeated_clear_thunk$(raw: $pbi$::RawRepeatedField);
        )rs");
      return;

    case Kernel::kUpb:
      msg.Emit(
          {
              {"new_thunk", Thunk(msg, "new")},
              {"serialize_thunk", Thunk(msg, "serialize")},
              {"deserialize_thunk", Thunk(msg, "parse")},
              {"minitable", MinitableName(msg)},
          },
          R"rs(
          fn $new_thunk$(arena: $pbi$::RawArena) -> $pbi$::RawMessage;
          fn $serialize_thunk$(msg: $pbi$::RawMessage, arena: $pbi$::RawArena, len: &mut usize) -> $NonNull$<u8>;
          fn $deserialize_thunk$(data: *const u8, size: usize, arena: $pbi$::RawArena) -> Option<$pbi$::RawMessage>;
          static $minitable$: std::ffi::c_void;
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
    unsafe { $delete_thunk$(self.inner.msg); }
  )rs");
}

void GetterForViewOrMut(Context<FieldDescriptor> field, bool is_mut) {
  auto fieldName = field.desc().name();
  auto fieldType = field.desc().type();
  auto getter_thunk = Thunk(field, "get");
  // If we're dealing with a Mut, the getter must be supplied
  // self.inner.msg() whereas a View has to be supplied self.msg
  auto self = is_mut ? "self.inner.msg()" : "self.msg";
  auto returnType = PrimitiveRsTypeName(field.desc());

  if (fieldType == FieldDescriptor::TYPE_STRING) {
    field.Emit(
        {
            {"field", fieldName},
            {"self", self},
            {"getter_thunk", getter_thunk},
            {"ReturnType", returnType},
        },
        R"rs(
              pub fn r#$field$(&self) -> &$ReturnType$ {
                let s = unsafe { $getter_thunk$($self$).as_ref() };
                unsafe { __pb::ProtoStr::from_utf8_unchecked(s) }
              }
            )rs");
  } else if (fieldType == FieldDescriptor::TYPE_BYTES) {
    field.Emit(
        {
            {"field", fieldName},
            {"self", self},
            {"getter_thunk", getter_thunk},
            {"ReturnType", returnType},
        },
        R"rs(
              pub fn r#$field$(&self) -> &$ReturnType$ {
                unsafe { $getter_thunk$($self$).as_ref() }
              }
            )rs");
  } else {
    field.Emit({{"field", fieldName},
                {"getter_thunk", getter_thunk},
                {"self", self},
                {"ReturnType", returnType}},
               R"rs(
            pub fn r#$field$(&self) -> $ReturnType$ {
              unsafe { $getter_thunk$($self$) }
            }
          )rs");
  }
}

void AccessorsForViewOrMut(Context<Descriptor> msg, bool is_mut) {
  for (int i = 0; i < msg.desc().field_count(); ++i) {
    auto field = msg.WithDesc(*msg.desc().field(i));
    if (field.desc().is_repeated()) continue;
    // TODO - add cord support
    if (field.desc().options().has_ctype()) continue;
    // TODO
    if (field.desc().type() == FieldDescriptor::TYPE_MESSAGE ||
        field.desc().type() == FieldDescriptor::TYPE_ENUM ||
        field.desc().type() == FieldDescriptor::TYPE_GROUP)
      continue;
    GetterForViewOrMut(field, is_mut);
    msg.printer().PrintRaw("\n");
  }
}

}  // namespace

void GenerateRs(Context<Descriptor> msg) {
  if (msg.desc().map_key() != nullptr) {
    ABSL_LOG(WARNING) << "unsupported map field: " << msg.desc().full_name();
    return;
  }
  msg.Emit(
      {
          {"Msg", msg.desc().name()},
          {"Msg::new", [&] { MessageNew(msg); }},
          {"Msg::serialize", [&] { MessageSerialize(msg); }},
          {"Msg::deserialize", [&] { MessageDeserialize(msg); }},
          {"Msg::drop", [&] { MessageDrop(msg); }},
          {"Msg_externs", [&] { MessageExterns(msg); }},
          {"QualifiedMsg", cpp::QualifiedClassName(&msg.desc())},
          {"new_thunk", Thunk(msg, "new")},
          {"copy_from_thunk", Thunk(msg, "copy_from")},
          {"accessor_fns",
           [&] {
             for (int i = 0; i < msg.desc().field_count(); ++i) {
               auto field = msg.WithDesc(*msg.desc().field(i));
               msg.Emit({{"comment", FieldInfoComment(field)}}, R"rs(
                 // $comment$
               )rs");
               GenerateAccessorMsgImpl(field);
               msg.printer().PrintRaw("\n");
             }
           }},
          {"oneof_accessor_fns",
           [&] {
             for (int i = 0; i < msg.desc().real_oneof_decl_count(); ++i) {
               GenerateOneofAccessors(
                   msg.WithDesc(*msg.desc().real_oneof_decl(i)));
               msg.printer().PrintRaw("\n");
             }
           }},
          {"accessor_externs",
           [&] {
             for (int i = 0; i < msg.desc().field_count(); ++i) {
               GenerateAccessorExternC(msg.WithDesc(*msg.desc().field(i)));
               msg.printer().PrintRaw("\n");
             }
           }},
          {"oneof_externs",
           [&] {
             for (int i = 0; i < msg.desc().real_oneof_decl_count(); ++i) {
               GenerateOneofExternC(
                   msg.WithDesc(*msg.desc().real_oneof_decl(i)));
               msg.printer().PrintRaw("\n");
             }
           }},
          {"nested_msgs",
           [&] {
             // If we have no nested types or oneofs, bail out without emitting
             // an empty mod SomeMsg_.
             if (msg.desc().nested_type_count() == 0 &&
                 msg.desc().real_oneof_decl_count() == 0) {
               return;
             }
             msg.Emit({{"Msg", msg.desc().name()},
                       {"nested_msgs",
                        [&] {
                          for (int i = 0; i < msg.desc().nested_type_count();
                               ++i) {
                            auto nested_msg =
                                msg.WithDesc(msg.desc().nested_type(i));
                            GenerateRs(nested_msg);
                          }
                        }},
                       {"oneofs",
                        [&] {
                          for (int i = 0;
                               i < msg.desc().real_oneof_decl_count(); ++i) {
                            GenerateOneofDefinition(
                                msg.WithDesc(*msg.desc().real_oneof_decl(i)));
                          }
                        }}},
                      R"rs(
                 #[allow(non_snake_case)]
                 pub mod $Msg$_ {
                   $nested_msgs$

                   $oneofs$
                 }  // mod $Msg$_
                )rs");
           }},
          {"accessor_fns_for_views",
           [&] { AccessorsForViewOrMut(msg, false); }},
          {"accessor_fns_for_muts", [&] { AccessorsForViewOrMut(msg, true); }},
          {"settable_impl",
           [&] {
             if (msg.is_cpp()) {
               msg.Emit(R"rs(
impl<'a> $pb$::SettableValue<$Msg$> for $Msg$View<'a> {
  fn set_on(self, _private: $pb$::__internal::Private, mutator: $pb$::Mut<$Msg$>) {
    unsafe { $copy_from_thunk$(mutator.inner.msg(), self.msg) };
  }
}
                )rs");
             }
             if (msg.is_upb()) {
               msg.Emit({{"minitable", MinitableName(msg)}},
                        R"rs(
impl<'a> $pb$::SettableValue<$Msg$> for $Msg$View<'a> {
  fn set_on(
    self, _private: $pb$::__internal::Private, mutator: $pb$::Mut<$Msg$>) {
    unsafe { $pbr$::upb_Message_DeepCopy(
      mutator.inner.msg(), self.msg, &$minitable$, mutator.inner.arena()) };
  }
}
          )rs");
             }
           }},
          {"repeated_len_thunk", Thunk(msg, "repeated_len")},
          {"repeated_get_thunk", Thunk(msg, "repeated_get")},
          {"repeated_add_thunk", Thunk(msg, "repeated_add")},
          {"repeated_set_thunk", Thunk(msg, "repeated_set")},
          {"repeated_clear_thunk", Thunk(msg, "repeated_clear")},
          {"repeated_impl",
           [&] {
             if (msg.is_cpp()) {
               msg.Emit(R"rs(
impl $pbi$::RepeatedMessageOps for $Msg$ {
  fn len(f: $pbr$::RepeatedField<'_, Self>) -> usize {
    unsafe { $repeated_len_thunk$(f.raw()) }
  }
  unsafe fn get(f: $pbr$::RepeatedField<'_, Self>, index: usize)
    -> Self::View<'_> {
    $Msg$View::new(
      $pbi$::Private, unsafe{ $repeated_get_thunk$(f.raw(), index) })
  }
  unsafe fn get_mut(mut f: $pbr$::RepeatedField<'_, Self>, index: usize)
    -> Self::Mut<'_> {
    let msg = unsafe { $repeated_get_thunk$(f.raw(), index) };
    let mutator = $pbr$::MutatorMessageRef::from_repeated(
      $pbi$::Private, &mut f, msg);
    $Msg$Mut{inner: mutator}
  }
  fn push_default(mut f: $pbr$::RepeatedField<'_, Self>) -> Self::Mut<'_> {
    let msg = unsafe { $repeated_add_thunk$(f.raw()) };
    let mutator = $pbr$::MutatorMessageRef::from_repeated(
      $pbi$::Private, &mut f, msg);
    $Msg$Mut{inner: mutator}
  }
  fn clear(f: $pbr$::RepeatedField<'_, Self>) {
    unsafe { $repeated_clear_thunk$(f.raw()) };
  }
}
            )rs");
             } else {
               msg.Emit(R"rs(
impl $pbi$::RepeatedMessageOps for $Msg$ {
  fn len(f: $pbr$::RepeatedField<'_, Self>) -> usize {
      unsafe { $pbr$::upb_Array_Size(f.raw()) }
  }
  unsafe fn get(f: $pbr$::RepeatedField<'_, Self>, index: usize)
    -> Self::View<'_> {
    $Msg$View::new(
      $pbi$::Private, unsafe{ $pbr$::upb_Array_Get(f.raw(), index).msg_val })
  }
  unsafe fn get_mut(f: $pbr$::RepeatedField<'_, Self>, index: usize)
    -> Self::Mut<'_> {
    let msg = unsafe { $pbr$::upb_Array_Get(f.raw(), index).msg_val };
    let mutator = $pbr$::MutatorMessageRef::new_ref(
      $pbi$::Private, msg, &f.inner().arena);
    $Msg$Mut{inner: mutator}
  }
  fn push_default(f: $pbr$::RepeatedField<'_, Self>)
    -> Self::Mut<'_> {
    let msg = unsafe { $new_thunk$(f.inner().arena.raw()) };
    unsafe {
      $pbr$::upb_Array_Append(
        f.raw(), $pbr$::upb_MessageValue{ msg_val: msg }, f.inner().arena.raw())
    };
    let mutator = $pbr$::MutatorMessageRef::new_ref(
      $pbi$::Private, msg, &f.inner().arena);
    $Msg$Mut{inner: mutator}
  }
  fn clear(f: $pbr$::RepeatedField<'_, Self>) {
    unsafe { $pbr$::upb_Array_Resize(f.raw(), 0, f.inner().arena.raw()) };
  }
}
           )rs");
             }
           }},
      },
      R"rs(
        #[allow(non_camel_case_types)]
        // TODO: Implement support for debug redaction
        #[derive(Debug)]
        pub struct $Msg$ {
          inner: $pbr$::MessageInner
        }

        // SAFETY:
        // - `$Msg$` does not provide shared mutation with its arena.
        // - `$Msg$Mut` is not `Send`, and so even in the presence of mutator
        //   splitting, synchronous access of an arena that would conflict with
        //   field access is impossible.
        unsafe impl Sync for $Msg$ {}

        impl $pb$::Proxied for $Msg$ {
          type View<'a> = $Msg$View<'a>;
          type Mut<'a> = $Msg$Mut<'a>;
        }

        #[derive(Debug, Copy, Clone)]
        #[allow(dead_code)]
        pub struct $Msg$View<'a> {
          msg: $pbi$::RawMessage,
          _phantom: $Phantom$<&'a ()>,
        }

        impl<'a> $Msg$View<'a> {
          #[doc(hidden)]
          pub fn new(_private: $pbi$::Private, msg: $pbi$::RawMessage) -> Self {
            Self { msg, _phantom: std::marker::PhantomData }
          }
          $accessor_fns_for_views$
        }

        // SAFETY:
        // - `$Msg$View` does not perform any mutation.
        // - While a `$Msg$View` exists, a `$Msg$Mut` can't exist to mutate
        //   the arena that would conflict with field access.
        // - `$Msg$Mut` is not `Send`, and so even in the presence of mutator
        //   splitting, synchronous access of an arena is impossible.
        unsafe impl Sync for $Msg$View<'_> {}
        unsafe impl Send for $Msg$View<'_> {}

        impl<'a> $pb$::ViewProxy<'a> for $Msg$View<'a> {
          type Proxied = $Msg$;

          fn as_view(&self) -> $pb$::View<'a, $Msg$> {
            *self
          }
          fn into_view<'shorter>(self) -> $pb$::View<'shorter, $Msg$> where 'a: 'shorter {
            self
          }
        }

        $settable_impl$

        #[derive(Debug)]
        #[allow(dead_code)]
        #[allow(non_camel_case_types)]
        pub struct $Msg$Mut<'a> {
          inner: $pbr$::MutatorMessageRef<'a>,
        }

        impl<'a> $Msg$Mut<'a> {
          #[doc(hidden)]
          pub fn new(_private: $pbi$::Private,
                     parent: &'a mut $pbr$::MessageInner,
                     msg: $pbi$::RawMessage)
            -> Self {
            Self {
              inner: $pbr$::MutatorMessageRef::from_parent(
                       $pbi$::Private, parent, msg)
            }
          }
          $accessor_fns_for_muts$
        }

        // SAFETY:
        // - `$Msg$Mut` does not perform any shared mutation.
        // - `$Msg$Mut` is not `Send`, and so even in the presence of mutator
        //   splitting, synchronous access of an arena is impossible.
        unsafe impl Sync for $Msg$Mut<'_> {}

        impl<'a> $pb$::MutProxy<'a> for $Msg$Mut<'a> {
          fn as_mut(&mut self) -> $pb$::Mut<'_, $Msg$> {
            $Msg$Mut { inner: self.inner }
          }
          fn into_mut<'shorter>(self) -> $pb$::Mut<'shorter, $Msg$> where 'a : 'shorter { self }
        }

        impl<'a> $pb$::ViewProxy<'a> for $Msg$Mut<'a> {
          type Proxied = $Msg$;
          fn as_view(&self) -> $pb$::View<'_, $Msg$> {
            $Msg$View { msg: self.inner.msg(), _phantom: std::marker::PhantomData }
          }
          fn into_view<'shorter>(self) -> $pb$::View<'shorter, $Msg$> where 'a: 'shorter {
            $Msg$View { msg: self.inner.msg(), _phantom: std::marker::PhantomData }
          }
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

          $oneof_accessor_fns$
        }  // impl $Msg$

        //~ We implement drop unconditionally, so that `$Msg$: Drop` regardless
        //~ of kernel.
        impl $std$::ops::Drop for $Msg$ {
          fn drop(&mut self) {
            $Msg::drop$
          }
        }

        $repeated_impl$

        extern "C" {
          $Msg_externs$

          $accessor_externs$

          $oneof_externs$
        }  // extern "C" for $Msg$

        $nested_msgs$
      )rs");

  if (msg.is_cpp()) {
    msg.printer().PrintRaw("\n");
    msg.Emit({{"Msg", msg.desc().name()}}, R"rs(
      impl $Msg$ {
        pub fn __unstable_wrap_cpp_grant_permission_to_break(msg: $pbi$::RawMessage) -> Self {
          Self { inner: $pbr$::MessageInner { msg } }
        }
        pub fn __unstable_cpp_repr_grant_permission_to_break(&mut self) -> $pbi$::RawMessage {
          self.inner.msg
        }
      }
    )rs");
  }
}

// Generates code for a particular message in `.pb.thunk.cc`.
void GenerateThunksCc(Context<Descriptor> msg) {
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
               GenerateThunksCc(nested_msg);
             }
           }},
          {"accessor_thunks",
           [&] {
             for (int i = 0; i < msg.desc().field_count(); ++i) {
               GenerateAccessorThunkCc(msg.WithDesc(*msg.desc().field(i)));
             }
           }},
          {"oneof_thunks",
           [&] {
             for (int i = 0; i < msg.desc().real_oneof_decl_count(); ++i) {
               GenerateOneofThunkCc(
                   msg.WithDesc(*msg.desc().real_oneof_decl(i)));
             }
           }},
          {"copy_from_thunk", Thunk(msg, "copy_from")},
          {"repeated_len_thunk", Thunk(msg, "repeated_len")},
          {"repeated_get_thunk", Thunk(msg, "repeated_get")},
          {"repeated_add_thunk", Thunk(msg, "repeated_add")},
          {"repeated_set_thunk", Thunk(msg, "repeated_set")},
          {"repeated_clear_thunk", Thunk(msg, "repeated_clear")},
      },
      R"cc(
        //~ $abi$ is a workaround for a syntax highlight bug in VSCode. However,
        //~ that confuses clang-format (it refuses to keep the newline after
        //~ `$abi${`). Disabling clang-format for the block.
        // clang-format off
        extern $abi$ {
        void* $new_thunk$() { return new $QualifiedMsg$(); }
        void $delete_thunk$(void* ptr) { delete static_cast<$QualifiedMsg$*>(ptr); }
        google::protobuf::rust_internal::SerializedData $serialize_thunk$($QualifiedMsg$* msg) {
          return google::protobuf::rust_internal::SerializeMsg(msg);
        }
        bool $deserialize_thunk$($QualifiedMsg$* msg,
                                 google::protobuf::rust_internal::SerializedData data) {
          return msg->ParseFromArray(data.data, data.len);
        }
        void $copy_from_thunk$($QualifiedMsg$* dst, const $QualifiedMsg$& src) {
          dst->CopyFrom(src);
        }

        size_t $repeated_len_thunk$(google::protobuf::RepeatedPtrField<$QualifiedMsg$>* field) {
          return field->size();
        }
        $QualifiedMsg$* $repeated_get_thunk$(google::protobuf::RepeatedPtrField<$QualifiedMsg$>* field, size_t index) {
          return field->Mutable(index);
        }
        $QualifiedMsg$* $repeated_add_thunk$(google::protobuf::RepeatedPtrField<$QualifiedMsg$>* field) {
          return field->Add();
        }
        void $repeated_clear_thunk$(google::protobuf::RepeatedPtrField<$QualifiedMsg$>* field) {
          field->Clear();
        }

        $accessor_thunks$

        $oneof_thunks$
        }  // extern $abi$
        // clang-format on

        $nested_msg_thunks$
      )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
