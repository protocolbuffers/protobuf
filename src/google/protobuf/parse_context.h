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

#ifndef GOOGLE_PROTOBUF_PARSE_CONTEXT_H__
#define GOOGLE_PROTOBUF_PARSE_CONTEXT_H__

#include <cstring>
#include <string>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/port.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {

class UnknownFieldSet;
class DescriptorPool;
class MessageFactory;

namespace internal {

// Template code below needs to know about the existence of these functions.
PROTOBUF_EXPORT void WriteVarint(uint32 num, uint64 val, std::string* s);
PROTOBUF_EXPORT
void WriteLengthDelimited(uint32 num, StringPiece val, std::string* s);
// Inline because it is just forwarding to s->WriteVarint
inline void WriteVarint(uint32 num, uint64 val, UnknownFieldSet* s);
inline void WriteLengthDelimited(uint32 num, StringPiece val,
                                 UnknownFieldSet* s);


// ParseContext contains state that needs to be preserved across buffer seams.

class ParseContext;

// The parser works by composing elementary parse functions, that are generated
// by the compiler, together to perform the full parse. To accomplish this the
// functionality of the elementary parse function is slightly increased which
// allows it to become composable.

// The basic abstraction ParseContext is designed for is a slight modification
// of the ZeroCopyInputStream (ZCIS) abstraction. A ZCIS presents a serialized
// stream as a series of buffers that concatenate to the full stream.
// Pictorially a ZCIS presents a stream in chunks like so
// [---------------------------------------------------------------]
// [---------------------] chunk 1
//                      [----------------------------] chunk 2
//                                          chunk 3 [--------------]
//
// Where the '-' represent the bytes which are vertically lined up with the
// bytes of the stream.
// ParseContext requires its input to be presented similarily with the extra
// property that the last kSlopBytes of a chunk overlaps with the first
// kSlopBytes of the next chunk, or if there is no next chunk at least its still
// valid to read those bytes. Again, pictorially, we now have
//
// [---------------------------------------------------------------]
// [-------------------....] chunk 1
//                    [------------------------....] chunk 2
//                                    chunk 3 [------------------..**]
//                                                      chunk 4 [--****]
// Here '-' mean the bytes of the stream or chunk and '.' means bytes past the
// chunk that match up with the start of the next chunk. Above each chunk has
// 4 '.' after the chunk. In the case these 'overflow' bytes represents bytes
// past the stream, indicated by '*' above, their values are unspecified. It is
// still legal to read them (ie. should not segfault). Reading past the
// end should be detected by the user and indicated as an error.
//
// The reason for this, admittedly, unconventional invariant is to ruthlessly
// optimize the protobuf parser. Having an overlap helps in two important ways.
// Firstly it alleviates having to performing bounds checks, if a piece of code
// will never read more than kSlopBytes. Secondly, and more importantly, the
// protobuf wireformat is such that there is always a fresh start of a tag
// within kSlopBytes. This allows the parser to exit parsing a chunk leaving
// the parse on a position inside the overlap where a fresh tag starts.

// The elementary parse function has the following signature

typedef const char* (*ParseFunc)(const char* ptr, const char* end, void* object,
                                 ParseContext* ctx);

// which parses the serialized data stored in the range [ptr, end) into object.
// A parse function together with its object forms a callable closure.
struct ParseClosure {
  ParseFunc func;
  void* object;

