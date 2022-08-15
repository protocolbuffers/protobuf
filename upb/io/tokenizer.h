/*
 * Copyright (c) 2009-2022, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Class for parsing tokenized text from a ZeroCopyInputStream.

#ifndef UPB_IO_TOKENIZER_H_
#define UPB_IO_TOKENIZER_H_

#include "upb/io/string.h"
#include "upb/io/zero_copy_input_stream.h"
#include "upb/upb.h"

// Must be included last.
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  kUpb_TokenType_Start,  // Next() has not yet been called.
  kUpb_TokenType_End,    // End of input reached. "text" is empty.

  // A sequence of letters, digits, and underscores, not starting with a digit.
  // It is an error for a number to be followed by an identifier with no space
  // in between.
  kUpb_TokenType_Identifier,

  // A sequence of digits representing an integer. Normally the digits are
  // decimal, but a prefix of "0x" indicates a hex number and a leading zero
  // indicates octal, just like with C numeric literals. A leading negative
  // sign is NOT included in the token; it's up to the parser to interpret the
  // unary minus operator on its own.
  kUpb_TokenType_Integer,

  // A floating point literal, with a fractional part and/or an exponent.
  // Always in decimal. Again, never negative.
  kUpb_TokenType_Float,

  // A quoted sequence of escaped characters.
  // Either single or double quotes can be used, but they must match.
  // A string literal cannot cross a line break.
  kUpb_TokenType_String,

  // Any other printable character, like '!' or '+'.
  // Symbols are always a single character, so "!+$%" is four tokens.
  kUpb_TokenType_Symbol,

  // A sequence of whitespace.
  // This token type is only produced if report_whitespace() is true.
  // It is not reported for whitespace within comments or strings.
  kUpb_TokenType_Whitespace,

  // A newline ('\n'). This token type is only produced if report_whitespace()
  // is true and report_newlines() is also true.
  // It is not reported for newlines in comments or strings.
  kUpb_TokenType_Newline,
} upb_TokenType;

typedef enum {
  // Set to allow floats to be suffixed with the letter 'f'. Tokens which would
  // otherwise be integers but which have the 'f' suffix will be forced to be
  // interpreted as floats. For all other purposes, the 'f' is ignored.
  kUpb_TokenizerOption_AllowFAfterFloat = 1 << 0,

  // If set, allow string literals to span multiple lines.
  // Do not use this; for Google-internal cleanup only.
  kUpb_TokenizerOption_AllowMultilineStrings = 1 << 1,

  // If set, allow a field name to appear immediately after a number without
  // requiring any intervening whitespace as a delimiter.
  // Do not use this; for Google-internal cleanup only.
  kUpb_TokenizerOption_AllowFieldImmediatelyAfterNumber = 1 << 2,

  // If set, whitespace tokens are reported by Next().
  kUpb_TokenizerOption_ReportWhitespace = 1 << 3,

  // If set, newline tokens are reported by Next(). Implies ReportWhitespace.
  kUpb_TokenizerOption_ReportNewlines = 1 << 4,

  // By default the tokenizer expects C-style (/* */) comments.
  // If set, it expects shell-style (#) comments instead.
  kUpb_TokenizerOption_CommentStyleShell = 1 << 5,
} upb_Tokenizer_Option;

// Abstract interface for an object which collects the errors that occur
// during parsing.  A typical implementation might simply print the errors
// to stdout.
typedef struct {
  // Indicates that there was an error in the input at the given line and
  // column numbers.  The numbers are zero-based, so you may want to add
  // 1 to each before printing them.
  void (*AddError)(int line, int column, const char* message, void* context);

  // Indicates that there was a warning in the input at the given line and
  // column numbers.  The numbers are zero-based, so you may want to add
  // 1 to each before printing them.
  void (*AddWarning)(int line, int column, const char* message, void* context);

  // Opaque pointer, passed an as argument to the above functions.
  void* context;
} upb_ErrorCollector;

typedef struct upb_Tokenizer upb_Tokenizer;

// Can be passed a flat array and/or a ZCIS as input.
// The array will be read first (if non-NULL), then the stream (if non-NULL).
upb_Tokenizer* upb_Tokenizer_New(const void* data, size_t size,
                                 upb_ZeroCopyInputStream* input,
                                 upb_ErrorCollector* error_collector,
                                 int options, upb_Arena* arena);

void upb_Tokenizer_Fini(upb_Tokenizer* t);
bool upb_Tokenizer_Next(upb_Tokenizer* t);

// Accessors for inspecting current/previous parse tokens,
// which are opaque to the tokenizer (to reduce copying).

upb_TokenType upb_Tokenizer_CurrentType(const upb_Tokenizer* t);
int upb_Tokenizer_CurrentColumn(const upb_Tokenizer* t);
int upb_Tokenizer_CurrentEndColumn(const upb_Tokenizer* t);
int upb_Tokenizer_CurrentLine(const upb_Tokenizer* t);
int upb_Tokenizer_CurrentTextSize(const upb_Tokenizer* t);
const char* upb_Tokenizer_CurrentTextData(const upb_Tokenizer* t);

upb_TokenType upb_Tokenizer_PreviousType(const upb_Tokenizer* t);
int upb_Tokenizer_PreviousColumn(const upb_Tokenizer* t);
int upb_Tokenizer_PreviousEndColumn(const upb_Tokenizer* t);
int upb_Tokenizer_PreviousLine(const upb_Tokenizer* t);

// Parses a TYPE_INTEGER token. Returns false if the result would be
// greater than max_value. Otherwise, returns true and sets *output to the
// result. If the text is not from a Token of type TYPE_INTEGER originally
// parsed by a Tokenizer, the result is undefined (possibly an assert
// failure).
bool upb_Parse_Integer(const char* text, uint64_t max_value, uint64_t* output);

// Parses a TYPE_FLOAT token. This never fails, so long as the text actually
// comes from a TYPE_FLOAT token parsed by Tokenizer. If it doesn't, the
// result is undefined (possibly an assert failure).
double upb_Parse_Float(const char* text);

// Identical to ParseString (below), but appends to output.
void upb_Parse_StringAppend(const char* text, upb_String* output);

// Parses a TYPE_STRING token. This never fails, so long as the text actually
// comes from a TYPE_STRING token parsed by Tokenizer. If it doesn't, the
// result is undefined (possibly an assert failure).
UPB_INLINE void upb_Parse_String(const char* text, upb_String* output) {
  upb_String_Clear(output);
  upb_Parse_StringAppend(text, output);
}

// External helper: validate an identifier.
bool upb_Tokenizer_IsIdentifier(const char* text, int size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif  // UPB_IO_TOKENIZER_H_
