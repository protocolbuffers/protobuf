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
#ifndef GOOGLE_PROTOBUF_STUBS_INT128_H_
#define GOOGLE_PROTOBUF_STUBS_INT128_H_

#include <google/protobuf/stubs/common.h>

#include <iosfwd>
#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace int128_internal {

// An unsigned 128-bit integer type. Thread-compatible.
class PROTOBUF_EXPORT uint128 {
 public:
  uint128() = default;

 private:
  // Use `MakeUint128` instead.
  constexpr uint128(uint64 top, uint64 bottom);

 public:
#ifndef SWIG
  constexpr uint128(int bottom);
  constexpr uint128(uint32 bottom);   // Top 96 bits = 0
#endif
  constexpr uint128(uint64 bottom);   // hi_ = 0

  // Trivial copy constructor, assignment operator and destructor.

  // Arithmetic operators.
  uint128& operator+=(const uint128& b);
  uint128& operator-=(const uint128& b);
  uint128& operator*=(const uint128& b);
  // Long division/modulo for uint128.
  uint128& operator/=(const uint128& b);
  uint128& operator%=(const uint128& b);
  uint128 operator++(int);
  uint128 operator--(int);
  uint128& operator<<=(int);
  uint128& operator>>=(int);
  uint128& operator&=(const uint128& b);
  uint128& operator|=(const uint128& b);
  uint128& operator^=(const uint128& b);
  uint128& operator++();
  uint128& operator--();

  friend constexpr uint64 Uint128Low64(const uint128& v);
  friend constexpr uint64 Uint128High64(const uint128& v);

  friend constexpr uint128 MakeUint128(uint64_t high, uint64_t low);

  // We add "std::" to avoid including all of port.h.
  PROTOBUF_EXPORT friend std::ostream& operator<<(std::ostream& o,
                                                  const uint128& b);

 private:
  static void DivModImpl(uint128 dividend, uint128 divisor,
                         uint128* quotient_ret, uint128* remainder_ret);

  // Little-endian memory order optimizations can benefit from
  // having lo_ first, hi_ last.
  // See util/endian/endian.h and Load128/Store128 for storing a uint128.
  uint64        lo_;
  uint64        hi_;

  // Not implemented, just declared for catching automatic type conversions.
  uint128(uint8) = delete;
  uint128(uint16) = delete;
  uint128(float v) = delete;
  uint128(double v) = delete;
};

// allow uint128 to be logged
PROTOBUF_EXPORT extern std::ostream& operator<<(std::ostream& o,
                                                const uint128& b);

// Methods to access low and high pieces of 128-bit value.
// Defined externally from uint128 to facilitate conversion
// to native 128-bit types when compilers support them.
inline constexpr uint64 Uint128Low64(const uint128& v) { return v.lo_; }
inline constexpr uint64 Uint128High64(const uint128& v) { return v.hi_; }

constexpr uint128 MakeUint128(uint64_t high, uint64_t low) {
  return uint128(high, low);
}

// TODO: perhaps it would be nice to have int128, a signed 128-bit type?

// --------------------------------------------------------------------------
//                      Implementation details follow
// --------------------------------------------------------------------------
inline bool operator==(const uint128& lhs, const uint128& rhs) {
  return (Uint128Low64(lhs) == Uint128Low64(rhs) &&
          Uint128High64(lhs) == Uint128High64(rhs));
}
inline bool operator!=(const uint128& lhs, const uint128& rhs) {
  return !(lhs == rhs);
}

inline constexpr uint128::uint128(uint64 top, uint64 bottom)
    : lo_(bottom), hi_(top) {}
inline constexpr uint128::uint128(uint64 bottom)
    : lo_(bottom), hi_(0) {}
#ifndef SWIG
inline constexpr uint128::uint128(uint32 bottom)
    : lo_(bottom), hi_(0) {}
inline constexpr uint128::uint128(int bottom)
    : lo_(bottom), hi_(static_cast<int64>((bottom < 0) ? -1 : 0)) {}
#endif

// Comparison operators.

#define CMP128(op)                                                \
inline bool operator op(const uint128& lhs, const uint128& rhs) { \
  return (Uint128High64(lhs) == Uint128High64(rhs)) ?             \
      (Uint128Low64(lhs) op Uint128Low64(rhs)) :                  \
      (Uint128High64(lhs) op Uint128High64(rhs));                 \
}

CMP128(<)
CMP128(>)
CMP128(>=)
CMP128(<=)

#undef CMP128

// Unary operators

inline uint128 operator-(const uint128& val) {
  const uint64 hi_flip = ~Uint128High64(val);
  const uint64 lo_flip = ~Uint128Low64(val);
  const uint64 lo_add = lo_flip + 1;
  if (lo_add < lo_flip) {
    return MakeUint128(hi_flip + 1, lo_add);
  }
  return MakeUint128(hi_flip, lo_add);
}

inline bool operator!(const uint128& val) {
  return !Uint128High64(val) && !Uint128Low64(val);
}

// Logical operators.

inline uint128 operator~(const uint128& val) {
  return MakeUint128(~Uint128High64(val), ~Uint128Low64(val));
}

#define LOGIC128(op)                                                 \
inline uint128 operator op(const uint128& lhs, const uint128& rhs) { \
  return MakeUint128(Uint128High64(lhs) op Uint128High64(rhs),       \
                     Uint128Low64(lhs) op Uint128Low64(rhs));        \
}

LOGIC128(|)
LOGIC128(&)
LOGIC128(^)

#undef LOGIC128

