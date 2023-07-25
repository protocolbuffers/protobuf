/*
 * Copyright (c) 2009-2021, Google LLC
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

#include <string.h>

#include "upb/reflection/def_builder_internal.h"
#include "upb/reflection/def_pool.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/field_def.h"

// Must be last.
#include "upb/port/def.inc"

/* The upb core does not generally have a concept of default instances. However
 * for descriptor options we make an exception since the max size is known and
 * modest (<200 bytes). All types can share a default instance since it is
 * initialized to zeroes.
 *
 * We have to allocate an extra pointer for upb's internal metadata. */
static const char opt_default_buf[_UPB_MAXOPT_SIZE + sizeof(void*)] = {0};
const char* kUpbDefOptDefault = &opt_default_buf[sizeof(void*)];

const char* _upb_DefBuilder_FullToShort(const char* fullname) {
  const char* p;

  if (fullname == NULL) {
    return NULL;
  } else if ((p = strrchr(fullname, '.')) == NULL) {
    /* No '.' in the name, return the full string. */
    return fullname;
  } else {
    /* Return one past the last '.'. */
    return p + 1;
  }
}

void _upb_DefBuilder_FailJmp(upb_DefBuilder* ctx) { UPB_LONGJMP(ctx->err, 1); }

void _upb_DefBuilder_Errf(upb_DefBuilder* ctx, const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_Status_VSetErrorFormat(ctx->status, fmt, argp);
  va_end(argp);
  _upb_DefBuilder_FailJmp(ctx);
}

void _upb_DefBuilder_OomErr(upb_DefBuilder* ctx) {
  upb_Status_SetErrorMessage(ctx->status, "out of memory");
  _upb_DefBuilder_FailJmp(ctx);
}

// Verify a relative identifier string. The loop is branchless for speed.
static void _upb_DefBuilder_CheckIdentNotFull(upb_DefBuilder* ctx,
                                              upb_StringView name) {
  bool good = name.size > 0;

  for (size_t i = 0; i < name.size; i++) {
    const char c = name.data[i];
    const char d = c | 0x20;  // force lowercase
    const bool is_alpha = (('a' <= d) & (d <= 'z')) | (c == '_');
    const bool is_numer = ('0' <= c) & (c <= '9') & (i != 0);

    good &= is_alpha | is_numer;
  }

  if (!good) _upb_DefBuilder_CheckIdentSlow(ctx, name, false);
}

const char* _upb_DefBuilder_MakeFullName(upb_DefBuilder* ctx,
                                         const char* prefix,
                                         upb_StringView name) {
  _upb_DefBuilder_CheckIdentNotFull(ctx, name);
  if (prefix) {
    // ret = prefix + '.' + name;
    size_t n = strlen(prefix);
    char* ret = _upb_DefBuilder_Alloc(ctx, n + name.size + 2);
    strcpy(ret, prefix);
    ret[n] = '.';
    memcpy(&ret[n + 1], name.data, name.size);
    ret[n + 1 + name.size] = '\0';
    return ret;
  } else {
    char* ret = upb_strdup2(name.data, name.size, ctx->arena);
    if (!ret) _upb_DefBuilder_OomErr(ctx);
    return ret;
  }
}

static bool remove_component(char* base, size_t* len) {
  if (*len == 0) return false;

  for (size_t i = *len - 1; i > 0; i--) {
    if (base[i] == '.') {
      *len = i;
      return true;
    }
  }

  *len = 0;
  return true;
}

