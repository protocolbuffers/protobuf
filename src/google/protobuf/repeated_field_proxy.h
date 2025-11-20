#ifndef GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__
#define GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string>
#include <type_traits>

#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/repeated_field.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

namespace internal {
class StringPieceField;
}  // namespace internal

namespace internal {

template <typename ElementType>
using RepeatedFieldType = std::conditional_t<
    std::is_base_of_v<MessageLite, ElementType> ||
        std::is_same_v<ElementType, std::string> ||
        std::is_same_v<ElementType, google::protobuf::internal::StringPieceField>,
    RepeatedPtrField<ElementType>, RepeatedField<ElementType>>;

template <typename ElementType, typename RepeatedFieldType>
class RepeatedFieldProxyBase {
  static_assert(!std::is_const_v<ElementType>);

 private:
  static constexpr bool kIsConst = std::is_const_v<RepeatedFieldType>;

 public:
  using value_type = ElementType;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using const_reference = typename RepeatedFieldType::const_reference;
  using reference = std::conditional_t<kIsConst, const_reference,
                                       typename RepeatedFieldType::reference>;
  using const_iterator = typename RepeatedFieldType::const_iterator;
  using iterator = std::conditional_t<kIsConst, const_iterator,
                                      typename RepeatedFieldType::iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator =
      std::conditional_t<kIsConst, const_reverse_iterator,
                         typename RepeatedFieldType::reverse_iterator>;

  // NOLINTNEXTLINE(google-explicit-constructor)
  PROTOBUF_ALWAYS_INLINE RepeatedFieldProxyBase(RepeatedFieldType& field)
      : field_(&field), arena_(field.GetArena()) {}

  RepeatedFieldProxyBase(const RepeatedFieldProxyBase&) = default;
  ~RepeatedFieldProxyBase() = default;

  PROTOBUF_ALWAYS_INLINE bool empty() const { return field_->empty(); }
  PROTOBUF_ALWAYS_INLINE size_type size() const {
    return static_cast<size_type>(field_->size());
  }

  PROTOBUF_ALWAYS_INLINE const_reference operator[](size_type index) const {
    return (*field_)[index];
  }

  PROTOBUF_ALWAYS_INLINE const_iterator cbegin() const {
    return field_->cbegin();
  }
  PROTOBUF_ALWAYS_INLINE const_iterator cend() const { return field_->cend(); }
  PROTOBUF_ALWAYS_INLINE const_iterator begin() const {
    return field().begin();
  }
  PROTOBUF_ALWAYS_INLINE iterator begin() { return field().begin(); }
  PROTOBUF_ALWAYS_INLINE const_iterator end() const { return field().end(); }
  PROTOBUF_ALWAYS_INLINE iterator end() { return field().end(); }
  PROTOBUF_ALWAYS_INLINE const_reverse_iterator rbegin() const {
    return field().rbegin();
  }
  PROTOBUF_ALWAYS_INLINE reverse_iterator rbegin() { return field().rbegin(); }
  PROTOBUF_ALWAYS_INLINE const_reverse_iterator rend() const {
    return field().rend();
  }
  PROTOBUF_ALWAYS_INLINE reverse_iterator rend() { return field().rend(); }

 protected:
  PROTOBUF_ALWAYS_INLINE RepeatedFieldProxyBase(RepeatedFieldType* field,
                                                Arena* arena)
      : field_(field), arena_(arena) {}

  PROTOBUF_ALWAYS_INLINE std::add_const_t<RepeatedFieldType>& field() const {
    return *field_;
  }
  PROTOBUF_ALWAYS_INLINE RepeatedFieldType& field() { return *field_; }

  PROTOBUF_ALWAYS_INLINE Arena* arena() const { return arena_; }

 private:
  RepeatedFieldType* field_;
  Arena* arena_;
};

}  // namespace internal

template <typename ElementType>
class RepeatedFieldProxy;