#define LOGICASSIGN128(op)                                   \
inline uint128& uint128::operator op(const uint128& other) { \
  hi_ op other.hi_;                                          \
  lo_ op other.lo_;                                          \
  return *this;                                              \
}

LOGICASSIGN128(|=)
LOGICASSIGN128(&=)
LOGICASSIGN128(^=)

#undef LOGICASSIGN128

// Shift operators.

void VerifyValidShift(std::string op, int amount);

inline uint128 operator<<(const uint128& val, int amount) {
  VerifyValidShift("<<", amount);

  // uint64 shifts of >= 64 are undefined, so we will need some special-casing.
  if (amount < 64) {
    if (amount == 0) {
      return val;
    }
    uint64 new_hi = (Uint128High64(val) << amount) |
                    (Uint128Low64(val) >> (64 - amount));
    uint64 new_lo = Uint128Low64(val) << amount;
    return MakeUint128(new_hi, new_lo);
  }
  return MakeUint128(Uint128Low64(val) << (amount - 64), 0);
}

inline uint128 operator>>(const uint128& val, int amount) {
  VerifyValidShift(">>", amount);

  // uint64 shifts of >= 64 are undefined, so we will need some special-casing.
  if (amount < 64) {
    if (amount == 0) {
      return val;
    }
    uint64 new_hi = Uint128High64(val) >> amount;
    uint64 new_lo = (Uint128Low64(val) >> amount) |
                    (Uint128High64(val) << (64 - amount));
    return MakeUint128(new_hi, new_lo);
  }

  return MakeUint128(0, Uint128High64(val) >> (amount - 64));
}

inline uint128& uint128::operator<<=(int amount) {
  // uint64 shifts of >= 64 are undefined, so we will need some special-casing.
  if (amount < 64) {
    if (amount != 0) {
      hi_ = (hi_ << amount) | (lo_ >> (64 - amount));
      lo_ = lo_ << amount;
    }
  } else if (amount < 128) {
    hi_ = lo_ << (amount - 64);
    lo_ = 0;
  } else {
    hi_ = 0;
    lo_ = 0;
  }
  return *this;
}

inline uint128& uint128::operator>>=(int amount) {
  // uint64 shifts of >= 64 are undefined, so we will need some special-casing.
  if (amount < 64) {
    if (amount != 0) {
      lo_ = (lo_ >> amount) | (hi_ << (64 - amount));
      hi_ = hi_ >> amount;
    }
  } else if (amount < 128) {
    lo_ = hi_ >> (amount - 64);
    hi_ = 0;
  } else {
    lo_ = 0;
    hi_ = 0;
  }
  return *this;
}

inline uint128 operator+(const uint128& lhs, const uint128& rhs) {
  return uint128(lhs) += rhs;
}

inline uint128 operator-(const uint128& lhs, const uint128& rhs) {
  return uint128(lhs) -= rhs;
}

inline uint128 operator*(const uint128& lhs, const uint128& rhs) {
  return uint128(lhs) *= rhs;
}

inline uint128 operator/(const uint128& lhs, const uint128& rhs) {
  return uint128(lhs) /= rhs;
}

inline uint128 operator%(const uint128& lhs, const uint128& rhs) {
  return uint128(lhs) %= rhs;
}

inline uint128& uint128::operator+=(const uint128& b) {
  hi_ += b.hi_;
  uint64 lolo = lo_ + b.lo_;
  if (lolo < lo_)
    ++hi_;
  lo_ = lolo;
  return *this;
}

inline uint128& uint128::operator-=(const uint128& b) {
  hi_ -= b.hi_;
  if (b.lo_ > lo_)
    --hi_;
  lo_ -= b.lo_;
  return *this;
}

inline uint128& uint128::operator*=(const uint128& b) {
  uint64 a96 = hi_ >> 32;
  uint64 a64 = hi_ & 0xffffffffu;
  uint64 a32 = lo_ >> 32;
  uint64 a00 = lo_ & 0xffffffffu;
  uint64 b96 = b.hi_ >> 32;
  uint64 b64 = b.hi_ & 0xffffffffu;
  uint64 b32 = b.lo_ >> 32;
  uint64 b00 = b.lo_ & 0xffffffffu;
  // multiply [a96 .. a00] x [b96 .. b00]
  // terms higher than c96 disappear off the high side
  // terms c96 and c64 are safe to ignore carry bit
  uint64 c96 = a96 * b00 + a64 * b32 + a32 * b64 + a00 * b96;
  uint64 c64 = a64 * b00 + a32 * b32 + a00 * b64;
  this->hi_ = (c96 << 32) + c64;
  this->lo_ = 0;
  // add terms after this one at a time to capture carry
  *this += uint128(a32 * b00) << 32;
  *this += uint128(a00 * b32) << 32;
  *this += a00 * b00;
  return *this;
}

inline uint128 uint128::operator++(int) {
  uint128 tmp(*this);
  *this += 1;
  return tmp;
}

inline uint128 uint128::operator--(int) {
  uint128 tmp(*this);
  *this -= 1;
  return tmp;
}

inline uint128& uint128::operator++() {
  *this += 1;
  return *this;
}

inline uint128& uint128::operator--() {
  *this -= 1;
  return *this;
}

constexpr uint128 Uint128Max() {
  return MakeUint128((std::numeric_limits<uint64>::max)(),
                     (std::numeric_limits<uint64>::max)());
}

}  // namespace int128_internal

using google::protobuf::int128_internal::uint128;
using google::protobuf::int128_internal::Uint128Max;
using google::protobuf::int128_internal::MakeUint128;

}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_STUBS_INT128_H_
