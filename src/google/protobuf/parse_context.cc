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

#include <google/protobuf/parse_context.h>

#if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER

#include <google/protobuf/stubs/stringprintf.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/string_piece_field_support.h>
#include <google/protobuf/wire_format_lite.h>
#include "third_party/absl/strings/str_format.h"
#include <google/protobuf/stubs/strutil.h>
#include "util/coding/varint.h"
#include "util/utf8/public/unilib.h"

#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace internal {

bool ParseContext::ParseEndsInSlopRegion(const char* begin, int overrun) const {
  ABSL_ASSERT(overrun >= 0);
  ABSL_ASSERT(overrun <= kSlopBytes);
  auto ptr = begin + overrun;
  auto end = begin + kSlopBytes;
  int n = end - ptr;
  if (n == 0) return false;
  // If limit_ != -1 then the parser will continue to parse at least limit_
  // bytes (or more if on the stack there are further limits)
  int d = depth_;
  if (limit_ != -1) {
    ABSL_ASSERT(d < start_depth_);  // Top-level never has a limit.
    // rewind the stack until all limits disappear.
    int limit = limit_;
    if (limit >= n) return false;
    while (d < start_depth_) {
      int delta = stack_[d++].delta_or_group_num;
      if (delta == -1) {
        // We found the first limit that was pushed. We should inspect from
        // this point on.
        ptr += limit;
        break;
      } else if (delta >= 0) {
        // We reached end of a length delimited subfield. Adjust limit
        limit += delta;
        if (limit >= n) return false;
      } else {
        // It's a group we assume the format is correct and this group
        // is properly ended before the limit is reached.
      }
    }
  }
  d = start_depth_ - d;  // We just keep track of the depth from start.
  // We verify that a generic parse of the buffer won't, validly, end the parse
  // before end, due to ending the top-level on a 0 or end-group tag.
  // IMPORTANT NOTE: we return false in failure cases. This is
  // important because we could fail due to overrunning the buffer and read
  // garbage data beyond the buffer (valid reads just left over garbage). So
  // failure doesn't imply the parse will fail. So if this loops fails while
  // the real parse would succeed it means the real parse will read beyond the
  // boundary. If the real parse fails we can't reasonably continue the stream
  // any way so we make no attempt to leave the stream at a well specified pos.
  while (ptr < end) {
    uint32 tag;
    ptr = Varint::Parse32(ptr, &tag);
    if (ptr == nullptr || ptr > end) return false;
    // ending on 0 tag is allowed and is the major reason for the necessity of
    // this function.
    if (tag == 0) return true;
    switch (tag & 7) {
      case 0: {  // Varint
        uint64 val;
        ptr = Varint::Parse64(ptr, &val);
        if (ptr == nullptr) return false;
        break;
      }
      case 1: {  // fixed64
        ptr += 8;
        break;
      }
      case 2: {  // len delim
        uint32 size;
        ptr = Varint::Parse32(ptr, &size);
        if (ptr == nullptr) return false;
        ptr += size;
        break;
      }
      case 3: {  // start group
        d++;
        break;
      }
      case 4: {                     // end group
        if (--d < 0) return true;   // We exit early
        break;
      }
      case 5: {  // fixed32
        ptr += 4;
        break;
      }
      default:
        return false;  // Unknown wireformat
    }
  }
  return false;
}

void ParseContext::SwitchStack() {
  stack_ = new State[start_depth_];
  std::memcpy(stack_ + inlined_depth_, inline_stack_, sizeof(inline_stack_));
  inlined_depth_ = -1;  // Special value to indicate stack_ needs to be deleted
}