  // Pre-conditions
  //   ptr < end is a non-empty range where ptr points to the start of a tag
  //     and it's okay to read the bytes in [end, end + kSlopBytes).
  //     Which will contain the bytes of the next chunk if the stream continues,
  //     or undefined in which case the parse will be guaranteed to fail.
  //
  // Post-conditions
  //   Parsed all tag/value pairs starting before end or if a group end
  //   tag is encountered returns the pointer to that tag.
  //   If a group end is encountered it verifies it matches the one that was
  //   pushed and the stack is popped.
  //   Otherwise it will parses the entire range pushing if end is inside one
  //   of the children those are pushed on the stack.
  //
  //   If an element is popped from the stack it ended on the correct end group
  //   returns pointer after end-group tag (posibly in overlap, but the start
  //   of end-group tag will be before end).
  //   If the stack is the same or deeper, returns pointer in overlap region
  //   (end <= retval < end + kSlopBytes).
  //   All tag/value pairs between in [begin, retval) are parsed and retval
  //   points to start of a tag.
  PROTOBUF_ALWAYS_INLINE  // Don't pay for extra stack frame in debug mode
      const char*
      operator()(const char* ptr, const char* end, ParseContext* ctx) {
    GOOGLE_DCHECK(ptr < end);
    return func(ptr, end, object, ctx);
  }
};

// To fully parse a stream, a driver loop repeatedly calls the parse function
// at the top of the stack, popping and resume parsing the parent message
// according to the recursive structure of the wireformat. This loop will also
// need to provide new buffer chunks and align the ptr correctly over the seams.
// The point of this framework is that chunk refresh logic is located in the
// outer loop, while the inner loop is almost free of it. The two code paths in
// the parse code dealing with seams are located in fallback paths whose checks
// are folded with input limit checks that are necessary anyway. In other words,
// all the parser code that deals with seams is located in what would otherwise
// be error paths of a parser that wouldn't need to deal with seams.

class PROTOBUF_EXPORT ParseContext {
 public:
  enum {
    // Tag is atmost 5 bytes, varint is atmost 10 resulting in 15 bytes. We
    // choose
    // 16 bytes for the obvious reason of alignment.
    kSlopBytes = 16,
    // Inlined stack size
    kInlinedDepth = 15,
  };

  // Arghh!!! here be tech-debt dragons
  struct ExtraParseData {
    const DescriptorPool* pool = nullptr;
    MessageFactory* factory = nullptr;

    // payload is used for MessageSetItem and maps
    std::string payload;
    bool (*parse_map)(const char* begin, const char* end, void* map_field,
                      ParseContext* ctx);

    void SetEnumValidator(bool (*validator)(int), void* unknown,
                          int field_num) {
      enum_validator = validator;
      unknown_fields = unknown;
      field_number = field_num;
    }
    void SetEnumValidatorArg(bool (*validator)(const void*, int),
                             const void* arg, void* unknown, int field_num) {
      arg_enum_validator = {validator, arg};
      unknown_fields = unknown;
      field_number = field_num;
    }
    template <typename Unknown>
    bool ValidateEnum(int val) const {
      if (enum_validator(val)) return true;
      WriteVarint(field_number, val, static_cast<Unknown*>(unknown_fields));
      return false;
    }
    template <typename Unknown>
    bool ValidateEnumArg(int val) const {
      if (arg_enum_validator(val)) return true;
      WriteVarint(field_number, val, static_cast<Unknown*>(unknown_fields));
      return false;
    }

    void SetFieldName(const void* name) {
      unknown_fields = const_cast<void*>(name);
    }
    const char* FieldName() const {
      return static_cast<const char*>(unknown_fields);
    }

    union {
      bool (*enum_validator)(int);
      struct {
        bool operator()(int val) const { return validator(arg, val); }
        bool (*validator)(const void*, int);
        const void* arg;
      } arg_enum_validator;
    };
    void* unknown_fields;
    int field_number;
    // 0 means no aliasing. If not zero aliasing is the delta between the
    // ptr and the buffer that needs to be aliased. If the value is
    // kNoDelta (1) this means delta is actually 0 (we're working directly in
    // the buffer).
    enum { kNoDelta = 1 };
    std::uintptr_t aliasing = 0;
  };

  ExtraParseData& extra_parse_data() { return extra_parse_data_; }
  const ExtraParseData& extra_parse_data() const { return extra_parse_data_; }

  // Helpers to detect if a parse of length delimited field is completed.
  bool AtLimit() const { return limit_ == 0; }
  int32 CurrentLimit() const { return limit_; }

  // Initializes ParseContext with a specific recursion limit (rec_limit)
  explicit ParseContext(int rec_limit)
      : depth_(rec_limit),
        start_depth_(rec_limit),
        stack_(inline_stack_ + kInlinedDepth - rec_limit),
        inlined_depth_(std::max(0, rec_limit - kInlinedDepth)) {}

  ~ParseContext() {
    if (inlined_depth_ == -1) delete[] stack_;
  }

  void StartParse(ParseClosure parser) { parser_ = parser; }