const void* _upb_DefBuilder_ResolveAny(upb_DefBuilder* ctx,
                                       const char* from_name_dbg,
                                       const char* base, upb_StringView sym,
                                       upb_deftype_t* type) {
  if (sym.size == 0) goto notfound;
  upb_value v;
  if (sym.data[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    if (!_upb_DefPool_LookupSym(ctx->symtab, sym.data + 1, sym.size - 1, &v)) {
      goto notfound;
    }
  } else {
    /* Remove components from base until we find an entry or run out. */
    size_t baselen = base ? strlen(base) : 0;
    char* tmp = malloc(sym.size + baselen + 1);
    while (1) {
      char* p = tmp;
      if (baselen) {
        memcpy(p, base, baselen);
        p[baselen] = '.';
        p += baselen + 1;
      }
      memcpy(p, sym.data, sym.size);
      p += sym.size;
      if (_upb_DefPool_LookupSym(ctx->symtab, tmp, p - tmp, &v)) {
        break;
      }
      if (!remove_component(tmp, &baselen)) {
        free(tmp);
        goto notfound;
      }
    }
    free(tmp);
  }

  *type = _upb_DefType_Type(v);
  return _upb_DefType_Unpack(v, *type);

notfound:
  _upb_DefBuilder_Errf(ctx, "couldn't resolve name '" UPB_STRINGVIEW_FORMAT "'",
                       UPB_STRINGVIEW_ARGS(sym));
}

const void* _upb_DefBuilder_Resolve(upb_DefBuilder* ctx,
                                    const char* from_name_dbg, const char* base,
                                    upb_StringView sym, upb_deftype_t type) {
  upb_deftype_t found_type;
  const void* ret =
      _upb_DefBuilder_ResolveAny(ctx, from_name_dbg, base, sym, &found_type);
  if (ret && found_type != type) {
    _upb_DefBuilder_Errf(ctx,
                         "type mismatch when resolving %s: couldn't find "
                         "name " UPB_STRINGVIEW_FORMAT " with type=%d",
                         from_name_dbg, UPB_STRINGVIEW_ARGS(sym), (int)type);
  }
  return ret;
}

// Per ASCII this will lower-case a letter. If the result is a letter, the
// input was definitely a letter. If the output is not a letter, this may
// have transformed the character unpredictably.
static char upb_ascii_lower(char ch) { return ch | 0x20; }

// isalpha() etc. from <ctype.h> are locale-dependent, which we don't want.
static bool upb_isbetween(uint8_t c, uint8_t low, uint8_t high) {
  return low <= c && c <= high;
}

static bool upb_isletter(char c) {
  char lower = upb_ascii_lower(c);
  return upb_isbetween(lower, 'a', 'z') || c == '_';
}

static bool upb_isalphanum(char c) {
  return upb_isletter(c) || upb_isbetween(c, '0', '9');
}

static bool TryGetChar(const char** src, const char* end, char* ch) {
  if (*src == end) return false;
  *ch = **src;
  *src += 1;
  return true;
}

static int TryGetHexDigit(const char** src, const char* end) {
  char ch;
  if (!TryGetChar(src, end, &ch)) return -1;
  if ('0' <= ch && ch <= '9') {
    return ch - '0';
  }
  ch = upb_ascii_lower(ch);
  if ('a' <= ch && ch <= 'f') {
    return ch - 'a' + 0xa;
  }
  *src -= 1;  // Char wasn't actually a hex digit.
  return -1;
}

static char upb_DefBuilder_ParseHexEscape(upb_DefBuilder* ctx,
                                          const upb_FieldDef* f,
                                          const char** src, const char* end) {
  int hex_digit = TryGetHexDigit(src, end);
  if (hex_digit < 0) {
    _upb_DefBuilder_Errf(
        ctx, "\\x must be followed by at least one hex digit (field='%s')",
        upb_FieldDef_FullName(f));
    return 0;
  }
  unsigned int ret = hex_digit;
  while ((hex_digit = TryGetHexDigit(src, end)) >= 0) {
    ret = (ret << 4) | hex_digit;
  }
  if (ret > 0xff) {
    _upb_DefBuilder_Errf(ctx, "Value of hex escape in field %s exceeds 8 bits",
                         upb_FieldDef_FullName(f));
    return 0;
  }
  return ret;
}

static char TryGetOctalDigit(const char** src, const char* end) {
  char ch;
  if (!TryGetChar(src, end, &ch)) return -1;
  if ('0' <= ch && ch <= '7') {
    return ch - '0';
  }
  *src -= 1;  // Char wasn't actually an octal digit.
  return -1;
}

static char upb_DefBuilder_ParseOctalEscape(upb_DefBuilder* ctx,
                                            const upb_FieldDef* f,
                                            const char** src, const char* end) {
  char ch = 0;
  for (int i = 0; i < 3; i++) {
    char digit;
    if ((digit = TryGetOctalDigit(src, end)) >= 0) {
      ch = (ch << 3) | digit;
    }
  }
  return ch;
}

char _upb_DefBuilder_ParseEscape(upb_DefBuilder* ctx, const upb_FieldDef* f,
                                 const char** src, const char* end) {
  char ch;
  if (!TryGetChar(src, end, &ch)) {
    _upb_DefBuilder_Errf(ctx, "unterminated escape sequence in field %s",
                         upb_FieldDef_FullName(f));
    return 0;
  }
  switch (ch) {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    case '\\':
      return '\\';
    case '\'':
      return '\'';
    case '\"':
      return '\"';
    case '?':
      return '\?';
    case 'x':
    case 'X':
      return upb_DefBuilder_ParseHexEscape(ctx, f, src, end);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      *src -= 1;
      return upb_DefBuilder_ParseOctalEscape(ctx, f, src, end);
  }
  _upb_DefBuilder_Errf(ctx, "Unknown escape sequence: \\%c", ch);
}

void _upb_DefBuilder_CheckIdentSlow(upb_DefBuilder* ctx, upb_StringView name,
                                    bool full) {
  const char* str = name.data;
  const size_t len = name.size;
  bool start = true;
  for (size_t i = 0; i < len; i++) {
    const char c = str[i];
    if (c == '.') {
      if (start || !full) {
        _upb_DefBuilder_Errf(
            ctx, "invalid name: unexpected '.' (" UPB_STRINGVIEW_FORMAT ")",
            UPB_STRINGVIEW_ARGS(name));
      }
      start = true;
    } else if (start) {
      if (!upb_isletter(c)) {
        _upb_DefBuilder_Errf(ctx,
                             "invalid name: path components must start with a "
                             "letter (" UPB_STRINGVIEW_FORMAT ")",
                             UPB_STRINGVIEW_ARGS(name));
      }
      start = false;
    } else if (!upb_isalphanum(c)) {
      _upb_DefBuilder_Errf(
          ctx,
          "invalid name: non-alphanumeric character (" UPB_STRINGVIEW_FORMAT
          ")",
          UPB_STRINGVIEW_ARGS(name));
    }
  }
  if (start) {
    _upb_DefBuilder_Errf(ctx,
                         "invalid name: empty part (" UPB_STRINGVIEW_FORMAT ")",
                         UPB_STRINGVIEW_ARGS(name));
  }

  // We should never reach this point.
  UPB_ASSERT(false);
}
