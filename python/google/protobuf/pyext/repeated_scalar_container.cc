// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: anuraag@google.com (Anuraag Agrawal)
// Author: tibell@google.com (Johan Tibell)

#include "google/protobuf/pyext/repeated_scalar_container.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "google/protobuf/pyext/message.h"
#include "google/protobuf/pyext/safe_numerics.h"
#include "google/protobuf/pyext/scoped_pyobject_ptr.h"

#define PyString_AsString(ob) \
  (PyUnicode_Check(ob) ? PyUnicode_AsUTF8(ob) : PyBytes_AsString(ob))

namespace google {
namespace protobuf {
namespace python {

// Bool that can be used with std::vector without bit packing issues.
enum class Bool : bool { kFalse = false, kTrue = true };

class RepeatedScalarContainerFriend {
 public:
  template <typename T>
  static absl::Span<const T> GetRepeatedFieldSpan(
      const Reflection* reflection, const Message* message,
      const FieldDescriptor* field_descriptor) {
    using TActual = std::conditional_t<std::is_same_v<T, Bool>, bool, T>;
    const auto& field = reflection->GetRepeatedFieldInternal<TActual>(
        *message, field_descriptor,
        Reflection::GetRepeatedFieldIntent::kHiddenOrInternal);
    return absl::MakeSpan(reinterpret_cast<const T*>(field.data()),
                          static_cast<size_t>(field.size()));
  }

  // Only `AddStringView` and `SetRepeatedStringView` are needed to be friends
  // with `Reflection` but having the overloads here makes the code easier to
  // call.
  template <typename T>
  static void MergeFrom(const Reflection* reflection, Message* message,
                        const FieldDescriptor* field_descriptor,
                        absl::Span<const T> other_values) {
    MutableRepeatedFieldRef<T> mutable_ref =
        reflection->GetMutableRepeatedFieldRef<T>(message, field_descriptor);
    mutable_ref.MergeFrom(other_values);
  }

  static void MergeFrom(const Reflection* reflection, Message* message,
                        const FieldDescriptor* field_descriptor,
                        absl::Span<const Bool> other_values) {
    MutableRepeatedFieldRef<bool> mutable_ref =
        reflection->GetMutableRepeatedFieldRef<bool>(message, field_descriptor);
    absl::Span<const bool> other_values_as_bool(
        reinterpret_cast<const bool*>(other_values.data()),
        other_values.size());
    mutable_ref.MergeFrom(other_values_as_bool);
  }

  static void MergeFrom(const Reflection* reflection, Message* message,
                        const FieldDescriptor* field_descriptor,
                        absl::Span<const absl::string_view> other_values) {
    for (absl::string_view value : other_values) {
      reflection->AddStringView(message, field_descriptor, value);
    }
  }

  template <typename T>
  static void Set(const Reflection* reflection, Message* message,
                  const FieldDescriptor* field_descriptor, int index,
                  const T& value) {
    MutableRepeatedFieldRef<T> mutable_ref =
        reflection->GetMutableRepeatedFieldRef<T>(message, field_descriptor);
    mutable_ref.Set(index, value);
  }

  static void Set(const Reflection* reflection, Message* message,
                  const FieldDescriptor* field_descriptor, int index,
                  const Bool& value) {
    MutableRepeatedFieldRef<bool> mutable_ref =
        reflection->GetMutableRepeatedFieldRef<bool>(message, field_descriptor);
    mutable_ref.Set(index, static_cast<bool>(value));
  }

  static void Set(const Reflection* reflection, Message* message,
                  const FieldDescriptor* field_descriptor, int index,
                  absl::string_view value) {
    reflection->SetRepeatedStringView(message, field_descriptor, index, value);
  }
};

namespace {

template <typename Dest, typename Source>
bool SafeCast(Source source, Dest* dest) {
  if constexpr (std::is_same_v<Dest, Bool>) {
    *dest = Dest(source != 0);
    return true;
  } else if constexpr (std::is_same_v<Source, Bool>) {
    *dest =
        static_cast<bool>(source) ? static_cast<Dest>(1) : static_cast<Dest>(0);
    return true;
  } else if constexpr (!std::is_same_v<Dest, Source>) {
    if (!IsValidNumericCast<Dest>(source)) {
      PyErr_SetString(PyExc_OverflowError, "Integer overflow");
      return false;
    }
  }
  *dest = static_cast<Dest>(source);
  return true;
}

bool SafeCast(float source, double* dest) {
  *dest = static_cast<double>(source);
  return true;
}

bool SafeCast(double source, float* dest) {
  // We don't overflow check here as this matches the existing behavior.
  *dest = static_cast<float>(source);
  return true;
}

bool GetContiguous1DView(PyObject* value, Py_buffer* view) {
  if (PyObject_GetBuffer(value, view, PyBUF_RECORDS_RO) != 0) {
    PyErr_Clear();
    return false;
  }
  if (view->format == nullptr) {
    PyBuffer_Release(view);
    return false;
  }
  if (((view->ndim == 1) &&
       (view->strides == nullptr || view->itemsize == view->strides[0]))) {
    return true;
  }

  PyBuffer_Release(view);
  return false;
}

template <typename ToT, typename FromT>
size_t ConvertSpanData(const FromT* src, ToT* dst, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (!SafeCast(src[i], &dst[i])) {
      return i;
    }
  }
  return count;
}

template <typename T>
class SourceSpan {
 public:
  explicit SourceSpan(absl::Span<const T> span, bool force_copy = false) {
    if (force_copy) {
      temp_ = std::unique_ptr<T[]>(new T[span.size()]);
      std::copy(span.begin(), span.end(), temp_.get());
      span_ = absl::MakeConstSpan(temp_.get(), span.size());
    } else {
      span_ = span;
    }
  }

  // Sets Python error if the conversion fails. Sets `state_` to `kError` if the
  // conversion fails. Otherwise, sets `state_` to `kSuccess`. `span_` will
  // contain the converted values up to the first conversion failure.
  template <typename Source>
  explicit SourceSpan(absl::Span<const Source> span) {
    temp_ = std::unique_ptr<T[]>(new T[span.size()]);
    size_t converted_count =
        ConvertSpanData<T, Source>(span.data(), temp_.get(), span.size());
    span_ = absl::MakeConstSpan(temp_.get(), converted_count);
  }

  absl::Span<const T> span() const { return span_; }

