/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2013 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Driver code for the x64 JIT compiler.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <sys/mman.h>
#include "upb/pb/decoder.h"
#include "upb/pb/decoder.int.h"
#include "upb/pb/varint.int.h"
#include "upb/shim/shim.h"

// These defines are necessary for DynASM codegen.
// See dynasm/dasm_proto.h for more info.
#define Dst_DECL jitcompiler *jc
#define Dst_REF (jc->dynasm)
#define Dst (jc)

// In debug mode, make DynASM do internal checks (must be defined before any
// dasm header is included.
#ifndef NDEBUG
#define DASM_CHECKS
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define DECODE_EOF -3

typedef struct {
  mgroup *group;
  uint32_t *pc;

  // This pointer is allocated by dasm_init() and freed by dasm_free().
  struct dasm_State *dynasm;

  // Maps arbitrary void* -> pclabel.
  upb_inttable pclabels;
  upb_inttable pcdefined;

  // For marking labels that should go into the generated code.
  // Maps pclabel -> char* label (string is owned by the table).
  upb_inttable asmlabels;

  // For checking that two asmlabels aren't defined for the same byte.
  int lastlabelofs;

  // The total number of pclabels currently defined.
  uint32_t pclabel_count;

  // Used by DynASM to store globals.
  void **globals;
} jitcompiler;

// Functions called by codegen.
static int pclabel(jitcompiler *jc, const void *here);
static int define_pclabel(jitcompiler *jc, const void *here);
static void asmlabel(jitcompiler *jc, const char *fmt, ...);
static int pcofs(jitcompiler* jc);

#include "dynasm/dasm_proto.h"
#include "dynasm/dasm_x86.h"
#include "upb/pb/compile_decoder_x64.h"

static jitcompiler *newjitcompiler(mgroup *group) {
  jitcompiler *jc = malloc(sizeof(jitcompiler));
  jc->group = group;
  jc->pclabel_count = 0;
  jc->lastlabelofs = -1;
  upb_inttable_init(&jc->pclabels, UPB_CTYPE_UINT32);
  upb_inttable_init(&jc->pcdefined, UPB_CTYPE_BOOL);
  upb_inttable_init(&jc->asmlabels, UPB_CTYPE_PTR);
  jc->globals = malloc(UPB_JIT_GLOBAL__MAX * sizeof(*jc->globals));

  dasm_init(jc, 1);
  dasm_setupglobal(jc, jc->globals, UPB_JIT_GLOBAL__MAX);
  dasm_setup(jc, upb_jit_actionlist);

  return jc;
}

static void freejitcompiler(jitcompiler *jc) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &jc->asmlabels);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    free(upb_value_getptr(upb_inttable_iter_value(&i)));
  }
  upb_inttable_uninit(&jc->asmlabels);
  upb_inttable_uninit(&jc->pclabels);
  upb_inttable_uninit(&jc->pcdefined);
  dasm_free(jc);
  free(jc->globals);
  free(jc);
}

// Returns a pclabel associated with the given arbitrary pointer.
static int pclabel(jitcompiler *jc, const void *here) {
  upb_value v;
  bool found = upb_inttable_lookupptr(&jc->pclabels, here, &v);
  if (!found) {
    upb_value_setuint32(&v, jc->pclabel_count++);
    dasm_growpc(jc, jc->pclabel_count);
    upb_inttable_insertptr(&jc->pclabels, here, v);
  }
  return upb_value_getuint32(v);
}

// Defines a pclabel associated with the given arbitrary pointer.
// May only be called once (to avoid redefining the pclabel).
static int define_pclabel(jitcompiler *jc, const void *here) {
  // Will assert-fail if it already exists.
  upb_inttable_insertptr(&jc->pcdefined, here, upb_value_bool(true));
  return pclabel(jc, here);
}

// Returns a bytecode pc offset relative to the beginning of the group's code.
static int pcofs(jitcompiler *jc) {
  return jc->pc - jc->group->bytecode;
}

static void upb_reg_jit_gdb(jitcompiler *jc);

static int getpclabel(jitcompiler *jc, const void *target) {
  return dasm_getpclabel(jc, pclabel(jc, target));
}

// Given a pcofs relative to method, returns the machine code offset for it
// (relative to the beginning of the machine code).
int nativeofs(jitcompiler *jc, const upb_pbdecodermethod *method, int pcofs) {
  void *target = jc->group->bytecode + method->code_base.ofs + pcofs;
  return getpclabel(jc, target);
}