  // Parses a chunk of memory given the current state of parse context (ie.
  // the active parser and stack) and overrun.
  // Pre-condition:
  //   chunk_ is not empty.
  //   limit_ > 0 (limit from begin) or -1 (no limit)
  // Post-condition:
  //   returns true on success with overrun_ptr adjusted to the new value, or
  //   false is the parse is finished. False means either a parse failure or
  //   or because the top-level was terminated on a 0 or end-group tag in which
  //   case overrun points to the position after the ending tag. You can call
  //   EndedOnTag() to find if the parse failed due to an error or ended on
  //   terminating tag.
  bool ParseRange(StringPiece chunk, int* overrun_ptr) {
    GOOGLE_DCHECK(!chunk.empty());
    int& overrun = *overrun_ptr;
    GOOGLE_DCHECK(overrun >= 0);
    if (overrun >= static_cast<int>(chunk.size())) {
      // This case can easily happen in patch buffers and we like to inline
      // this case.
      overrun -= chunk.size();
      return true;
    }
    auto res = ParseRangeWithLimit(chunk.begin() + overrun, chunk.end());
    overrun = res.second;
    return res.first;
  }

  bool ValidEnd(int overrun) { return depth_ == start_depth_ && overrun == 0; }
  bool EndedOnTag() const { return last_tag_minus_1_ != 0; }
  uint32 LastTag() const { return last_tag_minus_1_ + 1; }

  // Generically verifies for the slop region [begin, begin + kSlopBytes) if
  // the parse will be terminated by 0 or end-group tag. If true than you can
  // safely parse the slop region without having to load more data.
  bool ParseEndsInSlopRegion(const char* begin, int overrun) const;

  // Should only be called by Parse code.

  //////////////////////////////////////////////////////////////////////////////
  // Fast path helpers. These helpers maintain the state in parse context
  // through recursive calls. The whole design is to make this as minimal as
  // possible. Only recursion depth and limit are maintained at every recursion.
  //////////////////////////////////////////////////////////////////////////////

  bool ParseExactRange(ParseClosure parser, const char* begin,
                       const char* end) {
    if (PROTOBUF_PREDICT_FALSE(--depth_ < 0)) return false;
    auto old_limit = limit_;
    limit_ = 0;
    auto ptr = begin;
    if (ptr < end) ptr = parser(ptr, end, this);
    if (ptr != end || EndedOnTag()) return false;
    limit_ = old_limit;
    ++depth_;
    return true;
  }

  // Returns a pair of the pointer the parse is left and a boolean indicating
  // if the group is still continuing.
  std::pair<const char*, bool> ParseGroup(uint32 tag, ParseClosure parser,
                                          const char* begin, const char* end,
                                          int* depth) {
    if (PROTOBUF_PREDICT_FALSE(--depth_ < 0)) return {};
    *depth = depth_;
    auto ptr = begin;
    if (ptr < end) ptr = parser(ptr, end, this);
    if (ptr == nullptr) return {};
    if (!EndedOnTag()) {
      // The group hasn't been terminated by an end-group and thus continues,
      // hence it must have ended because it crossed "end".
      GOOGLE_DCHECK(ptr >= end);
      return {ptr, true};
    }
    // Verify that the terminating tag matches the start group tag. As an extra
    // subtlety it could have been terminated by an end-group tag but in a
    // length delimited sub field of the group. So we must also check that depth
    // matches, if it doesn't match it means a length delimited subfield got
    // terminated by an end group which is an error.
    if (tag != last_tag_minus_1_ || *depth != depth_) return {};
    last_tag_minus_1_ = 0;  // It must always be cleared.
    ++depth_;
    return {ptr, false};
  }

