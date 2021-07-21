/* Copyright 2012-2021
 * Kaz Kylheku <kaz@kylheku.com>
 * Vancouver, Canada
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

extern val mod_s, bit_s, minus_s;
val make_bignum(void);
val bignum(cnum cn);
val bignum_from_long(long l);
val bignum_from_uintptr(uint_ptr_t u);
val num_from_buffer(mem_t *buf, int bytes);
int num_to_buffer(val num, mem_t *buf, int bytes);
int highest_bit(int_ptr_t n);
val normalize(val bignum);
#if HAVE_DOUBLE_INTPTR_T
val bignum_dbl_ipt(double_intptr_t di);
val bignum_dbl_uipt(double_uintptr_t dui);
#endif
val in_int_ptr_range(val bignum);
#if HAVE_DOUBLE_INTPTR_T
double_intptr_t c_dbl_num(val num);
double_uintptr_t c_dbl_unum(val num);
#endif
val bignum_len(val num);
val cum_norm_dist(val x);
val inv_cum_norm(val p);
val n_choose_k(val n, val k);
val n_perm_k(val n, val k);
val divides(val d, val n);
val tofloat(val obj);
val toint(val obj, val base);
val tofloatz(val obj);
val tointz(val obj, val base);
val width(val num);
val bits(val obj);
val digpow(val n, val base);
val digits(val n, val base);
val poly(val x, val seq);
val rpoly(val x, val seq);

NORETURN void do_mp_error(val self, mp_err code);
void arith_init(void);
void arith_compat_fixup(int compat_ver);
void arith_free_all(void);