// Given a pcofs relative to this method's base, returns a machine code offset
// relative to pclabel(dispatch->array) (which is used in jitdispatch as the
// machine code base for dispatch table lookups).
uint32_t dispatchofs(jitcompiler *jc, const upb_pbdecodermethod *method,
                     int pcofs) {
  int ofs1 = getpclabel(jc, method->dispatch.array);
  int ofs2 = nativeofs(jc, method, pcofs);
  assert(ofs1 > 0);
  assert(ofs2 > 0);
  int ret = ofs2 - ofs1;
  assert(ret > 0);
  return ret;
}

// Rewrites the dispatch tables into machine code offsets.
static void patchdispatch(jitcompiler *jc) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &jc->group->methods);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_pbdecodermethod *method = upb_value_getptr(upb_inttable_iter_value(&i));
    method->is_native_ = true;

    upb_inttable *dispatch = &method->dispatch;
    upb_inttable_iter i2;
    upb_inttable_begin(&i2, dispatch);
    for (; !upb_inttable_done(&i2); upb_inttable_next(&i2)) {
      uintptr_t key = upb_inttable_iter_key(&i2);
      if (key == 0) continue;
      uint64_t val = upb_value_getuint64(upb_inttable_iter_value(&i2));
      uint64_t newval;
      if (key <= UPB_MAX_FIELDNUMBER) {
        // Primary slot.
        uint64_t oldofs = val >> 16;
        uint64_t newofs = dispatchofs(jc, method, oldofs);
        newval = (val & 0xffff) | (newofs << 16);
        assert((int64_t)newval > 0);
      } else {
        // Secondary slot.  Since we have 64 bits for the value, we use an
        // absolute offset.
        newval = (uint64_t)(jc->group->jit_code + nativeofs(jc, method, val));
      }
      bool ok = upb_inttable_replace(dispatch, key, upb_value_uint64(newval));
      UPB_ASSERT_VAR(ok, ok);
    }

    // Set this only *after* we have patched the offsets (nativeofs() above
    // reads this).
    method->code_base.ptr = jc->group->jit_code + getpclabel(jc, method);

    upb_byteshandler *h = &method->input_handler_;
    upb_byteshandler_setstartstr(h, upb_pbdecoder_startjit, NULL);
    upb_byteshandler_setstring(h, jc->group->jit_code, method->code_base.ptr);
    upb_byteshandler_setendstr(h, upb_pbdecoder_end, method);
  }
}