  void EndGroup(uint32 tag) {
    GOOGLE_DCHECK(tag == 0 || (tag & 7) == 4);
    // Because of the above assert last_tag_minus_1 is never set to 0, and the
    // caller can verify the child parser was terminated, by comparing to 0.
    last_tag_minus_1_ = tag - 1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Slow path helper functions when a child crosses the "end" of range.
  // This is either an error (if limit_ = 0) OR we need to store state.
  // These functions manage the task of updating the state correctly.
  //////////////////////////////////////////////////////////////////////////////

  // Helper function called by generated code in case of a length delimited
  // field that is going to cross the boundary.
  const char* StoreAndTailCall(const char* ptr, const char* end,
                               ParseClosure current_parser,
                               ParseClosure child_parser, int32 size) {
    // At this point ptr could be past end. Hence a malicious size could
    // overflow.
    int64 safe_new_limit = size - static_cast<int64>(end - ptr);
    if (safe_new_limit > INT_MAX) return nullptr;
    GOOGLE_DCHECK(safe_new_limit > 0);  // only call this if it's crossing end
    int32 new_limit = static_cast<int32>(safe_new_limit);
    int32 delta;
    if (limit_ != -1) {
      if (PROTOBUF_PREDICT_FALSE(new_limit > limit_)) return nullptr;
      delta = limit_ - new_limit;
    } else {
      delta = -1;  // special value
    }
    limit_ = new_limit;
    // Save the current closure on the stack.
    if (!Push(current_parser, delta)) return nullptr;
    // Ensure the active state is set correctly.
    parser_ = child_parser;
    return ptr < end ? child_parser(ptr, end, this) : ptr;
  }

  // Helper function for a child group that has crossed the boundary.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif  // defined(__GNUC__) && !defined(__clang__)
  bool StoreGroup(ParseClosure current_parser, ParseClosure child_parser,
                  int depth, uint32 tag) {
    // The group must still read an end-group tag, so it can't be at a limit.
    // By having this check we ensure that when limit_ = 0 we can't end in some
    // deeper recursion. Hence ParseExactRange does not need to check for
    // matching depth.
    if (limit_ == 0) return false;
    if (depth == depth_) {
      // This child group is the active parser. The fast path code assumes
      // everything will be parsed within a chunk and doesn't modify
      // parse context in this case. We need to make the child parser active.
      parser_ = child_parser;
    }
    if (PROTOBUF_PREDICT_FALSE(depth < inlined_depth_)) SwitchStack();
    stack_[depth] = {current_parser, static_cast<int32>(~(tag >> 3))};
    return true;
  }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // defined(__GNUC__) && !defined(__clang__)

 private:
  // This the "active" or current parser.
  ParseClosure parser_;
  // The context keeps an internal stack to keep track of the recursive
  // part of the parse state.
  // Current depth of the active parser, depth counts down.
  // This is used to limit recursion depth (to prevent overflow on malicious
  // data), but is also used to index in stack_ to store the current state.
  int depth_;
  int32 limit_ = -1;

  // A state is on the stack to save it, in order to continue parsing after
  // child is done.
  struct State {
    ParseClosure parser;
    // This element describes how to adjust the parse state after finishing
    // the child. If the child was a length delimited field, delta describes
    // the limit relative to the child's limit (hence >= 0).
    // If child was a sub group limit contains ~field num (hence < 0) in order
    // to verify the group ended on a correct end tag. No limit adjusting.
    // Note above the sign of delta is meaningful
    int32 delta_or_group_num;
  };
  int start_depth_;
  // This is used to return the end group (or 0 tag) that terminated the parse.
  // Actually it contains last_tag minus 1. Which is either the start group tag
  // or -1. This member should always be zero and the caller should immediately
  // check this member to verify what state the parser ended on and clear its
  // value.
  uint32 last_tag_minus_1_ = 0;

  ExtraParseData extra_parse_data_;
  State* stack_;
  State inline_stack_[kInlinedDepth];
  int inlined_depth_;

  bool Push(ParseClosure parser, int32 delta) {
    GOOGLE_DCHECK(delta >= -1);  // Make sure it's a valid len-delim
    if (PROTOBUF_PREDICT_FALSE(--depth_ < 0)) return false;
    if (PROTOBUF_PREDICT_FALSE(depth_ < inlined_depth_)) SwitchStack();
    stack_[depth_] = {parser, delta};
    return true;
  }

  State Pop() { return stack_[depth_++]; }

  void SwitchStack();

  // Parses a chunk of memory given the current state of parse context (ie.
  // the active parser and stack).
  // Pre-condition:
  //   begin < end (non-empty range)
  //   limit_ > 0 (limit from begin) or -1 (no limit)
  // Post-condition:
  //   returns either (true, overrun) for a successful parse that can continue,
  //   or (false, overrun) for a parse that can't continue. Either due to a
  //   corrupt data (parse failure) or because the top-level was terminated on a
  //   0 or end-group tag in which case overrun points to the position after the
  //   end.
  std::pair<bool, int> ParseRangeWithLimit(const char* begin, const char* end);
};

// This is wrapper to parse a sequence of buffers without the overlap property,
// like the sequence given by ZeroCopyInputStream (ZCIS) or ByteSource. This is
// done by copying data around the seams, hence the name EpsCopyParser.
// Pictorially if ZCIS presents a stream in chunks like so
// [---------------------------------------------------------------]
// [---------------------] chunk 1
//                      [----------------------------] chunk 2
//                                          chunk 3 [--------------]
// where '-' depicts bytes of the stream or chunks vertically alligned with the
// corresponding bytes between stream and chunk.
//
// This class will present chunks to the ParseContext like this
// [-----------------....] chunk 1
//                  [----....] patch
//                      [------------------------....] chunk 2
//                                              [----....] patch
//                                          chunk 3 [----------....]
//                                                      patch [----****]
// by using a fixed size buffer to patch over the seams. This requires
// copying of an "epsilon" neighboorhood around the seams. In the picture above
// dots mean bytes beyond the end of the new chunks. Each chunk is kSlopBytes
// smalller as its original chunk (above depicted as 4 dots) and the number of
// of chunks is doubled because each seam in the original stream introduces a
// new patch.
//
// The algorithm is simple but not entirely trivial. Two complications arise
// 1) The original chunk could be less than kSlopBytes. Hence we can't simply
// chop the last kSlopBytes of a chunk.
// 2) In some (infrequent) use cases, we don't necessarily parse unitl the end
// of a stream, but instead the parse is terminated by 0 or end-group tag. If
// this is allowed we must take care to leave the underlying stream at a
// position precisely after the terminating tag. If this happens in the slop
// region of a buffer we will already have loaded the next buffer. Not all
// streams allow backing up to a previous buffer blocking us from leaving the
// stream in the proper state. If terminating on 0 is allowed (in the old parser
// this means a call to MergePartialFromCodedStream without a subsequent call to
// ConsumedEntireMessage), this algorithm needs to ensure the parse won't end
// in the slop region before moving the next buffer.
//
// The core idea of EpsCopyParser is to parse ranges except the last kSlopBytes
// and store those in the patch buffer, until the next parse provides additional
// data to fill the slop region. So parsing a range means first parsing the slop
// bytes of the previous range using the new range to provide slop bytes for the
// patch, followed by parsing the actual range except the last kSlopBytes and
// store those. If no more data is available a call to Done finishes the parse
// by parsing the remaining slopbytes.
//
// In order to deal with problem 1, we need to deal with the case that a new
// chunk can be less or equal than kSlopBytes big. We can just copy the chunk
// to the end and return (buffer, chunk->size). Pictorially
// [--------] chunk 1
//         [--] chunk 2
//           [---] chunk 3
// will become
// [----....] chunk 1
//     [--....] patch (not full range of the patch buffer, only two hyphens)
//         [--] chunk 2 (too small so never parsed directly)
//       [---....] patch (not full range of the buffer, only three hyphens)
//           [---] chunk 3 (too small so never parsed directly)
//          [----****] patch (full range, last bytes are garbage)
// Because of this the source (the dots in above) can overlap with the
// destination buffer and so we have to use memmove.
//
// To solve problem 2, we use a generic parser together with knowledge of the
// nesting from the side stack to verify if the parse will be terminated in the
// slop region. If it terminates inside the slop region, we just parse it as
// well. See ParseEndsInSlopRegion in ParseContext for the implementation. This
// is only done if ensure_non_negative_skip is true, if it's false Skip() could
// return a negative number.
template <bool ensure_non_negative_skip>
class EpsCopyParser {
 public:
  EpsCopyParser(ParseClosure parser, ParseContext* ctx) : ctx_(ctx) {
    ctx_->StartParse(parser);
  }

