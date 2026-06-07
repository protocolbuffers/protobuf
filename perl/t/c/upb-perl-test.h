#ifndef PERLUPB_TEST_H
#define PERLUPB_TEST_H

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#define PCRE2_CODE_UNIT_WIDTH 8
#ifdef GOOGLE3
#include "third_party/pcre2/pcre2.h"
#else
#include <pcre2.h>
#endif
#endif

#include "EXTERN.h"
#include "XSUB.h"
#include "perl.h"  // NOLINT(build/include)
#include "upb/mem/arena.h"

// #include "ppport.h"

#define plan(n) fprintf(stderr, "%*s1..%d\n", indent_level * 4, "", (n))

extern const char* todo_reason;
extern int indent_level;

#define todo_start(reason) todo_reason = (reason)
#define todo_end() todo_reason = NULL

#define TODO                                       \
  for (int _todo_i = (todo_start(reason), 0); _todo_i < 1; \
       _todo_i++, todo_end())

#define ok(val, name)                                          \
  fprintf(stderr, "%*s%s %d - %s%s%s\n", indent_level * 4, "", \
          (val) ? "ok" : "not ok", ++test_num, (name),         \
          (todo_reason ? " # TODO " : ""), (todo_reason ? todo_reason : ""))
#define fail(name) ok(0, name)

#define is(got, expected, name)                                                \
  fprintf(stderr, "%*s%s %d - %s\n", indent_level * 4, "",                     \
          ((got) == (expected)) ? "ok" : "not ok", ++test_num, (name));        \
  if ((got) != (expected)) {                                                   \
    fprintf(stderr, "%*s  # Got: %" PRId64 "\n%*s  # Expected: %" PRId64 "\n", \
            indent_level * 4, "", (int64_t)(got), indent_level * 4, "",        \
            (int64_t)(expected));                                              \
  }

#define isnt(got, expected, name)                                       \
  fprintf(stderr, "%*s%s %d - %s\n", indent_level * 4, "",              \
          ((got) != (expected)) ? "ok" : "not ok", ++test_num, (name)); \
  if ((got) == (expected)) {                                            \
    fprintf(stderr, "%*s  # Got: %" PRId64 " (Expected different)\n",   \
            indent_level * 4, "", (int64_t)(got));                      \
  }

#define is_u(got, expected, name)                                              \
  fprintf(stderr, "%*s%s %d - %s\n", indent_level * 4, "",                     \
          ((got) == (expected)) ? "ok" : "not ok", ++test_num, (name));        \
  if ((got) != (expected)) {                                                   \
    fprintf(stderr, "%*s  # Got: %" PRIu64 "\n%*s  # Expected: %" PRIu64 "\n", \
            indent_level * 4, "", (uint64_t)(got), indent_level * 4, "",       \
            (uint64_t)(expected));                                             \
  }
#define isnt_u(got, expected, name)                                     \
  fprintf(stderr, "%*s%s %d - %s\n", indent_level * 4, "",              \
          ((got) != (expected)) ? "ok" : "not ok", ++test_num, (name)); \
  if ((got) == (expected)) {                                            \
    fprintf(stderr, "%*s  # Got: %" PRIu64 " (Expected different)\n",   \
            indent_level * 4, "", (uint64_t)(got));                     \
  }
#define is_string(got, expected, name)                                         \
  fprintf(stderr, "%*s%s %d - %s\n", indent_level * 4, "",                     \
          (strcmp((got), (expected)) == 0) ? "ok" : "not ok", ++test_num,      \
          (name));                                                             \
  if (strcmp((got), (expected)) != 0) {                                        \
    fprintf(stderr, "%*s  # Got: %s\n%*s  # Expected: %s\n", indent_level * 4, \
            "", (got), indent_level * 4, "", (expected));                      \
  }
#define is_blob(got, expected, len, name)                            \
  fprintf(stderr, "%*s%s %d - %s\n", indent_level * 4, "",           \
          (memcmp((got), (expected), (len)) == 0) ? "ok" : "not ok", \
          ++test_num, (name));                                       \
  if (memcmp((got), (expected), (len)) != 0) {                       \
    fprintf(stderr, "%*s  # Blobs differ\n", indent_level * 4, "");  \
  }
