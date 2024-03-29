/* Copyright 2009-2024
 * Kaz Kylheku <kaz@kylheku.com>
 * Vancouver, Canada
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if __MINGW32__ || (__CYGWIN__ && __i386__)
#define DEFUN(NAME) \
.global _ ## NAME ; \
_ ## NAME: ;
#elif __APPLE__ && __arm64__
#define DEFUN(NAME) \
.globl _ ## NAME %% \
.p2align 2 %% \
_ ## NAME: ;
#elif __APPLE__
#define DEFUN(NAME) \
.globl _ ## NAME ; \
_ ## NAME: ;
#elif (__arm__ && !__thumb__) || __aarch64__
#define DEFUN(NAME) \
.text ; \
.align 4 ; \
.global NAME ; \
.type NAME, %function ; \
NAME: ;
#elif __arm__ && __thumb__
#define DEFUN(NAME) \
.text ; \
.align 4 ; \
.global NAME ; \
.thumb ; \
.thumb_func ; \
.type NAME, %function ; \
NAME: ;
#elif __CYGWIN__ && __x86_64__
#define DEFUN(NAME) \
.globl NAME ; \
.def NAME; .scl 2; .type 32; .endef; \
NAME: ;
#elif __riscv
#define DEFUN(NAME) \
.global NAME ; \
.type NAME, %function ; \
NAME: ;
#else
#define DEFUN(NAME) \
.global NAME ; \
.type NAME, @function ; \
NAME: ;
#endif

#if __arm__
#define SIGIL() %
#else
#define SIGIL() @
#endif

#if __i386__

#define JEIP  0
#define JESP  4
#define JEBP  8
#define JEBX 12
#define JESI 16
#define JEDI 20

#define RETA  0
#define ARG1  4
#define ARG2  8

DEFUN(jmp_save)
  movl RETA(%esp), %ecx
  movl ARG1(%esp), %eax
  movl %ecx, JEIP(%eax)
  leal ARG1(%esp), %ecx
  movl %ecx, JESP(%eax)
  movl %ebp, JEBP(%eax)
  movl %ebx, JEBX(%eax)
  movl %esi, JESI(%eax)
  movl %edi, JEDI(%eax)
  xorl %eax, %eax
  ret

DEFUN(jmp_restore)
  movl ARG1(%esp), %edx
  movl ARG2(%esp), %eax
  mov JEDI(%edx),%edi
  mov JESI(%edx),%esi
  mov JEBX(%edx),%ebx
  mov JEBP(%edx),%ebp
  mov JESP(%edx),%ecx
  mov %ecx,%esp
  mov JEIP(%edx),%ecx
  jmp *%ecx

#elif __x86_64__ && !__CYGWIN__

#define JRIP  0
#define JRSP  8
#define JRBP 16
#define JRBX 24
#define JR12 32
#define JR13 40
#define JR14 48
#define JR15 56

#define RETA  0
#define ARG2  8

DEFUN(jmp_save)
  mov RETA(%rsp), %rdx
  mov %rdx, JRIP(%rdi)
  lea ARG2(%rsp), %rdx
  mov %rdx, JRSP(%rdi)
  mov %rbp, JRBP(%rdi)
  mov %rbx, JRBX(%rdi)
  mov %r12, JR12(%rdi)
  mov %r13, JR13(%rdi)
  mov %r14, JR14(%rdi)
  mov %r15, JR15(%rdi)
  xor %rax, %rax
  ret

DEFUN(jmp_restore)
  mov JR15(%rdi), %r15
  mov JR14(%rdi), %r14
  mov JR13(%rdi), %r13
  mov JR12(%rdi), %r12
  mov JRBX(%rdi), %rbx
  mov JRBP(%rdi), %rbp
  mov JRSP(%rdi), %rdx
  mov %rdx, %rsp
  mov JRIP(%rdi), %rdx
  mov %rsi, %rax
  jmp *%rdx

#elif __x86_64__ && __CYGWIN__

#define JRIP  0
#define JRSP  8
#define JRBP 16
#define JRBX 24
#define JR12 32
#define JR13 40
#define JR14 48
#define JR15 56
#define JRSI 64
#define JRDI 72

#define RETA  0
#define ARG2  8

DEFUN(jmp_save)
  mov RETA(%rsp), %rax
  mov %rax, JRIP(%rcx)
  lea ARG2(%rsp), %rax
  mov %rax, JRSP(%rcx)
  mov %rbp, JRBP(%rcx)
  mov %rbx, JRBX(%rcx)
  mov %r12, JR12(%rcx)
  mov %r13, JR13(%rcx)
  mov %r14, JR14(%rcx)
  mov %r15, JR15(%rcx)
  mov %rsi, JRSI(%rcx)
  mov %rdi, JRDI(%rcx)
  xor %rax, %rax
  ret

DEFUN(jmp_restore)
  mov JRDI(%rcx), %rdi
  mov JRSI(%rcx), %rsi
  mov JR15(%rcx), %r15
  mov JR14(%rcx), %r14
  mov JR13(%rcx), %r13
  mov JR12(%rcx), %r12
  mov JRBX(%rcx), %rbx
  mov JRBP(%rcx), %rbp
  mov JRSP(%rcx), %rax
  mov %rax, %rsp
  mov JRIP(%rcx), %rcx
  mov %rdx, %rax
  jmp *%rcx


#elif __arm__ && !__thumb__

DEFUN(jmp_save)
  stmia r0, {r4, r5, r6, r7, r8, r9, r10, fp, sp, lr}
  mov   r0, #0
  bx    lr

DEFUN(jmp_restore)
  ldmia r0, {r4, r5, r6, r7, r8, r9, r10, fp, sp, lr}
  mov   r0, r1
  bx    lr

#elif __arm__ && __thumb__

DEFUN(jmp_save)
  mov   r2, r0
  mov   r3, lr
  stmia r0!, {r3, r4, r5, r6, r7}
  mov   r3, r8
  mov   r4, r9
  mov   r5, r10
  mov   r6, fp
  mov   r7, sp
  stmia r0!, {r3, r4, r5, r6, r7}
  ldmia r2!, {r3, r4, r5, r6, r7}
  mov   r0, #0
  bx    lr

DEFUN(jmp_restore)
  mov   r2, r0
  add   r0, #5*4
  ldmia r0!, {r3, r4, r5, r6, r7}
  mov   r8, r3
  mov   r9, r4
  mov   r10, r5
  mov   fp, r6
  mov   sp, r7
  ldmia r2!, {r3, r4, r5, r6, r7}
  mov   r0, r1
  bx    r3

#elif __PPC64__


DEFUN(jmp_save)
  mflr  %r11
  mfcr  %r12
#if __ALTIVEC__
  stvx  %v31, 0, %r3
  addi  %r3, %r3, 32
#endif
  std   %r1, 0(%r3)
  std   %r2, 8(%r3)
  std   %r11, 16(%r3)
  std   %r12, 24(%r3)
  std   %r13, 32(%r3)
  std   %r14, 40(%r3)
  std   %r15, 48(%r3)
  std   %r16, 56(%r3)
  std   %r17, 64(%r3)
  std   %r18, 72(%r3)
  std   %r19, 80(%r3)
  std   %r20, 88(%r3)
  std   %r21, 96(%r3)
  std   %r22, 104(%r3)
  std   %r23, 112(%r3)
  std   %r24, 120(%r3)
  std   %r25, 128(%r3)
  std   %r26, 136(%r3)
  std   %r27, 144(%r3)
  std   %r28, 152(%r3)
  std   %r29, 160(%r3)
  std   %r30, 168(%r3)
  std   %r31, 176(%r3)
#if __ALTIVEC__
  std   %r11, 184(%r3)	/* clobber padding */