 private:
  std::unique_ptr<T[]> temp_;
  absl::Span<const T> span_;
};

template <typename ToT, typename Func>
bool CallWithSpanHandlingExceptions(absl::Span<const ToT> span, Func&& func) {
  bool ok = !PyErr_Occurred();
  PyObject* err_type = nullptr;
  PyObject* err_value = nullptr;
  PyObject* err_traceback = nullptr;
  if (PyErr_Occurred()) {
    ok = false;
    PyErr_Fetch(&err_type, &err_value, &err_traceback);
  }
  // Matches existing behavior where elements were appended lazily.
  // We may remove later in a separate CL to match UPB amd Python behavior.
  bool result = func(span);
  if (!ok) {
    if (!result) {
      PyErr_Clear();
    }
    PyErr_Restore(err_type, err_value, err_traceback);
  }
  return ok && result;
}

struct RaiiPyBuffer {
  Py_buffer* view;
  explicit RaiiPyBuffer(Py_buffer* view) : view(view) {}
  ~RaiiPyBuffer() { PyBuffer_Release(view); }
};

template <typename T>
std::optional<SourceSpan<T>> TrySourceSpanFromRepeatedScalarContainer(
    const Message* message, PyObject* value,
    const FieldDescriptor* field_descriptor) {
  if (Py_TYPE(value) == &RepeatedScalarContainer_Type) {
    const auto* container =
        reinterpret_cast<const RepeatedScalarContainer*>(value);
    const auto* field = container->parent_field_descriptor;
    const auto* value_message = container->parent->message;
    const Reflection* reflection = value_message->GetReflection();
    bool is_self_assign = field_descriptor == field && message == value_message;
    if constexpr (!std::is_same_v<T, absl::string_view>) {
      if (field->cpp_type() == field_descriptor->cpp_type()) {
        return SourceSpan<T>(
            RepeatedScalarContainerFriend::GetRepeatedFieldSpan<T>(
                reflection, value_message, field),
            is_self_assign);
      }
    }
    if constexpr (std::is_integral_v<T>) {
      switch (field->cpp_type()) {
        case FieldDescriptor::CPPTYPE_INT32:
          return SourceSpan<T>(
              RepeatedScalarContainerFriend::GetRepeatedFieldSpan<int32_t>(
                  reflection, value_message, field));
        case FieldDescriptor::CPPTYPE_INT64:
          return SourceSpan<T>(
              RepeatedScalarContainerFriend::GetRepeatedFieldSpan<int64_t>(
                  reflection, value_message, field));
        case FieldDescriptor::CPPTYPE_UINT32:
          return SourceSpan<T>(
              RepeatedScalarContainerFriend::GetRepeatedFieldSpan<uint32_t>(
                  reflection, value_message, field));
        case FieldDescriptor::CPPTYPE_UINT64:
          return SourceSpan<T>(
              RepeatedScalarContainerFriend::GetRepeatedFieldSpan<uint64_t>(
                  reflection, value_message, field));
        case FieldDescriptor::CPPTYPE_BOOL:
          return SourceSpan<T>(
              RepeatedScalarContainerFriend::GetRepeatedFieldSpan<Bool>(
                  reflection, value_message, field));
        default:
          break;
      }
    } else if constexpr (std::is_floating_point_v<T>) {
      switch (field->cpp_type()) {
        case FieldDescriptor::CPPTYPE_FLOAT: {
          return SourceSpan<T>(
              RepeatedScalarContainerFriend::GetRepeatedFieldSpan<float>(
                  reflection, value_message, field));
        }
        case FieldDescriptor::CPPTYPE_DOUBLE: {
          return SourceSpan<T>(
              RepeatedScalarContainerFriend::GetRepeatedFieldSpan<double>(
                  reflection, value_message, field));
        }
        default:
          break;
      }
    }
  }

  return std::nullopt;
}

template <typename T>
std::optional<SourceSpan<T>> TrySourceSpanFromBuffer(const Message* message,
                                                     const Py_buffer& view) {
  const char fmt = view.format != nullptr ? view.format[0] : 0;
  size_t size = static_cast<size_t>(view.len / view.itemsize);
  if (size > std::numeric_limits<int>::max()) {
    PyErr_SetString(PyExc_ValueError, "Repeated field too large");
    size = std::numeric_limits<int>::max();
  }
  // Only support integral-integral and float-float conversions.
  // The fall-back path below supports integral-float conversions.
  // It is an error to cast from float to integral.
  if constexpr (std::is_integral_v<T>) {
    switch (view.itemsize) {
      case 1:
        if (fmt == '?') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const Bool*>(view.buf), size));
        } else if (fmt == 'B') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const uint8_t*>(view.buf), size));
        } else if (fmt == 'b') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const int8_t*>(view.buf), size));
        }
        break;
      case 2:
        if (fmt == 'h') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const int16_t*>(view.buf), size));
        } else if (fmt == 'H') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const uint16_t*>(view.buf), size));
        }
        break;
      case 4:
        if (fmt == 'i' || fmt == 'l') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const int32_t*>(view.buf), size));
        } else if (fmt == 'I' || fmt == 'L') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const uint32_t*>(view.buf), size));
        }
        break;
      case 8:
        if (fmt == 'q' || fmt == 'l') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const int64_t*>(view.buf), size));
        } else if (fmt == 'Q' || fmt == 'L') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const uint64_t*>(view.buf), size));
        }
        break;
      default:
        break;
    }
  } else if constexpr (std::is_floating_point_v<T>) {
    switch (view.itemsize) {
      case 4:
        if (fmt == 'f') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const float*>(view.buf), size));
        }
        break;
      case 8:
        if (fmt == 'd') {
          return SourceSpan<T>(absl::MakeConstSpan(
              reinterpret_cast<const double*>(view.buf), size));
        }
        break;
      default:
        break;
    }
  }
  return std::nullopt;
}