std::pair<bool, int> ParseContext::ParseRangeWithLimit(const char* begin,
                                                       const char* end) {
  auto ptr = begin;
  do {
    ABSL_ASSERT(ptr < end);
    const char* limited_end;
    if (limit_ == -1) {
      limited_end = end;
    } else {
      ABSL_ASSERT(limit_ > 0);
      limited_end = ptr + std::min(static_cast<int32>(end - ptr), limit_);
      limit_ -= limited_end - ptr;
    }
    // Parse the range [ptr, limited_end). The only case (except for error) that
    // the parser can return prematurely (before limited_end) is on encountering
    // an end-group. If this is the case we continue parsing the range with
    // the parent parser.
    do {
      ABSL_ASSERT(ptr < limited_end);
      ptr = parser_(ptr, limited_end, this);
      if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
        // Clear last_tag_minus_1_ so that the hard error encountered is not
        // mistaken for ending on a tag.
        last_tag_minus_1_ = 0;
        return {};
      }
      if (!EndedOnTag()) {
        // The parser ended still parsing the initial message. This can only
        // happen because it crossed the end.
        ABSL_ASSERT(ptr >= limited_end);
        break;
      }
      // Child parser terminated on an end-group / 0 tag.
      ABSL_ASSERT(depth_ <= start_depth_);
      if (depth_ == start_depth_) {
        // The parse was already at the top-level and there is no parent.
        // This can happen due to encountering 0 or due to this parser being
        // called for parsing a sub-group message in custom parsing code.
        return {false, ptr - end};
      }
      auto state = Pop();
      // Verify the ending tag is correct and continue parsing the range with
      // the parent parser.
      uint32 group_number = last_tag_minus_1_ >> 3;
      // We need to clear last_tag_minus_1_, either for the next iteration
      // or if the if statement below returns.
      last_tag_minus_1_ = 0;
      if (state.delta_or_group_num != ~group_number) return {};
      parser_ = state.parser;  // Load parent parser
    } while (ptr < limited_end);
    int overrun = ptr - limited_end;
    ABSL_ASSERT(overrun >= 0);
    ABSL_ASSERT(overrun <= kSlopBytes);  // wireformat guarantees this limit
    if (limit_ != -1) {
      limit_ -= overrun;  // Adjust limit relative to new position.
      if (limit_ < 0) return {};  // We overrun the limit
      while (limit_ == 0) {
        // We are at an actual ending of a length delimited field.
        // The top level has no limit (ie. limit_ == -1) so we can assert
        // that the stack is non-empty.
        ABSL_ASSERT(depth_ < start_depth_);
        // else continue parsing the parent message.
        auto state = Pop();
        parser_ = state.parser;
        limit_ = state.delta_or_group_num;
        // No group ending is possible here. Any group on the stack still
        // needs to read its end-group tag and can't be on a limit_ == 0.
        if (limit_ < -1) return {};
      }
    }
  } while (ptr < end);
  return {true, ptr - end};
}

const char* StringParser(const char* begin, const char* end, void* object,
                         ParseContext*) {
  auto str = static_cast<string*>(object);
  str->append(begin, end - begin);
  return end;
}

const char* CordParser(const char* begin, const char* end, void* object,
                       ParseContext* ctx) {
  auto cord = static_cast<Cord*>(object);
  cord->Append(StringPiece(begin, end - begin));
  return end;
}

void StringPieceField::Append(const char *begin, size_t chunk_size, int limit) {
  if (size_ == 0) {
    auto tot = chunk_size + limit;
    if (tot > scratch_size_) {
      auto old_scratch_size = scratch_size_;
      scratch_size_ = tot;
      // TODO(gerbens) Security against big
      if (arena_ != NULL) {
        scratch_ = ::google::protobuf::Arena::CreateArray<char>(arena_, scratch_size_);
      } else {
        std::allocator<char>().deallocate(scratch_, old_scratch_size);
        scratch_ = std::allocator<char>().allocate(scratch_size_);
      }
    }
    data_ = scratch_;
  }
  std::memcpy(scratch_ + size_, begin, chunk_size);
  size_ += chunk_size;
}

const char* StringPieceParser(const char* begin, const char* end, void* object,
                              ParseContext* ctx) {
  auto s = static_cast<StringPieceField*>(object);
  auto limit = ctx->CurrentLimit();
  s->Append(begin, end - begin, limit);
  return end;
}

// Defined in wire_format_lite.cc
void PrintUTF8ErrorLog(const char* field_name, const char* operation_str,
                       bool emit_stacktrace);

bool VerifyUTF8(StringPiece str, ParseContext* ctx) {
  if (!IsStructurallyValidUTF8(str)) {
    PrintUTF8ErrorLog(ctx->extra_parse_data().FieldName(), "parsing", false);
    return false;
  }
  return true;
}