#endif
  li    %r3, 0
  blr

DEFUN(jmp_restore)
#if __ALTIVEC__
  lvx   %v31, 0, %r3
  addi  %r3, %r3, 32
#endif
  ld    %r31, 176(%r3)
  ld    %r30, 168(%r3)
  ld    %r29, 160(%r3)
  ld    %r28, 152(%r3)
  ld    %r27, 144(%r3)
  ld    %r26, 136(%r3)
  ld    %r25, 128(%r3)
  ld    %r24, 120(%r3)
  ld    %r23, 112(%r3)
  ld    %r22, 104(%r3)
  ld    %r21, 96(%r3)
  ld    %r20, 88(%r3)
  ld    %r17, 64(%r3)
  ld    %r19, 80(%r3)
  ld    %r18, 72(%r3)
  ld    %r16, 56(%r3)
  ld    %r15, 48(%r3)
  ld    %r14, 40(%r3)
  ld    %r13, 32(%r3)
  ld    %r12, 24(%r3)
  ld    %r11, 16(%r3)
  ld    %r2, 8(%r3)
  ld    %r1, 0(%r3)
  mtcr  %r12
  mtlr  %r11
  mr    %r3, %r4
  blr

#elif __aarch64__ || __arm64__

#if __APPLE__
  .section __TEXT,__text,regular,pure_instructions