#define is_string_view(got, expected, len, name)                               \
  fprintf(stderr, "%*s%s %d - %s\n", indent_level * 4, "",                     \
          (strncmp((got).data, (expected), (len)) == 0 && (got).size == (len)) \
              ? "ok"                                                           \
              : "not ok",                                                      \
          ++test_num, (name));                                                 \
  if (strncmp((got).data, (expected), (len)) != 0 || (got).size != (len)) {    \
    fprintf(stderr,                                                            \
            "%*s  # Got: %.*s (len %zu)\n%*s  # Expected: %s "                 \
            "(len %zu)\n",                                                     \
            indent_level * 4, "", (int)(got).size, (got).data,                 \
            (size_t)(got).size, indent_level * 4, "", (expected),              \
            (size_t)(len));                                                    \
  }

#define subtest(name, block)                                           \
  STMT_START {                                                         \
    fprintf(stderr, "%*s# Subtest: %s\n", indent_level * 4, "", name); \
    int _parent_test_num = test_num;                                   \
    test_num = 0;                                                      \
    indent_level++;                                                    \
    block;                                                             \
    indent_level--;                                                    \
    test_num = _parent_test_num;                                       \
    ok(1, name);                                                       \
  }                                                                    \
  STMT_END

#define LEAK_CHECK(arena, block, name)                                       \
  STMT_START {                                                               \
    size_t _before = upb_Arena_SpaceAllocated(arena, NULL);                  \
    block;                                                                   \
    size_t _after = upb_Arena_SpaceAllocated(arena, NULL);                   \
    ok(_after == _before, name);                                             \
    if (_after != _before) {                                                 \
      fprintf(stderr, "%*s  # Leaked %zu bytes in [%s]\n", indent_level * 4, \
              "", _after - _before, name);                                   \
    }                                                                        \
  }                                                                          \
  STMT_END

