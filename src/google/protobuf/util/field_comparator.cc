// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: ksroka@google.com (Krzysztof Sroka)

#include "google/protobuf/util/field_comparator.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/util/message_differencer.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace util {
namespace {
template <typename T>
struct Epsilon {};
template <>
struct Epsilon<float> {
  constexpr static auto value = 32 * FLT_EPSILON;
};
template <>
struct Epsilon<double> {
  constexpr static auto value = 32 * DBL_EPSILON;
};

template <typename T>
bool WithinFractionOrMargin(const T x, const T y, const T fraction,
                            const T margin) {
  ABSL_DCHECK(fraction >= T(0) && fraction < T(1) && margin >= T(0));

  if (!std::isfinite(x) || !std::isfinite(y)) {
    return false;
  }
  const T relative_margin = fraction * std::max(std::fabs(x), std::fabs(y));
  return std::fabs(x - y) <= std::max(margin, relative_margin);
}

}  // namespace

FieldComparator::FieldComparator() {}
FieldComparator::~FieldComparator() {}

SimpleFieldComparator::SimpleFieldComparator()
    : float_comparison_(EXACT),
      treat_nan_as_equal_(false),
      has_default_tolerance_(false) {}

SimpleFieldComparator::~SimpleFieldComparator() {}

FieldComparator::ComparisonResult SimpleFieldComparator::SimpleCompare(
    const Message& message_1, const Message& message_2,
    const FieldDescriptor* field, int index_1, int index_2,
    const util::FieldContext* /*field_context*/) {
  const Reflection* reflection_1 = message_1.GetReflection();
  const Reflection* reflection_2 = message_2.GetReflection();

  switch (field->cpp_type()) {
#define COMPARE_FIELD(METHOD)                                                 \
  if (field->is_repeated()) {                                                 \
    return ResultFromBoolean(Compare##METHOD(                                 \
        *field, reflection_1->GetRepeated##METHOD(message_1, field, index_1), \
        reflection_2->GetRepeated##METHOD(message_2, field, index_2)));       \
  } else {                                                                    \
    return ResultFromBoolean(                                                 \
        Compare##METHOD(*field, reflection_1->Get##METHOD(message_1, field),  \
                        reflection_2->Get##METHOD(message_2, field)));        \
  }                                                                           \
  break;  // Make sure no fall-through is introduced.

    case FieldDescriptor::CPPTYPE_BOOL:
      COMPARE_FIELD(Bool);
    case FieldDescriptor::CPPTYPE_DOUBLE:
      COMPARE_FIELD(Double);
    case FieldDescriptor::CPPTYPE_ENUM:
      COMPARE_FIELD(Enum);
    case FieldDescriptor::CPPTYPE_FLOAT:
      COMPARE_FIELD(Float);
    case FieldDescriptor::CPPTYPE_INT32:
      COMPARE_FIELD(Int32);
    case FieldDescriptor::CPPTYPE_INT64:
      COMPARE_FIELD(Int64);
    case FieldDescriptor::CPPTYPE_STRING:
      if (field->is_repeated()) {
        // Allocate scratch strings to store the result if a conversion is
        // needed.
        std::string scratch1;
        std::string scratch2;
        return ResultFromBoolean(
            CompareString(*field,
                          reflection_1->GetRepeatedStringReference(
                              message_1, field, index_1, &scratch1),
                          reflection_2->GetRepeatedStringReference(
                              message_2, field, index_2, &scratch2)));
      } else {
        // Allocate scratch strings to store the result if a conversion is
        // needed.
        std::string scratch1;
        std::string scratch2;
        return ResultFromBoolean(CompareString(
            *field,
            reflection_1->GetStringReference(message_1, field, &scratch1),
            reflection_2->GetStringReference(message_2, field, &scratch2)));
      }
      break;
    case FieldDescriptor::CPPTYPE_UINT32:
      COMPARE_FIELD(UInt32);
    case FieldDescriptor::CPPTYPE_UINT64:
      COMPARE_FIELD(UInt64);

#undef COMPARE_FIELD

    case FieldDescriptor::CPPTYPE_MESSAGE:
      return RECURSE;

    default:
      ABSL_LOG(FATAL) << "No comparison code for field " << field->full_name()
                      << " of CppType = " << field->cpp_type();
      return DIFFERENT;
  }
}

bool SimpleFieldComparator::CompareWithDifferencer(
    MessageDifferencer* differencer, const Message& message1,
    const Message& message2, const util::FieldContext* field_context) {
  const Descriptor* descriptor1 = message1.GetDescriptor();
  const Descriptor* descriptor2 = message2.GetDescriptor();
  if (descriptor1 != descriptor2) {
    ABSL_DLOG(FATAL) << "Comparison between two messages with different "
                     << "descriptors. " << descriptor1->full_name() << " vs "
                     << descriptor2->full_name();
    return false;
  }
  return differencer->Compare(message1, message2, false,
                              field_context->parent_fields());
}

void SimpleFieldComparator::SetDefaultFractionAndMargin(double fraction,
                                                        double margin) {
  default_tolerance_ = Tolerance(fraction, margin);
  has_default_tolerance_ = true;
}

void SimpleFieldComparator::SetFractionAndMargin(const FieldDescriptor* field,
                                                 double fraction,
                                                 double margin) {
  ABSL_CHECK(FieldDescriptor::CPPTYPE_FLOAT == field->cpp_type() ||
             FieldDescriptor::CPPTYPE_DOUBLE == field->cpp_type())
      << "Field has to be float or double type. Field name is: "
      << field->full_name();
  map_tolerance_[field] = Tolerance(fraction, margin);
}

bool SimpleFieldComparator::CompareDouble(const FieldDescriptor& field,
                                          double value_1, double value_2) {
  return CompareDoubleOrFloat(field, value_1, value_2);
}

bool SimpleFieldComparator::CompareEnum(const FieldDescriptor& /*field*/,
                                        const EnumValueDescriptor* value_1,
                                        const EnumValueDescriptor* value_2) {
  return value_1->number() == value_2->number();
}

bool SimpleFieldComparator::CompareFloat(const FieldDescriptor& field,
                                         float value_1, float value_2) {
  return CompareDoubleOrFloat(field, value_1, value_2);
}

template <typename T>
bool SimpleFieldComparator::CompareDoubleOrFloat(const FieldDescriptor& field,
                                                 T value_1, T value_2) {
  if (value_1 == value_2) {
    // Covers +inf and -inf (which are not within margin or fraction of
    // themselves), and is a shortcut for finite values.
    return true;
  } else if (float_comparison_ == EXACT) {
    if (treat_nan_as_equal_ && std::isnan(value_1) && std::isnan(value_2)) {
      return true;
    }
    return false;
  } else {
    if (treat_nan_as_equal_ && std::isnan(value_1) && std::isnan(value_2)) {
      return true;
    }
    // float_comparison_ == APPROXIMATE covers two use cases.
    Tolerance* tolerance = nullptr;
    if (has_default_tolerance_) tolerance = &default_tolerance_;

    auto it = map_tolerance_.find(&field);
    if (it != map_tolerance_.end()) {
      tolerance = &it->second;
    }

    if (tolerance != nullptr) {
      // Use user-provided fraction and margin. Since they are stored as
      // doubles, we explicitly cast them to types of values provided. This
      // is very likely to fail if provided values are not numeric.
      return WithinFractionOrMargin(value_1, value_2,
                                    static_cast<T>(tolerance->fraction),
                                    static_cast<T>(tolerance->margin));
    } else {
      if (std::fabs(value_1) <= Epsilon<T>::value &&
          std::fabs(value_2) <= Epsilon<T>::value) {
        return true;
      }
      return WithinFractionOrMargin(value_1, value_2, Epsilon<T>::value,
                                    Epsilon<T>::value);
    }
  }
}

FieldComparator::ComparisonResult SimpleFieldComparator::ResultFromBoolean(
    bool boolean_result) const {
  return boolean_result ? FieldComparator::SAME : FieldComparator::DIFFERENT;
}

}  // namespace util
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