#endif

DEFUN(jmp_save)
  stp x19, x20, [x0, 0]
  stp x21, x22, [x0, 16]
  stp x23, x24, [x0, 32]
  stp x25, x26, [x0, 48]
  stp x27, x28, [x0, 64]
  stp x29, x30, [x0, 80]
  stp d8, d9, [x0, 96]
  stp d10, d11, [x0, 112]
  stp d12, d13, [x0, 128]
  stp d14, d15, [x0, 144]
  mov x16, sp
  str x16, [x0, 160]
  mov w0, #0
  ret

DEFUN(jmp_restore)
  ldp x19, x20, [x0, 0]
  ldp x21, x22, [x0, 16]
  ldp x23, x24, [x0, 32]
  ldp x25, x26, [x0, 48]
  ldp x27, x28, [x0, 64]
  ldp x29, x30, [x0, 80]
  ldp d8, d9, [x0, 96]
  ldp d10, d11, [x0, 112]
  ldp d12, d13, [x0, 128]
  ldp d14, d15, [x0, 144]
  ldr x16, [x0, 160]
  mov sp, x16
  mov w0, w1
  br x30

#if __APPLE__
  .subsections_via_symbols
#endif

#elif _MIPS_SZPTR == 32

  .set noreorder

DEFUN(jmp_save)
  sw $16, 0($4)
  sw $17, 4($4)
  sw $18, 8($4)
  sw $19, 12($4)
  sw $20, 16($4)
  sw $21, 20($4)
  sw $22, 24($4)
  sw $23, 28($4)
  sw $28, 32($4)
  sw $29, 36($4)
  sw $30, 40($4)
  sw $ra, 44($4)
  jr $ra
  li $2, 0

DEFUN(jmp_restore)
  lw $16, 0($4)
  lw $17, 4($4)
  lw $18, 8($4)
  lw $19, 12($4)
  lw $20, 16($4)
  lw $21, 20($4)
  lw $22, 24($4)
  lw $23, 28($4)
  lw $28, 32($4)
  lw $29, 36($4)
  lw $ra, 44($4)
  lw $30, 40($4)
  jr $ra
  move $2, $5

#elif _MIPS_SZPTR == 64

  .set noreorder

DEFUN(jmp_save)
  sd $16, 0($4)
  sd $17, 8($4)
  sd $18, 16($4)
  sd $19, 24($4)
  sd $20, 32($4)
  sd $21, 40($4)
  sd $22, 48($4)
  sd $23, 56($4)
  sd $28, 64($4)
  sd $29, 72($4)
  sd $30, 80($4)
  sd $ra, 88($4)
  jr $ra
  li $2, 0

DEFUN(jmp_restore)
  ld $16, 0($4)
  ld $17, 8($4)
  ld $18, 16($4)
  ld $19, 24($4)
  ld $20, 32($4)
  ld $21, 40($4)
  ld $22, 48($4)
  ld $23, 56($4)
  ld $28, 64($4)
  ld $29, 72($4)
  ld $ra, 80($4)
  ld $30, 88($4)
  jr $ra
  move $2, $5

#elif __riscv

