/*
** Driver code for the x64 JIT compiler.
*/

/* Needed to ensure we get defines like MAP_ANON. */
#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include "upb/msg.h"
#include "upb/pb/decoder.h"
#include "upb/pb/decoder.int.h"
#include "upb/pb/varint.int.h"

/* To debug the JIT:
 *
 * 1. Uncomment:
 * #define UPB_JIT_LOAD_SO
 *
 * Note: this mode requires that we can shell out to gcc.
 *
 * 2. Run the test locally.  This will load the JIT code by building a
 *    .so (/tmp/upb-jit-code.so) and using dlopen, so more of the tooling will
 *    work properly (like GDB).
 *
 * IF YOU ALSO WANT AUTOMATIC JIT DEBUG OUTPUT:
 *
 * 3. Run: upb/pb/make-gdb-script.rb > script.gdb.  This reads
 *    /tmp/upb-jit-code.so as input and generates a GDB script that is specific
 *    to this jit code.
 *
 * 4. Run: gdb --command=script.gdb --args path/to/test
 *    This will drop you to a GDB prompt which you can now use normally.
 *    But when you run the test it will print a message to stdout every time
 *    the JIT executes assembly for a particular bytecode.  Sample output:
 *
 *    X.enterjit bytes=18
 *    buf_ofs=1 data_rem=17 delim_rem=-2 X.0x6.OP_PARSE_DOUBLE
 *    buf_ofs=9 data_rem=9 delim_rem=-10 X.0x7.OP_CHECKDELIM
 *    buf_ofs=9 data_rem=9 delim_rem=-10 X.0x8.OP_TAG1
 *    X.0x3.dispatch.DecoderTest
 *    X.parse_unknown
 *    X.0x3.dispatch.DecoderTest
 *    X.decode_unknown_tag_fallback
 *    X.exitjit
 *
 *    This output should roughly correspond to the output that the bytecode
 *    interpreter emits when compiled with UPB_DUMP_BYTECODE (modulo some
 *    extra JIT-specific output). */

/* These defines are necessary for DynASM codegen.
 * See dynasm/dasm_proto.h for more info. */
#define Dst_DECL jitcompiler *jc
#define Dst_REF (jc->dynasm)
#define Dst (jc)

/* In debug mode, make DynASM do internal checks (must be defined before any
 * dasm header is included. */
#ifndef NDEBUG
#define DASM_CHECKS
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef struct {
  mgroup *group;
  uint32_t *pc;

  /* This pointer is allocated by dasm_init() and freed by dasm_free(). */
  struct dasm_State *dynasm;

  /* Maps some key (an arbitrary void*) to a pclabel.
   *
   *  The pclabel represents a location in the generated code -- DynASM exposes
   *  a pclabel -> (machine code offset) lookup function.
   *
   *  The key can be anything.  There are two main kinds of keys:
   *    - bytecode location -- the void* points to the bytecode instruction
   *      itself.  We can then use this to generate jumps to this instruction.
   *    - other object (like dispatch table).  We use these to represent parts
   *      of the generated code that do not exactly correspond to a bytecode
   *      instruction. */
   upb_inttable jmptargets;

#ifndef NDEBUG
  /* Like jmptargets, but members are present in the table when they have had
   * define_jmptarget() (as opposed to jmptarget) called.  Used to verify that
   * define_jmptarget() is called exactly once for every target.
   * The value is ignored. */
  upb_inttable jmpdefined;

  /* For checking that two asmlabels aren't defined for the same byte. */
  int lastlabelofs;
#endif

#ifdef UPB_JIT_LOAD_SO
  /* For marking labels that should go into the generated code.
   * Maps pclabel -> char* label (string is owned by the table). */
  upb_inttable asmlabels;
#endif

  /* The total number of pclabels currently defined.
   * Note that this contains both jmptargets and asmlabels, which both use
   * pclabels but for different purposes. */
  uint32_t pclabel_count;

  /* Used by DynASM to store globals. */
  void **globals;
} jitcompiler;

/* Functions called by codegen. */
static int jmptarget(jitcompiler *jc, const void *key);
static int define_jmptarget(jitcompiler *jc, const void *key);
static void asmlabel(jitcompiler *jc, const char *fmt, ...);
static int pcofs(jitcompiler* jc);
static int alloc_pclabel(jitcompiler *jc);

#ifdef UPB_JIT_LOAD_SO
static char *upb_vasprintf(const char *fmt, va_list ap);
static char *upb_asprintf(const char *fmt, ...);
#endif

#include "third_party/dynasm/dasm_proto.h"
#include "third_party/dynasm/dasm_x86.h"
#include "upb/pb/compile_decoder_x64.h"