#define STRESS_THREADS(n, func, arg_array)                                     \
  STMT_START {                                                                 \
    pthread_t* _threads = (pthread_t*)malloc(sizeof(pthread_t) * (n));         \
    for (int _ti = 0; _ti < (n); _ti++) {                                      \
      void* _arg = (void*)&((arg_array)[_ti]);                                 \
      pthread_create(&_threads[_ti], NULL, (void* (*)(void*))func, _arg);      \
    }                                                                          \
    for (int _ti = 0; _ti < (n); _ti++) {                                      \
      pthread_join(_threads[_ti], NULL);                                       \
    }                                                                          \
    free(_threads);                                                            \
    ok(1,                                                                      \
       sdiagnostic("STRESS_THREADS: Finished %d threads for %s", (n), #func)); \
  }                                                                            \
  STMT_END

#ifdef _WIN32
static int _upb_test_matchhere(const char* regexp, const char* text);
static int _upb_test_matchstar(int c, const char* regexp, const char* text);

static int _upb_test_match(const char* regexp, const char* text) {
  if (regexp[0] == '^') return _upb_test_matchhere(regexp + 1, text);
  do {
    if (_upb_test_matchhere(regexp, text)) return 1;
  } while (*text++ != '\0');
  return 0;
}

static int _upb_test_matchhere(const char* regexp, const char* text) {
  if (regexp[0] == '\0') return 1;
  if (regexp[1] == '*') return _upb_test_matchstar(regexp[0], regexp + 2, text);
  if (regexp[0] == '$' && regexp[1] == '\0') return *text == '\0';
  if (*text != '\0' && (regexp[0] == '.' || regexp[0] == *text))
    return _upb_test_matchhere(regexp + 1, text + 1);
  return 0;
}

static int _upb_test_matchstar(int c, const char* regexp, const char* text) {
  do {
    if (_upb_test_matchhere(regexp, text)) return 1;
  } while (*text != '\0' && (*text++ == c || c == '.'));
  return 0;
}

#define like(str, pattern, name)                                               \
  STMT_START {                                                                 \
    bool _like_pass = false;                                                   \
    if (str == NULL) {                                                         \
      fprintf(stderr, "%*s# String to match is NULL for [%s]\n",               \
              indent_level * 4, "", name);                                     \
    } else if (pattern == NULL) {                                              \
      fprintf(stderr, "%*s# Pattern is NULL for [%s]\n", indent_level * 4, "", \
              name);                                                           \
    } else {                                                                   \
      if (_upb_test_match(pattern, str)) {                                     \
        _like_pass = true;                                                     \
      } else {                                                                 \
        fprintf(stderr,                                                        \
                "%*s  # String [%s] does not match pattern "                   \
                "[%s] for [%s]\n",                                             \
                indent_level * 4, "", str, pattern, name);                     \
      }                                                                        \
    }                                                                          \
    ok(_like_pass, name);                                                      \
  }                                                                            \
  STMT_END

#define like_n(str, str_len, pattern, name)                                    \
  STMT_START {                                                                 \
    bool _like_pass = false;                                                   \
    if (!str) {                                                                \
      fprintf(stderr, "%*s# String to match is NULL for [%s]\n",               \
              indent_level * 4, "", name);                                     \
    } else if (!pattern) {                                                     \
      fprintf(stderr, "%*s# Pattern is NULL for [%s]\n", indent_level * 4, "", \
              name);                                                           \
    } else {                                                                   \
      char* temp_str = malloc(str_len + 1);                                    \
      if (temp_str) {                                                          \
        memcpy(temp_str, str, str_len);                                        \
        temp_str[str_len] = '\0';                                              \
        if (_upb_test_match(pattern, temp_str)) {                              \
          _like_pass = true;                                                   \
        } else {                                                               \
          fprintf(stderr,                                                      \
                  "%*s  # String [%.*s] does not match pattern "               \
                  "[%s] for [%s]\n",                                           \
                  indent_level * 4, "", (int)str_len, str, pattern, name);     \
        }                                                                      \
        free(temp_str);                                                        \
      }                                                                        \
    }                                                                          \
    ok(_like_pass, name);                                                      \
  }                                                                            \
  STMT_END
#else
#define like(str, pattern, name)                                               \
  STMT_START {                                                                 \
    bool _like_pass = false;                                                   \
    if (str == NULL) {                                                         \
      fprintf(stderr, "%*s# String to match is NULL for [%s]\n",               \
              indent_level * 4, "", name);                                     \
    } else if (pattern == NULL) {                                              \
      fprintf(stderr, "%*s# Pattern is NULL for [%s]\n", indent_level * 4, "", \
              name);                                                           \
    } else {                                                                   \
      int errornumber;                                                         \
      PCRE2_SIZE erroroffset;                                                  \
      pcre2_code* re =                                                         \
          pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 0,         \
                        &errornumber, &erroroffset, NULL);                     \
      if (re == NULL) {                                                        \
        PCRE2_UCHAR buffer[256];                                               \
        pcre2_get_error_message(errornumber, buffer, sizeof(buffer));          \
        fprintf(stderr,                                                        \
                "%*s# Regex compilation failed for [%s] at "                   \
                "offset %d: %s\n",                                             \
                indent_level * 4, "", name, (int)erroroffset, buffer);         \
      } else {                                                                 \
        pcre2_match_data* match_data =                                         \
            pcre2_match_data_create_from_pattern(re, NULL);                    \
        int rc = pcre2_match(re, (PCRE2_SPTR)str, strlen(str), 0, 0,           \
                             match_data, NULL);                                \
        if (rc >= 0) {                                                         \
          _like_pass = true;                                                   \
        } else {                                                               \
          fprintf(stderr,                                                      \
                  "%*s  # String [%s] does not match pattern "                 \
                  "[%s] for [%s]\n",                                           \
                  indent_level * 4, "", str, pattern, name);                   \
        }                                                                      \
        pcre2_match_data_free(match_data);                                     \
        pcre2_code_free(re);                                                   \
      }                                                                        \
    }                                                                          \
    ok(_like_pass, name);                                                      \
  }                                                                            \
  STMT_END

#define like_n(str, str_len, pattern, name)                                    \
  STMT_START {                                                                 \
    bool _like_pass = false;                                                   \
    if (!str) {                                                                \
      fprintf(stderr, "%*s# String to match is NULL for [%s]\n",               \
              indent_level * 4, "", name);                                     \
    } else if (!pattern) {                                                     \
      fprintf(stderr, "%*s# Pattern is NULL for [%s]\n", indent_level * 4, "", \
              name);                                                           \
    } else {                                                                   \
      int errornumber;                                                         \
      PCRE2_SIZE erroroffset;                                                  \
      pcre2_code* re =                                                         \
          pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 0,         \
                        &errornumber, &erroroffset, NULL);                     \
      if (re == NULL) {                                                        \
        PCRE2_UCHAR buffer[256];                                               \
        pcre2_get_error_message(errornumber, buffer, sizeof(buffer));          \
        fprintf(stderr,                                                        \
                "%*s# Regex compilation failed for [%s] at "                   \
                "offset %d: %s\n",                                             \
                indent_level * 4, "", name, (int)erroroffset, buffer);         \
      } else {                                                                 \
        pcre2_match_data* match_data =                                         \
            pcre2_match_data_create_from_pattern(re, NULL);                    \
        int rc =                                                               \
            pcre2_match(re, (PCRE2_SPTR)str, str_len, 0, 0, match_data, NULL); \
        if (rc >= 0) {                                                         \
          _like_pass = true;                                                   \
        } else {                                                               \
          fprintf(stderr,                                                      \
                  "%*s  # String [%.*s] does not match pattern "               \
                  "[%s] for [%s]\n",                                           \
                  indent_level * 4, "", (int)str_len, str, pattern, name);     \
        }                                                                      \
        pcre2_match_data_free(match_data);                                     \
        pcre2_code_free(re);                                                   \
      }                                                                        \
    }                                                                          \
    ok(_like_pass, name);                                                      \
  }                                                                            \
  STMT_END
