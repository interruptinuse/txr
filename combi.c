/* Copyright 2010-2024
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

#include <wctype.h>
#include <wchar.h>
#include <signal.h>
#include "config.h"
#include "lib.h"
#include "signal.h"
#include "unwind.h"
#include "eval.h"
#include "hash.h"
#include "combi.h"

static val perm_while_fun(val state)
{
  val self = lit("perm");
  val vec = vecref(state, zero);
  cnum k = c_num(vecref(state, one), self);
  val c = vecref(state, two);
  cnum n = c_num(length(vec), self);
  cnum i, j;

  for (i = k - 1, j = n - k + 1; i >= 0; i--, j++) {
    cnum ci = c_num(c->v.vec[i], self) + 1;

    if (ci >= j) {
      if (i == 0)
        return nil;
      c->v.vec[i] = zero;
    } else {
      c->v.vec[i] = num_fast(ci);
      break;
    }
  }

  return t;
}

static cnum perm_index(cnum n, val b)
{
  cnum i, j;

  for (i = 0, j = 0; i < n; i++, j++) {
    while (b->v.vec[j])
      j++;
  }

  while (b->v.vec[j])
    j++;

  return j;
}

static void perm_gen_fun_common(val state, val out,
                                void (*fill)(val out, cnum i, val v))
{
  val self = lit("perm");
  val vec = vecref(state, zero);
  val kk = vecref(state, one);
  val c = vecref(state, two);
  val nn = length(vec);
  val b = vector(nn, nil);
  cnum k = c_num(kk, self);
  cnum i;

  for (i = 0; i < k; i++) {
    cnum ci = c_num(c->v.vec[i], self);
    cnum j = perm_index(ci, b);
    fill(out, i, vec->v.vec[j]);
    b->v.vec[j] = t;
  }
}

static val perm_init(val vec, val k_null, val seq)
{
  uses_or2;
  val n = length(vec);
  val k = or2(k_null, n);

  if (!fixnump(n))
    uw_throwf(error_s, lit("perm: sequence length ~s is out of fixnum range"),
              n, nao);

  if (gt(k, n)) {
    return nil;
  } else {
    val state = vector(four, nil);
    val c = vector(k, zero);
    set(vecref_l(state, zero), vec);
    set(vecref_l(state, one), k);
    set(vecref_l(state, two), c);
    set(vecref_l(state, three), seq);
    deref(vecref_l(c, negone)) = negone;
    return state;
  }
}

static void perm_vec_gen_fill(val out, cnum i, val v)
{
  out->v.vec[i] = v;
}

static val perm_vec_gen_fun(val state)
{
  val kk = vecref(state, one);
  val out = vector(kk, nil);
  perm_gen_fun_common(state, out, perm_vec_gen_fill);
  return out;
}

static val perm_vec(val vec, val k)
{
  k = default_arg(k, length_vec(vec));

  if (k == zero) {
    return cons(vector(zero, nil), nil);
  } else {
    val state = perm_init(vec, k, nil);
    if (!state)
      return nil;
    return generate(func_f0(state, perm_while_fun),
                    func_f0(state, perm_vec_gen_fun));
  }
}

static void perm_list_gen_fill(val out, cnum i, val v)
{
  val tail = cdr(out);
  val nc = cons(v, nil);
  (void) i;
  if (tail)
    rplacd(tail, nc);
  else
    rplaca(out, nc);
  rplacd(out, nc);
}

static val perm_list_gen_fun(val state)
{
  val out = cons(nil, nil);
  perm_gen_fun_common(state, out, perm_list_gen_fill);
  return car(out);
}

static val perm_list(val list, val k)
{
  if (k == zero || (!k && !list)) {
    return cons(nil, nil);
  } else {
    val state = perm_init(vec_list(list), k, nil);
    if (!state)
      return nil;
    return generate(func_f0(state, perm_while_fun),
                    func_f0(state, perm_list_gen_fun));
  }
}

static void perm_str_gen_fill(val out, cnum i, val v)
{
  out->st.str[i] = c_chr(v);
}

static val perm_str_gen_fun(val state)
{
  val self = lit("perm");
  val kk = vecref(state, one);
  val out = mkustring(kk);
  perm_gen_fun_common(state, out, perm_str_gen_fill);
  out->st.str[c_num(kk, self)] = 0;
  return out;
}

static val perm_str(val str, val k)
{
  k = default_arg(k, length_str(str));

  if (k == zero) {
    return cons(string(L""), nil);
  } else {
    val state = perm_init(vec_list(list_str(str)), k, nil);
    if (!state)
      return nil;
    return generate(func_f0(state, perm_while_fun),
                    func_f0(state, perm_str_gen_fun));
  }
}

static val perm_seq_gen_fun(val state)
{
  val list = perm_list_gen_fun(state);
  val seq = vecref(state, three);
  return make_like(list, seq);
}

static val perm_seq(val seq, val k)
{
  if (k == zero) {
    return cons(make_like(nil, seq), nil);
  } else {
    val vec = vec_seq(seq);
    val state = perm_init(vec, k, seq);
    if (!state)
      return nil;
    return generate(func_f0(state, perm_while_fun),
                    func_f0(state, perm_seq_gen_fun));
  }
}

val perm(val seq, val k)
{
  if (null_or_missing_p(k)) {
    k = nil;
  } else {
    if (!integerp(k))
      type_mismatch(lit("perm: ~s is not an integer"), k, nao);

    if (minusp(k))
      uw_throwf(numeric_error_s, lit("perm: ~s is not a positive integer"),
                k, nao);
  }

  switch (type(seq)) {
  case CONS:
  case LCONS:
  case NIL:
    return perm_list(seq, k);
  case VEC:
    return perm_vec(seq, k);
  case STR:
  case LSTR:
  case LIT:
    return perm_str(seq, k);
  default:
    return perm_seq(seq, k);
  }
}

static val rperm_init(val list, val k, val extra)
{
  val vec = vector(k, list);
  val env = vector(three, nil);
  set(vecref_l(env, zero), list);
  set(vecref_l(env, one), vec);
  if (extra)
    set(vecref_l(env, two), extra);
  return env;
}

static val rperm_while_fun(val env)
{
  val vec = env->v.vec[1];
  return consp(vecref(vec, zero));
}

static val rperm_gen_fun(val env)
{
  val self = lit("rperm");
  val list = env->v.vec[0];
  val vec = env->v.vec[1];
  list_collect_decl(out, ptail);
  cnum i;
  cnum len = c_num(length_vec(vec), self);

  for (i = 0; i < len; i++)
    ptail = list_collect(ptail, car(vec->v.vec[i]));

  for (i = len-1; i >= 0; i--) {
    pop(&vec->v.vec[i]);
    if (atom(vec->v.vec[i]) && i > 0)
      vec->v.vec[i] = list;
    else
      break;
  }

  return out;
}

static val rperm_list(val list, val k)
{
  val env = rperm_init(list, k, nil);
  return generate(func_f0(env, rperm_while_fun),
                  func_f0(env, rperm_gen_fun));
}

static val rperm_vec_gen_fun(val env)
{
  val list = rperm_gen_fun(env);
  return vec_list(list);
}

static val rperm_vec(val ve, val k)
{
  val list = list_vec(ve);
  val env = rperm_init(list, k, nil);
  return generate(func_f0(env, rperm_while_fun),
                  func_f0(env, rperm_vec_gen_fun));
}

static val rperm_str_gen_fun(val env)
{
  val list = rperm_gen_fun(env);
  return cat_str(list, nil);
}

static val rperm_str(val str, val k)
{
  val list = list_str(str);
  val env = rperm_init(list, k, nil);
  return generate(func_f0(env, rperm_while_fun),
                  func_f0(env, rperm_str_gen_fun));
}

static val rperm_seq_gen_fun(val env)
{
  val list = rperm_gen_fun(env);
  val seq = env->v.vec[2];
  return make_like(list, seq);
}

static val rperm_seq(val seq, val k)
{
  val list = list_seq(seq);
  val env = rperm_init(list, k, seq);
  return generate(func_f0(env, rperm_while_fun),
                  func_f0(env, rperm_seq_gen_fun));
}

val rperm(val seq, val k)
{
  if (!integerp(k))
    type_mismatch(lit("rperm: ~s is not an integer"), k, nao);

  if (minusp(k))
    uw_throwf(numeric_error_s, lit("rperm: ~s is not a positive integer"),
              k, nao);

  switch (type(seq)) {
  case NIL:
    if (zerop(k))
      return cons(nil, nil);
    return nil;
  case CONS:
  case LCONS:
    if (zerop(k))
      return cons(nil, nil);
    return rperm_list(seq, k);
  case VEC:
    if (zerop(k))
      return cons(vector(zero, nil), nil);
    return rperm_vec(seq, k);
  case STR:
  case LSTR:
  case LIT:
    if (zerop(k))
      return cons(string(L""), nil);
    return rperm_str(seq, k);
  default:
    return rperm_seq(seq, k);
  }
}

static val k_conses(val list, val k, val self)
{
  cnum i, n = c_num(k, self);
  val iter, out = vector(num(n), nil);

  for (iter = list, i = 0; i < n && iter; iter = cdr(iter), i++)
    out->v.vec[i] = iter;

  return (i >= n) ? out : nil;
}

static val comb_init(val list, val k)
{
  if (zerop(k))
    return nil;
  return k_conses(list, k, lit("comb"));
}

static val comb_while_fun(val state)
{
  if (state) {
    cnum nn = c_num(length_vec(state), lit("comb"));
    return tnil(nn > 0 && state->v.vec[0]);
  }
  return nil;
}

static void comb_gen_fun_common(val state)
{
  cnum i, nn = c_num(length_vec(state), lit("comb"));

  for (i = nn - 1; i >= 0; i--)
  {
    val cur = state->v.vec[i];
    val re = rest(cur);

    if ((i == nn - 1 && re) ||
        (i < nn - 1 && re != state->v.vec[i + 1]))
    {
      state->v.vec[i] = re;
      return;
    } else if (i > 0) {
      val nxt = state->v.vec[i - 1];
      val nxt_r = cdr(nxt);
      val nxt_r_r = cdr(nxt_r);

      if (nxt_r != cur && consp(nxt_r_r)) {
        cnum j;
        for (j = i; j < nn; j++, nxt_r_r = cdr(nxt_r_r))
          state->v.vec[j] = nxt_r_r;
      }
    }
  }

  state->v.vec[0] = nil;
}

static val comb_list_gen_fun(val state)
{
  val out = mapcar_listout(car_f, state);
  comb_gen_fun_common(state);
  return out;
}

static val comb_list(val list, val k)
{
  val state = comb_init(list, k);
  return state ? generate(func_f0(state, comb_while_fun),
                          func_f0(state, comb_list_gen_fun))
               : nil;
}

static val comb_vec_gen_fun(val state)
{
  val self = lit("comb");
  val nn = length_vec(state);
  cnum i, n = c_num(nn, self);
  val out = vector(nn, nil);

  for (i = 0; i < n; i++)
    out->v.vec[i] = car(state->v.vec[i]);

  comb_gen_fun_common(state);
  return out;
}

static val comb_vec(val vec, val k)
{
  val state = comb_init(list_vec(vec), k);
  return generate(func_f0(state, comb_while_fun),
                  func_f0(state, comb_vec_gen_fun));
}

static val comb_str_gen_fun(val state)
{
  val self = lit("comb");
  val nn = length_vec(state);
  cnum i, n = c_num(nn, self);
  val out = mkustring(nn);

  out->st.str[n] = 0;

  for (i = 0; i < n; i++)
    out->st.str[i] = c_chr(car(state->v.vec[i]));

  comb_gen_fun_common(state);
  return out;
}

static val comb_str(val str, val k)
{
  val state = comb_init(list_str(str), k);
  return generate(func_f0(state, comb_while_fun),
                  func_f0(state, comb_str_gen_fun));
}

static val comb_hash_gen_fun(val hstate)
{
  val self = lit("comb");
  cons_bind (state, hash, hstate);
  val nn = length_vec(state);
  cnum i, n = c_num(nn, self);
  val out = make_similar_hash(hash);

  for (i = 0; i < n; i++) {
    val pair = car(state->v.vec[i]);
    sethash(out, car(pair), cdr(pair));
  }

  comb_gen_fun_common(state);
  return out;
}

static val comb_hash(val hash, val k)
{
  val state = comb_init(hash_alist(hash), k);
  val hstate = cons(state, hash);
  return generate(func_f0(state, comb_while_fun),
                  func_f0(hstate, comb_hash_gen_fun));
}

static val comb_seq_gen_fun(val sstate)
{
  cons_bind (state, seq, sstate);
  val list = comb_list_gen_fun(state);
  return make_like(list, seq);
}

static val comb_seq(val seq, val k)
{
  val state = comb_init(list_seq(seq), k);
  val sstate = cons(state, seq);
  return generate(func_f0(state, comb_while_fun),
                  func_f0(sstate, comb_seq_gen_fun));
}

val comb(val seq, val k)
{
  if (!integerp(k))
    type_mismatch(lit("comb: ~s is not an integer"), k, nao);

  if (minusp(k))
    uw_throwf(numeric_error_s, lit("comb: ~s is not a positive integer"),
              k, nao);

  switch (type(seq)) {
  case CONS:
  case LCONS:
  case NIL:
    if (k == zero)
      return cons(nil, nil);
    return comb_list(seq, k);
  case VEC:
    if (k == zero)
      return cons(vector(zero, nil), nil);
    if (gt(k, length(seq)))
      return nil;
    return comb_vec(seq, k);
  case STR:
  case LSTR:
  case LIT:
    if (k == zero)
      return cons(string(L""), nil);
    if (gt(k, length(seq)))
      return nil;
    return comb_str(seq, k);
  default:
    if (hashp(seq)) {
      if (k == zero)
        return cons(make_similar_hash(seq), nil);
      if (gt(k, hash_count(seq)))
        return nil;
      return comb_hash(seq, k);
    }
    return comb_seq(seq, k);
  }
}

static val rcomb_while_fun(val state)
{
  return car(state);
}

static void rcomb_gen_fun_common(val state)
{
  val iter;
  val next;

  for (iter = state, next = cdr(state);
       consp(iter);
       iter = next, next = cdr(iter))
  {
    val curr = first(iter);
    val curr_rest = rest(curr);

    if (consp(curr_rest)) {
      val jter;
      for (jter = state; jter != next; jter = cdr(jter))
        rplaca(jter, curr_rest);
      return;
    } else if (next) {
      val next = second(iter);
      if (curr != next)
        rplaca(iter, rest(next));
    }
  }

  rplaca(state, nil);
}

static val rcomb_list_gen_fun(val state)
{
  val out = nreverse(mapcar(car_f, state));
  rcomb_gen_fun_common(state);
  return out;
}

static val rcomb_list(val list, val k)
{
  val state = nreverse(list_vec(vector(k, list)));
  return generate(func_f0(state, rcomb_while_fun),
                  func_f0(state, rcomb_list_gen_fun));
}

static val rcomb_vec_gen_fun(val state)
{
  val self = lit("rcomb");
  val nn = length_list(state);
  cnum i, n = c_num(nn, self);
  val iter, out = vector(nn, nil);

  for (iter = state, i = n - 1; i >= 0; iter = cdr(iter), i--)
    out->v.vec[i] = car(car(iter));

  rcomb_gen_fun_common(state);
  return out;
}

static val rcomb_vec(val vec, val k)
{
  val state = nreverse(list_vec(vector(k, list_vec(vec))));
  return generate(func_f0(state, rcomb_while_fun),
                  func_f0(state, rcomb_vec_gen_fun));
}

static val rcomb_str_gen_fun(val state)
{
  val self = lit("rcomb");
  val nn = length_list(state);
  cnum i, n = c_num(nn, self);
  val iter, out = mkustring(nn);

  out->st.str[n] = 0;

  for (iter = state, i = n - 1; i >= 0; iter = cdr(iter), i--)
    out->st.str[i] = c_chr(car(car(iter)));

  rcomb_gen_fun_common(state);
  return out;
}

static val rcomb_str(val str, val k)
{
  val state = nreverse(list_vec(vector(k, list_str(str))));
  return generate(func_f0(state, rcomb_while_fun),
                  func_f0(state, rcomb_str_gen_fun));
}

static val rcomb_seq_gen_fun(val sstate)
{
  cons_bind (state, seq, sstate);
  val list = rcomb_list_gen_fun(state);
  return make_like(list, seq);
}

static val rcomb_seq(val seq, val k)
{
  val state = nreverse(list_vec(vector(k, list_seq(seq))));
  val sstate = cons(state, seq);
  return generate(func_f0(state, rcomb_while_fun),
                  func_f0(sstate, rcomb_seq_gen_fun));
}

val rcomb(val seq, val k)
{
  if (!integerp(k))
    type_mismatch(lit("rcomb: ~s is not an integer"), k, nao);

  if (minusp(k))
    uw_throwf(numeric_error_s, lit("rcomb: ~s is not a positive integer"),
              k, nao);

  switch (type(seq)) {
  case CONS:
  case LCONS:
  case NIL:
    if (k == zero)
      return cons(nil, nil);
    return rcomb_list(seq, k);
  case VEC:
    if (k == zero)
      return cons(vector(zero, nil), nil);
    return rcomb_vec(seq, k);
  case STR:
  case LSTR:
  case LIT:
    if (k == zero)
      return cons(string(L""), nil);
    return rcomb_str(seq, k);
  default:
    return rcomb_seq(seq, k);
  }
}
