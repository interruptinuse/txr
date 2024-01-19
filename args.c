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

#include <stddef.h>
#include <signal.h>
#include <string.h>
#include "config.h"
#include "alloca.h"
#include "lib.h"
#include "signal.h"
#include "unwind.h"
#include "gc.h"
#include "args.h"

val args_cons_list(varg args);

val args_add_checked(val name, varg args, val arg)
{
  if (args->fill >= args->argc)
    uw_throwf(assert_s, lit("~a: argument list size exceeded"), name, nao);
  return args_add(args, arg);
}

void args_normalize_exact(varg args, cnum fill)
{
  bug_unless (fill <= args->argc);

  while (args->fill > fill)
    args->list = cons(z(args->arg[--args->fill]), args->list);

  while (args->fill < fill && args->list)
    args_add(args, pop(&args->list));

}

void args_normalize_least(varg args, cnum minfill)
{
  bug_unless (minfill <= args->argc);

  while (args->fill < minfill && args->list)
    args_add(args, pop(&args->list));
}

void args_normalize_fill(varg args, cnum minfill, cnum maxfill)
{
  args_normalize_least(args, maxfill);

  if (args->fill >= minfill)
    while (args->fill < maxfill)
      args_add(args, colon_k);
}

val args_get_checked(val name, varg args, cnum *arg_index)
{
  if (*arg_index >= args->fill && !args->list)
    uw_throwf(assert_s, lit("~a: insufficient arguments"), name, nao);
  return args_get(args, arg_index);
}

varg args_copy(varg to, varg from)
{
  to->fill = from->fill;
  to->list = from->list;
  memcpy(to->arg, from->arg, sizeof *from->arg * from->fill);
  return to;
}

varg args_copy_zap(varg to, varg from)
{
  args_copy(to, from);
  memset(from->arg, 0, sizeof *from->arg * from->fill);
  return to;
}

varg args_cat(varg to, varg from)
{
  size_t size = sizeof *from->arg * from->fill;
  to->list = from->list;
  memcpy(to->arg + to->fill, from->arg, size);
  to->fill += from->fill;
  return to;
}

varg args_cat_from(varg to, varg from, cnum index)
{
  size_t size = sizeof *from->arg * (from->fill - index);
  to->list = from->list;
  memcpy(to->arg + to->fill, from->arg + index, size);
  to->fill += from->fill - index;
  return to;
}

varg args_cat_zap(varg to, varg from)
{
  size_t size = sizeof *from->arg * from->fill;
  to->list = from->list;
  memcpy(to->arg + to->fill, from->arg, size);
  to->fill += from->fill;
  memset(from->arg, 0, size);
  return to;
}

varg args_cat_zap_from(varg to, varg from, cnum index)
{
  size_t size = sizeof *from->arg * (from->fill - index);
  to->list = from->list;
  memcpy(to->arg + to->fill, from->arg + index, size);
  to->fill += from->fill - index;
  memset(from->arg + index, 0, size);
  return to;
}

varg args_copy_reverse(varg to, varg from, cnum nargs)
{
  cnum i, index = 0;

  for (i = nargs - 1; i >= 0; i--)
    to->arg[i] = args_get(from, &index);

  to->fill = nargs;
  return to;
}

val args_copy_to_list(varg args)
{
  list_collect_decl (out, ptail);
  cnum i;

  for (i = 0; i < args->fill; i++)
    ptail = list_collect(ptail, args->arg[i]);

  list_collect_nconc(ptail, args->list);

  return out;
}

void args_for_each(varg args,
                   int (*fn)(val arg, int ix, mem_t *ctx),
                   mem_t *ctx)
{
  int i;
  val iter;

  for (i = 0; i < args->fill; i++)
    if (fn(args->arg[i], i, ctx))
      args->arg[i] = nil;

  for (iter = args->list; iter; iter = cdr(iter), i++)
    if (fn(car(iter), i, ctx))
      rplaca(iter, nil);
}

struct args_bool_ctx {
  struct args_bool_key *akv;
  int n;
  val *next_arg_store;
};

static int args_key_check_store(val arg, int ix, mem_t *ctx)
{
  struct args_bool_ctx *acx = coerce(struct args_bool_ctx *, ctx);
  int i, n = acx->n;

  (void) ix;

  if (acx->next_arg_store != 0) {
    *acx->next_arg_store = arg;
    acx->next_arg_store = 0;
    return 0;
  }

  for (i = 0; i < n; i++) {
    struct args_bool_key *akl = &acx->akv[i];
    if (akl->key == arg) {
      if (akl->arg_p)
        acx->next_arg_store = akl->store;
      else
        *akl->store = t;
      break;
    }
  }

  return 0;
}

void args_keys_extract(varg args, struct args_bool_key *akv, int n)
{
  if (n > 0) {
    struct args_bool_ctx acx;
    acx.akv = akv;
    acx.n = n;
    acx.next_arg_store = 0;
    args_for_each(args, args_key_check_store, coerce(mem_t *, &acx));
  }
}

val dyn_args(varg args, val car, val cdr)
{
  size_t size = offsetof(struct args, arg) + sizeof (val) * args->argc;
  varg copy = coerce(varg , chk_copy_obj(coerce(mem_t *, args), size));
  val obj = make_obj();

  obj->a.type = DARG;
  obj->a.car = car;
  obj->a.cdr = cdr;
  obj->a.args = copy;

  return obj;
}