// Define for JIT debugging.
#ifdef UPB_JIT_LOAD_SO
static void load_so(jitcompiler *jc) {
  // Dump to a .so file in /tmp and load that, so all the tooling works right
  // (for example, debuggers and profilers will see symbol names for the JIT-ted
  // code).  This is the same goal of the GDB JIT code below, but the GDB JIT
  // interface is only used/understood by GDB.  Hopefully a standard will
  // develop for registering JIT-ted code that all tools will recognize,
  // rendering this obsolete.
  //
  // Requires that gcc is available from the command-line.

  // Convert all asm labels from pclabel offsets to machine code offsets.
  upb_inttable_iter i;
  upb_inttable mclabels;
  upb_inttable_init(&mclabels, UPB_CTYPE_PTR);
  upb_inttable_begin(&i, &jc->asmlabels);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_inttable_insert(&mclabels,
                        dasm_getpclabel(jc, upb_inttable_iter_key(&i)),
                        upb_inttable_iter_value(&i));
  }

  FILE *f = fopen("/tmp/upb-jit-code.s", "w");
  if (f) {
    uint8_t *jit_code = (uint8_t*)jc->group->jit_code;
    fputs("  .text\n\n", f);
    size_t linelen = 0;
    for (size_t i = 0; i < jc->group->jit_size; i++) {
      upb_value v;
      if (upb_inttable_lookup(&mclabels, i, &v)) {
        const char *label = upb_value_getptr(v);
        // "X." makes our JIT syms recognizable as such, which we build into
        // other tooling.
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
    fprintf(stderr, "Couldn't open /tmp/upb-jit-code.s for writing\n");
    abort();
  }

  // TODO: racy
  if (system("gcc -shared -o /tmp/upb-jit-code.so /tmp/upb-jit-code.s") != 0) {
    fprintf(stderr, "Error compiling upb-jit-code.s\n");
    abort();
  }

  jc->group->dl = dlopen("/tmp/upb-jit-code.so", RTLD_LAZY);
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
  group->debug_info = NULL;
  group->dl = NULL;

  assert(group->bytecode);
  jitcompiler *jc = newjitcompiler(group);
  emit_static_asm(jc);
  jitbytecode(jc);

  int dasm_status = dasm_link(jc, &jc->group->jit_size);
  if (dasm_status != DASM_S_OK) {
    fprintf(stderr, "DynASM error; returned status: 0x%08x\n", dasm_status);
    abort();
  }

  char *jit_code = mmap(NULL, jc->group->jit_size, PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
  dasm_encode(jc, jit_code);
  mprotect(jit_code, jc->group->jit_size, PROT_EXEC | PROT_READ);
  upb_reg_jit_gdb(jc);
  jc->group->jit_code = (upb_string_handlerfunc *)jit_code;

#ifdef UPB_JIT_LOAD_SO
  load_so(jc);
#endif

  patchdispatch(jc);

  freejitcompiler(jc);

  // Now the bytecode is no longer needed.
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
    munmap(group->jit_code, group->jit_size);
  }
  free(group->debug_info);
  // TODO: unregister GDB JIT interface.
}

// To debug JIT-ted code with GDB we need to tell GDB about the JIT-ted code
// at runtime.  GDB 7.x+ has defined an interface for doing this, and these
// structure/function defintions are copied out of gdb/jit.h
//
// We need to give GDB an ELF file at runtime describing the symbols we have
// generated.  To avoid implementing the ELF format, we generate an ELF file
// at compile-time and compile it in as a character string.  We can replace
// a few key constants (address of JIT-ted function and its size) by looking
// for a few magic numbers and doing a dumb string replacement.
//
// Unfortunately this approach is showing its limits; we can only define one
// symbol, and this approach only works with GDB.  The .so approach above is
// more reliable.

#ifndef __APPLE__
const unsigned char upb_jit_debug_elf_file[] = {
#include "upb/pb/jit_debug_elf_file.h"
};

typedef enum {
  GDB_JIT_NOACTION = 0,
  GDB_JIT_REGISTER,
  GDB_JIT_UNREGISTER
} jit_actions_t;

typedef struct gdb_jit_entry {
  struct gdb_jit_entry *next_entry;
  struct gdb_jit_entry *prev_entry;
  const char *symfile_addr;
  uint64_t symfile_size;
} gdb_jit_entry;

typedef struct {
  uint32_t version;
  uint32_t action_flag;
  gdb_jit_entry *relevant_entry;
  gdb_jit_entry *first_entry;
} gdb_jit_descriptor;

gdb_jit_descriptor __jit_debug_descriptor = {1, GDB_JIT_NOACTION, NULL, NULL};

void __attribute__((noinline)) __jit_debug_register_code() {
  __asm__ __volatile__("");
}

static void upb_reg_jit_gdb(jitcompiler *jc) {
  // Create debug info.
  size_t elf_len = sizeof(upb_jit_debug_elf_file);
  jc->group->debug_info = malloc(elf_len);
  memcpy(jc->group->debug_info, upb_jit_debug_elf_file, elf_len);
  uint64_t *p = (void *)jc->group->debug_info;
  for (; (void *)(p + 1) <= (void *)jc->group->debug_info + elf_len; ++p) {
    if (*p == 0x12345678) {
      *p = (uintptr_t)jc->group->jit_code;
    }
    if (*p == 0x321) {
      *p = jc->group->jit_size;
    }
  }

  // Register the JIT-ted code with GDB.
  gdb_jit_entry *e = malloc(sizeof(gdb_jit_entry));
  e->next_entry = __jit_debug_descriptor.first_entry;
  e->prev_entry = NULL;
  if (e->next_entry) e->next_entry->prev_entry = e;
  e->symfile_addr = jc->group->debug_info;
  e->symfile_size = elf_len;
  __jit_debug_descriptor.first_entry = e;
  __jit_debug_descriptor.relevant_entry = e;
  __jit_debug_descriptor.action_flag = GDB_JIT_REGISTER;
  __jit_debug_register_code();
}

#else

static void upb_reg_jit_gdb(jitcompiler *jc) { (void)jc; }

#endif