bool VerifyUTF8Cord(const Cord& value, ParseContext* ctx) {
  if (!UniLib::CordIsStructurallyValid(value)) {
    PrintUTF8ErrorLog(ctx->extra_parse_data().FieldName(), "parsing", false);
    return false;
  }
  return true;
}

const char* StringParserUTF8(const char* begin, const char* end, void* object,
                             ParseContext* ctx) {
  StringParser(begin, end, object, ctx);
  if (ctx->AtLimit()) {
    auto str = static_cast<string*>(object);
    GOOGLE_PROTOBUF_PARSER_ASSERT(VerifyUTF8(*str, ctx));
  }
  return end;
}

const char* CordParserUTF8(const char* begin, const char* end, void* object,
                           ParseContext* ctx) {
  CordParser(begin, end, object, ctx);
  if (ctx->AtLimit()) {
    auto str = static_cast<Cord*>(object);
    GOOGLE_PROTOBUF_PARSER_ASSERT(VerifyUTF8Cord(*str, ctx));
  }
  return end;
}

const char* StringPieceParserUTF8(const char* begin, const char* end,
                                  void* object, ParseContext* ctx) {
  StringPieceParser(begin, end, object, ctx);
  if (ctx->AtLimit()) {
    auto s = static_cast<StringPieceField*>(object);
    GOOGLE_PROTOBUF_PARSER_ASSERT(VerifyUTF8(s->Get(), ctx));
  }
  return end;
}

const char* StringParserUTF8Verify(const char* begin, const char* end,
                                   void* object, ParseContext* ctx) {
  StringParser(begin, end, object, ctx);
#ifndef NDEBUG
  if (ctx->AtLimit()) {
    auto str = static_cast<string*>(object);
    VerifyUTF8(*str, ctx);
  }
#endif
  return end;
}

const char* CordParserUTF8Verify(const char* begin, const char* end,
                                 void* object, ParseContext* ctx) {
  CordParser(begin, end, object, ctx);
#ifndef NDEBUG
  if (ctx->AtLimit()) {
    auto str = static_cast<Cord*>(object);
    VerifyUTF8Cord(*str, ctx);
  }
#endif
  return end;
}

const char* StringPieceParserUTF8Verify(const char* begin, const char* end,
                                        void* object, ParseContext* ctx) {
  return StringPieceParser(begin, end, object, ctx);
#ifndef NDEBUG
  if (ctx->AtLimit()) {
    auto s = static_cast<StringPieceField*>(object);
    VerifyUTF8(s->Get(), ctx);
  }
#endif
  return end;
}

const char* GreedyStringParser(const char* begin, const char* end, void* object,
                         ParseContext* ctx) {
  auto str = static_cast<string*>(object);
  auto limit = ctx->CurrentLimit();
  ABSL_ASSERT(limit != -1);  // Always length delimited
  end += std::min<int>(limit, ParseContext::kSlopBytes);
  str->append(begin, end - begin);
  return end;
}

const char* GreedyStringParserUTF8(const char* begin, const char* end, void* object,
                             ParseContext* ctx) {
  auto limit = ctx->CurrentLimit();
  ABSL_ASSERT(limit != -1);  // Always length delimited
  bool at_end;
  if (limit <= ParseContext::kSlopBytes) {
    end += limit;
    at_end = true;
  } else {
    end += ParseContext::kSlopBytes;
    at_end =false;
  }
  auto str = static_cast<string*>(object);
  str->append(begin, end - begin);
  if (at_end) {
    GOOGLE_PROTOBUF_PARSER_ASSERT(VerifyUTF8(*str, ctx));
  }
  return end;
}

const char* GreedyStringParserUTF8Verify(const char* begin, const char* end, void* object,
                             ParseContext* ctx) {
  auto limit = ctx->CurrentLimit();
  ABSL_ASSERT(limit != -1);  // Always length delimited
  bool at_end;
  if (limit <= ParseContext::kSlopBytes) {
    end += limit;
    at_end = true;
  } else {
    end += ParseContext::kSlopBytes;
    at_end =false;
  }
  auto str = static_cast<string*>(object);
  str->append(begin, end - begin);
  if (at_end) {
#ifndef NDEBUG
    VerifyUTF8(*str, ctx);
#endif
  }
  return end;
}