static jitcompiler *newjitcompiler(mgroup *group) {
  jitcompiler *jc = malloc(sizeof(jitcompiler));
  jc->group = group;
  jc->pclabel_count = 0;
  upb_inttable_init(&jc->jmptargets, UPB_CTYPE_UINT32);
#ifndef NDEBUG
  jc->lastlabelofs = -1;
  upb_inttable_init(&jc->jmpdefined, UPB_CTYPE_BOOL);
#endif
#ifdef UPB_JIT_LOAD_SO
  upb_inttable_init(&jc->asmlabels, UPB_CTYPE_PTR);
#endif
  jc->globals = malloc(UPB_JIT_GLOBAL__MAX * sizeof(*jc->globals));

  dasm_init(jc, 1);
  dasm_setupglobal(jc, jc->globals, UPB_JIT_GLOBAL__MAX);
  dasm_setup(jc, upb_jit_actionlist);

  return jc;
}

static void freejitcompiler(jitcompiler *jc) {
#ifdef UPB_JIT_LOAD_SO
  upb_inttable_iter i;
  upb_inttable_begin(&i, &jc->asmlabels);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    free(upb_value_getptr(upb_inttable_iter_value(&i)));
  }
  upb_inttable_uninit(&jc->asmlabels);
#endif
#ifndef NDEBUG
  upb_inttable_uninit(&jc->jmpdefined);
#endif
  upb_inttable_uninit(&jc->jmptargets);
  dasm_free(jc);
  free(jc->globals);
  free(jc);
}

#ifdef UPB_JIT_LOAD_SO

/* Like sprintf except allocates the string, which is returned and owned by the
 * caller.
 *
 * Like the GNU extension asprintf(), except we abort on error (since this is
 * only for debugging). */
static char *upb_vasprintf(const char *fmt, va_list args) {
  /* Run once to get the length of the string. */
  va_list args_copy;
  va_copy(args_copy, args);
  int len = _upb_vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  char *ret = malloc(len + 1);  /* + 1 for NULL terminator. */
  if (!ret) abort();
  int written = _upb_vsnprintf(ret, len + 1, fmt, args);
  UPB_ASSERT(written == len);

  return ret;
}

static char *upb_asprintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char *ret = upb_vasprintf(fmt, args);
  va_end(args);
  return ret;
}

#endif

static int alloc_pclabel(jitcompiler *jc) {
  int newpc = jc->pclabel_count++;
  dasm_growpc(jc, jc->pclabel_count);
  return newpc;
}

static bool try_getjmptarget(jitcompiler *jc, const void *key, int *pclabel) {
  upb_value v;
  if (upb_inttable_lookupptr(&jc->jmptargets, key, &v)) {
    *pclabel = upb_value_getuint32(v);
    return true;
  } else {
    return false;
  }
}

/* Gets the pclabel for this bytecode location's jmptarget.  Requires that the
 * jmptarget() has been previously defined. */
static int getjmptarget(jitcompiler *jc, const void *key) {
  int pclabel = 0;
  bool ok;

  UPB_ASSERT_DEBUGVAR(upb_inttable_lookupptr(&jc->jmpdefined, key, NULL));
  ok = try_getjmptarget(jc, key, &pclabel);
  UPB_ASSERT(ok);
  return pclabel;
}

/* Returns a pclabel that serves as a jmp target for the given bytecode pointer.
 * This should only be called for code that is jumping to the target; code
 * defining the target should use define_jmptarget().
 *
 * Creates/allocates a pclabel for this target if one does not exist already. */
static int jmptarget(jitcompiler *jc, const void *key) {
  // Optimizer sometimes can't figure out that initializing this is unnecessary.
  int pclabel = 0;
  if (!try_getjmptarget(jc, key, &pclabel)) {
    pclabel = alloc_pclabel(jc);
    upb_inttable_insertptr(&jc->jmptargets, key, upb_value_uint32(pclabel));
  }
  return pclabel;
}

/* Defines a pclabel associated with the given bytecode location.
 * Must be called exactly once by the code that is generating the code for this
 * bytecode.
 *
 * Must be called exactly once before bytecode generation is complete (this is a
 * sanity check to make sure the label is defined exactly once). */
static int define_jmptarget(jitcompiler *jc, const void *key) {
#ifndef NDEBUG
  upb_inttable_insertptr(&jc->jmpdefined, key, upb_value_bool(true));
#endif
  return jmptarget(jc, key);
}

/* Returns a bytecode pc offset relative to the beginning of the group's
 * code. */
static int pcofs(jitcompiler *jc) {
  return jc->pc - jc->group->bytecode;
}

/* Returns a machine code offset corresponding to the given key.
 * Requires that this key was defined with define_jmptarget. */