  // Parse the bytes as provided by the non-empty range.
  // Returns true on a successful parse ready to accept more data, if there is
  // no more data call Done() to finish the parse.
  // Returns false if the parse is terminated. Termination is either due to a
  // parse error or due to termination on an end-group or 0 tag. You can call
  // EndedOnTag() on the underlying ParseContext to find out if the parse ended
  // correctly on a terminating tag.
  bool Parse(StringPiece range) {
    GOOGLE_DCHECK(!range.empty());
    auto size = range.size();
    if (size > kSlopBytes) {
      // The buffer is large enough to be able to parse the (size - kSlopBytes)
      // prefix directly. However we still need to parse the data in buffer_,
      // that holds the slop region of the previous buffer.
      if (overrun_ == kSlopBytes) {
        // We overrun the whole slop region of the previous buffer.
        // Optimization, we can skip the patch buffer.
        overrun_ = 0;
      } else {
        std::memcpy(buffer_ + kSlopBytes, range.begin(), kSlopBytes);
        if (!ParseRange({buffer_, kSlopBytes}, 0)) return false;
      }
      range.remove_suffix(kSlopBytes);
    } else {
      std::memcpy(buffer_ + kSlopBytes, range.begin(), size);
      range = {buffer_, size};
    }
    if (!ParseRange(range, size - kSlopBytes)) return false;
    std::memmove(buffer_, range.end(), kSlopBytes);
    if (ensure_non_negative_skip &&
        ctx_->ParseEndsInSlopRegion(buffer_, overrun_)) {
      // We care about leaving the stream at the right place and the stream will
      // indeed terminate, so just parse it.
      auto res = ParseRange({buffer_, kSlopBytes}, size);
      GOOGLE_DCHECK(!res);
      return false;
    }
    return true;
  }

