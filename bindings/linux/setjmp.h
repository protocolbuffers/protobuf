/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

// Linux doesn't provide setjmp/longjmp, boo.

typedef void *jmp_buf[3];  // rsp, rbp, rip

static inline int _setjmp(jmp_buf env) {
  register int ret asm ("%eax");
  __asm__ __volatile__ goto (
      "  movq %%rsp, 0(%0)\n"  // Save rsp
      "  movq %%rbp, 8(%0)\n"  // Save rbp
      "  movq $1f,  16(%0)\n"  // Save rip
      "  jmp %l[setup]\n"
      "1:"
      : // No outputs allowed (GCC limitation).
      : "b" (env)  // Input
      : // All registers clobbered except rsp, which save in env.
        "%rcx", "%rdx", "%rdi", "%rsi", "memory", "cc",
        "%r8" , "%r9" , "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
      : setup      // Target labels.
  );
  return ret;

setup:
  return 0;
}

__attribute__((__noreturn__))
static inline void _longjmp(jmp_buf env, int val) {
  __asm__ __volatile__ (
      "  movq  0(%0), %%rsp\n"  // Restore rsp
      "  movq  8(%0), %%rbp\n"  // Restore rbp
      "  movq  %0,    %%rbx\n"  // Restore rbx
      "  jmpq  *16(%0)\n"       // Jump to saved rip
    :  // No output.
    : "r" (env), "a" (val)  // Move val to %eax
  );
  __builtin_unreachable();
}