template <typename T, bool sign>
const char* VarintParser(const char* begin, const char* end, void* object,
                         ParseContext*) {
  auto repeated_field = static_cast<RepeatedField<T>*>(object);
  auto ptr = begin;
  while (ptr < end) {
    uint64 varint;
    ptr = Varint::Parse64(ptr, &varint);
    if (!ptr) return nullptr;
    T val;
    if (sign) {
      if (sizeof(T) == 8) {
        val = WireFormatLite::ZigZagDecode64(varint);
      } else {
        val = WireFormatLite::ZigZagDecode32(varint);
      }
    } else {
      val = varint;
    }
    repeated_field->Add(val);
  }
  return ptr;
}

template <typename T>
const char* FixedParser(const char* begin, const char* end, void* object,
                        ParseContext*) {
  auto repeated_field = static_cast<RepeatedField<T>*>(object);
  int num = (end - begin + sizeof(T) - 1) / sizeof(T);

  const int old_entries = repeated_field->size();
  repeated_field->Reserve(old_entries + num);
  std::memcpy(repeated_field->AddNAlreadyReserved(num), begin, num * sizeof(T));
  return begin + num * sizeof(T);
}

const char* PackedInt32Parser(const char* begin, const char* end, void* object,
                              ParseContext* ctx) {
  return VarintParser<int32, false>(begin, end, object, ctx);
}
const char* PackedUInt32Parser(const char* begin, const char* end, void* object,
                               ParseContext* ctx) {
  return VarintParser<uint32, false>(begin, end, object, ctx);
}
const char* PackedInt64Parser(const char* begin, const char* end, void* object,
                              ParseContext* ctx) {
  return VarintParser<int64, false>(begin, end, object, ctx);
}
const char* PackedUInt64Parser(const char* begin, const char* end, void* object,
                               ParseContext* ctx) {
  return VarintParser<uint64, false>(begin, end, object, ctx);
}
const char* PackedSInt32Parser(const char* begin, const char* end, void* object,
                               ParseContext* ctx) {
  return VarintParser<int32, true>(begin, end, object, ctx);
}
const char* PackedSInt64Parser(const char* begin, const char* end, void* object,
                               ParseContext* ctx) {
  return VarintParser<int64, true>(begin, end, object, ctx);
}

const char* PackedEnumParser(const char* begin, const char* end, void* object,
                             ParseContext* ctx) {
  return VarintParser<int, false>(begin, end, object, ctx);
}

const char* PackedValidEnumParserLite(const char* begin, const char* end,
                                      void* object, ParseContext* ctx) {
  auto repeated_field = static_cast<RepeatedField<int>*>(object);
  auto ptr = begin;
  while (ptr < end) {
    uint64 varint;
    ptr = Varint::Parse64(ptr, &varint);
    if (!ptr) return nullptr;
    int val = varint;
    if (ctx->extra_parse_data().ValidateEnum<string>(val))
      repeated_field->Add(val);
  }
  return ptr;
}

const char* PackedValidEnumParserLiteArg(const char* begin, const char* end,
                                         void* object, ParseContext* ctx) {
  auto repeated_field = static_cast<RepeatedField<int>*>(object);
  auto ptr = begin;
  while (ptr < end) {
    uint64 varint;
    ptr = Varint::Parse64(ptr, &varint);
    if (!ptr) return nullptr;
    int val = varint;
    if (ctx->extra_parse_data().ValidateEnumArg<string>(val))
      repeated_field->Add(val);
  }
  return ptr;
}

const char* PackedBoolParser(const char* begin, const char* end, void* object,
                             ParseContext* ctx) {
  return VarintParser<bool, false>(begin, end, object, ctx);
}