static int machine_code_ofs(jitcompiler *jc, const void *key) {
  int pclabel = getjmptarget(jc, key);
  /* Despite its name, this function takes a pclabel and returns the
   * corresponding machine code offset. */
  return dasm_getpclabel(jc, pclabel);
}

/* Returns a machine code offset corresponding to the given method-relative
 * bytecode offset.  Note that the bytecode offset is relative to the given
 * method, but the returned machine code offset is relative to the beginning of
 * *all* the machine code. */
static int machine_code_ofs2(jitcompiler *jc, const upb_pbdecodermethod *method,
                             int pcofs) {
  void *bc_target = jc->group->bytecode + method->code_base.ofs + pcofs;
  return machine_code_ofs(jc, bc_target);
}

/* Given a pcofs relative to this method's base, returns a machine code offset
 * relative to jmptarget(dispatch->array) (which is used in jitdispatch as the
 * machine code base for dispatch table lookups). */
uint32_t dispatchofs(jitcompiler *jc, const upb_pbdecodermethod *method,
                     int pcofs) {
  int mc_base = machine_code_ofs(jc, method->dispatch.array);
  int mc_target = machine_code_ofs2(jc, method, pcofs);
  int ret;

  UPB_ASSERT(mc_base > 0);
  UPB_ASSERT(mc_target > 0);
  ret = mc_target - mc_base;
  UPB_ASSERT(ret > 0);
  return ret;
}

/* Rewrites the dispatch tables into machine code offsets. */
static void patchdispatch(jitcompiler *jc) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &jc->group->methods);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    upb_inttable *dispatch = &method->dispatch;
    upb_inttable_iter i2;

    method->is_native_ = true;

    /* Remove DISPATCH_ENDMSG -- only the bytecode interpreter needs it.
     * And leaving it around will cause us to find field 0 improperly. */
    upb_inttable_remove(dispatch, DISPATCH_ENDMSG, NULL);

    upb_inttable_begin(&i2, dispatch);
    for (; !upb_inttable_done(&i2); upb_inttable_next(&i2)) {
      uintptr_t key = upb_inttable_iter_key(&i2);
      uint64_t val = upb_value_getuint64(upb_inttable_iter_value(&i2));
      uint64_t newval;
      bool ok;
      if (key <= UPB_MAX_FIELDNUMBER) {
        /* Primary slot. */
        uint64_t ofs;
        uint8_t wt1;
        uint8_t wt2;
        upb_pbdecoder_unpackdispatch(val, &ofs, &wt1, &wt2);

        /* Update offset and repack. */
        ofs = dispatchofs(jc, method, ofs);
        newval = upb_pbdecoder_packdispatch(ofs, wt1, wt2);
        UPB_ASSERT((int64_t)newval > 0);
      } else {
        /* Secondary slot.  Since we have 64 bits for the value, we use an
         * absolute offset. */
        int mcofs = machine_code_ofs2(jc, method, val);
        newval = (uint64_t)((char*)jc->group->jit_code + mcofs);
      }
      ok = upb_inttable_replace(dispatch, key, upb_value_uint64(newval));
      UPB_ASSERT(ok);
    }

    /* Update entry point for this method to point at mc base instead of bc
     * base.  Set this only *after* we have patched the offsets
     * (machine_code_ofs2() uses this). */
    method->code_base.ptr = (char*)jc->group->jit_code + machine_code_ofs(jc, method);

    {
      upb_byteshandler *h = &method->input_handler_;
      upb_byteshandler_setstartstr(h, upb_pbdecoder_startjit, NULL);
      upb_byteshandler_setstring(h, jc->group->jit_code, method->code_base.ptr);
      upb_byteshandler_setendstr(h, upb_pbdecoder_end, method);
    }
  }
}

#ifdef UPB_JIT_LOAD_SO