#endif

#define cdiag(fmt, ...)                                                  \
  STMT_START {                                                           \
    const char* verbose = getenv("VERBOSE_TESTS");                       \
    if (verbose && strcmp(verbose, "1") == 0) {                          \
      fprintf(stderr, "%*s# " fmt, indent_level * 4, "", ##__VA_ARGS__); \
      fprintf(stderr, "\n");                                             \
    }                                                                    \
  }                                                                      \
  STMT_END

// Enhanced Assertion Macros
#define ASSERT_PROTO_MATCH(msg1, msg2, mdef, name)                             \
  STMT_START {                                                                 \
    const upb_MiniTable* mt = upb_MessageDef_MiniTable(mdef);                  \
    bool _match = upb_Message_IsEqual(msg1, msg2, mt);                         \
    ok(_match, name);                                                          \
    if (!_match) {                                                             \
      fprintf(stderr, "%*s  # Messages do not match\n", indent_level * 4, ""); \
    }                                                                          \
  }                                                                            \
  STMT_END

#define ASSERT_ARENA_CLEAN(arena, name)                            \
  STMT_START {                                                     \
    /* For now, just check if it's non-NULL. Real leak check would \
     * need upb internals */                                       \
    ok((arena) != NULL, name);                                     \
  }                                                                \
  STMT_END

#define TEST_PERL_CALL(code, name)                                     \
  STMT_START {                                                         \
    (void)eval_pv(code, TRUE);                                         \
    ok(!SvTRUE(ERRSV), name);                                          \
    if (SvTRUE(ERRSV)) {                                               \
      fprintf(stderr, "%*s  # Perl error: %s\n", indent_level * 4, "", \
              SvPV_nolen(ERRSV));                                      \
    }                                                                  \
  }                                                                    \
  STMT_END

// Helper for formatted test names
static char* sdiagnostic(const char* fmt, ...) {
  static char buffer[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  return buffer;
}

extern int test_num;

// Helper to get a new arena for testing
upb_Arena* test_arena_new(void);

// Helper to initialize and destroy a Perl interpreter for testing
PerlInterpreter* test_perl_init(int argc, char** argv);
void test_perl_destroy(PerlInterpreter* my_perl);

#define SKIP(reason, count)                                           \
  STMT_START {                                                        \
    for (int _skip_i = 0; _skip_i < (count); _skip_i++)               \
      fprintf(stderr, "%*sok %d - # skip %s\n", indent_level * 4, "", \
              ++test_num, (reason));                                  \
  }                                                                   \
  STMT_END

#define SKIP_BLOCK(condition, count, reason) \
  if (condition) {                           \
    SKIP(reason, count);                     \
  } else  // NOLINT(readability/braces)

#ifdef _WIN32
#undef stderr
#undef stdout
#undef stdin
#define stderr (__acrt_iob_func(2))
#define stdout (__acrt_iob_func(1))
#define stdin (__acrt_iob_func(0))
#undef fprintf
#undef printf
#undef malloc
#undef free
#undef calloc
#undef realloc
#undef getenv
#endif

#endif  // PERLUPB_TEST_H

// TODO: Implement world-class build and test capabilities
// 1. Configurable sanitizer support (ASan, UBSan, MSan)
// 2. Automate C-level benchmark compilation and execution (make
// bench)
// 3. Linker-level symbol visibility control for ABI stability
// 4. Indented subtest support in the C harness (TAP 13 compliant)
// 5. LEAK_CHECK { ... } block for local allocation neutrality
// 6. STRESS_THREADS(n, func) macro for concurrency verification