  // Finish the parse by parsing the remaining data and verify success.
  bool Done() {
    return ParseRange({buffer_, kSlopBytes}, 0) && ctx_->ValidEnd(overrun_);
  }

  // If the parse was terminated by a end-group or 0 tag. Skip returns the
  // offset where the parse left off relative to the start of the last range
  // parsed.
  // NOTE: This could be negative unless ensure_non_negative_skip is true.
  int Skip() {
    // The reason of ensure_non_negative_skip and ParseEndsInSlopRegion is that
    // the following assert holds. Which implies the stream doesn't need to
    // backup.
    GOOGLE_DCHECK(!ensure_non_negative_skip || overrun_ >= 0);
    return overrun_;
  }

 private:
  constexpr static int kSlopBytes = ParseContext::kSlopBytes;
  // overrun_ stores where in the slop region of the previous parse the parse
  // was left off. This is used to start the parse of the next region at the
  // correct point. Initially overrun_ should be set to kSlopBytes which means
  // that the parse starts at precisely the beginning of new buffer provided.
  int overrun_ = kSlopBytes;
  // The first kSlopBytes of buffer_ contains the slop region of the previous
  // parsed region.
  char buffer_[2 * kSlopBytes] = {};
  ParseContext* ctx_;

  bool ParseRange(StringPiece range, int delta) {
    auto res = ctx_->ParseRange(range, &overrun_);
    if (!res) overrun_ += delta;
    return res;
  }
};

// Add any of the following lines to debug which parse function is failing.

#define GOOGLE_PROTOBUF_ASSERT_RETURN(predicate, ret) \
  if (!(predicate)) {                                  \
    /*  raise(SIGINT); */                              \
    /*  GOOGLE_LOG(ERROR) << "Parse failure"; */              \
    return ret;                                        \
  }

#define GOOGLE_PROTOBUF_PARSER_ASSERT(predicate) \
    GOOGLE_PROTOBUF_ASSERT_RETURN(predicate, nullptr)

template <typename T>
std::pair<const char*, bool> FieldParser(uint64 tag, ParseClosure parent,
                                         T field_parser, const char* begin,
                                         const char* end, ParseContext* ctx) {
  auto ptr = begin;
  uint32 number = tag >> 3;
  if (PROTOBUF_PREDICT_FALSE(number == 0)) {
    GOOGLE_PROTOBUF_ASSERT_RETURN(tag == 0, {});
    // Special case scenario of 0 termination.
    ctx->EndGroup(tag);
    return {ptr, true};
  }
  using WireType = internal::WireFormatLite::WireType;
  switch (tag & 7) {
    case WireType::WIRETYPE_VARINT: {
      uint64 value;
      ptr = io::Parse64(ptr, &value);
      GOOGLE_PROTOBUF_ASSERT_RETURN(ptr != nullptr, {});
      field_parser.AddVarint(number, value);
      break;
    }
    case WireType::WIRETYPE_FIXED64: {
      uint64 value = io::UnalignedLoad<uint64>(ptr);
      ptr += 8;
      field_parser.AddFixed64(number, value);
      break;
    }
    case WireType::WIRETYPE_LENGTH_DELIMITED: {
      int32 size;
      ptr = io::ReadSize(ptr, &size);
      GOOGLE_PROTOBUF_ASSERT_RETURN(ptr != nullptr, {});
      ParseClosure child = field_parser.AddLengthDelimited(number, size);
      if (size > end - ptr) {
        return {ctx->StoreAndTailCall(ptr, end, parent, child, size), true};
      }
      auto newend = ptr + size;
      GOOGLE_PROTOBUF_ASSERT_RETURN(ctx->ParseExactRange(child, ptr, newend),
                                     {});
      ptr = newend;
      break;
    }
    case WireType::WIRETYPE_START_GROUP: {
      int depth;
      ParseClosure child = field_parser.StartGroup(number);
      auto res = ctx->ParseGroup(tag, child, ptr, end, &depth);
      ptr = res.first;
      GOOGLE_PROTOBUF_ASSERT_RETURN(ptr != nullptr, {});
      if (res.second) {
        GOOGLE_PROTOBUF_ASSERT_RETURN(
            ctx->StoreGroup(parent, child, depth, tag), {});
        return {ptr, true};
      }
      break;
    }
    case WireType::WIRETYPE_END_GROUP: {
      field_parser.EndGroup(number);
      ctx->EndGroup(tag);
      return {ptr, true};
    }
    case WireType::WIRETYPE_FIXED32: {
      uint32 value = io::UnalignedLoad<uint32>(ptr);
      ptr += 4;
      field_parser.AddFixed32(number, value);
      break;
    }
    default:
      GOOGLE_PROTOBUF_ASSERT_RETURN(false, {});
  }
  GOOGLE_DCHECK(ptr != nullptr);
  return {ptr, false};
}

template <typename T>
const char* WireFormatParser(ParseClosure parent, T field_parser,
                             const char* begin, const char* end,
                             ParseContext* ctx) {
  auto ptr = begin;
  while (ptr < end) {
    uint32 tag;
    ptr = io::Parse32(ptr, &tag);
    GOOGLE_PROTOBUF_PARSER_ASSERT(ptr != nullptr);
    auto res = FieldParser(tag, parent, field_parser, ptr, end, ctx);
    ptr = res.first;
    GOOGLE_PROTOBUF_PARSER_ASSERT(ptr != nullptr);
    if (res.second) return ptr;
  }
  return ptr;
}

// Here are the elementary parsers for length delimited subfields that contain
// plain data (ie not a protobuf). These are trivial as they don't recurse,
// except for the UnknownGroupLiteParse that parses a group into a string.
// Some functions need extra arguments that the function signature allows,
// these are passed through variables in ParseContext::ExtraParseData that the
// caller needs to set prior to the call.

// The null parser does not do anything, but is useful as a substitute.
PROTOBUF_EXPORT
const char* NullParser(const char* begin, const char* end, void* object,
                       ParseContext*);

// Helper for verification of utf8
PROTOBUF_EXPORT
bool VerifyUTF8(StringPiece s, ParseContext* ctx);
// All the string parsers with or without UTF checking and for all CTypes.
PROTOBUF_EXPORT
const char* StringParser(const char* begin, const char* end, void* object,
                         ParseContext*);
PROTOBUF_EXPORT
const char* CordParser(const char* begin, const char* end, void* object,
                       ParseContext*);
PROTOBUF_EXPORT
const char* StringPieceParser(const char* begin, const char* end, void* object,
                              ParseContext*);
PROTOBUF_EXPORT
const char* StringParserUTF8(const char* begin, const char* end, void* object,
                             ParseContext*);
PROTOBUF_EXPORT
const char* CordParserUTF8(const char* begin, const char* end, void* object,
                           ParseContext*);
PROTOBUF_EXPORT
const char* StringPieceParserUTF8(const char* begin, const char* end,
                                  void* object, ParseContext*);
PROTOBUF_EXPORT
const char* StringParserUTF8Verify(const char* begin, const char* end,
                                   void* object, ParseContext*);
PROTOBUF_EXPORT
const char* CordParserUTF8Verify(const char* begin, const char* end,
                                 void* object, ParseContext*);
PROTOBUF_EXPORT
const char* StringPieceParserUTF8Verify(const char* begin, const char* end,
                                        void* object, ParseContext*);
// Parsers that also eat the slopbytes if possible. Can only be called in a
// ParseContext where limit_ is set properly.
PROTOBUF_EXPORT
const char* GreedyStringParser(const char* begin, const char* end, void* object,
                         ParseContext*);
PROTOBUF_EXPORT
const char* GreedyStringParserUTF8(const char* begin, const char* end, void* object,
                             ParseContext*);
PROTOBUF_EXPORT
const char* GreedyStringParserUTF8Verify(const char* begin, const char* end,
                                   void* object, ParseContext*);

// This is the only recursive parser.
PROTOBUF_EXPORT
const char* UnknownGroupLiteParse(const char* begin, const char* end,
                                  void* object, ParseContext* ctx);
// This is a helper to for the UnknownGroupLiteParse but is actually also
// useful in the generated code. It uses overload on string* vs
// UnknownFieldSet* to make the generated code isomorphic between full and lite.
PROTOBUF_EXPORT
std::pair<const char*, bool> UnknownFieldParse(uint32 tag, ParseClosure parent,
                                               const char* begin,
                                               const char* end, std::string* unknown,
                                               ParseContext* ctx);

// The packed parsers parse repeated numeric primitives directly into  the
// corresponding field

// These are packed varints
PROTOBUF_EXPORT
const char* PackedInt32Parser(const char* begin, const char* end, void* object,
                              ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedUInt32Parser(const char* begin, const char* end, void* object,
                               ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedInt64Parser(const char* begin, const char* end, void* object,
                              ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedUInt64Parser(const char* begin, const char* end, void* object,
                               ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedSInt32Parser(const char* begin, const char* end, void* object,
                               ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedSInt64Parser(const char* begin, const char* end, void* object,
                               ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedBoolParser(const char* begin, const char* end, void* object,
                             ParseContext* ctx);

// Enums in proto3 do not require verification
PROTOBUF_EXPORT
const char* PackedEnumParser(const char* begin, const char* end, void* object,
                             ParseContext* ctx);
// Enums in proto2 require verification. So an additional verification function
// needs to be passed into ExtraParseData.
// If it's a generated verification function we only need the function pointer.
PROTOBUF_EXPORT
const char* PackedValidEnumParserLite(const char* begin, const char* end,
                                      void* object, ParseContext* ctx);
// If it's reflective we need a function that takes an additional argument.
PROTOBUF_EXPORT
const char* PackedValidEnumParserLiteArg(const char* begin, const char* end,
                                         void* object, ParseContext* ctx);

// These are the packed fixed field parsers.
PROTOBUF_EXPORT
const char* PackedFixed32Parser(const char* begin, const char* end,
                                void* object, ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedSFixed32Parser(const char* begin, const char* end,
                                 void* object, ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedFixed64Parser(const char* begin, const char* end,
                                void* object, ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedSFixed64Parser(const char* begin, const char* end,
                                 void* object, ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedFloatParser(const char* begin, const char* end, void* object,
                              ParseContext* ctx);
PROTOBUF_EXPORT
const char* PackedDoubleParser(const char* begin, const char* end, void* object,
                               ParseContext* ctx);

// Maps key/value's are stored in a MapEntry length delimited field. If this
// crosses a seam we fallback to first store in payload. The object points
// to a MapField in which we parse the payload upon done (we detect this when
// this function is called with limit_ == 0), by calling parse_map (also stored
// in ctx) on the resulting string.
PROTOBUF_EXPORT
const char* SlowMapEntryParser(const char* begin, const char* end, void* object,
                               internal::ParseContext* ctx);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_PARSE_CONTEXT_H__