template <typename T>
absl::Span<const T> SourceSpanFromPyIterable(
    PyObject* value, const FieldDescriptor* field_descriptor,
    std::vector<ScopedPyObjectPtr>& keep_objects, std::vector<T>& values) {
  Py_ssize_t length = PyObject_LengthHint(value, 0);
  if (length < 0) {
    PyErr_Clear();
    length = 0;
  } else if (length > 0) {
    values.reserve(static_cast<size_t>(length));
    if constexpr (std::is_same_v<T, absl::string_view>) {
      keep_objects.reserve(static_cast<size_t>(length));
    }
  }
  ScopedPyObjectPtr iter(PyObject_GetIter(value));
  if (iter == nullptr) {
    PyErr_SetString(PyExc_TypeError, "Value must be iterable");
    return absl::MakeConstSpan(values);
  }
  ScopedPyObjectPtr next;
  while ((next.reset(PyIter_Next(iter.get()))) != nullptr) {
    if (values.size() >= static_cast<size_t>(std::numeric_limits<int>::max())) {
      PyErr_SetString(PyExc_ValueError, "Repeated field too large");
      return absl::MakeConstSpan(values);
    }
    PyObject* arg = next.get();
    if constexpr (std::is_same_v<T, Bool>) {
      bool bool_value;
      if (!CheckAndGetBool(arg, &bool_value)) {
        return absl::MakeConstSpan(values);
      }
      values.push_back(Bool(bool_value));
    } else if constexpr (std::is_same_v<T, float>) {
      if (!CheckAndGetFloat(arg, &values.emplace_back())) {
        values.pop_back();
        return absl::MakeConstSpan(values);
      }
    } else if constexpr (std::is_same_v<T, double>) {
      if (!CheckAndGetDouble(arg, &values.emplace_back())) {
        values.pop_back();
        return absl::MakeConstSpan(values);
      }
    } else if constexpr (std::is_integral_v<T>) {
      if (!CheckAndGetInteger(arg, &values.emplace_back())) {
        values.pop_back();
        return absl::MakeConstSpan(values);
      }
    } else if constexpr (std::is_same_v<T, absl::string_view>) {
      std::optional<absl::string_view> string_value =
          CheckString(arg, field_descriptor);
      if (!string_value.has_value()) {
        return absl::MakeConstSpan(values);
      }
      keep_objects.emplace_back(next.release());
      values.push_back(*string_value);
    } else {
      static_assert(sizeof(T) == -1,
                    "Unsupported type (use absl::string_view for strings)");
    }
  }
  return absl::MakeConstSpan(values);
}

template <typename T, typename Func>
bool CallWithSpanImpl(const Message* message, PyObject* value,
                      const FieldDescriptor* field_descriptor, Func&& func) {
  std::optional<SourceSpan<T>> source_span =
      TrySourceSpanFromRepeatedScalarContainer<T>(message, value,
                                                  field_descriptor);
  Py_buffer view;
  std::optional<RaiiPyBuffer> view_raii;
  if (!source_span.has_value() && GetContiguous1DView(value, &view)) {
    view_raii.emplace(&view);
    source_span = TrySourceSpanFromBuffer<T>(message, view);
  }
  std::vector<ScopedPyObjectPtr> keep_objects;
  std::vector<T> values;
  if (!source_span.has_value()) {
    source_span = SourceSpan<T>(SourceSpanFromPyIterable<T>(
        value, field_descriptor, keep_objects, values));
  }
  return CallWithSpanHandlingExceptions<T>(source_span->span(),
                                           std::forward<Func>(func));
}

template <typename F>
bool CallWithSpan(const Message* message,
                  const FieldDescriptor* field_descriptor, PyObject* value,
                  F&& op) {
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return CallWithSpanImpl<int32_t>(message, value, field_descriptor,
                                       std::forward<F>(op));
    case FieldDescriptor::CPPTYPE_ENUM:
      if (field_descriptor->legacy_enum_field_treated_as_closed()) {
        return CallWithSpanImpl<int32_t>(
            message, value, field_descriptor,
            [&](absl::Span<const int32_t> values) {
              const EnumDescriptor* enum_descriptor =
                  field_descriptor->enum_type();
              size_t failed_index = 0;
              for (int32_t val : values) {
                if (enum_descriptor->FindValueByNumber(val) == nullptr) {
                  break;
                }
                ++failed_index;
              }
              if (failed_index != values.size()) {
                auto valid_values = values.subspan(0, failed_index);
                bool result = std::forward<F>(op)(valid_values);
                if (result) {
                  PyErr_Format(
                      PyExc_ValueError, "Unknown enum value: %d at index %d",
                      values[failed_index], static_cast<int>(failed_index));
                }
                return false;
              } else {
                return std::forward<F>(op)(values);
              }
            });
      }
      return CallWithSpanImpl<int32_t>(message, value, field_descriptor,
                                       std::forward<F>(op));
    case FieldDescriptor::CPPTYPE_INT64:
      return CallWithSpanImpl<int64_t>(message, value, field_descriptor,
                                       std::forward<F>(op));
    case FieldDescriptor::CPPTYPE_UINT32:
      return CallWithSpanImpl<uint32_t>(message, value, field_descriptor,
                                        std::forward<F>(op));
    case FieldDescriptor::CPPTYPE_UINT64:
      return CallWithSpanImpl<uint64_t>(message, value, field_descriptor,
                                        std::forward<F>(op));
    case FieldDescriptor::CPPTYPE_FLOAT:
      return CallWithSpanImpl<float>(message, value, field_descriptor,
                                     std::forward<F>(op));
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return CallWithSpanImpl<double>(message, value, field_descriptor,
                                      std::forward<F>(op));
    case FieldDescriptor::CPPTYPE_BOOL:
      return CallWithSpanImpl<Bool>(message, value, field_descriptor,
                                    std::forward<F>(op));
    case FieldDescriptor::CPPTYPE_STRING:
      return CallWithSpanImpl<absl::string_view>(
          message, value, field_descriptor, std::forward<F>(op));
    default:
      PyErr_Format(PyExc_SystemError,
                   "CallWithSpan on a field of unknown type %d",
                   field_descriptor->cpp_type());
      return false;
  }
}

}  // namespace

namespace repeated_scalar_container {

static PyObject* SetContainerFrozenError() {
  return SetFrozenError("Container is immutable");
}

static int InternalAssignRepeatedField(RepeatedScalarContainer* self,
                                       PyObject* list) {
  Message* message = cmessage::AssureWritable(self->parent);
  if (message == nullptr) return -1;
  message->GetReflection()->ClearField(message, self->parent_field_descriptor);
  for (Py_ssize_t i = 0; i < PyList_GET_SIZE(list); ++i) {
    PyObject* value = PyList_GET_ITEM(list, i);
    if (ScopedPyObjectPtr(Append(self, value)) == nullptr) {
      return -1;
    }
  }
  return 0;
}

static Py_ssize_t Len(PyObject* pself) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);
  const Message* message = self->parent->message;
  return message->GetReflection()->FieldSize(*message,
                                             self->parent_field_descriptor);
}