const char* PackedFixed32Parser(const char* begin, const char* end,
                                void* object, ParseContext* ctx) {
  return FixedParser<uint32>(begin, end, object, ctx);
}
const char* PackedSFixed32Parser(const char* begin, const char* end,
                                 void* object, ParseContext* ctx) {
  return FixedParser<int32>(begin, end, object, ctx);
}
const char* PackedFixed64Parser(const char* begin, const char* end,
                                void* object, ParseContext* ctx) {
  return FixedParser<uint64>(begin, end, object, ctx);
}
const char* PackedSFixed64Parser(const char* begin, const char* end,
                                 void* object, ParseContext* ctx) {
  return FixedParser<int64>(begin, end, object, ctx);
}
const char* PackedFloatParser(const char* begin, const char* end, void* object,
                              ParseContext* ctx) {
  return FixedParser<float>(begin, end, object, ctx);
}
const char* PackedDoubleParser(const char* begin, const char* end, void* object,
                               ParseContext* ctx) {
  return FixedParser<double>(begin, end, object, ctx);
}

const char* NullParser(const char* begin, const char* end, void* object,
                       ParseContext* ctx) {
  return end;
}

void WriteVarint(uint64 val, string* s) {
  while (val >= 128) {
    uint8 c = val | 0x80;
    s->push_back(c);
    val >>= 7;
  }
  s->push_back(val);
}

void WriteVarint(uint32 num, uint64 val, string* s) {
  WriteVarint(num << 3, s);
  WriteVarint(val, s);
}

void WriteLengthDelimited(uint32 num, StringPiece val, string* s) {
  WriteVarint((num << 3) + 2, s);
  WriteVarint(val.size(), s);
  s->append(val.data(), val.size());
}

class UnknownFieldLiteParserHelper {
 public:
  explicit UnknownFieldLiteParserHelper(string* unknown) : unknown_(unknown) {}

  void AddVarint(uint32 num, uint64 value) {
    if (unknown_ == nullptr) return;
    WriteVarint(num * 8, unknown_);
    WriteVarint(value, unknown_);
  }
  void AddFixed64(uint32 num, uint64 value) {
    if (unknown_ == nullptr) return;
    WriteVarint(num * 8 + 1, unknown_);
    char buffer[8];
    std::memcpy(buffer, &value, 8);
    unknown_->append(buffer, 8);
  }
  ParseClosure AddLengthDelimited(uint32 num, uint32 size) {
    if (unknown_ == nullptr) return {NullParser, nullptr};
    WriteVarint(num * 8 + 2, unknown_);
    WriteVarint(size, unknown_);
    return {StringParser, unknown_};
  }
  ParseClosure StartGroup(uint32 num) {
    if (unknown_ == nullptr) return {UnknownGroupLiteParse, nullptr};
    WriteVarint(num * 8 + 3, unknown_);
    return {UnknownGroupLiteParse, unknown_};
  }
  void EndGroup(uint32 num) {
    if (unknown_ == nullptr) return;
    WriteVarint(num * 8 + 4, unknown_);
  }
  void AddFixed32(uint32 num, uint32 value) {
    if (unknown_ == nullptr) return;
    WriteVarint(num * 8 + 5, unknown_);
    char buffer[4];
    std::memcpy(buffer, &value, 4);
    unknown_->append(buffer, 4);
  }

 private:
  string* unknown_;
};

const char* UnknownGroupLiteParse(const char* begin, const char* end,
                                  void* object, ParseContext* ctx) {
  UnknownFieldLiteParserHelper field_parser(static_cast<string*>(object));
  return WireFormatParser({UnknownGroupLiteParse, object}, field_parser, begin,
                          end, ctx);
}

std::pair<const char*, bool> UnknownFieldParse(uint32 tag, ParseClosure parent,
                                               const char* begin,
                                               const char* end, string* unknown,
                                               ParseContext* ctx) {
  UnknownFieldLiteParserHelper field_parser(unknown);
  return FieldParser(tag, parent, field_parser, begin, end, ctx);
}

const char* SlowMapEntryParser(const char* begin, const char* end, void* object,
                               internal::ParseContext* ctx) {
  ctx->extra_parse_data().payload.append(begin, end - begin);
  if (ctx->AtLimit()) {
    // Move payload out of extra_parse_data. Parsing maps could trigger
    // payload on recursive maps.
    string to_parse = std::move(ctx->extra_parse_data().payload);
    StringPiece chunk = to_parse;
    if (!ctx->extra_parse_data().parse_map(chunk.begin(), chunk.end(), object,
                                           ctx)) {
      return nullptr;
    }
  }
  return end;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