static void load_so(jitcompiler *jc) {
  /* Dump to a .so file in /tmp and load that, so all the tooling works right
   * (for example, debuggers and profilers will see symbol names for the JIT-ted
   * code).  This is the same goal of the GDB JIT code below, but the GDB JIT
   * interface is only used/understood by GDB.  Hopefully a standard will
   * develop for registering JIT-ted code that all tools will recognize,
   * rendering this obsolete.
   *
   * jc->asmlabels maps:
   *   pclabel -> char* label
   *
   * Use this to build mclabels, which maps:
   *   machine code offset -> char* label
   *
   * Then we can use mclabels to emit the labels as we iterate over the bytes we
   * are outputting. */
  upb_inttable_iter i;
  upb_inttable mclabels;
  upb_inttable_init(&mclabels, UPB_CTYPE_PTR);
  upb_inttable_begin(&i, &jc->asmlabels);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_inttable_insert(&mclabels,
                        dasm_getpclabel(jc, upb_inttable_iter_key(&i)),
                        upb_inttable_iter_value(&i));
  }

  /* We write a .s file in text format, as input to the assembler.
   * Then we run gcc to turn it into a .so file.
   *
   * The last "XXXXXX" will be replaced with something randomly generated by
   * mkstmemp().  We don't add ".s" to this filename because it makes the string
   * processing for mkstemp() and system() more complicated. */
  char s_filename[] = "/tmp/upb-jit-codeXXXXXX";
  int fd = mkstemp(s_filename);
  FILE *f;
  if (fd >= 0 && (f = fdopen(fd, "wb")) != NULL) {
    uint8_t *jit_code = (uint8_t*)jc->group->jit_code;
    size_t linelen = 0;
    size_t i;
    fputs("  .text\n\n", f);
    for (i = 0; i < jc->group->jit_size; i++) {
      upb_value v;
      if (upb_inttable_lookup(&mclabels, i, &v)) {
        const char *label = upb_value_getptr(v);
        /* "X." makes our JIT syms recognizable as such, which we build into
         * other tooling. */
        fprintf(f, "\n\nX.%s:\n", label);
        fprintf(f, "  .globl X.%s", label);
        linelen = 1000;
      }
      if (linelen >= 77) {
        linelen = fprintf(f, "\n  .byte %u", jit_code[i]);
      } else {
        linelen += fprintf(f, ",%u", jit_code[i]);
      }
    }
    fputs("\n", f);
    fclose(f);
  } else {
    fprintf(stderr, "Error opening tmp file for JIT debug output.\n");
    abort();
  }

  /* This is exploitable if you have an adversary on your machine who can write
   * to this tmp directory.  But this is just for debugging so we don't worry
   * too much about that.  It shouldn't be prone to races against concurrent
   * (non-adversarial) upb JIT's because we used mkstemp(). */
  char *cmd = upb_asprintf("gcc -shared -o %s.so -x assembler %s", s_filename,
                           s_filename);
  if (system(cmd) != 0) {
    fprintf(stderr, "Error compiling %s\n", s_filename);
    abort();
  }
  free(cmd);

  char *so_filename = upb_asprintf("%s.so", s_filename);

  /* Some convenience symlinks.
   * This is racy, but just for convenience. */
  int ret;
  unlink("/tmp/upb-jit-code.so");
  unlink("/tmp/upb-jit-code.s");
  ret = symlink(s_filename, "/tmp/upb-jit-code.s");
  ret = symlink(so_filename, "/tmp/upb-jit-code.so");
  UPB_UNUSED(ret);  // We don't care if this fails.

  jc->group->dl = dlopen(so_filename, RTLD_LAZY);
  free(so_filename);
  if (!jc->group->dl) {
    fprintf(stderr, "Couldn't dlopen(): %s\n", dlerror());
    abort();
  }

  munmap(jc->group->jit_code, jc->group->jit_size);
  jc->group->jit_code = dlsym(jc->group->dl, "X.enterjit");
  if (!jc->group->jit_code) {
    fprintf(stderr, "Couldn't find enterjit sym\n");
    abort();
  }

  upb_inttable_uninit(&mclabels);
}

#endif

void upb_pbdecoder_jit(mgroup *group) {
  jitcompiler *jc;
  char *jit_code;
  int dasm_status;

  group->debug_info = NULL;
  group->dl = NULL;

  UPB_ASSERT(group->bytecode);
  jc = newjitcompiler(group);
  emit_static_asm(jc);
  jitbytecode(jc);

  dasm_status = dasm_link(jc, &jc->group->jit_size);
  if (dasm_status != DASM_S_OK) {
    fprintf(stderr, "DynASM error; returned status: 0x%08x\n", dasm_status);
    abort();
  }

  jit_code = mmap(NULL, jc->group->jit_size, PROT_READ | PROT_WRITE,
                  MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
  dasm_encode(jc, jit_code);
  mprotect(jit_code, jc->group->jit_size, PROT_EXEC | PROT_READ);
  jc->group->jit_code = (upb_string_handlerfunc *)jit_code;

#ifdef UPB_JIT_LOAD_SO
  load_so(jc);
#endif

  patchdispatch(jc);

  freejitcompiler(jc);

  /* Now the bytecode is no longer needed. */
  free(group->bytecode);
  group->bytecode = NULL;
}

void upb_pbdecoder_freejit(mgroup *group) {
  if (!group->jit_code) return;
  if (group->dl) {
#ifdef UPB_JIT_LOAD_SO
    dlclose(group->dl);
#endif
  } else {
    munmap((void*)group->jit_code, group->jit_size);
  }
  free(group->debug_info);
}