static int AssignItem(PyObject* pself, Py_ssize_t index_zd, PyObject* arg) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  Message* message = cmessage::AssureWritable(self->parent);
  if (message == nullptr) return -1;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;

  const Reflection* reflection = message->GetReflection();
  int field_size = reflection->FieldSize(*message, field_descriptor);
  if (index_zd < 0) {
    index_zd = field_size + index_zd;
  }
  if (index_zd < 0 || index_zd >= field_size) {
    PyErr_Format(PyExc_IndexError, "list assignment index (%zd) out of range",
                 index_zd);
    return -1;
  }
  int index = static_cast<int>(index_zd);

  if (arg == nullptr) {
    ScopedPyObjectPtr py_index(PyLong_FromLong(index));
    return cmessage::DeleteRepeatedField(self->parent, field_descriptor,
                                         py_index.get());
  }

  if (PySequence_Check(arg) && !(PyBytes_Check(arg) || PyUnicode_Check(arg))) {
    PyErr_SetString(PyExc_TypeError, "Value must be scalar");
    return -1;
  }

  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      PROTOBUF_CHECK_GET_INT32(arg, value, -1);
      reflection->SetRepeatedInt32(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      PROTOBUF_CHECK_GET_INT64(arg, value, -1);
      reflection->SetRepeatedInt64(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      PROTOBUF_CHECK_GET_UINT32(arg, value, -1);
      reflection->SetRepeatedUInt32(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      PROTOBUF_CHECK_GET_UINT64(arg, value, -1);
      reflection->SetRepeatedUInt64(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      PROTOBUF_CHECK_GET_FLOAT(arg, value, -1);
      reflection->SetRepeatedFloat(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      PROTOBUF_CHECK_GET_DOUBLE(arg, value, -1);
      reflection->SetRepeatedDouble(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      PROTOBUF_CHECK_GET_BOOL(arg, value, -1);
      reflection->SetRepeatedBool(message, field_descriptor, index, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      if (!CheckAndSetString(arg, message, field_descriptor, reflection, false,
                             index)) {
        return -1;
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      PROTOBUF_CHECK_GET_INT32(arg, value, -1);
      if (!field_descriptor->legacy_enum_field_treated_as_closed()) {
        reflection->SetRepeatedEnumValue(message, field_descriptor, index,
                                         value);
      } else {
        const EnumDescriptor* enum_descriptor = field_descriptor->enum_type();
        const EnumValueDescriptor* enum_value =
            enum_descriptor->FindValueByNumber(value);
        if (enum_value != nullptr) {
          reflection->SetRepeatedEnum(message, field_descriptor, index,
                                      enum_value);
        } else {
          ScopedPyObjectPtr s(PyObject_Str(arg));
          if (s != nullptr) {
            PyErr_Format(PyExc_ValueError, "Unknown enum value: %s",
                         PyString_AsString(s.get()));
          }
          return -1;
        }
      }
      break;
    }
    default:
      PyErr_Format(PyExc_SystemError,
                   "Adding value to a field of unknown type %d",
                   field_descriptor->cpp_type());
      return -1;
  }
  return 0;
}

static PyObject* Item(PyObject* pself, Py_ssize_t index) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  const Message* message = self->parent->message;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;
  const Reflection* reflection = message->GetReflection();

  int field_size = reflection->FieldSize(*message, field_descriptor);
  if (index < 0) {
    index = field_size + index;
  }
  if (index < 0 || index >= field_size) {
    PyErr_Format(PyExc_IndexError, "list index (%zd) out of range", index);
    return nullptr;
  }

  PyObject* result = nullptr;
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      int32_t value =
          reflection->GetRepeatedInt32(*message, field_descriptor, index);
      result = PyLong_FromLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      int64_t value =
          reflection->GetRepeatedInt64(*message, field_descriptor, index);
      result = PyLong_FromLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      uint32_t value =
          reflection->GetRepeatedUInt32(*message, field_descriptor, index);
      result = PyLong_FromLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      uint64_t value =
          reflection->GetRepeatedUInt64(*message, field_descriptor, index);
      result = PyLong_FromUnsignedLongLong(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      float value =
          reflection->GetRepeatedFloat(*message, field_descriptor, index);
      result = PyFloat_FromDouble(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      double value =
          reflection->GetRepeatedDouble(*message, field_descriptor, index);
      result = PyFloat_FromDouble(value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      bool value =
          reflection->GetRepeatedBool(*message, field_descriptor, index);
      result = PyBool_FromLong(value ? 1 : 0);
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      const EnumValueDescriptor* enum_value =
          message->GetReflection()->GetRepeatedEnum(*message, field_descriptor,
                                                    index);
      result = PyLong_FromLong(enum_value->number());
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      std::string scratch;
      const std::string& value = reflection->GetRepeatedStringReference(
          *message, field_descriptor, index, &scratch);
      result = ToStringObject(field_descriptor, value);
      break;
    }
    default:
      PyErr_Format(PyExc_SystemError,
                   "Getting value from a repeated field of unknown type %d",
                   field_descriptor->cpp_type());
  }

  return result;
}

static PyObject* Subscript(PyObject* pself, PyObject* slice) {
  Py_ssize_t from;
  Py_ssize_t to;
  Py_ssize_t step;
  Py_ssize_t length;
  Py_ssize_t slicelength;
  bool return_list = false;
  if (PyLong_Check(slice)) {
    from = to = PyLong_AsLong(slice);
    if (from == -1 && PyErr_Occurred()) {
      return nullptr;
    }
  } else if (PyIndex_Check(slice)) {
    from = to = PyNumber_AsSsize_t(slice, PyExc_ValueError);
    if (from == -1 && PyErr_Occurred()) {
      return nullptr;
    }
  } else if (PySlice_Check(slice)) {
    length = Len(pself);
    if (PySlice_GetIndicesEx(slice, length, &from, &to, &step, &slicelength) ==
        -1) {
      return nullptr;
    }
    return_list = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "list indices must be integers");
    return nullptr;
  }

  if (!return_list) {
    return Item(pself, from);
  }

  PyObject* list = PyList_New(0);
  if (list == nullptr) {
    return nullptr;
  }
  if (from <= to) {
    if (step < 0) {
      return list;
    }
    for (Py_ssize_t index = from; index < to; index += step) {
      if (index < 0 || index >= length) {
        break;
      }
      ScopedPyObjectPtr s(Item(pself, index));
      PyList_Append(list, s.get());
    }
  } else {
    if (step > 0) {
      return list;
    }
    for (Py_ssize_t index = from; index > to; index += step) {
      if (index < 0 || index >= length) {
        break;
      }
      ScopedPyObjectPtr s(Item(pself, index));
      PyList_Append(list, s.get());
    }
  }
  return list;
}

PyObject* Append(RepeatedScalarContainer* self, PyObject* item) {
  Message* message = cmessage::AssureWritable(self->parent);
  if (message == nullptr) return nullptr;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;

  const Reflection* reflection = message->GetReflection();
  switch (field_descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: {
      PROTOBUF_CHECK_GET_INT32(item, value, nullptr);
      reflection->AddInt32(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_INT64: {
      PROTOBUF_CHECK_GET_INT64(item, value, nullptr);
      reflection->AddInt64(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT32: {
      PROTOBUF_CHECK_GET_UINT32(item, value, nullptr);
      reflection->AddUInt32(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_UINT64: {
      PROTOBUF_CHECK_GET_UINT64(item, value, nullptr);
      reflection->AddUInt64(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_FLOAT: {
      PROTOBUF_CHECK_GET_FLOAT(item, value, nullptr);
      reflection->AddFloat(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_DOUBLE: {
      PROTOBUF_CHECK_GET_DOUBLE(item, value, nullptr);
      reflection->AddDouble(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_BOOL: {
      PROTOBUF_CHECK_GET_BOOL(item, value, nullptr);
      reflection->AddBool(message, field_descriptor, value);
      break;
    }
    case FieldDescriptor::CPPTYPE_STRING: {
      if (!CheckAndSetString(item, message, field_descriptor, reflection, true,
                             -1)) {
        return nullptr;
      }
      break;
    }
    case FieldDescriptor::CPPTYPE_ENUM: {
      PROTOBUF_CHECK_GET_INT32(item, value, nullptr);
      if (!field_descriptor->legacy_enum_field_treated_as_closed()) {
        reflection->AddEnumValue(message, field_descriptor, value);
      } else {
        const EnumDescriptor* enum_descriptor = field_descriptor->enum_type();
        const EnumValueDescriptor* enum_value =
            enum_descriptor->FindValueByNumber(value);
        if (enum_value != nullptr) {
          reflection->AddEnum(message, field_descriptor, enum_value);
        } else {
          ScopedPyObjectPtr s(PyObject_Str(item));
          if (s != nullptr) {
            PyErr_Format(PyExc_ValueError, "Unknown enum value: %s",
                         PyString_AsString(s.get()));
          }
          return nullptr;
        }
      }
      break;
    }
    default:
      PyErr_Format(PyExc_SystemError,
                   "Adding value to a field of unknown type %d",
                   field_descriptor->cpp_type());
      return nullptr;
  }

  Py_RETURN_NONE;
}

static PyObject* AppendMethod(PyObject* self, PyObject* item) {
  return Append(reinterpret_cast<RepeatedScalarContainer*>(self), item);
}

static int AssSubscript(PyObject* pself, PyObject* slice, PyObject* value) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);
  Py_ssize_t from;
  Py_ssize_t to;
  Py_ssize_t step;
  Py_ssize_t length;
  Py_ssize_t slicelength;
  bool create_list = false;

  Message* message = cmessage::AssureWritable(self->parent);
  if (message == nullptr) return -1;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;

  if (value == nullptr) {
    return cmessage::DeleteRepeatedField(self->parent, field_descriptor, slice);
  }

  if (PyLong_Check(slice)) {
    from = to = PyLong_AsLong(slice);
    if (from == -1 && PyErr_Occurred()) {
      return -1;
    }
  } else if (PySlice_Check(slice)) {
    const Reflection* reflection = message->GetReflection();
    length = reflection->FieldSize(*message, field_descriptor);
    if (PySlice_GetIndicesEx(slice, length, &from, &to, &step, &slicelength) ==
        -1) {
      return -1;
    }
    create_list = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "list indices must be integers");
    return -1;
  }

  if (!create_list) {
    return AssignItem(pself, from, value);
  }
  const Reflection* reflection = message->GetReflection();

  bool ok = CallWithSpan(message, field_descriptor, value, [&](auto values) {
    int start_index;
    int count;
    int num_values;
    int tail;
    if (!SafeCast(from, &start_index) || !SafeCast(slicelength, &count) ||
        !SafeCast(values.size(), &num_values) ||
        !SafeCast(reflection->FieldSize(*message, field_descriptor) -
                      (from + slicelength),
                  &tail)) {
      return false;
    }
    int old_size = reflection->FieldSize(*message, field_descriptor);
    if (step == 1 && tail >= 0) {
      if (start_index == 0 && count == old_size) {
        reflection->ClearField(message, field_descriptor);
        RepeatedScalarContainerFriend::MergeFrom(reflection, message,
                                                 field_descriptor, values);
      } else if (count == 0 && tail == 0) {
        RepeatedScalarContainerFriend::MergeFrom(reflection, message,
                                                 field_descriptor, values);
      } else {
        // This could be arena allocated strings, so swap the elements
        // in place to avoid a full copy.
        auto reverse_range = [&](int first, int last) {
          int i = first;
          int j = last - 1;
          while (i < j) {
            reflection->SwapElements(message, field_descriptor, i, j);
            ++i;
            --j;
          }
        };

        auto rotate_range = [&](int first, int middle, int last) {
          reverse_range(first, middle);
          reverse_range(middle, last);
          reverse_range(first, last);
        };

        // Move deleted elements to the end.
        int b_start = start_index;
        int c_start = b_start + count;
        int c_end = old_size;
        rotate_range(b_start, c_start, c_end);

        for (Py_ssize_t i = 0; i < count; ++i) {
          reflection->RemoveLast(message, field_descriptor);
        }

        RepeatedScalarContainerFriend::MergeFrom(reflection, message,
                                                 field_descriptor, values);

        int tail_start = b_start;
        int tail_end;
        int x_end;
        if (!SafeCast(static_cast<int64_t>(tail_start) + tail, &tail_end) ||
            !SafeCast(static_cast<int64_t>(tail_end) + num_values, &x_end)) {
          return false;
        }
        rotate_range(tail_start, tail_end, x_end);
      }
    } else {
      if (count != num_values) {
        PyErr_SetString(PyExc_ValueError,
                        "Cannot assign to sub slice of a different length");
        return false;
      }
      for (int i = 0; i < count; ++i) {
        int next_idx;
        if (!SafeCast(start_index + static_cast<int64_t>(i) * step,
                      &next_idx)) {
          return false;
        }
        RepeatedScalarContainerFriend::Set(reflection, message,
                                           field_descriptor, next_idx,
                                           values[static_cast<size_t>(i)]);
      }
    }
    return true;
  });
  return ok ? 0 : -1;
}

PyObject* Extend(RepeatedScalarContainer* self, PyObject* value) {
  cmessage::AssureWritable(self->parent);
  Message* message = cmessage::AssureWritable(self->parent);
  if (message == nullptr) return nullptr;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;
  const Reflection* reflection = message->GetReflection();
  if (CallWithSpan(message, field_descriptor, value, [&](auto values) {
        RepeatedScalarContainerFriend::MergeFrom(reflection, message,
                                                 field_descriptor, values);
        return true;
      })) {
    Py_RETURN_NONE;
  }
  return nullptr;
}

static PyObject* Insert(PyObject* pself, PyObject* args) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  Py_ssize_t index;
  PyObject* value;
  if (!PyArg_ParseTuple(args, "nO", &index, &value)) {
    return nullptr;
  }
  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  ScopedPyObjectPtr new_list(Subscript(pself, full_slice.get()));
  if (PyList_Insert(new_list.get(), index, value) < 0) {
    return nullptr;
  }
  int ret = InternalAssignRepeatedField(self, new_list.get());
  if (ret < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* Remove(PyObject* pself, PyObject* value) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  // Even if the value doesn't exist in the container, raise immutability error
  // prior to value error if applicable.
  if (self->parent->state == python::MESSAGE_FROZEN) {
    return SetContainerFrozenError();
  }

  Py_ssize_t match_index = -1;
  for (Py_ssize_t i = 0, len = Len(pself); i < len; ++i) {
    ScopedPyObjectPtr elem(Item(pself, i));
    if (PyObject_RichCompareBool(elem.get(), value, Py_EQ)) {
      match_index = i;
      break;
    }
  }
  if (match_index == -1) {
    PyErr_SetString(PyExc_ValueError, "remove(x): x not in container");
    return nullptr;
  }
  if (AssignItem(pself, match_index, nullptr) < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* ExtendMethod(PyObject* self, PyObject* value) {
  return Extend(reinterpret_cast<RepeatedScalarContainer*>(self), value);
}

static PyObject* RichCompare(PyObject* pself, PyObject* other, int opid) {
  if (opid != Py_EQ && opid != Py_NE) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  // Copy the contents of this repeated scalar container, and other if it is
  // also a repeated scalar container, into Python lists so we can delegate
  // to the list's compare method.

  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return nullptr;
  }

  ScopedPyObjectPtr other_list_deleter;
  if (PyObject_TypeCheck(other, &RepeatedScalarContainer_Type)) {
    other_list_deleter.reset(Subscript(other, full_slice.get()));
    other = other_list_deleter.get();
  }

  ScopedPyObjectPtr list(Subscript(pself, full_slice.get()));
  if (list == nullptr) {
    return nullptr;
  }
  return PyObject_RichCompare(list.get(), other, opid);
}

// CPython equivalent of nparray.astype(dtype_requested, copy=copy) in python.
PyObject* NpArrayAsType(PyObject* nparray, PyObject* dtype_requested,
                        bool copy) {
  ScopedPyObjectPtr astype_args(Py_BuildValue("(O)", dtype_requested));
  ScopedPyObjectPtr keywords(PyDict_New());
  PyDict_SetItemString(keywords.get(), "copy", copy ? Py_True : Py_False);
  ScopedPyObjectPtr np_array_astype(PyObject_GetAttrString(nparray, "astype"));

  return PyObject_Call(np_array_astype.get(), astype_args.get(),
                       keywords.get());
}

// Takes in an iterable[str], and returns the length (in bytes) of the
// largest str in the iterable.
Py_ssize_t CalculateMaxLengthOfStrObjects(PyObject* pself) {
  ScopedPyObjectPtr iter(PyObject_GetIter(pself));
  if (iter.get() == nullptr) {
    PyErr_SetString(PyExc_TypeError, "Unable to get an iterator.");
    return -1;
  }
  ScopedPyObjectPtr next;

  // Default to 1 since empty byte arrays are expected to be represented with
  // length of 1.
  Py_ssize_t max_length_of_str = 1;
  while (next.reset(PyIter_Next(iter.get())) != nullptr) {
    Py_ssize_t length_of_str = 0;
    PyUnicode_AsUTF8AndSize(next.get(), &length_of_str);
    // Previous statement has some basic error checking, so check if
    // an error occurred via PyErr_Occurred().
    if (PyErr_Occurred()) {
      return -1;
    }

    max_length_of_str = std::max<Py_ssize_t>(length_of_str, max_length_of_str);
  }
  return max_length_of_str;
}

// Note: Returns a new reference.
PyObject* ConstructArrayByIteration(PyObject* pself, PyObject* np_module) {
  Py_ssize_t array_length = Len(pself);
  ScopedPyObjectPtr nparray(
      PyObject_CallMethod(np_module, "empty", "is", array_length, "O"));

  for (Py_ssize_t i = 0; i != array_length; ++i) {
    ScopedPyObjectPtr item(Item(pself, i));
    PySequence_SetItem(nparray.as_pyobject(), i, item.as_pyobject());
  }

  return nparray.release();
}

std::string GetDefaultDTypeStr(FieldDescriptor::CppType cpp_type) {
  switch (cpp_type) {
    case FieldDescriptor::CPPTYPE_FLOAT:
      return "f4";
    case FieldDescriptor::CPPTYPE_INT32:
      return "i4";
    case FieldDescriptor::CPPTYPE_INT64:
      return "i8";
    case FieldDescriptor::CPPTYPE_UINT32:
      return "u4";
    case FieldDescriptor::CPPTYPE_UINT64:
      return "u8";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "f8";
    case FieldDescriptor::CPPTYPE_BOOL:
      return "?";  // Boolean is '?' per official docs.
    case FieldDescriptor::CPPTYPE_ENUM:
      return "i4";
    case FieldDescriptor::CPPTYPE_STRING:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return "";
  }
  return "";
}

PyObject* CreateArrayFromView(PyObject* pself, PyObject* np_module) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);
  const Message* message = self->parent->message;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;
  const Reflection* reflection = message->GetReflection();
  std::string out_dtype = GetDefaultDTypeStr(field_descriptor->cpp_type());
  const void* out_ptr;
  Py_ssize_t out_buffer_size_bytes;

  switch (field_descriptor->cpp_type()) {
#define HANDLE_TYPE(TYPE, type)                                         \
  case FieldDescriptor::CPPTYPE_##TYPE: {                               \
    const auto& rf =                                                    \
        reflection->GetRepeatedField<type>(*message, field_descriptor); \
    out_ptr = reinterpret_cast<const void*>(rf.data());                 \
    out_buffer_size_bytes = static_cast<Py_ssize_t>(sizeof(type)) *     \
                            static_cast<Py_ssize_t>(rf.size());         \
    break;                                                              \
  }
    HANDLE_TYPE(FLOAT, float)
    HANDLE_TYPE(INT32, int)
    HANDLE_TYPE(INT64, int64_t)
    HANDLE_TYPE(UINT32, uint32_t)
    HANDLE_TYPE(UINT64, uint64_t)
    HANDLE_TYPE(DOUBLE, double)
    HANDLE_TYPE(BOOL, bool)
    HANDLE_TYPE(ENUM, int32_t)
#undef HANDLE_TYPE
    case FieldDescriptor::CPPTYPE_MESSAGE:
    case FieldDescriptor::CPPTYPE_STRING: {
      PyErr_Format(PyExc_SystemError,
                   "Code should never reach here: cpp type should never be "
                   "string nor message in GetDTypeAndBuffer().");
      return nullptr;
    }
  }
  if (out_buffer_size_bytes == 0) {
    return PyObject_CallMethod(np_module, "empty", "is", 0, out_dtype.c_str());
  }
  ScopedPyObjectPtr view(PyMemoryView_FromMemory(
      const_cast<char*>(reinterpret_cast<const char*>(out_ptr)),
      out_buffer_size_bytes, PyBUF_READ));
  return PyObject_CallMethod(np_module, "frombuffer", "Os", view.as_pyobject(),
                             out_dtype.c_str());
}

PyObject* AsNpArray(PyObject* pself, PyObject* args, PyObject* kwargs) {
  static const char* kwlist[] = {"dtype", "copy", nullptr};
  PyObject* dtype_requested = nullptr;
  PyObject* copy = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO",
                                   const_cast<char**>(kwlist), &dtype_requested,
                                   &copy)) {
    return nullptr;
  }

  ScopedPyObjectPtr np_module(PyImport_ImportModule("numpy"));
  if (np_module.get() == nullptr) {
    PyErr_Format(PyExc_ImportError, "Unable to import numpy.");
    return nullptr;
  }

  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;
  FieldDescriptor::CppType cpp_type = field_descriptor->cpp_type();
  ScopedPyObjectPtr default_dtype;
  if (cpp_type == FieldDescriptor::CPPTYPE_STRING) {
    ScopedPyObjectPtr nparray(
        ConstructArrayByIteration(pself, np_module.get()));
    if (dtype_requested == nullptr) {
      if (field_descriptor->type() == FieldDescriptor::TYPE_STRING) {
        dtype_requested =
            PyObject_CallMethod(np_module.get(), "dtype", "s", "str");
      } else {
        dtype_requested =
            PyObject_CallMethod(np_module.get(), "dtype", "s", "S");
      }
      // For ref-counting
      default_dtype.reset(dtype_requested);
    }
    return NpArrayAsType(nparray.get(), dtype_requested, false);
  }

  // For non string repeated scalar
  if (dtype_requested == nullptr) {
    std::string returned_dtype = GetDefaultDTypeStr(cpp_type);
    if (Len(pself) == 0) {
      return PyObject_CallMethod(np_module.get(), "empty", "is", 0,
                                 returned_dtype.c_str());
    }
    // Get default dtype for the current field.
    dtype_requested = PyObject_CallMethod(np_module.get(), "dtype", "s",
                                          returned_dtype.c_str());
    // For ref-counting.
    default_dtype.reset(dtype_requested);
  }
  // Create an np array from a memoryview.
  ScopedPyObjectPtr nparray_nocopy(CreateArrayFromView(pself, np_module.get()));
  if (nparray_nocopy.get() == nullptr) {
    return nullptr;
  }
  // Return using astype(). This will copy.
  return NpArrayAsType(nparray_nocopy.as_pyobject(), dtype_requested, true);
}

PyObject* Reduce(PyObject* unused_self, PyObject* unused_other) {
  PyErr_Format(PickleError_class,
               "can't pickle repeated message fields, convert to list first");
  return nullptr;
}

static PyObject* Sort(PyObject* pself, PyObject* args, PyObject* kwds) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  if (self->parent->state == python::MESSAGE_FROZEN) {
    return SetContainerFrozenError();
  }

  // Support the old sort_function argument for backwards
  // compatibility.
  if (kwds != nullptr) {
    PyObject* sort_func = PyDict_GetItemString(kwds, "sort_function");
    if (sort_func != nullptr) {
      // Must set before deleting as sort_func is a borrowed reference
      // and kwds might be the only thing keeping it alive.
      if (PyDict_SetItemString(kwds, "cmp", sort_func) == -1) return nullptr;
      if (PyDict_DelItemString(kwds, "sort_function") == -1) return nullptr;
    }
  }

  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr list(Subscript(pself, full_slice.get()));
  if (list == nullptr) {
    return nullptr;
  }
  if (PyList_GET_SIZE(list.get()) == 0) {
    Py_RETURN_NONE;
  }
  ScopedPyObjectPtr m(PyObject_GetAttrString(list.get(), "sort"));
  if (m == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr res(PyObject_Call(m.get(), args, kwds));
  if (res == nullptr) {
    return nullptr;
  }
  int ret = InternalAssignRepeatedField(
      reinterpret_cast<RepeatedScalarContainer*>(pself), list.get());
  if (ret < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* Reverse(PyObject* pself) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  if (self->parent->state == python::MESSAGE_FROZEN) {
    return SetContainerFrozenError();
  }

  // TODO: b/517235198 - Reify even for empty sequences.
  if (Len(pself) == 0) {
    Py_RETURN_NONE;
  }

  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr list(Subscript(pself, full_slice.get()));
  if (list == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr res(PyObject_CallMethod(list.get(), "reverse", nullptr));
  if (res == nullptr) {
    return nullptr;
  }
  int ret = InternalAssignRepeatedField(
      reinterpret_cast<RepeatedScalarContainer*>(pself), list.get());
  if (ret < 0) {
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* Clear(PyObject* pself) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  // TODO: b/517235198 - Reify even for empty sequences.
  if (Len(pself) == 0) {
    Py_RETURN_NONE;
  }

  CMessage* cmessage = self->parent;
  Message* message = cmessage::AssureWritable(cmessage);
  if (message == nullptr) return nullptr;
  const FieldDescriptor* field_descriptor = self->parent_field_descriptor;
  message->GetReflection()->ClearField(message, field_descriptor);
  Py_RETURN_NONE;
}

static PyObject* Pop(PyObject* pself, PyObject* args) {
  RepeatedScalarContainer* self =
      reinterpret_cast<RepeatedScalarContainer*>(pself);

  // Even if the value doesn't exist in the container, raise immutability error
  // prior to value error.
  if (self->parent->state == python::MESSAGE_FROZEN) {
    return SetContainerFrozenError();
  }

  Py_ssize_t index = -1;
  if (!PyArg_ParseTuple(args, "|n", &index)) {
    return nullptr;
  }
  PyObject* item = Item(pself, index);
  if (item == nullptr) {
    PyErr_Format(PyExc_IndexError, "list index (%zd) out of range", index);
    return nullptr;
  }
  if (AssignItem(pself, index, nullptr) < 0) {
    return nullptr;
  }
  return item;
}

static PyObject* ToStr(PyObject* pself) {
  ScopedPyObjectPtr full_slice(PySlice_New(nullptr, nullptr, nullptr));
  if (full_slice == nullptr) {
    return nullptr;
  }
  ScopedPyObjectPtr list(Subscript(pself, full_slice.get()));
  if (list == nullptr) {
    return nullptr;
  }
  return PyObject_Repr(list.get());
}

static PyObject* MergeFrom(PyObject* pself, PyObject* arg) {
  return Extend(reinterpret_cast<RepeatedScalarContainer*>(pself), arg);
}

// The private constructor of RepeatedScalarContainer objects.
RepeatedScalarContainer* NewContainer(
    CMessage* parent, const FieldDescriptor* parent_field_descriptor) {
  if (!CheckFieldBelongsToMessage(parent_field_descriptor, parent->message)) {
    return nullptr;
  }

  RepeatedScalarContainer* self = reinterpret_cast<RepeatedScalarContainer*>(
      PyType_GenericAlloc(&RepeatedScalarContainer_Type, 0));
  if (self == nullptr) {
    return nullptr;
  }

  Py_INCREF(parent);
  self->parent = parent;
  self->parent_field_descriptor = parent_field_descriptor;

  return self;
}

PyObject* DeepCopy(PyObject* pself, PyObject* arg) {
  return reinterpret_cast<RepeatedScalarContainer*>(pself)->DeepCopy();
}

static void Dealloc(PyObject* pself) {
  reinterpret_cast<RepeatedScalarContainer*>(pself)->RemoveFromParentCache();
  Py_TYPE(pself)->tp_free(pself);
}

static PySequenceMethods SqMethods = {
    Len,       /* sq_length */
    nullptr,   /* sq_concat */
    nullptr,   /* sq_repeat */
    Item,      /* sq_item */
    nullptr,   /* sq_slice */
    AssignItem /* sq_ass_item */
};

static PyMappingMethods MpMethods = {
    Len,          /* mp_length */
    Subscript,    /* mp_subscript */
    AssSubscript, /* mp_ass_subscript */
};

static PyMethodDef Methods[] = {
    {"__deepcopy__", DeepCopy, METH_VARARGS, "Makes a deep copy of the class."},
    {"__array__", (PyCFunction)AsNpArray, METH_VARARGS | METH_KEYWORDS,
     "Returns a np.array."},
    {"__reduce__", Reduce, METH_NOARGS,
     "Outputs picklable representation of the repeated field."},
    {"append", AppendMethod, METH_O,
     "Appends an object to the repeated container."},
    {"extend", ExtendMethod, METH_O,
     "Appends objects to the repeated container."},
    {"insert", Insert, METH_VARARGS,
     "Inserts an object at the specified position in the container."},
    {"pop", Pop, METH_VARARGS,
     "Removes an object from the repeated container and returns it."},
    {"remove", Remove, METH_O,
     "Removes an object from the repeated container."},
    {"sort", reinterpret_cast<PyCFunction>(Sort), METH_VARARGS | METH_KEYWORDS,
     "Sorts the repeated container."},
    {"reverse", reinterpret_cast<PyCFunction>(Reverse), METH_NOARGS,
     "Reverses elements order of the repeated container."},
    {"clear", reinterpret_cast<PyCFunction>(Clear), METH_NOARGS,
     "Clears the repeated container."},
    {"MergeFrom", static_cast<PyCFunction>(MergeFrom), METH_O,
     "Merges a repeated container into the current container."},
    {nullptr, nullptr}};

}  // namespace repeated_scalar_container

PyTypeObject RepeatedScalarContainer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) FULL_MODULE_NAME
    ".RepeatedScalarContainer",          // tp_name
    sizeof(RepeatedScalarContainer),     // tp_basicsize
    0,                                   //  tp_itemsize
    repeated_scalar_container::Dealloc,  //  tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
    0,  //  tp_vectorcall_offset
#else
    nullptr,  //  tp_print
#endif
    nullptr,                                //  tp_getattr
    nullptr,                                //  tp_setattr
    nullptr,                                //  tp_compare
    repeated_scalar_container::ToStr,       //  tp_repr
    nullptr,                                //  tp_as_number
    &repeated_scalar_container::SqMethods,  //  tp_as_sequence
    &repeated_scalar_container::MpMethods,  //  tp_as_mapping
    PyObject_HashNotImplemented,            //  tp_hash
    nullptr,                                //  tp_call
    nullptr,                                //  tp_str
    nullptr,                                //  tp_getattro
    nullptr,                                //  tp_setattro
    nullptr,                                //  tp_as_buffer
#if PY_VERSION_HEX >= 0x030A0000
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_SEQUENCE,  //  tp_flags
#else
    Py_TPFLAGS_DEFAULT,  //  tp_flags
#endif
    "A Repeated scalar container",           //  tp_doc
    nullptr,                                 //  tp_traverse
    nullptr,                                 //  tp_clear
    repeated_scalar_container::RichCompare,  //  tp_richcompare
    0,                                       //  tp_weaklistoffset
    nullptr,                                 //  tp_iter
    nullptr,                                 //  tp_iternext
    repeated_scalar_container::Methods,      //  tp_methods
    nullptr,                                 //  tp_members
    nullptr,                                 //  tp_getset
    nullptr,                                 //  tp_base
    nullptr,                                 //  tp_dict
    nullptr,                                 //  tp_descr_get
    nullptr,                                 //  tp_descr_set
    0,                                       //  tp_dictoffset
    nullptr,                                 //  tp_init
};

}  // namespace python
}  // namespace protobuf
}  // namespace google