DEFUN(jmp_save)
  sd ra,    0(a0)
  sd sp,    8(a0)
  sd fp,    16(a0)
  sd s1,    24(a0)
  sd s2,    32(a0)
  sd s3,    40(a0)
  sd s4,    48(a0)
  sd s5,    56(a0)
  sd s6,    64(a0)
  sd s7,    72(a0)
  sd s8,    80(a0)
  sd s9,    88(a0)
  sd s10,   96(a0)
  sd s11,   104(a0)
#if 0 && !__riscv_float_abi_soft
  fsd fs0,  112(a0)
  fsd fs1,  120(a0)
  fsd fs2,  128(a0)
  fsd fs3,  136(a0)
  fsd fs4,  144(a0)
  fsd fs5,  152(a0)
  fsd fs6,  160(a0)
  fsd fs7,  168(a0)
  fsd fs8,  176(a0)
  fsd fs9,  184(a0)
  fsd fs10, 192(a0)
  fsd fs11, 200(a0)
#endif
  li a0,    0
  ret

DEFUN(jmp_restore)
  ld ra,    0(a0)
  ld sp,    8(a0)
  ld fp,    16(a0)
  ld s1,    24(a0)
  ld s2,    32(a0)
  ld s3,    40(a0)
  ld s4,    48(a0)
  ld s5,    56(a0)
  ld s6,    64(a0)
  ld s7,    72(a0)
  ld s8,    80(a0)
  ld s9,    88(a0)
  ld s10,   96(a0)
  ld s11,   104(a0)
#if 0 && !__riscv_float_abi_soft
  fld fs0,  112(a0)
  fld fs1,  120(a0)
  fld fs2,  128(a0)
  fld fs3,  136(a0)
  fld fs4,  144(a0)
  fld fs5,  152(a0)
  fld fs6,  160(a0)
  fld fs7,  168(a0)
  fld fs8,  176(a0)
  fld fs9,  184(a0)
  fld fs10, 192(a0)
  fld fs11, 200(a0)
#endif
  mv a0,    a1
  ret

#elif __loongarch64

DEFUN(jmp_save)
  st.d  $ra, $a0, 0
  st.d  $sp, $a0, 8
  st.d  $fp, $a0, 16
  st.d  $s0, $a0, 24
  st.d  $s1, $a0, 32
  st.d  $s2, $a0, 40
  st.d  $s3, $a0, 48
  st.d  $s4, $a0, 56
  st.d  $s5, $a0, 64
  st.d  $s6, $a0, 72
  st.d  $s7, $a0, 80
  st.d  $s8, $a0, 88
#if 0 && !__loongarch64_soft_float
  fst.d  $fs0, $a0, 96
  fst.d  $fs1, $a0, 104
  fst.d  $fs2, $a0, 112
  fst.d  $fs3, $a0, 120
  fst.d  $fs4, $a0, 128
  fst.d  $fs5, $a0, 136
  fst.d  $fs6, $a0, 144
  fst.d  $fs7, $a0, 152
#endif
  xor $v0, $v0, $v0
  jirl $zero, $ra, 0

DEFUN(jmp_restore)
  ld.d $ra, $a0, 0
  ld.d $sp, $a0, 8
  ld.d $fp, $a0, 16
  ld.d $s0, $a0, 24
  ld.d $s1, $a0, 32
  ld.d $s2, $a0, 40
  ld.d $s3, $a0, 48
  ld.d $s4, $a0, 56
  ld.d $s5, $a0, 64
  ld.d $s6, $a0, 72
  ld.d $s7, $a0, 80
  ld.d $s8, $a0, 88
#if 0 && !__loongarch64_soft_float
  fld.d $fs0, $a0, 96
  fld.d $fs1, $a0, 104
  fld.d $fs2, $a0, 112
  fld.d $fs3, $a0, 120
  fld.d $fs4, $a0, 128
  fld.d $fs5, $a0, 136
  fld.d $fs6, $a0, 144
  fld.d $fs7, $a0, 152
#endif
  add.d $v0, $a1, $zero
  jirl  $zero, $ra, 0

#else
#error port me!
#endif

#if !defined(__APPLE__) && !defined(__MINGW32__) && !defined(__CYGWIN__)
/* This is needed so our assembly code doesn't cause
   the program to require an executable stack! */
.section .note.GNU-stack,"",SIGIL()progbits
#endif