template <typename ElementType>
class RepeatedFieldProxy
    : public internal::RepeatedFieldProxyBase<
          ElementType, internal::RepeatedFieldType<ElementType>> {
  static_assert(!std::is_const_v<ElementType>);

  using RepeatedFieldType = internal::RepeatedFieldType<ElementType>;
  using Base = internal::RepeatedFieldProxyBase<ElementType, RepeatedFieldType>;

 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  PROTOBUF_ALWAYS_INLINE RepeatedFieldProxy(RepeatedFieldType& field)
      : Base(field) {}

  PROTOBUF_ALWAYS_INLINE Base::reference operator[](Base::size_type index) {
    return Base::field()[index];
  }

  PROTOBUF_ALWAYS_INLINE void push_back(ElementType&& value) {
    Base::field().AddWithArena(Base::arena(), std::move(value));
  }
  PROTOBUF_ALWAYS_INLINE void push_back(const ElementType& value) {
    Base::field().AddWithArena(Base::arena(), value);
  }

  template <typename Iter>
  PROTOBUF_ALWAYS_INLINE void insert(Base::const_iterator pos, Iter begin,
                                     Iter end) {
    ABSL_DCHECK(pos == end());
    Base::field().AddWithArena(Base::arena(), begin, end);
  }

  template <typename... Args>
  Base::reference emplace_back(Args&&... args) {
    return *Base::field().AddWithArena(Base::arena()) =
               ElementType(std::forward<Args>(args)...);
  }

  PROTOBUF_ALWAYS_INLINE void pop_back() { Base::field().RemoveLast(); }

  PROTOBUF_ALWAYS_INLINE void clear() { Base::field().Clear(); }

  PROTOBUF_ALWAYS_INLINE Base::iterator erase(Base::const_iterator position) {
    return Base::field().erase(position);
  }
  PROTOBUF_ALWAYS_INLINE Base::iterator erase(Base::const_iterator first,
                                              Base::const_iterator last) {
    return Base::field().erase(first, last);
  }

  template <typename Iter>
  PROTOBUF_ALWAYS_INLINE void assign(Iter begin, Iter end) {
    Base::field().Assign(begin, end);
  }

  PROTOBUF_ALWAYS_INLINE void assign(
      std::initializer_list<ElementType> values) {
    assign(values.begin(), values.end());
  }

  PROTOBUF_ALWAYS_INLINE void reserve(int new_size) {
    Base::field().Reserve(new_size);
  }

  PROTOBUF_ALWAYS_INLINE void swap(RepeatedFieldProxy other) {
    Base::field().Swap(other.field_);
  }

  template <typename R = RepeatedFieldType,
            typename = std::enable_if_t<
                std::is_base_of_v<internal::RepeatedFieldBase, R>>>
  PROTOBUF_ALWAYS_INLINE void resize(Base::size_type new_size,
                                     const ElementType& value) {
    Base::field().Resize(new_size, value);
  }
};

template <typename ElementType>
class RepeatedFieldProxy<const ElementType>
    : public internal::RepeatedFieldProxyBase<
          ElementType, const internal::RepeatedFieldType<ElementType>> {
  using RepeatedFieldType = internal::RepeatedFieldType<ElementType>;
  using Base =
      internal::RepeatedFieldProxyBase<ElementType, const RepeatedFieldType>;

 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldProxy(RepeatedFieldProxy<ElementType> other);

  // NOLINTNEXTLINE(google-explicit-constructor)
  PROTOBUF_ALWAYS_INLINE RepeatedFieldProxy(const RepeatedFieldType& field)
      : Base(field) {}

  // NOLINTNEXTLINE(google-explicit-constructor)
  RepeatedFieldProxy(RepeatedFieldType& field)
      : RepeatedFieldProxy(static_cast<const RepeatedFieldType&>(field)) {}
};

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_REPEATED_FIELD_PROXY_H__
