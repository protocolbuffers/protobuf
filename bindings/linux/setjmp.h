/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

// Linux doesn't provide setjmp/longjmp, boo.

typedef void *jmp_buf[8];

extern int _setjmp(jmp_buf env);
__attribute__((__noreturn__)) extern void _longjmp(jmp_buf env, int val);
