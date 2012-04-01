/*
 * Copyright (c) 2003 Peter Wemm.
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

.globl _setjmp, _longjmp

_setjmp:
  movq    %rbx,0(%rdi)                    /* save rbx */
  movq    %rsp,8(%rdi)                    /* save rsp */
  movq    %rbp,16(%rdi)                   /* save rbp */
  movq    %r12,24(%rdi)                   /* save r12 */
  movq    %r13,32(%rdi)                   /* save r13 */
  movq    %r14,40(%rdi)                   /* save r14 */
  movq    %r15,48(%rdi)                   /* save r15 */
  movq    0(%rsp),%rdx                    /* get rta */
  movq    %rdx,56(%rdi)                   /* save rip */
  xorl    %eax,%eax                       /* return(0); */
  ret

_longjmp:
  movq    0(%rdi),%rbx                    /* restore rbx */
  movq    8(%rdi),%rsp                    /* restore rsp */
  movq    16(%rdi),%rbp                   /* restore rbp */
  movq    24(%rdi),%r12                   /* restore r12 */
  movq    32(%rdi),%r13                   /* restore r13 */
  movq    40(%rdi),%r14                   /* restore r14 */
  movq    48(%rdi),%r15                   /* restore r15 */
  movq    56(%rdi),%rdx                   /* get rta */
  movq    %rdx,0(%rsp)                    /* put in return frame */
  xorl    %eax,%eax                       /* return(1); */
  incl    %eax
  ret
