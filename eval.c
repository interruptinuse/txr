/* Copyright 2010-2021
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include "config.h"
#include "alloca.h"
#include "lib.h"
#include "gc.h"
#include "args.h"
#include "arith.h"
#include "signal.h"
#include "unwind.h"
#include "regex.h"
#include "stream.h"
#include "y.tab.h"
#include "parser.h"
#include "hash.h"
#include "debug.h"
#include "match.h"
#include "txr.h"
#include "combi.h"
#include "lisplib.h"
#include "struct.h"
#include "cadr.h"
#include "filter.h"
#include "tree.h"
#include "vm.h"
#include "eval.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

typedef val (*opfun_t)(val, val);

struct c_var {
  val *loc;
  val bind;
};

val top_vb, top_fb, top_mb, top_smb, special, builtin;
val op_table, pm_table;
val dyn_env;

val eval_error_s;
val dwim_s, progn_s, prog1_s, prog2_s, sys_blk_s;
val let_s, let_star_s, lambda_s, call_s, dvbind_s;
val sys_catch_s, handler_bind_s, cond_s, if_s, iflet_s, when_s, usr_var_s;
val defvar_s, defvarl_s, defparm_s, defparml_s, defun_s, defmacro_s, macro_s;
val tree_case_s, tree_bind_s, mac_param_bind_s;
val sys_mark_special_s;
val caseq_s, caseql_s, casequal_s;
val caseq_star_s, caseql_star_s, casequal_star_s;
val memq_s, memql_s, memqual_s;
val eq_s, eql_s, equal_s, less_s;
val car_s, cdr_s, not_s, vecref_s;
val setq_s, setqf_s, sys_lisp1_value_s, sys_lisp1_setq_s;
val sys_l1_val_s, sys_l1_setq_s;
val inc_s, zap_s;
val for_s, for_star_s, each_s, each_star_s, collect_each_s, collect_each_star_s;
val for_op_s, each_op_s;
val append_each_s, append_each_star_s, while_s, while_star_s, until_star_s;
val dohash_s;
val uw_protect_s, return_s, return_from_s, sys_abscond_from_s, block_star_s;
val list_s, list_star_s, append_s, apply_s, sys_apply_s, iapply_s;
val gen_s, gun_s, generate_s, rest_s;
val promise_s, promise_forced_s, promise_inprogress_s, force_s;
val op_s, identity_s;
val hash_lit_s, hash_construct_s, struct_lit_s, qref_s, uref_s;
val vector_lit_s, vec_list_s, tree_lit_s, tree_construct_s;
val macro_time_s, macrolet_s;
val defsymacro_s, symacrolet_s, prof_s, switch_s, struct_s;
val fbind_s, lbind_s, flet_s, labels_s;
val load_path_s, load_recursive_s;
val load_time_s, load_time_lit_s;
val eval_only_s, compile_only_s;

val special_s, unbound_s;
val whole_k, form_k, symacro_k;

val last_form_evaled;

val call_f, iter_begin_f, iter_from_binding_f, iter_more_f;
val iter_item_f, iter_step_f;

val origin_hash;

val make_env(val vbindings, val fbindings, val up_env)
{
  val env = make_obj();
  env->e.type = ENV;
  env->e.vbindings = vbindings;
  env->e.fbindings = fbindings;
  env->e.up_env = up_env;
  return env;
}

val copy_env(val oenv)
{
  type_check(lit("copy-env"), oenv, ENV);

  {
    val nenv = make_obj();

    nenv->e.type = ENV;
    nenv->e.vbindings = copy_alist(oenv->e.vbindings);
    nenv->e.fbindings = copy_alist(oenv->e.fbindings);
    nenv->e.up_env = oenv->e.up_env;
    return nenv;
  }
}

val deep_copy_env(val oenv)
{
  type_check(lit("deep-copy-env"), oenv, ENV);

  {
    val nenv = make_obj();
    nenv->e.type = ENV;
    nenv->e.vbindings = copy_alist(oenv->e.vbindings);
    nenv->e.fbindings = copy_alist(oenv->e.fbindings);

    nenv->e.up_env = if2(oenv->e.up_env != nil,
                         deep_copy_env(oenv->e.up_env));
    return nenv;
  }
}

/*
 * Wrapper for performance reasons: don't make make_env
 * process default arguments.
 */
static val make_env_intrinsic(val vbindings, val fbindings, val up_env)
{
  vbindings = default_null_arg(vbindings);
  fbindings = default_null_arg(fbindings);
  up_env = default_null_arg(up_env);
  return make_env(vbindings, fbindings, up_env);
}

val env_fbind(val env, val sym, val fun)
{
  val self = lit("env-fbind");

  if (env) {
    val cell;
    type_check(self, env, ENV);
    cell = acons_new_c(sym, nulloc, mkloc(env->e.fbindings, env));
    return rplacd(cell, fun);
  } else {
    loc pcdr = gethash_l(self, top_fb, sym, nulloc);
    val cell = deref(pcdr);
    if (cell)
      return rplacd(cell, fun);
    return set(pcdr, cons(sym, fun));
  }
}

val env_vbind(val env, val sym, val obj)
{
  val self = lit("env-vbind");

  if (env) {
    val cell;
    type_check(self, env, ENV);
    cell = acons_new_c(sym, nulloc, mkloc(env->e.vbindings, env));
    return rplacd(cell, obj);
  } else {
    loc pcdr = gethash_l(self, top_vb, sym, nulloc);
    val cell = deref(pcdr);
    if (cell)
      return rplacd(cell, obj);
    return set(pcdr, cons(sym, obj));
  }
}

static val env_vbindings(val env)
{
  val self = lit("env-vbindings");
  type_check(self, env, ENV);
  return env->e.vbindings;
}

static val env_fbindings(val env)
{
  val self = lit("env-fbindings");
  type_check(self, env, ENV);
  return env->e.fbindings;
}

static val env_next(val env)
{
  val self = lit("env-next");
  type_check(self, env, ENV);
  return env->e.up_env;
}

static void env_vb_to_fb(val env)
{
  if (env) {
    type_check(lit("expand"), env, ENV);
    env->e.fbindings = env->e.vbindings;
    env->e.vbindings = nil;
  }
}

static val env_to_menv(val env, val self, val menv)
{
  if (env == nil) {
    return menv;
  } else {
    val iter;
    type_check(self, env, ENV);

    if (!menv)
      menv = make_env(nil, nil, nil);

    if (env->e.up_env)
      menv = env_to_menv(env->e.up_env, self, menv);

    for (iter = env->e.vbindings; iter; iter = cdr(iter)) {
      val binding = car(iter);
      env_vbind(menv, car(binding), special_s);
    }

    for (iter = env->e.fbindings; iter; iter = cdr(iter)) {
      val binding = car(iter);
      env_fbind(menv, car(binding), special_s);
    }

    return menv;
  }
}

val ctx_form(val obj)
{
  if (consp(obj))
    return obj;
  if (interp_fun_p(obj))
    return obj->f.f.interp_fun;
  return nil;
}

val ctx_name(val obj)
{
  if (consp(obj)) {
    if (car(obj) == lambda_s)
      return list(lambda_s, second(obj), nao);
    else
      return car(obj);
  }

  if (interp_fun_p(obj))
    return func_get_name(obj, obj->f.env);
  return nil;
}

static void eval_exception(val sym, val ctx, val fmt, va_list vl)
{
  uses_or2;
  val form = ctx_form(ctx);
  val stream = make_string_output_stream();
  val loc = or2(source_loc_str(form, nil),
                source_loc_str(last_form_evaled, nil));

  if (loc)
    format(stream, lit("(~a) "), loc, nao);

  (void) vformat(stream, fmt, vl);

  uw_release_deferred_warnings();

  uw_rthrow(sym, get_string_from_stream(stream));
}

NORETURN val eval_error(val ctx, val fmt, ...)
{
  va_list vl;
  va_start (vl, fmt);
  eval_exception(eval_error_s, ctx, fmt, vl);
  va_end (vl);
  abort();
}

static val eval_warn(val ctx, val fmt, ...)
{
  va_list vl;

  uw_catch_begin (cons(continue_s, nil), exsym, exvals);

  va_start (vl, fmt);
  eval_exception(warning_s, ctx, fmt, vl);
  va_end (vl);

  uw_catch(exsym, exvals) { (void) exsym; (void) exvals; }

  uw_unwind;

  uw_catch_end;

  return nil;
}

static val eval_defr_warn(val ctx, val tag, val fmt, ...)
{
  uses_or2;
  va_list vl;

  va_start (vl, fmt);

  uw_catch_begin (cons(continue_s, nil), exsym, exvals);

  {
    val form = ctx_form(ctx);
    val stream = make_string_output_stream();
    val loc = or2(source_loc_str(form, nil),
                  source_loc_str(last_form_evaled, nil));

    if (loc)
      format(stream, lit("(~a) "), loc, nao);

    (void) vformat(stream, fmt, vl);

    uw_rthrow(defr_warning_s,
              cons(get_string_from_stream(stream), cons(tag, nil)));
  }

  uw_catch(exsym, exvals) { (void) exsym; (void) exvals; }

  uw_unwind;

  uw_catch_end;

  va_end (vl);

  return nil;
}

val lookup_origin(val form)
{
  return gethash(origin_hash, form);
}

static val set_origin(val form, val origin)
{
  if (origin && form != origin && is_ptr(form) &&
      (!symbolp(form) || !symbol_package(form)) &&
      is_ptr(origin) && (!symbolp(origin) || !symbol_package(origin)))
    sethash(origin_hash, form, origin);
  return form;
}

val set_last_form_evaled(val form)
{
  val prev = last_form_evaled;
  last_form_evaled = form;
  return prev;
}

void error_trace(val exsym, val exvals, val out_stream, val prefix)
{
  val last = last_form_evaled;
  val xlast = uw_last_form_expanded();
  val info = source_loc_str(last, nil);

  if (cdr(exvals) || !stringp(car(exvals)))
    format(out_stream, lit("~a exception args: ~!~s\n"),
           prefix, exvals, nao);
  else
    format(out_stream, lit("~a ~!~a\n"), prefix, car(exvals), nao);

  if (info) {
    val first, origin, oinfo = nil;

    for (first = t; last; last = origin, first = nil) {
      info = oinfo ? oinfo : info;
      origin = lookup_origin(last);
      oinfo = source_loc_str(origin, nil);

      if (first) {
        if (origin)
          format(out_stream, lit("~a during evaluation of form ~!~s\n"),
                 prefix, last, nao);
        else if (!uw_exception_subtype_p(exsym, eval_error_s))
          format(out_stream, lit("~a during evaluation at ~a of form ~!~s\n"),
                 prefix, info, last, nao);
      }

      if (origin) {
        format(out_stream, lit("~a ... an expansion of ~!~s\n"),
               prefix, origin, nao);
      } else if (!first) {
        if (info)
          format(out_stream, lit("~a which is located at ~a\n"), prefix,
                 info, nao);
        else
          format(out_stream, lit("~a whose location is unavailable\n"), prefix,
                 nao);
      }

      if (origin == last)
        break;
    }
  }

  if (xlast) {
    val ex_info = source_loc_str(xlast, nil);
    val form = xlast;

    if (ex_info)
      format(out_stream, lit("~a during expansion at ~a of form ~!~s\n"),
             prefix, ex_info, xlast, nao);
    else
      format(out_stream, lit("~a during expansion of form ~!~s\n"),
             prefix, xlast, nao);

    if (info)
      format(out_stream, lit("~a by macro code located at ~a\n"), prefix,
             info, nao);

    for (;;) {
      val origin = lookup_origin(xlast);
      val oinfo = source_loc_str(origin, nil);

      if (origin) {
        if (oinfo)
          format(out_stream, lit("~a ... an expansion at ~a of ~!~s\n"),
                 prefix, oinfo, origin, nao);
        else
          format(out_stream, lit("~a ... an expansion of ~!~s\n"),
                 prefix, origin, nao);

        if (origin == form)
          break;

        form = origin;
        continue;
      }

      break;
    }
  }

#if CONFIG_DEBUG_SUPPORT
  if (dbg_backtrace) {
    format(out_stream, lit("~a backtrace:\n"), prefix, nao);
    debug_dump_backtrace(out_stream, prefix);
  } else {
    format(std_error, lit("~a run with --backtrace to enable backtraces\n"), prefix, nao);
  }
#else
  format(std_error, lit("~a not compiled with backtrace support\n"), prefix, nao);
#endif
}

val lookup_global_var(val sym)
{
  uses_or2;
  return or2(gethash(top_vb, sym),
             if2(lisplib_try_load(sym), gethash(top_vb, sym)));
}

val lookup_var(val env, val sym)
{
  if (env) {
    type_check(lit("variable lookup"), env, ENV);

    for (; env; env = env->e.up_env) {
      val binding = assoc(sym, env->e.vbindings);
      if (binding) {
        if (cdr(binding) == unbound_s)
          break;
        return binding;
      }
    }
  }

  for (env = dyn_env; env; env = env->e.up_env) {
    val binding = assoc(sym, env->e.vbindings);
    if (binding)
      return if3(us_cdr(binding) == unbound_s, nil, binding);
  }

  return lookup_global_var(sym);
}

val lookup_sym_lisp1(val env, val sym)
{
  uses_or2;

  if (env) {
    type_check(lit("lisp-1-style lookup"), env, ENV);

    for (; env; env = env->e.up_env) {
      val binding = or2(assoc(sym, env->e.vbindings),
                        assoc(sym, env->e.fbindings));
      if (binding) {
        if (cdr(binding) == unbound_s)
          break;
        return binding;
      }
    }
  }

  for (env = dyn_env; env; env = env->e.up_env) {
      val binding = or2(assoc(sym, env->e.vbindings),
                        assoc(sym, env->e.fbindings));
    if (binding)
      return binding;
  }

  return or3(gethash(top_vb, sym),
             if2(lisplib_try_load(sym),
                 gethash(top_vb, sym)),
             gethash(top_fb, sym));
}

loc lookup_var_l(val env, val sym)
{
  val binding = lookup_var(env, sym);
  return if3(binding, cdr_l(binding), nulloc);
}

loc lookup_global_var_l(val sym)
{
  val binding = lookup_global_var(sym);
  return if3(binding, cdr_l(binding), nulloc);
}

static val lookup_mac(val menv, val sym);

val lookup_fun(val env, val sym)
{
  uses_or2;

  if (consp(sym)) {
    if (car(sym) == meth_s) {
      val strct = cadr(sym);
      val slot = caddr(sym);
      val type = or2(find_struct_type(strct),
                     if2(lisplib_try_load(strct),
                       find_struct_type(strct)));
      if (slot == init_k) {
        return cons(sym, struct_get_initfun(type));
      } else if (slot == postinit_k) {
        return cons(sym, struct_get_postinitfun(type));
      } else {
        return if2(and2(type, static_slot_p(type, slot)),
                   cons(sym, static_slot(type, slot)));
      }
    } else if (car(sym) == macro_s) {
      return lookup_mac(nil, cadr(sym));
    } else if (car(sym) == lambda_s) {
      return cons(sym, func_interp(env, expand(sym, nil)));
    } else {
      return nil;
    }
  }

  if (env) {
    type_check(lit("function lookup"), env, ENV);

    for (; env; env = env->e.up_env) {
      val binding = assoc(sym, env->e.fbindings);
      if (binding)
        return binding;
    }
  }

  return or2(gethash(top_fb, sym),
             if2(lisplib_try_load(sym), gethash(top_fb, sym)));
}

val func_get_name(val fun, val env)
{
  val self = lit("func-get-name");
  env = default_null_arg(env);

  type_check(self, fun, FUN);

  if (env) {
    type_check(self, env, ENV);

    {
      val iter;
      for (; env; env = env->e.up_env) {
        for (iter = env->e.fbindings; iter; iter = cdr(iter)) {
          val binding = car(iter);
          if (cdr(binding) == fun)
            return car(binding);
        }
      }
    }
  }

  {
    val name;

    if ((name = hash_revget(top_fb, fun, eq_f, cdr_f)))
      return name;

    if ((name = hash_revget(top_mb, fun, eq_f, cdr_f)))
      return list(macro_s, name, nao);

    if ((name = method_name(fun)))
      return name;

    if (interp_fun_p(fun))
      return func_get_form(fun);
  }

  return nil;
}

static val lookup_mac(val menv, val sym)
{
  uses_or2;

  if (nilp(menv)) {
    return or2(gethash(top_mb, sym),
               if2(lisplib_try_load(sym), gethash(top_mb, sym)));
  } else  {
    type_check(lit("macro lookup"), menv, ENV);

    {
      val binding = assoc(sym, menv->e.fbindings);
      if (binding) /* special_s: see make_fun_shadowing_env */
        return (cdr(binding) == special_s) ? nil : binding;
      return lookup_mac(menv->e.up_env, sym);
    }
  }
}

static val lookup_symac(val menv, val sym)
{
  uses_or2;

  if (nilp(menv)) {
    return or2(gethash(top_smb, sym),
               if2(lisplib_try_load(sym), gethash(top_smb, sym)));
  } else  {
    type_check(lit("symacro lookup"), menv, ENV);

    {
      val binding = assoc(sym, menv->e.vbindings);
      if (binding) /* special_s: see make_var_shadowing_env */
        return (cdr(binding) == special_s) ? nil : binding;
      return lookup_symac(menv->e.up_env, sym);
    }
  }
}

static val lookup_symac_lisp1(val menv, val sym)
{
  uses_or2;

  if (nilp(menv)) {
    return or2(gethash(top_smb, sym),
               if2(lisplib_try_load(sym), gethash(top_smb, sym)));
  } else  {
    type_check(lit("symacro lookup"), menv, ENV);

    /* Of course, we are not looking for symbol macros in the operator macro
     * name space.  Rather, the object of the lookup rule implemented by this
     * function is to allow lexical function bindings to shadow symbol macros.
     */
    {
      val vbinding = assoc(sym, menv->e.vbindings);
      if (vbinding) {
        return (cdr(vbinding) == special_s) ? nil : vbinding;
      } else {
        val fbinding = assoc(sym, menv->e.fbindings);
        if (fbinding && cdr(fbinding) == special_s)
          return nil;
      }

      return lookup_symac(menv->e.up_env, sym);
    }
  }
}

static val reparent_env(val child, val parent)
{
  child->e.up_env = parent;
  return child;
}

static val lexical_var_p(val menv, val sym)
{
  if (nilp(menv)) {
    return nil;
  } else {
    type_check(lit("lexical-var-p"), menv, ENV);

    {
      val binding = assoc(sym, menv->e.vbindings);

      if (binding) /* special_s: see make_var_shadowing_env */
        return tnil(cdr(binding) == special_s);
      return lexical_var_p(menv->e.up_env, sym);
    }
  }
}

static val lexical_fun_p(val menv, val sym)
{
  if (nilp(menv)) {
    return nil;
  } else {
    type_check(lit("lexical-fun-p"), menv, ENV);

    {
      val binding = assoc(sym, menv->e.fbindings);

      if (binding) /* special_s: see make_var_shadowing_env */
        return tnil(cdr(binding) == special_s);
      return lexical_fun_p(menv->e.up_env, sym);
    }
  }
}

static val lexical_lisp1_binding(val menv, val sym)
{
  if (nilp(menv)) {
    return nil;
  } else {
    type_check(lit("lexical-lisp1-binding"), menv, ENV);

    {
      val binding = assoc(sym, menv->e.vbindings);

      if (binding) /* special_s: see make_var_shadowing_env */
        return if3(cdr(binding) == special_s,
                   var_k, symacro_k);
    }

    {
      val binding = assoc(sym, menv->e.fbindings);

      if (binding && cdr(binding) == special_s)
        return fun_k;
      return lexical_lisp1_binding(menv->e.up_env, sym);
    }
  }
}

static val mark_special(val sym)
{
  assert (sym != nil);
  return sethash(special, sym, t);
}

static val special_var_p(val sym)
{
  uses_or2;
  return or2(gethash(special, sym),
             if2(lisplib_try_load(sym), gethash(special, sym)));
}

static void copy_env_handler(mem_t *ptr)
{
  val *penv = coerce(val *, ptr);
  *penv = copy_env(*penv);
}

static val squash_menv_deleting_range(val menv, val upto_menv)
{
  val varshadows = nil, funshadows = nil;
  val iter, next, out_env;

  if (!upto_menv)
    return nil;

  out_env = make_env(nil, nil, nil);

  for (iter = menv; iter && iter != upto_menv; iter = next) {
    type_check(lit("expand-with-free-refs"), iter, ENV);
    varshadows = append2(varshadows, mapcar(car_f, iter->e.vbindings));
    funshadows = append2(funshadows, mapcar(car_f, iter->e.fbindings));
    next = iter->e.up_env;
  }

  if (!iter)
    return nil;

  for (; iter; iter = next) {
    val viter, fiter;

    for (viter = iter->e.vbindings; viter; viter = cdr(viter)) {
      val binding = car(viter);
      val sym = car(binding);
      if (memq(sym, varshadows))
        continue;
      if (cdr(binding) != special_s)
        continue;
      push(sym, &varshadows);
      env_vbind(out_env, sym, special_s);
    }

    for (fiter = iter->e.fbindings; fiter; fiter = cdr(fiter)) {
      val binding = car(fiter);
      val sym = car(binding);
      if (memq(sym, funshadows))
        continue;
      if (cdr(binding) != special_s)
        continue;
      push(sym, &funshadows);
      env_fbind(out_env, sym, special_s);
    }
    next = iter->e.up_env;
  }

  return out_env;
}

INLINE val lex_or_dyn_bind_seq(val *dyn_made, val lex_env, val sym, val obj)
{
  if (special_var_p(sym)) {
    if (!*dyn_made) {
      dyn_env = make_env(nil, nil, dyn_env);
      *dyn_made = t;
    }
    env_vbind(dyn_env, sym, obj);
  } else {
    lex_env = make_env(nil, nil, lex_env);
    env_vbind(lex_env, sym, obj);
  }

  return lex_env;
}

INLINE void lex_or_dyn_bind(val *dyn_made, val lex_env, val sym, val obj)
{
  if (special_var_p(sym)) {
    if (!*dyn_made) {
      dyn_env = make_env(nil, nil, dyn_env);
      *dyn_made = t;
    }
    env_vbind(dyn_env, sym, obj);
  } else {
    env_vbind(lex_env, sym, obj);
  }
}

static val bind_args(val env, val params, struct args *args, val ctx)
{
  val new_env = make_env(nil, nil, env);
  val dyn_env_made = nil;
  val optargs = nil;
  cnum index = 0;
  uw_frame_t uw_cc;

  uw_push_cont_copy(&uw_cc, coerce(mem_t *, &new_env), copy_env_handler);

  for (; args_more(args, index) && consp(params); params = cdr(params)) {
    val param = car(params);
    val arg;
    val initform = nil;
    val presentsym = nil;

    if (param == colon_k) {
      optargs = t;
      continue;
    }

    if (consp(param)) {
      val sym = pop(&param);
      if (optargs) {
        initform = pop(&param);
        presentsym = pop(&param);
        param = sym;
      } else {
        eval_error(ctx, lit("~s: bad object ~s in param list"),
                   ctx_name(ctx), sym, nao);
      }
    }

    arg = args_get(args, &index);

    if (optargs) {
      val initval = nil;
      val present = nil;

      if (arg == colon_k) {
        if (initform) {
          initval = eval(initform, new_env, ctx);
          new_env = lex_or_dyn_bind_seq(&dyn_env_made, new_env,
                                        param, initval);
        }
      } else {
        lex_or_dyn_bind(&dyn_env_made, new_env, param, arg);
        present = t;
      }

      if (presentsym)
        lex_or_dyn_bind(&dyn_env_made, new_env, presentsym, present);
    } else {
      lex_or_dyn_bind(&dyn_env_made, new_env, param, arg);
    }
  }

  if (consp(params)) {
    if (car(params) == colon_k) {
      optargs = t;
      params = cdr(params);
    }
    if (!optargs)
      eval_error(ctx, lit("~s: too few arguments"), ctx_name(ctx), nao);
    while (consp(params)) {
      val param = car(params);
      if (consp(param)) {
        val sym = pop(&param);
        val initform = pop(&param);
        val presentsym = pop(&param);
        val initval = eval(initform, new_env, ctx);

        new_env = lex_or_dyn_bind_seq(&dyn_env_made, new_env,
                                      sym, initval);

        if (presentsym)
          lex_or_dyn_bind(&dyn_env_made, new_env, presentsym, nil);
      } else {
        lex_or_dyn_bind(&dyn_env_made, new_env, param, nil);
      }
      params = cdr(params);
    }
    if (bindable(params))
      lex_or_dyn_bind(&dyn_env_made, new_env, params, nil);
  } else if (params) {
    lex_or_dyn_bind(&dyn_env_made, new_env, params,
                    args_get_rest(args, index));
  } else if (args_more(args, index)) {
    eval_error(ctx, lit("~s: too many arguments"),
               ctx_name(ctx), nao);
  }

  uw_pop_frame(&uw_cc);

  return new_env;
}

NORETURN static val not_bindable_error(val form, val sym)
{
  eval_error(form, lit("~s: ~s is not a bindable symbol"),
             car(form), sym, nao);
}

static val not_bindable_warning(val form, val sym)
{
  return eval_warn(form, lit("~s: ~s is not a bindable symbol"),
                   car(form), sym, nao);
}

static val make_var_shadowing_env(val menv, val vars);

static val get_param_syms(val params);

static val expand_params_rec(val params, val menv, val macro_style_p,
                             val form);

static val expand_opt_params_rec(val params, val menv,
                                 val macro_style_p, val form)
{
  if (!params) {
    return params;
  } else if (atom(params)) {
    if (!bindable(params))
      not_bindable_error(form, params);
    return params;
  } else {
    val pair = car(params);
    if (atom(pair)) {
      val new_menv = menv;

      if (pair == whole_k || pair == form_k || pair == env_k) {
        if (!macro_style_p)
          eval_error(form, lit("~s: ~s not usable in function parameter list"),
                     car(form), pair, nao);
        if (!consp(cdr(params)))
          eval_error(form, lit("~s: ~s parameter requires name"),
                     car(form), pair, nao);
        if (pair == env_k && !bindable(cadr(params)))
          eval_error(form, lit("~s: ~s parameter requires bindable symbol"),
                     car(form), pair, nao);
      } else if (!bindable(pair)) {
        if (pair == colon_k)
          eval_error(form, lit("~s: multiple colons in parameter list"),
                     car(form), nao);
        not_bindable_error(form, pair);
      } else {
        new_menv = make_var_shadowing_env(menv, pair);
      }

      {
        val params_ex = expand_opt_params_rec(cdr(params), new_menv,
                                              macro_style_p, form);


        if (params_ex == cdr(params))
          return params;
        return rlcp(cons(pair, params_ex), cdr(params));
      }
    } else if (!macro_style_p && !bindable(car(pair))) {
      eval_error(form, lit("~s: parameter symbol expected, not ~s"),
                 car(form), car(pair), nao);
    } else {
      val param = car(pair);
      val param_ex = expand_params_rec(param, menv,
                                       macro_style_p,
                                       form);
      val initform = cadr(pair);
      val initform_ex = rlcp(expand(initform, menv), initform);
      val opt_sym = caddr(pair);
      val form_ex = rlcp(cons(param_ex, cons(initform_ex,
                                             cons(opt_sym, nil))),
                         pair);
      val new_menv = make_var_shadowing_env(menv, get_param_syms(param_ex));

      if (cdddr(pair))
        eval_error(form, lit("~s: extra forms ~s in ~s"),
                   car(form), pair, cdddr(pair), nao);

      if (opt_sym) {
        if (!bindable(opt_sym))
          not_bindable_error(form, opt_sym);
      }

      return rlcp(cons(form_ex, expand_opt_params_rec(rest(params), new_menv,
                                                      macro_style_p, form)),
                  cdr(params));
    }
  }
}

static val expand_params_rec(val params, val menv,
                             val macro_style_p, val form)
{
  if (!params) {
    return params;
  } else if (atom(params)) {
    if (!bindable(params))
      not_bindable_error(form, params);
    return params;
  } else if (car(params) == colon_k) {
    val params_ex = expand_opt_params_rec(cdr(params), menv,
                                          macro_style_p, form);
    if (params_ex == cdr(params))
      return params;
    return rlcp(cons(colon_k, params_ex), cdr(params));
  } else if (!macro_style_p && consp(car(params))) {
    eval_error(form, lit("~s: parameter symbol expected, not ~s"),
               car(form), car(params), nao);
  } else {
    val param = car(params);
    val param_ex;
    val new_menv = menv;

    if (param == whole_k || param == form_k || param == env_k) {
      if (!macro_style_p)
        eval_error(form, lit("~s: ~s not usable in function parameter list"),
                   car(form), param, nao);
      if (!consp(cdr(params)))
        eval_error(form, lit("~s: ~s parameter requires name"),
                   car(form), param, nao);
      if (param == env_k && !bindable(cadr(params)))
        eval_error(form, lit("~s: ~s parameter requires bindable symbol"),
                   car(form), param, nao);
      param_ex = param;
    } else if (bindable(param) || (macro_style_p && listp(param))) {
      param_ex = expand_params_rec(param, menv, t, form);
      new_menv = make_var_shadowing_env(menv, get_param_syms(param_ex));
    } else {
      not_bindable_error(form, param);
    }

    {
      val params_ex = expand_params_rec(cdr(params), new_menv,
                                        macro_style_p,
                                        form);
      if (param_ex == car(params) && params_ex == cdr(params))
        return params;
      return rlcp(cons(param_ex, params_ex), params);
    }
  }
}

static val expand_param_macro(val params, val body, val menv, val form)
{
  if (atom(params)) {
    return cons(params, body);
  } else {
    val sym = car(params);
    val pmac = gethash(pm_table, sym);

    if (!keywordp(sym) || sym == whole_k || sym == form_k ||
        sym == env_k ||sym == colon_k)
      return cons(params, body);

    if (!pmac) {
      lisplib_try_load(sym);
      pmac = gethash(pm_table, sym);
      if (!pmac)
        eval_error(form, lit("~s: keyword ~s has no param macro binding"),
                   car(form), sym, nao);
    }

    {
      val prest = cdr(params);
      cons_bind (prest_ex0, body_ex0, expand_param_macro(prest, body,
                                                         menv, form));
      cons_bind (prest_ex, body_ex, funcall4(pmac, prest_ex0, body_ex0,
                                             menv, form));
      if (body_ex != body)
        rlcp(body_ex, body);
      return expand_param_macro(prest_ex, body_ex, menv, form);
    }
  }
}

static val expand_params(val params, val body, val menv,
                         val macro_style_p, val form)
{
  cons_bind (params_ex0, body_ex, expand_param_macro(params, body, menv, form));
  val params_ex = expand_params_rec(params_ex0, menv, macro_style_p, form);
  return cons(params_ex, body_ex);
}

static val get_opt_param_syms(val params)
{
  if (bindable(params)) {
    return cons(params, nil);
  } else if (atom(params)) {
    return nil;
  } else {
    val spec = car(params);

    if (atom(spec)) {
      val rest_syms = get_opt_param_syms(cdr(params));
      if (bindable(spec))
        return cons(spec, rest_syms);
      return rest_syms;
    } else {
      val pat_var = car(spec);
      val pat_p_var = caddr(spec);
      val syms = nappend2(get_param_syms(pat_p_var),
                          get_opt_param_syms(cdr(params)));
      return nappend2(get_param_syms(pat_var), syms);
    }
  }
}

static val get_param_syms(val params)
{
  if (bindable(params)) {
    return cons(params, nil);
  } else if (atom(params)) {
    return nil;
  } else if (car(params) == colon_k) {
    return get_opt_param_syms(cdr(params));
  } else if (consp(car(params))) {
    return nappend2(get_param_syms(car(params)),
                    get_param_syms(cdr(params)));
  } else if (bindable(car(params))) {
    return cons(car(params), get_param_syms(cdr(params)));
  } else {
    return get_param_syms(cdr(params));
  }
}

val apply(val fun, val arglist)
{
  args_decl_list(args, ARGS_MIN, arglist);
  return generic_funcall(fun, args);
}

static val apply_frob_args(val args)
{
  if (!cdr(args)) {
    return car(args);
  } else {
    list_collect_decl (out, ptail);

    for (; cdr(args); args = cdr(args))
      ptail = list_collect(ptail, car(args));

    list_collect_nconc(ptail, car(args));
    return out;
  }
}

static val apply_intrinsic_frob_args(val args)
{
  if (!args) {
    uw_throwf(error_s, lit("apply: trailing-args argument missing"), nao);
  } if (!cdr(args)) {
    return tolist(car(args));
  } else {
    list_collect_decl (out, ptail);

    for (; cdr(args); args = cdr(args))
      ptail = list_collect(ptail, car(args));

    list_collect_nconc(ptail, tolist(car(args)));
    return out;
  }
}

val applyv(val fun, struct args *args)
{
  args_normalize_least(args, 1);

  if (!args->fill)
    uw_throwf(error_s, lit("apply: trailing-args argument missing"), nao);

  if (!args->list)
    args->list = tolist(z(args->arg[--args->fill]));
  else
    args->list = apply_intrinsic_frob_args(args->list);

  return generic_funcall(fun, args);
}

static loc term(loc head)
{
  while (consp(deref(head)))
    head = cdr_l(deref(head));
  return head;
}

static val iapply(val fun, struct args *args)
{
  cnum index = 0;
  list_collect_decl (mod_args, ptail);
  loc saved_ptail;
  val last_arg = nil;

  while (args_two_more(args, index))
    ptail = list_collect(ptail, args_get(args, &index));

  saved_ptail = ptail;

  if (args_more(args, index)) {
    last_arg = tolist(args_get(args, &index));
    ptail = list_collect_nconc(ptail, last_arg);
  }

  {
    loc pterm = term(ptail);
    val tatom = deref(pterm);

    if (tatom) {
      deref(ptail) = nil;
      ptail = list_collect_nconc(saved_ptail, copy_list(last_arg));
      set(term(ptail), cons(tatom, nil));
    }
  }

  return apply(fun, z(mod_args));
}

static val list_star_intrinsic(struct args *args)
{
  return apply_frob_args(args_get_list(args));
}

static val bind_macro_params(val env, val menv, val params, val form,
                             val loose_p, val ctx_form)
{
  val new_env = make_env(nil, nil, env);
  val dyn_env_made = nil;
  val whole = form;
  val optargs = nil;
  uw_frame_t uw_cc;

  uw_push_cont_copy(&uw_cc, coerce(mem_t *, &new_env), copy_env_handler);

  while (consp(params)) {
    val param = car(params);

    if (param == whole_k || param == form_k || param == env_k) {
      val nparam;
      val next = cdr(params);
      val bform = if3(param == whole_k, whole,
                      if3(param == form_k,
                          ctx_form, menv));
      if (!consp(next))
        eval_error(ctx_form, lit("~s: dangling ~s in param list"),
                   car(ctx_form), param, nao);
      nparam = car(next);

      if (atom(nparam)) {
        lex_or_dyn_bind(&dyn_env_made, new_env, nparam, bform);
      } else {
        new_env = bind_macro_params(new_env, menv,
                                    nparam, bform,
                                    loose_p, ctx_form);
        if (!new_env)
          goto nil_out;
      }
      params = cdr(next);
      continue;
    }

    if (param == colon_k) {
      optargs = t;
      params = cdr(params);
      continue;
    }

    if (consp(form)) {
      if (optargs && opt_compat && opt_compat <= 190 && car(form) == colon_k) {
        form = cdr(form);
        goto noarg;
      }

      if (!listp(param)) {
        lex_or_dyn_bind(&dyn_env_made, new_env, param, car(form));
      } else {
        if (optargs) {
          val nparam = pop(&param);
          val initform = pop(&param);
          val presentsym = pop(&param);

          (void) initform;

          new_env = bind_macro_params(new_env, menv,
                                      nparam, car(form), t, ctx_form);

          if (presentsym)
            lex_or_dyn_bind(&dyn_env_made, new_env, presentsym, t);
        } else {
          new_env = bind_macro_params(new_env, menv,
                                      param, car(form),
                                      loose_p, ctx_form);
          if (!new_env)
            goto nil_out;
        }
      }
      params = cdr(params);
      form = cdr(form);
      continue;
    }

    if (form) {
      if (loose_p == colon_k)
        goto nil_out;
      eval_error(ctx_form, lit("~s: atom ~s not matched by params ~s"),
                 car(ctx_form), form, params, nao);
    }

    if (!optargs) {
      if (!loose_p)
        eval_error(ctx_form, lit("~s: missing arguments for params ~s"),
                   car(ctx_form), params, nao);
      if (loose_p == colon_k)
        goto nil_out;
    }

noarg:
    if (!listp(param)) {
      lex_or_dyn_bind(&dyn_env_made, new_env, param, nil);
    } else {
      val nparam = pop(&param);
      val initform = pop(&param);
      val presentsym = pop(&param);

      if (initform) {
        val initval = eval(initform, new_env, ctx_form);
        new_env = bind_macro_params(new_env, menv,
                                    nparam, initval, t, ctx_form);
      } else {
        new_env = bind_macro_params(new_env, menv,
                                    nparam, nil, t, ctx_form);
      }

      if (presentsym)
        lex_or_dyn_bind(&dyn_env_made, new_env, presentsym, nil);
    }

    params = cdr(params);
  }

  if (params) {
    lex_or_dyn_bind(&dyn_env_made, new_env, params, form);
    goto out;
  }

  if (form) {
    if (loose_p == colon_k)
      goto nil_out;
    eval_error(ctx_form,
               lit("~s: extra form part ~s not matched by parameter list"),
               car(ctx_form), form, nao);
  }

out:
  uw_pop_frame(&uw_cc);
  return new_env;

nil_out:
  uw_pop_frame(&uw_cc);
  return nil;
}

static val do_eval(val form, val env,
                   val ctx, val (*lookup)(val env, val sym));

static void do_eval_args(val form, val env, val ctx,
                         val (*lookup)(val env, val sym),
                         struct args *args)
{
  for (; form; form = cdr(form))
    args_add(args, do_eval(car(form), env, ctx, lookup));
}

val set_dyn_env(val de)
{
  val old = dyn_env;
  dyn_env = de;
  return old;
}

val funcall_interp(val interp_fun, struct args *args)
{
  val env = interp_fun->f.env;
  val fun = interp_fun->f.f.interp_fun;
  val def = cdr(fun);
  val params = car(def);
  val body = cdr(def);
  val saved_de = dyn_env;
  val fun_env = bind_args(env, params, args, interp_fun);
  val ret = eval_progn(body, fun_env, body);
  dyn_env = saved_de;
  return ret;
}

static val expand_eval(val form, val env, val menv)
{
  val lfe_save = last_form_evaled;
  val form_ex = (last_form_evaled = nil,
                 expand(form, menv));
  val loading = cdr(lookup_var(dyn_env, load_recursive_s));
  val ret = ((void) (loading || uw_release_deferred_warnings()),
             eval(form_ex, default_null_arg(env), form));
  last_form_evaled = lfe_save;
  return ret;
}

static val macroexpand(val form, val menv);

val eval_intrinsic(val form, val env)
{
  val menv = env_to_menv(default_null_arg(env), lit("eval"), nil);
  val form_ex = macroexpand(form, menv);
  val op;

  if (consp(form_ex) &&
      ((op = car(form_ex)) == progn_s || op == eval_only_s ||
       op == compile_only_s))
  {
    val res = nil, next = cdr(form_ex);

    while (next) {
      res = expand_eval(car(next), env, menv);
      next = cdr(next);
    }

    return res;
  }

  return expand_eval(form_ex, env, menv);
}

val eval_intrinsic_noerr(val form, val env, val *error_p)
{
  val result = nil;
  uw_frame_t uw_handler;
  uw_push_handler(&uw_handler, cons(defr_warning_s, nil),
                  func_n1v(uw_muffle_warning));

  uw_catch_begin (cons(t, nil), exsym, exvals);

  result = eval_intrinsic(form, env);

  uw_catch(exsym, exvals) {
    (void) exsym; (void) exvals;
    *error_p = t;
    break;
  }

  uw_unwind;

  uw_catch_end;

  uw_pop_frame(&uw_handler);

  return result;
}

static val do_eval(val form, val env, val ctx,
                   val (*lookup)(val env, val sym))
{
  val self = lit("eval");
  uw_frame_t *ev = 0;
  val ret = nil;

  if (dbg_backtrace) {
    ev = coerce(uw_frame_t *, alloca(sizeof *ev));
    uw_push_eval(ev, form, env);
  }

  sig_check_fast();

  if (form && symbolp(form)) {
    if (!bindable(form)) {
      ret = form;
    } else {
      val binding = lookup(env, form);
      if (binding) {
        ret = cdr(binding);
      } else {
        eval_error(ctx, lit("unbound variable ~s"), form, nao);
        abort();
      }
    }
  } else if (consp(form)) {
    val oper = car(form);
    val entry = gethash(op_table, oper);

    if (entry) {
      opfun_t fp = coerce(opfun_t, cptr_get(entry));
      val lfe_save = last_form_evaled;
      last_form_evaled = form;
      ret = fp(form, env);
      last_form_evaled = lfe_save;
    } else {
      val fbinding = lookup_fun(env, oper);

      if (!fbinding) {
        last_form_evaled = form;
        eval_error(form, lit("~s does not name a function or operator"), oper, nao);
        abort();
      } else {
        val arglist = rest(form);
        cnum alen = if3(consp(arglist), c_num(length(arglist), self), 0);
        cnum argc = max(alen, ARGS_MIN);
        val lfe_save = last_form_evaled;
        args_decl(args, argc);

        last_form_evaled = form;

        do_eval_args(rest(form), env, form, &lookup_var, args);
        ret = generic_funcall(cdr(fbinding), args);

        last_form_evaled = lfe_save;
      }
    }
  } else {
    ret = form;
  }

  if (ev != 0)
    uw_pop_frame(ev);

  return ret;
}

val eval(val form, val env, val ctx)
{
  return do_eval(form, env, ctx, &lookup_var);
}

static void eval_args_lisp1(val form, val env, val ctx, struct args *args)
{
  do_eval_args(form, env, ctx, &lookup_sym_lisp1, args);
}

static val eval_lisp1(val form, val env, val ctx)
{
  return do_eval(form, env, ctx, &lookup_sym_lisp1);
}

val bindable(val obj)
{
  return (obj && symbolp(obj) && obj != t && !keywordp(obj)) ? t : nil;
}

val eval_progn(val forms, val env, val ctx)
{
  val retval = nil;

  if (!forms) {
    sig_check_fast();
    return retval;
  }

  for (; forms; forms = cdr(forms))
    retval = eval(car(forms), env, ctx);

  return retval;
}

static val eval_prog1(val forms, val env, val ctx)
{
  val retval = nil;

  if (forms) {
    retval = eval(car(forms), env, ctx);
    forms = cdr(forms);
  }

  for (; forms; forms = cdr(forms))
    eval(car(forms), env, ctx);

  return retval;
}

static val op_error(val form, val env)
{
  (void) env;
  eval_error(form, lit("unexpanded ~s encountered"), car(form), nao);
  abort();
}

static val op_meta_error(val form, val env)
{
  (void) env;
  eval_error(form, lit("meta with no meaning: ~s"), form, nao);
}

static val op_quote(val form, val env)
{
  val d = cdr(form);
  (void) env;

  if (!consp(d) || cdr(d))
    eval_error(form, lit("bad quote syntax: ~s"), form, nao);
  return second(form);
}

static val op_qquote_error(val form, val env)
{
  (void) env;
  eval_error(form, lit("unexpanded quasiquote encountered"), nao);
}

static val op_unquote_error(val form, val env)
{
  (void) env;
  eval_error(form, lit("unquote/splice without matching quote"), nao);
}

struct bindings_helper_vars {
  val ne;
};

static void copy_bh_env_handler(mem_t *ptr)
{
  struct bindings_helper_vars *pv = coerce(struct bindings_helper_vars *, ptr);
  pv->ne = copy_env(pv->ne);
}

static val bindings_helper(val vars, val env, val sequential,
                           val ret_new_bindings, val ctx)
{
  val iter, var;
  list_collect_decl (new_bindings, ptail);

  if (sequential || vars == nil || cdr(vars) == nil) {
    for (iter = vars; iter; iter = cdr(iter)) {
      val item = car(iter);
      val value = nil;

      if (consp(item)) {
        var = pop(&item);
        value = eval(pop(&item), env, ctx);
      } else {
        var = item;
      }

      {
        val le = make_env(nil, nil, env);
        val binding = env_vbind(le, var, value);
        if (ret_new_bindings)
          ptail = list_collect (ptail, binding);
        env = le;
      }
    }

    return env;
  } else {
    struct bindings_helper_vars v;
    uw_frame_t uw_cc;
    val de_in = dyn_env, new_de = de_in;

    v.ne = make_env(nil, nil, env);

    uw_push_cont_copy(&uw_cc, coerce(mem_t *, &v), copy_bh_env_handler);

    for (iter = vars; iter; iter = cdr(iter)) {
      val item = car(iter);
      val value = nil;

      if (consp(item)) {
        var = pop(&item);
        value = eval(pop(&item), env, ctx);
        if (dyn_env != de_in) {
          reparent_env(dyn_env, new_de);
          new_de = dyn_env;
          dyn_env = de_in;
        }
      } else {
        var = item;
      }

      {
        val binding = env_vbind(v.ne, var, value);
        if (ret_new_bindings)
          ptail = list_collect (ptail, binding);
      }
    }
    dyn_env = new_de;

    uw_pop_frame(&uw_cc);

    return v.ne;
  }
}

static val fbindings_helper(val vars, val env, val lbind, val ctx)
{
  val iter;
  val nenv = make_env(nil, nil, env);
  val lenv = if3(lbind, nenv, env);

  for (iter = vars; iter; iter = cdr(iter)) {
    val item = car(iter);
    val var = pop(&item);
    val value = eval(pop(&item), lenv, ctx);

    if (bindable(var)) {
      (void) env_fbind(nenv, var, value);
    } else {
      eval_error(ctx, lit("~s: ~s is not a bindable symbol"),
                 ctx_name(ctx), var, nao);
    }
  }

  return nenv;
}

static val op_progn(val form, val env)
{
  return eval_progn(rest(form), env, form);
}

static val op_prog1(val form, val env)
{
  return eval_prog1(rest(form), env, form);
}

static val op_let(val form, val env)
{
  val let = first(form);
  val args = rest(form);
  val vars = first(args);
  val body = rest(args);
  val saved_de = dyn_env;
  val new_env = bindings_helper(vars, env, eq(let, let_star_s), nil, form);
  val ret = eval_progn(body, new_env, form);
  dyn_env = saved_de;
  return ret;
}

static val op_fbind(val form, val env)
{
  val oper = first(form);
  val args = rest(form);
  val vars = first(args);
  val body = rest(args);
  val new_env = fbindings_helper(vars, env, eq(oper, lbind_s), form);
  return eval_progn(body, new_env, form);
}

static val op_dvbind(val form, val env)
{
  val args = rest(form);
  val sym = pop(&args);
  val initform = pop(&args);
  val initval = eval(initform, env, form);
  val de = make_env(nil, nil, dyn_env);
  env_vbind(de, sym, initval);
  dyn_env = de;
  return unbound_s;
}

static val get_bindings(val vars, val env)
{
  list_collect_decl (out, iter);
  for (; vars; vars = cdr(vars))
    iter = list_collect(iter, lookup_var(env, car(vars)));
  return out;
}

static val op_each(val form, val env)
{
  val args = rest(form);
  val each = pop(&args);
  val vars = pop(&args);
  val body = args;
  val collect = eq(each, collect_each_s);
  val append = eq(each, append_each_s);
  val bindings = if3(vars == t,
                     env->e.vbindings,
                     get_bindings(vars, env));
  val iters = mapcar(iter_from_binding_f, bindings);
  list_collect_decl (collection, ptail);

  uw_block_begin (nil, result);

  for (;;) {
    val biter, iiter;

    for (biter = bindings, iiter = iters; biter;
         biter = cdr(biter), iiter = cdr(iiter))
    {
      val binding = car(biter);
      val iter = car(iiter);
      if (!iter_more(iter))
        goto out;
      rplacd(binding, iter_item(iter));
      rplaca(iiter, iter_step(iter));
    }

    {
      val res = eval_progn(body, env, form);
      if (collect)
        ptail = list_collect(ptail, res);
      else if (append)
        ptail = list_collect_append(ptail, res);
    }
  }

out:
  result = collection;

  uw_block_end;

  return result;
}

static val op_lambda(val form, val env)
{
  return func_interp(env, form);
}

static val op_fun(val form, val env)
{
  val name = second(form);
  val fbinding = lookup_fun(env, name);

  if (!fbinding)
    eval_error(form, lit("no function exists named ~s"), name, nao);

  return cdr(fbinding);
}

static val op_cond(val form, val env)
{
  val iter = rest(form);

  for (; iter; iter = cdr(iter)) {
    val group = car(iter);
    val restgroup = rest(group);
    val firstval = eval(first(group), env, group);
    if (firstval)
      return if3(restgroup, eval_progn(rest(group), env, group), firstval);
  }

  return nil;
}

static val op_if(val form, val env)
{
  val args = rest(form);

  return if3(eval(first(args), env, form),
             eval(second(args), env, form),
             eval(third(args), env, form));
}

static val op_and(val form, val env)
{
  val args = rest(form);
  val result = t;

  for (; args; args = cdr(args))
    if (!(result = eval(first(args), env, form)))
      return nil;

  return result;
}

static val op_or(val form, val env)
{
  val args = rest(form);

  for (; args; args = cdr(args)) {
    val result;
    if ((result = eval(first(args), env, form)))
      return result;
  }

  return nil;
}

static val rt_defvarl(val sym)
{
  val self = lit("defvar");
  val new_p;
  val cell = (lisplib_try_load(sym),
              gethash_c(self, top_vb, sym, mkcloc(new_p)));

  if (new_p || !cdr(cell)) {
    uw_purge_deferred_warning(cons(var_s, sym));
    uw_purge_deferred_warning(cons(sym_s, sym));
    remhash(top_smb, sym);
    return cell;
  }

  return nil;
}

static val op_defvarl(val form, val env)
{
  val args = rest(form);
  val sym = first(args);
  val cell = rt_defvarl(sym);

  if (cell) {
    val value = eval(second(args), env, form);
    rplacd(cell, cons(sym, value));
  }

  return sym;
}

static val op_defsymacro(val form, val env)
{
  val args = rest(form);
  val sym = first(args);

  (void) env;

  lisplib_try_load(sym);
  remhash(top_vb, sym);
  if (!opt_compat || opt_compat > 143)
    remhash(special, sym);
  sethash(top_smb, sym, cons(sym, second(args)));
  return sym;
}

static val rt_defsymacro(val sym, val def)
{
  lisplib_try_load(sym);
  remhash(top_vb, sym);
  remhash(special, sym);
  sethash(top_smb, sym, cons(sym, def));
  return sym;
}

static val op_defmacro(val form, val env);

void trace_check(val name)
{
  if (trace_loaded) {
    val trcheck = lookup_fun(nil,
                             intern(lit("trace-redefine-check"),
                                    system_package));
    if (trcheck)
      funcall1(cdr(trcheck), name);
  }
}

static val rt_defun(val name, val function)
{
  lisplib_try_load(name);
  sethash(top_fb, name, cons(name, function));
  uw_purge_deferred_warning(cons(fun_s, name));
  uw_purge_deferred_warning(cons(sym_s, name));
  return name;
}

static val rt_defmacro(val sym, val name, val function)
{
  lisplib_try_load(sym);
  sethash(top_mb, sym, cons(name, function));
  return name;
}

static val op_defun(val form, val env)
{
  val args = rest(form);
  val name = first(args);
  val params = second(args);
  val body = rest(rest(args));

  trace_check(name);

  if (!consp(name)) {
    val block = cons(sys_blk_s, cons(name, body));
    val fun = rlcp(cons(name, cons(params, cons(block, nil))), form);
    return rt_defun(name, func_interp(env, fun));
  } else if (car(name) == meth_s) {
    val binding = lookup_fun(nil, intern(lit("define-method"), system_package));
    val type_sym = second(name);
    val meth_name = third(name);
    val block = cons(block_s, cons(meth_name, body));
    val fun = rlcp(cons(meth_name, cons(params, cons(block, nil))), form);

    bug_unless (binding);

    return funcall3(cdr(binding), type_sym, meth_name, func_interp(env, fun));
  } else if (car(name) == macro_s) {
    val sym = cadr(name);
    val block = cons(sys_blk_s, cons(sym, body));
    val fun = rlcp(cons(name, cons(params, cons(block, nil))), form);

    if (!bindable(sym))
      eval_error(form, lit("defun: ~s isn't a bindable symbol in ~s"),
                 sym, name, nao);

    if (gethash(op_table, sym))
      eval_error(form, lit("defun: ~s is a special operator in ~s"),
                 sym, name, nao);

    return rt_defmacro(sym, name, func_interp(env, fun));
  } else {
    eval_error(form, lit("defun: ~s isn't recognized function name syntax"),
               name, nao);
  }
}

static val me_interp_macro(val expander, val form, val menv)
{
  val arglist = rest(form);
  val env = car(expander);
  val params = cadr(expander);
  val body = cddr(expander);
  val saved_de = set_dyn_env(make_env(nil, nil, dyn_env));
  val exp_env = bind_macro_params(env, menv, params, arglist, nil, form);
  val result = eval_progn(body, exp_env, body);
  set_dyn_env(saved_de);
  set_origin(result, form);
  return result;
}

static val op_defmacro(val form, val env)
{
  val args = rest(form);
  val name = first(args);
  val params = second(args);
  val body = rest(rest(args));
  val block = rlcp(cons(block_s, cons(name, body)), form);

  if (gethash(op_table, name))
    eval_error(form, lit("defmacro: ~s is a special operator"), name, nao);

  trace_check(name);

  /* defmacro captures lexical environment, so env is passed */
  sethash(top_mb, name,
          rlcp_tree(cons(name, func_f2(cons(env, cons(params, cons(block, nil))),
                                       me_interp_macro)),
                    block));
  return name;
}

static val expand_macro(val form, val mac_binding, val menv)
{
  val expander = cdr(mac_binding);
  val expanded = funcall2(expander, form, menv);
  if (form == expanded) {
    val sym = car(form);
    val up_binding = mac_binding;

    while (menv && up_binding == mac_binding) {
      menv = menv->e.up_env;
      up_binding = lookup_mac(menv, sym);
    }

    if (up_binding && up_binding != mac_binding)
      return expand_macro(form, up_binding, menv);

    return form;
  }
  set_origin(expanded, form);
  return expanded;
}

static val maybe_progn(val forms)
{
  return if3(cdr(forms), rlcp(cons(progn_s, forms), forms), car(forms));
}

static val self_evaluating_p(val form)
{
  if (nilp(form) || form == t)
    return t;

  if (symbolp(form))
    return if2(keywordp(form), t);

  if (atom(form))
    return t;

  return nil;
}

static val maybe_quote(val form)
{
  if (self_evaluating_p(form))
    return form;
  return cons(quote_s, cons(form, nil));
}

static void builtin_reject_test(val op, val sym, val form, val def_kind)
{
    val builtin_kind = gethash(builtin, sym);
    val is_operator = gethash(op_table, sym);

    if (op == defun_s && consp(sym) &&
        (car(sym) == meth_s || car(sym) == macro_s))
    {
      return;
    } else if (!bindable(sym)) {
      eval_error(form, lit("~s: cannot bind ~s, which is not a bindable symbol"),
                 op, sym, nao);
    } else if (opt_compat && opt_compat <= 107) {
      /* empty */
    } else if (sym == expr_s || sym == var_s) {
      /* empty */
    } else if (builtin_kind) {
      if (builtin_kind == def_kind)
        eval_warn(form, lit("~s: redefining ~s, which is a built-in ~s"),
                  op, sym, builtin_kind, nao);
      else
        eval_warn(form, lit("~s: defining ~s, which is also a built-in ~s"),
                  op, sym, builtin_kind, nao);
    } else if (is_operator) {
      eval_warn(form, lit("~s: redefining ~s, which is a built-in operator"),
                op, sym, nao);
    }
}

/*
 * Generate a symbol macro environment in which every
 * variable in the binding list vars is listed
 * as a binding, with the value sys:special.
 * This is a shadow entry, which allows ordinary
 * bindings to shadow symbol macro bindings.
 */
static val make_var_shadowing_env(val menv, val vars)
{
  if (nilp(vars)) {
    return menv;
  } else if (atom(vars)) {
    return make_env(cons(cons(vars, special_s), nil), nil, menv);
  } else {
    list_collect_decl (shadows, ptail);

    for (; vars; vars = cdr(vars)) {
      val var = car(vars);

      ptail = list_collect(ptail, cons(if3(consp(var),
                                           car(var),
                                           var), special_s));
    }

    return make_env(shadows, nil, menv);
  }
}

static val expand_macrolet(val form, val menv)
{
  val op = car(form);
  val body = cdr(form);
  val macs = pop(&body);
  val new_env = make_env(nil, nil, menv);

  for (; macs; macs = cdr(macs)) {
    val macro = car(macs);
    val name = pop(&macro);
    val params = pop(&macro);
    cons_bind (params_ex, macro_ex,
               expand_params(params, macro, menv, t, form));
    val new_menv = make_var_shadowing_env(menv, get_param_syms(params_ex));
    val macro_out = expand_forms(macro_ex, new_menv);
    val block = rlcp_tree(cons(block_s, cons(name, macro_out)), macro_ex);

    builtin_reject_test(op, name, form, defmacro_s);

    /* We store the macrolet in the same form as a top level defmacro,
     * so they can be treated uniformly. The nil at the head of the
     * environment object is the ordinary lexical environment: a macrolet
     * doesn't capture that.
     */
    rlcp_tree(env_fbind(new_env, name,
                        func_f2(cons(nil, cons(params_ex, cons(block, nil))),
                                me_interp_macro)), block);
  }

  return rlcp_tree(maybe_progn(expand_forms(body, new_env)), body);
}

static val expand_symacrolet(val form, val menv)
{
  val body = cdr(form);
  val symacs = pop(&body);
  val new_env = make_env(nil, nil, menv);

  for (; symacs; symacs = cdr(symacs)) {
    val macro = car(symacs);
    val name = pop(&macro);
    val repl = pop(&macro);
    env_vbind(new_env, name,
              if3(opt_compat && opt_compat <= 137,
                  expand(repl, menv), repl));
  }

  return maybe_progn(expand_forms(body, new_env));
}

static val make_fun_shadowing_env(val menv, val funcs)
{
  val env = make_var_shadowing_env(menv, funcs);
  env_vb_to_fb(env);
  return env;
}

static val op_tree_case(val form, val env)
{
  val cases = form;
  val expr = (pop(&cases), pop(&cases));

  val expr_val = eval(expr, env, form);

  for (; consp(cases); cases = cdr(cases)) {
    val onecase = car(cases);
    cons_bind (params, forms, onecase);
    val saved_de = dyn_env;
    val new_env = bind_macro_params(env, nil, params, expr_val,
                                    colon_k, onecase);
    if (new_env) {
      val ret = eval_progn(forms, new_env, forms);
      dyn_env = saved_de;
      if (ret != colon_k)
        return ret;
    }
    dyn_env = saved_de;
  }

  return nil;
}

static val expand_tree_cases(val cases, val menv, val form)
{
  if (atom(cases)) {
    return cases;
  } else {
    val onecase = car(cases);

    if (atom(onecase)) {
      val rest_ex = expand_tree_cases(cdr(cases), menv, form);
      if (rest_ex == cdr(cases))
        return cases;
      return rlcp(cons(onecase, rest_ex), cases);
    } else {
      val dstr_args = car(onecase);
      val forms = cdr(onecase);
      cons_bind (dstr_args_ex, forms_ex0,
                 expand_params(dstr_args, forms, menv, t, form));
      val new_menv = make_var_shadowing_env(menv, get_param_syms(dstr_args_ex));
      val forms_ex = expand_forms(forms_ex0, new_menv);
      val rest_ex = expand_tree_cases(cdr(cases), menv, form);

      if (dstr_args_ex == dstr_args && forms_ex == forms &&
          rest_ex == cdr(cases))
        return cases;

      return rlcp(cons(cons(dstr_args_ex, forms_ex), rest_ex), cases);
    }
  }
}

static val expand_tree_case(val form, val menv)
{
  val sym = first(form);
  val expr = second(form);
  val tree_cases = rest(rest(form));
  val expr_ex = expand(expr, menv);
  val tree_cases_ex = expand_tree_cases(tree_cases, menv, form);

  if (expr_ex == expr && tree_cases_ex == tree_cases)
    return form;

  return rlcp(cons(sym, cons(expr_ex, tree_cases_ex)), form);
}

static val op_tree_bind(val form, val env)
{
  val params = second(form);
  val expr = third(form);
  val body = rest(rest(rest(form)));
  val expr_val = eval(expr, env, expr);
  val saved_de = dyn_env;
  val new_env = bind_macro_params(env, nil, params, expr_val, nil, form);
  val ret = eval_progn(body, new_env, body);
  dyn_env = saved_de;
  return ret;
}

static val op_mac_param_bind(val form, val env)
{
  val body = cdr(form);
  val ctx_form = pop(&body);
  val params = pop(&body);
  val expr = pop(&body);
  val ctx_val = eval(ctx_form, env, ctx_form);
  val expr_val = eval(expr, env, expr);
  val saved_de = dyn_env;
  val new_env = bind_macro_params(env, nil, params, expr_val, nil, ctx_val);
  val ret = eval_progn(body, new_env, body);
  dyn_env = saved_de;
  return ret;
}

static val op_setq(val form, val env)
{
  val args = rest(form);
  val var = pop(&args);
  val newval = pop(&args);
  val binding = lookup_var(env, var);
  if (nilp(binding))
    eval_error(form, lit("unbound variable ~s"), var, nao);
  return sys_rplacd(binding, eval(newval, env, form));
}

static val op_lisp1_setq(val form, val env)
{
  val args = rest(form);
  val var = pop(&args);
  val newval = pop(&args);

  val binding = lookup_sym_lisp1(env, var);
  if (nilp(binding))
    eval_error(form, lit("unbound variable/function ~s"), var, nao);
  return sys_rplacd(binding, eval(newval, env, form));
}

static val expand_lisp1(val form, val menv);

static val expand_lisp1_value(val form, val menv)
{
  if (length(form) != two)
    eval_error(form, lit("~s: invalid syntax"), first(form), nao);

  {
    val sym = cadr(form);
    val sym_ex = expand_lisp1(sym, menv);
    val binding_type = lexical_lisp1_binding(menv, sym_ex);

    if (nilp(binding_type)) {
      if (!bindable(sym_ex))
        eval_error(form, lit("~s: misapplied to form ~s"),
                   first(form), sym_ex, nao);
      return form;
    }

    if (binding_type == var_k)
      return sym_ex;

    if (binding_type == fun_k)
      return rlcp(cons(fun_s, cons(sym_ex, nil)), form);

    eval_error(form, lit("~s: applied to unexpanded symbol macro ~s"),
               first(form), sym_ex, nao);
  }
}

static val expand_lisp1_setq(val form, val menv)
{
  if (!consp(cdr(form)) || !consp(cddr(form)) || cdddr(form))
    eval_error(form, lit("~s: invalid syntax"), car(form), nao);

  {
    val op = car(form);
    val sym = cadr(form);
    val sym_ex = expand_lisp1(sym, menv);
    val newval = caddr(form);
    val binding_type = lexical_lisp1_binding(menv, sym_ex);

    if (nilp(binding_type)) {
      if (!bindable(sym_ex))
        eval_error(form, lit("~s: misapplied to form ~s"),
                   op, sym_ex, nao);
      return rlcp(cons(op, cons(sym_ex, cons(expand(newval, menv), nil))),
                  form);
    }

    if (binding_type == var_k)
      return expand(rlcp(cons(setq_s, cons(sym_ex, cddr(form))), form), menv);

    if (binding_type == fun_k)
      eval_error(form, lit("~s: cannot assign lexical function ~s"), op, sym_ex, nao);

    eval_error(form, lit("~s: applied to unexpanded symbol macro ~s"), op, sym_ex, nao);
  }
}

static val expand_setqf(val form, val menv)
{
  if (!consp(cdr(form)) || !consp(cddr(form)) || cdddr(form))
    eval_error(form, lit("~s: invalid syntax"), car(form), nao);

  {
    val op = car(form);
    val sym = cadr(form);
    val newval = caddr(form);

    if (lexical_fun_p(menv, sym))
      eval_error(form, lit("~s: cannot assign lexical function ~s"), op, sym, nao);

    if (!lookup_fun(nil, sym))
      eval_defr_warn(uw_last_form_expanded(),
                     cons(fun_s, sym), lit("~s: unbound function ~s"),
                     op, sym, nao);

    return rlcp(cons(op, cons(sym, cons(expand(newval, menv), nil))), form);
  }
}

static val op_lisp1_value(val form, val env)
{
  val args = rest(form);
  val arg = car(args);

  if (!bindable(arg)) {
    return eval(arg, env, form);
  } else {
    val binding = lookup_sym_lisp1(env, arg);
    if (nilp(binding))
      eval_error(form, lit("unbound variable/function ~s"), arg, nao);
    return cdr(binding);
  }
}

static val op_setqf(val form, val env)
{
  val args = rest(form);
  val var = pop(&args);
  val newval = pop(&args);
  val binding = lookup_fun(env, var);
  if (nilp(binding))
    eval_error(form, lit("unbound function ~s"), var, nao);
  if (binding != lookup_fun(env, nil))
    eval_error(form, lit("cannot assign lexical function ~s"), var, nao);
  return sys_rplacd(binding, eval(newval, env, form));
}

static val op_for(val form, val env)
{
  val args = rest(form);
  val inits = pop(&args);
  val cond = pop(&args);
  val incs = pop(&args);
  val forms = args;
  int oldscope = opt_compat && opt_compat <= 123;

  eval_progn(inits, env, form);

  if (oldscope) {
    uw_block_begin (nil, result);

    for (; cond == nil || eval(car(cond), env, form);
         eval_progn(incs, env, form))
      eval_progn(forms, env, form);

    result = eval_progn(rest(cond), env, form);

    uw_block_end;

    return result;
  }

  for (; cond == nil || eval(car(cond), env, form);
       eval_progn(incs, env, form))
    eval_progn(forms, env, form);

  return eval_progn(rest(cond), env, form);
}

static val op_dohash(val form, val env)
{
  val op = first(form);
  val spec = second(form);
  val keysym = first(spec);
  val valsym = second(spec);
  val hashform = third(spec);
  val resform = fourth(spec);
  val body = rest(rest(form));
  val keyvar = cons(keysym, nil);
  val valvar = cons(valsym, nil);
  val new_env = make_env(cons(keyvar, cons(valvar, nil)), nil, env);
  val cell;
  struct hash_iter hi;

  hash_iter_init(&hi, eval(hashform, env, hashform), op);

  uw_block_begin (nil, result);

  while ((cell = hash_iter_next(&hi)) != nil) {
    /* These assignments are gc-safe, because keyvar and valvar
       are newer objects than existing entries in the hash,
       unless the body mutates hash by inserting newer objects,
       and also deleting them such that these variables end up
       with the only reference. But in that case, those objects
       will be noted in the GC's check list. */
    *us_cdr_p(keyvar) = us_car(cell);
    *us_cdr_p(valvar) = us_cdr(cell);
    eval_progn(body, new_env, form);
  }

  result = eval(resform, new_env, form);

  uw_block_end;

  return result;
}

static val op_unwind_protect(val form, val env)
{
  val prot_form = second(form);
  val cleanup_forms = rest(rest(form));
  val result = nil;

  uw_simple_catch_begin;

  result = eval(prot_form, env, prot_form);

  uw_unwind {
    eval_progn(cleanup_forms, env, cleanup_forms);
  }

  uw_catch_end;

  return result;
}

static val op_block(val form, val env)
{
  val sym = second(form);
  val body = rest(rest(form));

  uw_block_begin (sym, result);
  result = eval_progn(body, env, form);
  uw_block_end;

  return result;
}

static val op_block_star(val form, val env)
{
  val sym_form = second(form);
  val body = rest(rest(form));
  val sym = eval(sym_form, env, form);

  uw_block_begin (sym, result);
  result = eval_progn(body, env, form);
  uw_block_end;

  return result;
}

static val op_return(val form, val env)
{
  val retval = eval(second(form), env, form);
  uw_block_return(nil, retval);
  eval_error(form, lit("return: no anonymous block is visible"), nao);
  abort();
}

static val op_return_from(val form, val env)
{
  val name = second(form);
  val retval = eval(third(form), env, form);
  uw_block_return(name, retval);
  eval_error(form, lit("return-from: no block named ~s is visible"),
             name, nao);
  abort();
}

static val op_abscond_from(val form, val env)
{
  val name = second(form);
  val retval = eval(third(form), env, form);
  uw_block_abscond(name, retval);
  eval_error(form, lit("sys:abscond-from: no block named ~s is visible"),
             name, nao);
  abort();
}

static val op_dwim(val form, val env)
{
  val argexps = rest(form);
  val objexpr = pop(&argexps);
  cnum alen = if3(consp(argexps), c_num(length(argexps), car(form)), 0);
  cnum argc = max(alen, ARGS_MIN);
  args_decl(args, argc);

  if (!consp(cdr(form)))
    eval_error(form, lit("~s: missing argument"), car(form), nao);

  {
    val func = eval_lisp1(objexpr, env, form);
    eval_args_lisp1(argexps, env, form, args);
    return generic_funcall(func, args);
  }
}

static val op_catch(val form, val env)
{
  val args = cdr(form);
  val catch_syms = pop(&args);
  val try_form = pop(&args);
  val desc = pop(&args);
  val catches = args;
  val result = nil;

  uw_catch_begin_w_desc (catch_syms, exsym, exvals, desc);

  result = eval(try_form, env, try_form);

  uw_catch(exsym, exvals) {
    args_decl(args, ARGS_MIN);
    val iter;

    args_add(args, exsym);
    args_add_list(args, exvals);

    for (iter = catches; iter; iter = cdr(iter)) {
      val clause = car(iter);
      val type = first(clause);

      if (uw_exception_subtype_p(exsym, type)) {
        val params = second(clause);
        val saved_de = set_dyn_env(make_env(nil, nil, dyn_env));
        val clause_env = bind_args(env, params, args, clause);
        result = eval_progn(rest(rest(clause)), clause_env, clause);
        set_dyn_env(saved_de);
        break;
      }
    }
  }

  uw_unwind;

  uw_catch_end;

  return result;
}

static val op_handler_bind(val form, val env)
{
  val args = rest(form);
  val fun = pop(&args);
  val handle_syms = pop(&args);
  val body = args;
  val result;
  uw_frame_t uw_handler;

  uw_push_handler(&uw_handler, handle_syms, eval(fun, env, form));

  result = eval_progn(body, env, form);

  uw_pop_frame(&uw_handler);

  return result;
}

static val fmt_tostring(val obj)
{
  return if3(stringp(obj),
             obj,
             if3(if3(opt_compat && opt_compat <= 174,
                     listp(obj), seqp(obj)),
                 obj,
                 tostringp(obj)));
}

static val fmt_cat(val obj, val sep)
{
  return if3(stringp(obj),
             obj,
             if3(if3(opt_compat && opt_compat <= 174, listp(obj), seqp(obj)),
                 cat_str(mapcar(func_n1(tostringp), obj), sep),
                 tostringp(obj)));
}

static val do_format_field(val obj, val n, val sep,
                           val range_ix, val plist,
                           val filter)
{
  val str;

  if (integerp(range_ix)) {
    obj = ref(obj, range_ix);
  } else if (rangep(range_ix)) {
    obj = sub(obj, from(range_ix), to(range_ix));
  } else if (range_ix) {
    uw_throwf(query_error_s,
              lit("bad field format: index ~s expected to be integer or range"),
              range_ix, nao);
  }

  str = fmt_cat(obj, sep);

  {
    val filter_sym = getplist(plist, filter_k);

    if (filter_sym) {
      filter = get_filter(filter_sym);

      if (!filter) {
        uw_throwf(query_error_s,
                  lit("bad field format: ~s specifies unknown filter"),
                  filter_sym, nao);
      }
    }

    if (filter)
      str = filter_string_tree(filter, str);
  }

  {
    val right = minusp(n);
    val width = if3(minusp(n), neg(n), n);
    val diff = minus(width, length_str(str));

    if (le(diff, zero))
      return str;

    if (ge(length_str(str), width))
      return str;

    {
      val padding = mkstring(diff, chr(' '));

      return if3(right,
                 scat(nil, padding, str, nao),
                 scat(nil, str, padding, nao));
    }
  }
}

val format_field(val obj, val modifier, val filter, val eval_fun)
{
  val n = zero, sep = lit(" ");
  val plist = nil;
  val range_ix = nil;

  for (; modifier; pop(&modifier)) {
    val item = first(modifier);
    if (regexp(item)) {
      uw_throw(query_error_s, lit("bad field format: regex modifier in output var"));
    } else if (keywordp(item)) {
      plist = modifier;
      break;
    } else if ((!opt_compat || opt_compat > 128) &&
               consp(item) && car(item) == expr_s)
    {
      item = cadr(item);
      goto eval;
    } else if (consp(item) && car(item) == dwim_s) {
      val arg_expr = second(item);

      if (consp(arg_expr) && car(arg_expr) == range_s) {
        val from = funcall1(eval_fun, second(arg_expr));
        val to = funcall1(eval_fun, third(arg_expr));

        range_ix = rcons(from, to);
      } else {
        range_ix = funcall1(eval_fun, arg_expr);
      }
    } else eval: {
      val v = funcall1(eval_fun, item);
      if (fixnump(v))
        n = v;
      else if (stringp(v))
        sep = v;
      else
        uw_throwf(query_error_s,
                  lit("bad field format: modifier ~s expected to be fixnum or string"),
                  v, nao);
    }
  }

  return do_format_field(obj, n, sep, range_ix, plist, filter);
}

static val fmt_simple(val obj, val n, val sep,
                          val range_ix, val plist)
{
  return do_format_field(fmt_tostring(obj),
                         default_arg(n, zero),
                         default_arg(sep, lit(" ")),
                         default_null_arg(range_ix),
                         default_null_arg(plist),
                         nil);
}

static val fmt_flex(val obj, val plist, struct args *args)
{
  cnum ix = 0;
  val n = zero, sep = lit(" ");
  val range_ix = nil;

  while (args_more(args, ix)) {
    val arg = args_get(args, &ix);

    if (integerp(arg)) {
      n = arg;
    } else if (stringp(arg)) {
      sep = arg;
    } else if (rangep(arg)) {
      if (to(arg))
        range_ix = arg;
      else
        range_ix = from(arg);
    }
  }

  return do_format_field(fmt_tostring(obj), n, sep, range_ix, plist, nil);
}

val subst_vars(val forms, val env, val filter)
{
  list_collect_decl(out, iter);

  while (forms) {
    val form = first(forms);

    if (consp(form)) {
      val sym = first(form);

      if (sym == var_s) {
        val expr = second(form);
        val modifiers = third(form);
        val str = eval(expr, env, form);

        /* If the object is a sequence, we let format_field deal with the
           conversion to text, because the modifiers influence how
           it is done. */
        str = fmt_tostring(str);

        if (modifiers) {
          forms = cons(format_field(str, modifiers, filter,
                                    pa_123_1(func_n3(eval), env, form)),
                       rest(forms));
        } else {
          str = fmt_cat(str, lit(" "));
          forms = cons(filter_string_tree(filter, str), rest(forms));
        }

        continue;
      } else if (sym == quasi_s) {
        val nested = subst_vars(rest(form), env, filter);
        iter = list_collect_append(iter, nested);
        forms = cdr(forms);
        continue;
      } else {
        val str = eval(form, env, form);
        if (listp(str))
          str = cat_str(mapcar(func_n1(tostringp), str), lit(" "));
        else if (!stringp(str))
          str = tostringp(str);
        forms = cons(filter_string_tree(filter, tostringp(str)), rest(forms));
        continue;
      }
    } else if (bindable(form)) {
      forms = cons(cons(var_s, cons(form, nil)), cdr(forms));
      continue;
    }

    iter = list_collect(iter, form);
    forms = cdr(forms);
  }

  return out;
}

static val op_quasi_lit(val form, val env)
{
  return cat_str(subst_vars(rest(form), env, nil), nil);
}

val prof_call(val (*fun)(mem_t *ctx), mem_t *ctx)
{
  clock_t start_time = clock();
  alloc_bytes_t start_mlbytes = malloc_bytes;
  alloc_bytes_t start_gcbytes = gc_bytes;
  val result = fun(ctx);
  alloc_bytes_t delta_mlbytes = malloc_bytes - start_mlbytes;
  alloc_bytes_t delta_gcbytes = gc_bytes - start_gcbytes;
#if SIZEOF_ALLOC_BYTES_T > SIZEOF_PTR
  val dmb = if3(delta_mlbytes <= (alloc_bytes_t) INT_PTR_MAX,
                unum(delta_mlbytes),
                logior(ash(unum(delta_mlbytes >> 32), num_fast(32)),
                       unum(delta_mlbytes & 0xFFFFFFFF)));
  val dgc = if3(delta_gcbytes <= (alloc_bytes_t) INT_PTR_MAX,
                unum(delta_gcbytes),
                logior(ash(unum(delta_gcbytes >> 32), num_fast(32)),
                       unum(delta_gcbytes & 0xFFFFFFFF)));
#else
  val dmb = unum(delta_mlbytes);
  val dgc = unum(delta_gcbytes);
#endif

  return list(result,
              dmb, dgc,
              trunc(mul(num(clock() - start_time), num_fast(1000)), num_fast(CLOCKS_PER_SEC)),
              nao);
}

struct prof_ctx {
  val form, env;
};

static val op_prof_callback(mem_t *ctx)
{
  struct prof_ctx *pctx = coerce(struct prof_ctx *, ctx);
  return eval_progn(rest(pctx->form), pctx->env, pctx->form);
}

static val op_prof(val form, val env)
{
  struct prof_ctx ctx = { form, env };
  return prof_call(op_prof_callback, coerce(mem_t *, &ctx));
}

static val op_switch(val form, val env)
{
  val args = cdr(form);
  val expr = pop(&args);
  val branches = car(args);
  val index = eval(expr, env, expr);
  val forms = ref(branches, index);
  return eval_progn(forms, env, forms);
}

static val op_upenv(val form, val env)
{
  val args = cdr(form);
  val expr = pop(&args);
  type_check(car(form), env, ENV);
  return eval(expr, env->e.up_env, expr);
}

static val op_load_time_lit(val form, val env)
{
  (void) env;
  val args = cdr(form);
  if (car(args)) {
    return cadr(args);
  } else {
    rplaca(args, t);
    args = cdr(args);
    return sys_rplaca(args, eval(car(args), nil, form));
  }
}

static val me_def_variable(val form, val menv)
{
  val args = rest(form);
  val op = first(form);
  val sym = first(args);
  val initform = second(args);
  val mkspecial = if2(op == defvar_s || op == defparm_s,
                      cons(list(sys_mark_special_s,
                                list(quote_s, sym, nao), nao), nil));
  val setval = if2(op == defparm_s || op == defparml_s,
                   cons(list(set_s, sym, initform, nao), nil));
  val mksv = nappend2(mkspecial, setval);

  (void) menv;

  if (op != defvar_s && length(args) != two)
    eval_error(form, lit("~s: two arguments expected"), op, nao);

  if (!bindable(sym))
    not_bindable_error(form, sym);

  if (mkspecial) {
    mark_special(sym);
    if (uw_warning_exists(cons(var_s, sym)))
      eval_warn(form, lit("~s: global ~s marked special after lexical uses"),
                op, sym, nao);
  }

  return apply_frob_args(list(prog1_s,
                              cons(defvarl_s,
                                   cons(sym, if2(op == defvar_s,
                                                 cons(initform, nil)))),
                              mksv, nao));
}

static val get_var_syms(val vars)
{
  list_collect_decl (out, iter);
  for (; vars; vars = cdr(vars)) {
    val spec = car(vars);
    if (atom(spec))
      iter = list_collect(iter, spec);
    else
      iter = list_collect(iter, car(spec));
  }
  return out;
}

static val me_each(val form, val menv)
{
  uses_or2;
  val each = first(form);
  val args = rest(form);
  val vars = pop(&args);
  val star = or3(eq(each, each_star_s),
                 eq(each, collect_each_star_s),
                 eq(each, append_each_star_s));
  val var_syms = get_var_syms(vars);
  val specials_occur = some_satisfy(var_syms, func_n1(special_var_p), identity_f);
  val collect = or2(eq(each, collect_each_s), eq(each, collect_each_star_s));
  val append = or2(eq(each, append_each_s), eq(each, append_each_star_s));
  val eff_each = if3(collect, collect_each_s,
                     if3(append, append_each_s, each_s));
  (void) menv;
  return list(if3(star, let_star_s, let_s), vars,
              cons(each_op_s, cons(eff_each,
                                   cons(if3(!vars || star || specials_occur,
                                            var_syms, t),
                                        args))), nao);
}

static val me_for(val form, val menv)
{
  val forsym = first(form);
  val args = rest(form);
  val vars = first(args);
  val body = rest(args);
  int oldscope = opt_compat && opt_compat <= 123;
  val basic = list(if3(forsym == for_star_s, let_star_s, let_s),
                   vars, cons(for_op_s, cons(nil, body)), nao);
  (void) menv;
  return if3(oldscope,
             basic,
             list(block_s, nil, basic, nao));
}

static val me_gen(val form, val menv)
{
  (void) menv;
  return list(generate_s,
              list(lambda_s, nil, second(form), nao),
              list(lambda_s, nil, third(form), nao), nao);
}

static val me_gun(val form, val menv)
{
  val var = gensym(nil);
  val expr = second(form);
  (void) menv;
  return list(let_s, cons(var, nil),
              list(gen_s, list(set_s, var, expr, nao), var, nao), nao);
}

static val me_delay(val form, val menv)
{
  (void) menv;
  rlcp_tree(rest(form), second(form));
  return list(cons_s,
              cons(quote_s, cons(promise_s, nil)),
              list(cons_s, cons(lambda_s, cons(nil, rest(form))),
                   cons(quote_s, cons(form, nil)), nao),
              nao);
}

static val me_pprof(val form, val menv)
{
  (void) menv;
  return list(intern(lit("rt-pprof"), system_package),
              cons(prof_s, rest(form)), nao);
}

static val rt_pprof(val prof_list)
{
  val retval = pop(&prof_list);
  val malloc_bytes = pop(&prof_list);
  val gc_bytes = pop(&prof_list);
  val msecs = pop(&prof_list);

  format(t, lit("malloc bytes:  ~12a\n"
                "gc heap bytes: ~12a\n"
                "total:         ~12a\n"
                "milliseconds:  ~12a\n"),
         malloc_bytes, gc_bytes,
         plus(malloc_bytes, gc_bytes),
         msecs, nao);

  return retval;
}

static val me_when(val form, val menv)
{
  (void) menv;

  return if3(cdddr(form),
             cons(cond_s, cons(cdr(form), nil)),
             cons(if_s, cdr(form)));
}

static val me_unless(val form, val menv)
{
  val test = cadr(form);
  val body = cddr(form);

  (void) menv;

  return list(if_s, test, nil, maybe_progn(body), nao);
}

static val me_while_until(val form, val menv)
{
  val cond = cadr(form);
  val test = if3(car(form) == until_s, cons(not_s, cons(cond, nil)), cond);

  (void) menv;
  return apply_frob_args(list(for_s, nil, cons(test, nil), nil,
                              rest(rest(form)), nao));
}

static val me_while_until_star(val form, val menv)
{
  val once = gensym(lit("once-"));
  val cond = cadr(form);
  val test = if3(car(form) == until_star_s, cons(not_s, cons(cond, nil)), cond);

  (void) menv;
  return apply_frob_args(list(for_s, cons(list(once, t, nao), nil),
                              cons(list(or_s, once, test, nao), nil),
                              cons(list(set_s, once, nil, nao), nil),
                              rest(rest(form)), nao));
}


static val me_quasilist(val form, val menv)
{
  (void) menv;
  return cons(list_s, cdr(form));
}

static val imp_list_to_list(val list)
{
  list_collect_decl (out, ptail);

  for (; consp(list); list = cdr(list))
    ptail = list_collect(ptail, car(list));

  list_collect(ptail, list);
  return out;
}

static val dot_meta_list_p(val list)
{
  if (!consp(list))
    return list;

  list = cdr(list);

  for (; consp(list); list = cdr(list)) {
    val a = car(list);
    if (a == var_s || a == expr_s)
      return list;
  }

  return nil;
}

static val dot_to_apply(val form, val lisp1_p)
{
  val args = nil, meta = nil;

  if (opt_compat) {
    if (opt_compat <= 137)
      return form;
    if (opt_compat <= 184 && proper_list_p(form))
      return form;
  }

  if ((meta = dot_meta_list_p(form)) != nil) {
    val pfx = ldiff(form, meta);
    args = append2(cdr(pfx), cons(meta, nil));
  } else if (!proper_list_p(form)) {
    args = imp_list_to_list(cdr(form));
  }

  if (args) {
    val fun = car(form);
    val nofun = nil;

    while (fun == call_s) {
      fun = car(args);
      nofun = t;
      pop(&args);
    }
    return cons(sys_apply_s, cons(if3(lisp1_p || nofun,
                                      fun,
                                      list(fun_s, fun, nao)),
                                  args));
  }

  return form;
}

NORETURN static void dotted_form_error(val form)
{
  uw_throwf(error_s, lit("dotted argument ~!~s not supported by form"),
            form, nao);
}

val expand_forms(val form, val menv)
{
  if (atom(form)) {
    if (!form || (opt_compat && opt_compat <= 137))
      return form;
    dotted_form_error(form);
  } else {
    val f = car(form);
    val r = cdr(form);
    val ex_f = expand(f, menv);
    val ex_r = expand_forms(r, menv);

    if (ex_f == f && ex_r == r)
      return form;

    return rlcp(cons(ex_f, ex_r), form);
  }
}

static val expand_forms_ss(val forms, val menv, val ss_hash)
{
  val fh;

  if (atom(forms)) {
    if (!forms)
      return forms;
    dotted_form_error(forms);
  } else if ((fh = gethash(ss_hash, forms)) != nil) {
    return fh;
  } else {
    val f = car(forms);
    val r = cdr(forms);
    val ex_f = expand(f, menv);
    val ex_r = expand_forms_ss(r, menv, ss_hash);

    if (ex_f == f && ex_r == r)
      return sethash(ss_hash, forms, forms);

    return sethash(ss_hash, forms, rlcp(cons(ex_f, ex_r), forms));
  }
}

static val constantp(val form, val env_in);

static val expand_progn(val form, val menv)
{
  if (atom(form)) {
    return form;
  } else {
    val f = car(form);
    val r = cdr(form);
    val ex_f = expand(f, menv);
    val ex_r = expand_progn(r, menv);

    if (consp(ex_f) && car(ex_f) == progn_s) {
      if (ex_r)
        return expand_progn(rlcp_tree(append2(cdr(ex_f), ex_r), form), menv);
      return rlcp(cdr(ex_f), form);
    }

    if ((symbolp(ex_f) || constantp(ex_f, menv)) && ex_r)
      return rlcp(ex_r, form);

    if (ex_f == f && ex_r == r)
      return form;

    return rlcp(cons(ex_f, ex_r), form);
  }
}

static val expand_lisp1(val form, val menv)
{
tail:
  if (bindable(form)) {
    val symac_bind = lookup_symac_lisp1(menv, form);

    if (symac_bind) {
      val symac = cdr(symac_bind);
      if (symac == form)
        return form;
      form = rlcp_tree(symac, form);
      goto tail;
    }
    if (!lookup_var(menv, form) && !lookup_fun(menv, form) &&
        !uw_tentative_def_exists(cons(var_s, form)) &&
        !uw_tentative_def_exists(cons(fun_s, form)))
    {
      eval_defr_warn(uw_last_form_expanded(),
                     cons(sym_s, form),
                     lit("unbound variable/function ~s"), form, nao);
    }
    return form;
  }

  if (atom(form))
    return form;

  return expand(form, menv);
}

static val expand_forms_lisp1(val form, val menv)
{
  if (atom(form)) {
    if (!form || (opt_compat && opt_compat <= 137))
      return form;
    dotted_form_error(form);
  } else {
    val f = car(form);
    val r = cdr(form);
    val ex_f = expand_lisp1(f, menv);
    val ex_r = expand_forms_lisp1(r, menv);

    if (ex_f == f && ex_r == r)
      return form;
    return rlcp(cons(ex_f, ex_r), form);
  }
}

static val expand_cond_pairs(val form, val menv)
{
  if (atom(form)) {
    return form;
  } else {
    val pair = first(form);
    val others = rest(form);
    val pair_ex = expand_forms(pair, menv);
    val others_ex = expand_cond_pairs(others, menv);

    if (pair_ex == pair && others_ex == others)
      return form;
    return rlcp(cons(pair_ex, others_ex), form);
  }
}

static val consp_f, second_f, list_form_p_f, quote_form_p_f;
static val xform_listed_quote_f;

static void qquote_init(void)
{
  val eq_to_list_f = pa_12_1(eq_f, list_s);
  val eq_to_quote_f = pa_12_1(eq_f, quote_s);
  val cons_f = func_n2(cons);

  protect(&consp_f, &second_f, &list_form_p_f,
          &quote_form_p_f, &xform_listed_quote_f, convert(val *, 0));

  eq_to_list_f = pa_12_1(eq_f, list_s);
  consp_f = func_n1(consp);
  second_f = func_n1(second);
  list_form_p_f = andf(consp_f,
                       chain(car_f, eq_to_list_f, nao),
                       nao);
  quote_form_p_f = andf(consp_f,
                        chain(cdr_f, consp_f, nao),
                        chain(cdr_f, cdr_f, null_f, nao),
                        chain(car_f, eq_to_quote_f, nao),
                        nao);
  xform_listed_quote_f = iffi(andf(consp_f,
                                   chain(car_f, eq_to_list_f, nao),
                                   chain(cdr_f, consp_f, nao),
                                   chain(cdr_f, cdr_f, null_f, nao),
                                   chain(cdr_f, car_f, consp_f, nao),
                                   chain(cdr_f, car_f, car_f, eq_to_quote_f, nao),
                                   nao),
                              chain(cdr_f, car_f, cdr_f,
                                    pa_12_1(cons_f, nil),
                                    pa_12_2(cons_f, quote_s),
                                    nao),
                              nil);
}

static val optimize_qquote_form(val form)
{
  if (atom(form)) {
    return form;
  } else {
    val sym = car(form);
    val args = cdr(form);

    if (sym == append_s) {
      if (all_satisfy(args, list_form_p_f, nil))
      {
        sym = list_s;
        args = mappend(cdr_f, args);
      } else {
        val blargs = butlast(args, nil);

        if (all_satisfy(blargs, list_form_p_f, nil))
          return rlcp_tree(cons(list_star_s, nappend2(mappend(cdr_f, blargs),
                                                      last(args, one))), form);
      }
    }

    if (sym == list_s) {
      if (all_satisfy(args, quote_form_p_f, nil))
        return rlcp_tree(cons(quote_s, cons(mapcar(second_f, args), nil)), form);
      return rlcp_tree(cons(list_s, args), form);
    }

    if (sym == rcons_s) {
      if (args && cdr(args) && !cddr(args) &&
          all_satisfy(args, quote_form_p_f, nil))
      {
        val args_noq = mapcar(second_f, args);
        return rlcp(rcons(first(args_noq), second(args_noq)), form);
      }
      return form;
    }

    return form;
  }
}

static val optimize_qquote_args(val form)
{
  if (atom(form)) {
    return form;
  } else {
    val sym = car(form);
    val args = cdr(form);

    if (sym == list_s || sym == append_s || sym == list_star_s)
      return rlcp_tree(cons(sym, mapcar(xform_listed_quote_f, args)), form);

    return form;
  }
}

static val optimize_qquote(val form)
{
  return optimize_qquote_args(optimize_qquote_form(form));
}

static val expand_qquote(val qquoted_form, val qq, val unq, val spl);

static val expand_qquote_rec(val qquoted_form, val qq, val unq, val spl)
{
  if (nilp(qquoted_form)) {
    return nil;
  } else if (rangep(qquoted_form)) {
    val frexp = expand_qquote(from(qquoted_form), qq, unq, spl);
    val toexp = expand_qquote(to(qquoted_form), qq, unq, spl);
    return rlcp(list(rcons_s, frexp, toexp, nao), qquoted_form);
  } else if (tnodep(qquoted_form)) {
    val kyexp = expand_qquote(key(qquoted_form), qq, unq, spl);
    val leexp = expand_qquote(left(qquoted_form), qq, unq, spl);
    val riexp = expand_qquote(right(qquoted_form), qq, unq, spl);
    return rlcp(list(tnode_s, kyexp, leexp, riexp, nao), qquoted_form);
  } else if (atom(qquoted_form)) {
    return cons(quote_s, cons(qquoted_form, nil));
  } else {
    val sym = car(qquoted_form);

    if (sym == spl) {
      val error_msg = if3(spl == sys_splice_s,
                          lit("the splice ,*~s cannot occur outside of a list "
                              "or in the dotted position of a list"),
                          lit("(splice ~s) cannot occur outside of a list "
                              "or in the dotted position of a list"));
        eval_error(qquoted_form, error_msg,
                   second(qquoted_form), nao);
    } else if (sym == unq) {
      return second(qquoted_form);
    } else if (sym == qq) {
      return rlcp(expand_qquote_rec(expand_qquote(second(qquoted_form),
                                                  qq, unq, spl),
                                    qq, unq, spl),
                  qquoted_form);
    } else if (sym == hash_lit_s) {
      val args = expand_qquote(second(qquoted_form), qq, unq, spl);
      val pairs = expand_qquote(rest(rest(qquoted_form)), qq, unq, spl);
      return rlcp(list(hash_construct_s, args, pairs, nao), qquoted_form);
    } else if (sym == vector_lit_s) {
      val args = expand_qquote(second(qquoted_form), qq, unq, spl);
      return rlcp(list(vec_list_s, args, nao), qquoted_form);
    } else if (sym == struct_lit_s) {
      val args = expand_qquote(second(qquoted_form), qq, unq, spl);
      val pairs = expand_qquote(rest(rest(qquoted_form)), qq, unq, spl);
      return rlcp(list(make_struct_lit_s, args, pairs, nao), qquoted_form);
    } else if (sym == tree_lit_s) {
      val opts = expand_qquote(second(qquoted_form), qq, unq, spl);
      val keys = expand_qquote(rest(rest(qquoted_form)), qq, unq, spl);
      return rlcp(list(tree_construct_s, opts, keys, nao), qquoted_form);
    } else {
      val f = sym;
      val r = cdr(qquoted_form);
      val f_ex;
      val r_ex = expand_qquote_rec(r, qq, unq, spl);

      if (consp(f)) {
        val qsym = car(f);
        if (qsym == spl) {
          f_ex = second(f);
        } else if (qsym == unq) {
          f_ex = cons(list_s, cons(second(f), nil));
        } else if (qsym == qq) {
          f_ex = cons(list_s, cons(expand_qquote_rec(expand_qquote(second(f),
                                                                   qq, unq,
                                                                   spl),
                                                     qq, unq, spl), nil));
        } else {
          f_ex = cons(list_s, cons(expand_qquote(f, qq, unq, spl), nil));
        }
      } else {
        f_ex = cons(list_s, cons(expand_qquote(f, qq, unq, spl), nil));
      }

      if (nilp(r_ex)) {
        return rlcp_tree(cons(append_s, cons(f_ex, nil)), qquoted_form);
      } else if (atom(r_ex)) {
        return rlcp_tree(cons(append_s, cons(f_ex, cons(r_ex, nil))), qquoted_form);
      } else {
        if (consp(r) && car(r) == unq)
          r_ex = cons(r_ex, nil);
        else if (car(r_ex) == append_s)
          r_ex = cdr(r_ex);
        else if (car(r_ex) == quote_s)
          r_ex = cons(r_ex, nil);
        return rlcp_tree(cons(append_s, cons(f_ex, r_ex)), qquoted_form);
      }
    }
  }
  abort();
}

static val expand_qquote(val qquoted_form, val qq, val unq, val spl)
{
  val exp = expand_qquote_rec(qquoted_form, qq, unq, spl);
  return optimize_qquote(exp);
}


static val me_qquote(val form, val menv)
{
  (void) menv;
  if (first(form) == sys_qquote_s)
    return expand_qquote(second(form), sys_qquote_s,
                         sys_unquote_s, sys_splice_s);
  return expand_qquote(second(form), qquote_s, unquote_s, splice_s);
}

static val me_equot(val form, val menv)
{
  if (!cdr(form) || cddr(form))
    eval_error(form, lit("~s: one argument required"), car(form), nao);

  return rlcp(cons(quote_s, cons(expand(cadr(form), menv), nil)), form);
}

static val expand_vars(val vars, val menv, val form, int seq_p)
{
  val sym;

  if (nilp(vars)) {
    return nil;
  } else if (atom(vars)) {
    eval_error(form, lit("~a is an invalid variable binding syntax"),
               vars, nao);
    return vars;
  } else if (symbolp(sym = car(vars))) {
    val rest_vars = rest(vars);
    val menv_new = seq_p ? make_var_shadowing_env(menv, cons(sym, nil)) : menv;
    val rest_vars_ex = expand_vars(rest_vars, menv_new, form, seq_p);

    if (!bindable(sym))
      not_bindable_error(form, sym);
    else if (special_var_p(sym))
      sym = list(sym, list(dvbind_s, sym, nil, nao), nao);
    else if (rest_vars == rest_vars_ex)
      return vars;
    return rlcp(cons(sym, rest_vars_ex), vars);
  } else if (consp(sym)) {
    val stuff = sym;
    val var = pop(&stuff);
    val init = pop(&stuff);
    val rest_vars = rest(vars);
    /* This var's init form sees a previous symbol macro whose name is
       the same as the variable, so menv is used. */
    val init_ex = rlcp(expand(init, menv), init);
    /* The initforms of subsequent vars in a sequential binding
       do not see a previous symbol macro; they see the var. */
    val menv_new = seq_p ? make_var_shadowing_env(menv, cons(var, nil)) : menv;
    val rest_vars_ex = rlcp(expand_vars(rest_vars, menv_new, form, seq_p),
                            rest_vars);

    if (!bindable(var))
      not_bindable_error(form, var);

    if (stuff)
      eval_warn(form, lit("extra forms in var-init pair ~s"), sym, nao);

    if (special_var_p(var) && (atom(init_ex) || car(init_ex) != dvbind_s ||
                               cadr(init_ex) != var))
    {
      init_ex = rlcp(list(dvbind_s, var, init_ex, nao), init_ex);
    } else if (init == init_ex && rest_vars == rest_vars_ex) {
      return vars;
    }
    return rlcp(cons(cons(var, cons(init_ex, nil)), rest_vars_ex), vars);
  } else {
    eval_error(form, lit("variable binding expected, not ~s"), sym, nao);
  }
}

static val expand_fbind_vars(val vars, val menv, val form)
{
  val sym;

  if (nilp(vars)) {
    return nil;
  } else if (atom(vars)) {
    eval_error(form, lit("~a is an invalid function binding syntax"),
               vars, nao);
    return vars;
  } else if (symbolp(sym = car(vars))) {
    eval_error(form, lit("symbols in this construct require initforms"), nao);
  } else {
    cons_bind (var, init, sym);
    val rest_vars = rest(vars);
    /* This var's init form sees a previous macro whose name is
       the same as the symbol, so menv is used. */
    val init_ex = rlcp(expand_forms(init, menv), init);
    /* The initforms of subsequent vars in a sequential binding
       do not see a previous symbol macro; they see the var. */
    val rest_vars_ex = rlcp(expand_fbind_vars(rest_vars, menv, form),
                            rest_vars);

    builtin_reject_test(car(form), var, form, defun_s);

    if (init == init_ex && rest_vars == rest_vars_ex)
      return vars;
    return rlcp(cons(cons(var, init_ex), rest_vars_ex), vars);
  }
}

static val expand_var_mods(val mods, val menv)
{
  if (atom(mods))
    return mods;

  {
    val mod = car(mods);
    val modr = cdr(mods);

    if (mod == filter_k && consp(modr)) {
      val modrr = cdr(modr);
      val modrr_ex = expand_var_mods(modrr, menv);
      if (modrr_ex == modrr)
        return mods;

      return rlcp(cons(mod, cons(car(modr), modrr_ex)), mods);
    } else {
      val mod_ex = expand(mod, menv);
      val modr_ex = expand_var_mods(modr, menv);
      if (mod_ex == mod && modr_ex == modr)
        return mods;
      return rlcp(cons(mod_ex, modr_ex), mods);
    }
  }
}

val expand_quasi(val quasi_forms, val menv)
{
  if (nilp(quasi_forms)) {
    return nil;
  } else {
    val form = first(quasi_forms);
    val form_ex = form;

    if (consp(form)) {
      val sym = car(form);
      int comp_184 = opt_compat && opt_compat <= 184;

      if (!comp_184)
        form_ex = expand(form, menv);

      if (sym == var_s) {
        val param = second(form);
        val mods = third(form);
        val param_ex = expand(param, menv);
        val mods_ex = expand_var_mods(mods, menv);

        if (param_ex != param || mods_ex != mods)
          form_ex = rlcp(list(sym, param_ex, mods_ex, nao), form);
      } else {
        if (comp_184)
          form_ex = expand(form, menv);
      }
    }

    {
      val rest_forms = rest(quasi_forms);
      val rest_ex = expand_quasi(rest_forms, menv);

      if (form == form_ex && rest_ex == rest_forms)
        return quasi_forms;

      return rlcp(cons(form_ex, rest_ex), quasi_forms);
    }
  }
}

static val format_op_arg(val num)
{
  return format(nil, lit("arg-~,02s-"), num, nao);
}

static val meta_meta_p(val form)
{
  uses_or2;
  if (atom(form))
    return nil;
  if (cdr(form))
    return if2(car(form) == var_s && meta_meta_p(cdr(form)), t);
  return or2(integerp(car(form)), eq(car(form), rest_s));
}

static val meta_meta_strip(val args)
{
  uses_or2;
  val strip = cdr(args);
  return if3(or2(integerp(car(strip)), eq(car(strip), rest_s)),
             cons(var_s, strip),
             cons(expr_s, strip));
}

static val transform_op(val forms, val syms, val rg)
{
  if (atom(forms)) {
    return cons(syms, forms);
  } else {
    val fi = first(forms);
    val re = rest(forms);

    if (fi == expr_s && meta_meta_p(car(re)))
      return cons(syms, rlcp(meta_meta_strip(car(re)), forms));

    /* This handles improper list forms like (a b c . @42)
       when the recursion hits the @42 part. */
    if (fi == var_s && (integerp(car(re)) || car(re) == rest_s)) {
      cons_bind (outsyms, outforms, transform_op(cons(forms, nil), syms, rg));
      return cons(outsyms, rlcp(car(outforms), outforms));
    }

    if (consp(fi) && car(fi) == var_s && consp(cdr(fi))) {
      val vararg = car(cdr(fi));

      if (integerp(vararg)) {
        val newsyms = syms;
        val new_p;
        val cell = acons_new_c(vararg, mkcloc(new_p), mkcloc(newsyms));
        val sym = cdr(if3(new_p,
                          rplacd(cell, gensym(format_op_arg(vararg))),
                          cell));
        val sym_form = if3(cdr(cdr(fi)),
                           cons(var_s, cons(sym, cdr(cdr(fi)))), sym);
        cons_bind (outsyms, outforms, transform_op(re, newsyms, rg));
        return cons(outsyms, rlcp(cons(sym_form, outforms), outforms));
      } else if (vararg == rest_s) {
        val sym_form = if3(cdr(cdr(fi)),
                           cons(var_s, cons(rg, cdr(cdr(fi)))), rg);
        cons_bind (outsyms, outforms, transform_op(re, syms, rg));
        return cons(outsyms, rlcp(cons(sym_form, outforms), outforms));
      }
    }

    {
      cons_bind (fisyms, fiform, transform_op(fi, syms, rg));
      cons_bind (resyms, reforms, transform_op(re, fisyms, rg));
      return cons(resyms, rlcp(cons(fiform, reforms), fiform));
    }
  }
}

static val cons_find(val obj, val structure, val test)
{
  uses_or2;

  if (funcall2(test, obj, structure))
    return structure;
  if (atom(structure))
    return nil;
  return or2(cons_find(obj, car(structure), test),
             cons_find(obj, cdr(structure), test));
}

static val supplement_op_syms(val ssyms)
{
  list_collect_decl (outsyms, tl);
  val si, ni;

  for (si = ssyms, ni = one; si; ni = plus(ni, one), si = cdr(si))
  {
    val entry = car(si);
    val num = car(entry);

    for (; lt(ni, num); ni = plus(ni, one))
      tl = list_collect(tl, cons(ni, gensym(format_op_arg(ni))));
    tl = list_collect(tl, entry);
  }

  return outsyms;
}

static val me_op(val form, val menv)
{
  cons_bind (sym, body, form);
  uw_frame_t uw_handler;
  val new_menv = make_var_shadowing_env(menv, cons(rest_s, nil));
  val body_ex = (uw_push_handler(&uw_handler, cons(defr_warning_s, nil),
                                 func_n1v(uw_muffle_warning)),
                 if3(sym == op_s,
                    expand_forms_lisp1(body, new_menv),
                    expand(body, new_menv)));
  val rest_gensym = gensym(lit("rest-"));
  cons_bind (syms, body_trans, transform_op(body_ex, nil, rest_gensym));
  val ssyms = nsort(syms, func_n2(lt), car_f);
  val nums = mapcar(car_f, ssyms);
  val max = if3(nums, maxl(car(nums), cdr(nums)), zero);
  val min = if3(nums, minl(car(nums), cdr(nums)), zero);
  val has_rest = cons_find(rest_gensym, body_trans, eq_f);
  val is_op = and3(sym == do_s, consp(body_trans),
                   gethash(op_table, car(body_trans)));
  uw_pop_frame(&uw_handler);

  if (c_num(max, sym) > 1024)
    eval_error(form, lit("~a: @~a calls for function with too many arguments"),
               sym, max, nao);

  if (!eql(max, length(nums)) && !zerop(min))
    ssyms = supplement_op_syms(ssyms);

  rlcp(body_trans, body);

  {
    uses_or2;
    val dwim_body = rlcp_tree(cons(dwim_s,
                                   if3(or4(is_op, has_rest, ssyms,
                                           null(proper_list_p(body_trans))),
                                       body_trans,
                                       append2(body_trans, rest_gensym))),
                              body_trans);

    if (sym == do_s)
      dwim_body = rlcp(cdr(dwim_body), dwim_body);

    set_origin(dwim_body, form);

    return cons(lambda_s,
                cons(append2(mapcar(cdr_f, ssyms), rest_gensym),
                     cons(dwim_body, nil)));
  }
}

static val me_flet_labels(val form, val menv)
{
  val body = form;
  val sym = pop(&body);
  val funcs = pop(&body);
  list_collect_decl (lambdas, ptail);

  (void) menv;

  for (; funcs; funcs = cdr(funcs)) {
    val func = car(funcs);
    val name = pop(&func);
    val params = pop(&func);
    val lambda = cons(lambda_s, cons(params, func));

    ptail = list_collect (ptail, cons(name, cons(lambda, nil)));
  }

  return cons(if3(eq(sym, flet_s), fbind_s, lbind_s),
              cons(lambdas, body));
}

static val compares_with_eq(val obj)
{
  return tnil(fixnump(obj) || chrp(obj) || symbolp(obj));
}

static val hash_min_max(val env, val key, val value)
{
  cons_bind (minkey, maxkey, env);
  (void) value;
  if (!minkey || lt(key, minkey))
    minkey = key;
  if (!maxkey || gt(key, maxkey))
    maxkey = key;
  rplaca(env, minkey);
  rplacd(env, maxkey);
  return nil;
}

static val me_case(val form, val menv)
{
  val form_orig = form;
  val casesym = pop(&form);
  val testform = pop(&form);
  val tformsym = gensym(lit("test-"));
  val memfuncsym, eqfuncsym;
  val lofnil = cons(nil, nil);
  val star = tnil(casesym == caseq_star_s || casesym == caseql_star_s ||
                  casesym == casequal_star_s);
  int compat = (opt_compat && opt_compat <= 156 && !star);
  val comp_eq_f = func_n1(compares_with_eq);
  val integerp_f = func_n1(integerp);
  val chrp_f = func_n1(chrp);
  val all_keys_eq = t;
  val all_keys_integer = t;
  val all_keys_chr = t;
  val hash_fallback_clause = nil;
  val hash = nil;
  val index = zero;
  val idxsym = gensym(lit("index-"));
  list_collect_decl (condpairs, ptail);
  list_collect_decl (hashforms, qtail);

  (void) menv;

  if (atom(cdr(form_orig)))
    eval_error(form_orig, lit("~s: missing test form"), casesym, nao);

  if (casesym == caseq_s || casesym == caseq_star_s) {
    memfuncsym = memq_s;
    eqfuncsym = eq_s;
    hash = make_hash(nil, nil, nil);
  } else if (casesym == caseql_s || casesym == caseql_star_s) {
    memfuncsym = memql_s;
    eqfuncsym = eql_s;
    hash = make_hash(nil, nil, nil);
  } else {
    memfuncsym = memqual_s;
    eqfuncsym = equal_s;
    hash = make_hash(nil, nil, t);
  }

  for (; consp(form); form = cdr(form)) {
    cons_bind (clause, rest, form);
    cons_bind (keys, forms, clause);
    val hash_keys = if3(atom(keys), cons(keys, nil), keys);

    if (!rest && keys == t) {
      hash_fallback_clause = clause;
      ptail = list_collect(ptail, clause);
      break;
    }

    if (keys == t)
      eval_error(form_orig, lit("~s: symbol t used as key"), casesym, nao);

    if (star) {
      if (atom(keys))
        hash_keys = cons(keys = eval(keys, nil, form), nil);
      else
        hash_keys = keys = eval(cons(list_s, keys), nil, form);
    }

    if (consp(keys) && !cdr(keys))
      keys = car(keys);

    if (atom(keys)) {
      sethash(hash, keys, index);
      if (!compares_with_eq(keys))
        all_keys_eq = nil;
      if (!integerp(keys))
        all_keys_integer = nil;
      if (!chrp(keys))
        all_keys_chr = nil;
    } else {
      val iter;
      for (iter = hash_keys; iter; iter = cdr(iter))
        sethash(hash, car(iter), index);
      if (!all_satisfy(keys, comp_eq_f, nil))
        all_keys_eq = nil;
      if (!all_satisfy(keys, integerp_f, nil))
        all_keys_integer = nil;
      if (!all_satisfy(keys, chrp_f, nil))
        all_keys_chr = nil;
    }

    qtail = list_collect(qtail, forms);
    index = succ(index);

    if (compat) {
      ptail = list_collect(ptail,
                           cons(list(if3(atom(keys), eqfuncsym, memfuncsym),
                                     tformsym,
                                     if3(atom(keys),
                                         keys,
                                         list(quote_s, keys, nao)),
                                     nao),
                                forms));
    } else {
      uses_or2;
      ptail = list_collect(ptail,
                           cons(list(if3(atom(keys), eqfuncsym, memfuncsym),
                                     tformsym,
                                     list(quote_s, keys, nao),
                                     nao),
                                or2(forms, lofnil)));
    }
  }

  if (form && atom(form))
    eval_error(form_orig, lit("~s: improper form terminated by ~s"), casesym, form, nao);

  if (!compat && (all_keys_integer || all_keys_chr)) {
    val minmax = cons(nil, nil);
    val nkeys = (maphash(func_f2(minmax, hash_min_max), hash),
                 (hash_count(hash)));
    val empty = zerop(nkeys);
    val minkey = if3(empty, zero, car(minmax));
    val maxkey = if3(empty, zero, cdr(minmax));
    val i, range = minus(maxkey, minkey);
    val swres = gensym(lit("swres-"));
    val uniq = list(quote_s, make_sym(lit("nohit")), nao);
    val uniqf = cons(uniq, nil);

    if (ge(nkeys, num_fast(6)) && gt(nkeys, divi(mul(range, three), four)) &&
        ((casesym != caseq_s && casesym != caseq_star_s) ||
         (!bignump(minkey) && !bignump(maxkey))))
    {
      list_collect_decl (indexed_clauses, rtail);

      for (i = minkey; i <= maxkey; i = succ(i)) {
        val lookup = gethash_e(casesym, hash, i);
        rtail = list_collect(rtail, if3(lookup,
                                        ref(hashforms, cdr(lookup)),
                                        uniqf));
      }

      return list(let_s, list(list(tformsym, testform, nao),
                              list(swres, uniq, nao),
                              nao),
                  list(and_s,
                       list(intern(if3(all_keys_integer,
                                       lit("integerp"), lit("chrp")),
                                   user_package),
                            tformsym, nao),
                       list(intern(lit("<="), user_package),
                            minkey, tformsym, maxkey, nao),
                       list(set_s,
                            swres,
                            list(switch_s,
                                 if3(minkey == 0,
                                     tformsym,
                                     list(intern(lit("-"), user_package),
                                          tformsym, minkey, nao)),
                                 vec_list(indexed_clauses), nao),
                            nao), nao),
                  list(if_s,
                       list(eq_s, swres, uniq, nao),
                       cons(progn_s, cdr(hash_fallback_clause)),
                       swres,
                       nao), nao);
    }
  }

  if (!compat && gt(hash_count(hash), num_fast(10)) &&
      ((casesym != caseq_s && casesym != caseq_star_s) || all_keys_eq))
  {
    return list(let_star_s, list(list(tformsym, testform, nao),
                                 list(idxsym,
                                      list(intern(lit("gethash"), user_package),
                                           hash,
                                           tformsym,
                                           nao),
                                      nao),
                                 nao),
                list(if_s, idxsym,
                     list(switch_s, idxsym, vec_list(hashforms), nao),
                     cons(progn_s, cdr(hash_fallback_clause)),
                     nao), nao);
  }

  return list(let_s, cons(list(tformsym, testform, nao), nil),
              cons(cond_s, condpairs), nao);
}

static val me_prog2(val form, val menv)
{
  val arg1 = cadr(form);

  (void) menv;

  return list(progn_s, arg1,
              cons(prog1_s, cddr(form)), nao);
}

static val me_tb(val form, val menv)
{
  val opsym = pop(&form);
  val pat = pop(&form);
  val body = form;
  val args = gensym(lit("args-"));

  (void) opsym;
  (void) menv;

  return list(lambda_s, args,
              cons(tree_bind_s, cons(pat, cons(args, body))), nao);
}

static val me_tc(val form, val menv)
{
  val opsym = pop(&form);
  val cases = form;
  val args = gensym(lit("args-"));

  (void) opsym;
  (void) menv;

  return list(lambda_s, args,
              cons(tree_case_s, cons(args, cases)), nao);
}

static val me_ignerr(val form, val menv)
{
  (void) menv;
  return list(catch_s, cons(progn_s, rest(form)),
              list(error_s, error_s, nao), nao);
}

static val me_whilet(val form, val env)
{
  val body = form;
  val sym = pop(&body);
  val lets = pop(&body);
  val lastlet_cons = last(lets, nil);
  val lastlet = car(lastlet_cons);
  val not_done = gensym(lit("not-done"));

  (void) env;

  if (nilp(lastlet_cons))
    eval_error(form, lit("~s: empty binding list"), sym, nao);

  if (!cdr(lastlet)) {
    val var = car(lastlet);
    if (symbolp(var) && bindable(var))
      eval_warn(form, lit("~s: ~s is init-form here, not new variable"),
                sym, var, nao);
    push(gensym(nil), &lastlet);
    lets = append2(butlast(lets, nil), cons(lastlet, nil));
  }

  return list(let_s, cons(list(not_done, t, nao), nil),
              list(while_s, not_done,
                   list(let_star_s, lets,
                        list(if_s, car(lastlet),
                             cons(progn_s, body),
                             list(set_s, not_done, nil, nao), nao), nao), nao), nao);
}

static val me_iflet_whenlet(val form, val env)
{
  val args = form;
  val sym = pop(&args);
  val lets = pop(&args);

  (void) env;

  if (atom(lets)) {
    return apply_frob_args(list(if3(sym == iflet_s, if_s, when_s),
                                lets, args, nao));
  } else {
    val lastlet_cons = last(lets, nil);
    val lastlet = car(lastlet_cons);

    if (nilp(lastlet))
      eval_error(form, lit("~s: empty binding list"), sym, nao);

    if (!consp(lastlet))
      eval_error(form, lit("~s: bad binding syntax ~s"), sym, lastlet, nao);

    {
      val var = car(lastlet);

      if (!cdr(lastlet)) {
        if (symbolp(var) && bindable(var))
          eval_warn(form, lit("~s: ~s is init-form here, not new variable"),
                    sym, var, nao);
        push(gensym(nil), &lastlet);
        lets = append2(butlast(lets, nil), cons(lastlet, nil));
      }
    }

    return list(let_star_s, lets,
                cons(if3(sym == iflet_s, if_s, when_s),
                     cons(car(lastlet), args)), nao);
  }
}

static val me_dotimes(val form, val env)
{
  val count = gensym(lit("count-"));
  val args = rest(form);
  val spec = pop(&args);
  val counter = pop(&spec);
  val count_form = pop(&spec);
  val result = pop(&spec);
  val body = args;
  val lt = intern(lit("<"), user_package);
  val raw = list(for_s, list(list(counter, zero, nao),
                             list(count, count_form, nao),
                             nao),
                 list(list(lt, counter, count, nao), result, nao),
                 list(list(inc_s, counter, nao), nao),
                 body, nao);
  (void) env;

  return apply_frob_args(raw);
}

static val me_lcons(val form, val menv)
{
  val car_form = second(form);
  val cdr_form = third(form);
  val lc_sym = gensym(lit("lcons-"));
  val make_lazy_cons = intern(lit("make-lazy-cons"), user_package);
  val rplaca = intern(lit("rplaca"), user_package);
  val rplacd = intern(lit("rplacd"), user_package);
  (void) menv;

  return list(make_lazy_cons,
              list(lambda_s, cons(lc_sym, nil),
                   list(rplaca, lc_sym, car_form, nao),
                   list(rplacd, lc_sym, cdr_form, nao), nao), nao);
}

static val me_mlet(val form, val menv)
{
  uses_or2;
  val body = cdr(form);
  val bindings = pop(&body);
  val symacrolet = intern(lit("symacrolet"), user_package);
  val delay = intern(lit("delay"), user_package);

  list_collect_decl (ordinary_syms, ptail_osyms);
  list_collect_decl (syms, ptail_syms);
  list_collect_decl (inits, ptail_inits);
  list_collect_decl (gensyms, ptail_gensyms);
  list_collect_decl (smacs, ptail_smacs);
  list_collect_decl (sets, ptail_sets);

  (void) menv;

  for (; consp(bindings); bindings = cdr(bindings)) {
    val binding = car(bindings);

    if (atom(binding)) {
      if (!bindable(binding))
        not_bindable_error(form, binding);
      ptail_osyms = list_collect(ptail_osyms, binding);
    } else {
      val sym = car(binding);

      if (!bindable(sym))
        not_bindable_error(form, sym);

      if (cdr(binding)) {
        val init = car(cdr(binding));
        val gen = gensym(nil);
        ptail_syms = list_collect(ptail_syms, sym);
        ptail_inits = list_collect(ptail_inits, init);
        ptail_gensyms = list_collect(ptail_gensyms, gen);
        ptail_smacs = list_collect(ptail_smacs,
                                   list(sym, list(force_s, gen, nao), nao));
        ptail_sets = list_collect(ptail_sets,
                                  list(set_s, gen,
                                       list(delay, init, nao), nao));
      } else {
        ptail_osyms = list_collect(ptail_osyms, sym);
      }
    }
  }

  if (bindings)
    uw_throwf(error_s, lit("mlet: misplaced atom ~s in binding syntax"),
              bindings, nao);

  return list(let_s, append2(ordinary_syms, gensyms),
              apply_frob_args(list(symacrolet, smacs,
                                   append2(sets, or2(body, cons(nil, nil))),
                                   nao)), nao);
}

static val me_load_time(val form, val menv)
{
  val expr = cadr(form);
  (void) menv;
  return list(load_time_lit_s, nil, expr, nao);
}

static val me_load_for(val form, val menv)
{
  val sym = car(form);
  val args = cdr(form);
  val rt_load_for_s = intern(lit("rt-load-for"), system_package);
  list_collect_decl (out, ptail);
  val iter;

  (void) menv;

  for (iter = args; iter; iter = cdr(iter)) {
    val arg = car(iter);

    if (consp(arg)) {
      val kind = car(arg);
      if (kind != usr_var_s && kind != fun_s && kind != macro_s
          && kind != struct_s && kind != pkg_s)
        eval_error(form, lit("~s: unrecognized clause symbol ~s"),
                   sym, kind, nao);
      if (!bindable(cadr(arg)))
        eval_error(form, lit("~s: first argument in ~s must be bindable symbol"),
                   sym, arg, nao);
      if (length(arg) != three)
        eval_error(form, lit("~s: clause ~s expected to have two arguments"),
                   sym, arg, nao);
      ptail = list_collect(ptail, list(list_s,
                                       list(quote_s, car(arg), nao),
                                       list(quote_s, cadr(arg), nao),
                                       caddr(arg),
                                       nao));
    } else {
      eval_error(form, lit("~s: invalid clause ~s"), sym, arg, nao);
    }
  }

  if (!out)
    return nil;

  return cons(rt_load_for_s, out);
}

val load(val target)
{
  val self = lit("load");
  uses_or2;
  val parent = or2(load_path, null_string);
  val path = if3(!pure_rel_path_p(target),
                 target,
                 cat_str(nappend2(sub_list(split_str(parent, lit("/")),
                                           zero, negone),
                                  cons(target, nil)), lit("/")));
  val name, stream;
  val txr_lisp_p = t;
  val saved_dyn_env = dyn_env;
  val rec = cdr(lookup_var(saved_dyn_env, load_recursive_s));

  open_txr_file(path, &txr_lisp_p, &name, &stream);

  if (match_str(or2(get_line(stream), lit("")), lit("#!"), nil))
    parser_set_lineno(self, stream, two);
  else
    seek_stream(stream, zero, from_start_k);

  uw_simple_catch_begin;

  dyn_env = make_env(nil, nil, dyn_env);
  env_vbind(dyn_env, load_path_s, if3(opt_compat && opt_compat <= 215,
                                      path,
                                      stream_get_prop(stream, name_k)));
  env_vbind(dyn_env, load_recursive_s, t);
  env_vbind(dyn_env, package_s, cur_package);

  if (txr_lisp_p == t) {
    if (!read_eval_stream(self, stream, std_error)) {
      close_stream(stream, nil);
      uw_throwf(error_s, lit("~a: ~a contains errors"), self, path, nao);
    }
  } else if (txr_lisp_p == chr('o')) {
    if (!read_compiled_file(self, stream, std_error)) {
      close_stream(stream, nil);
      uw_throwf(error_s, lit("~a: unable to load compiled file ~a"),
                self, path, nao);
    }
  } else {
    int gc = gc_state(0);
    val parser_obj = ensure_parser(stream, name);
    parser_t *parser = parser_get_impl(self, parser_obj);
    parse_once(self, stream, name);
    gc_state(gc);

    close_stream(stream, nil);

    if (parser->errors) {
      uw_release_deferred_warnings();
      uw_throwf(query_error_s, lit("~a: parser errors in ~a"),
                self, path, nao);
    }

    {
      val match_ctx = uw_get_match_context();
      val bindings = cdr(match_ctx);
      val result = extract(parser->syntax_tree, nil, bindings);
      cons_bind (new_bindings, success, result);
      if (success)
        uw_set_match_context(cons(car(match_ctx), new_bindings));
    }
  }

  dyn_env = saved_dyn_env;

  if (!rec)
    uw_release_deferred_warnings();

  uw_unwind {
    close_stream(stream, nil);
    if (!rec)
      uw_dump_deferred_warnings(std_null);
  }

  uw_catch_end;

  return nil;
}

static val rt_load_for(struct args *args)
{
  val self = lit("sys:rt-load-for");
  cnum index = 0;

  while (args_more(args, index)) {
    val clause = args_get(args, &index);
    val kind = pop(&clause);
    val sym = pop(&clause);
    val file = car(clause);
    val (*testfun)(val);

    if (kind == usr_var_s)
      testfun = boundp;
    else if (kind == fun_s)
      testfun = fboundp;
    else if (kind == macro_s)
      testfun = mboundp;
    else if (kind == struct_s)
      testfun = find_struct_type;
    else if (kind == pkg_s)
      testfun = find_package;
    else
      uw_throwf(error_s, lit("~a: unrecognized kind ~s"),
                self, kind, nao);

    if (!testfun(sym)) {
      load(file);
      if (!testfun(sym))
        uw_throwf(error_s, lit("~a: file ~s didn't define ~a ~s"),
                  self, file, kind, sym, nao);
    }
  }

  return nil;
}

static val expand_catch_clause(val form, val menv)
{
  val sym = first(form);
  val params = second(form);
  val body = rest(rest(form));
  cons_bind (params_ex, body_ex0,
             expand_params(params, body, menv, nil, form));
  val new_menv = make_var_shadowing_env(menv, get_param_syms(params_ex));
  val body_ex = expand_forms(body_ex0, new_menv);
  if (!symbolp(sym))
    eval_error(form, lit("catch: ~s isn't a symbol"), sym, nao);
  if (body == body_ex && params == params_ex)
    return form;
  return rlcp(cons(sym, cons(params_ex, body_ex)), form);
}

static val expand_catch(val form, val menv)
{
  val args = form;
  val sym = pop(&args);
  val catch_syms = pop(&args);
  val try_form = pop(&args);
  val desc = pop(&args);
  val catch_clauses = args;
  val try_form_ex = expand(try_form, menv);
  val desc_ex = expand(desc, menv);
  val catch_clauses_ex = rlcp(mapcar(pa_12_1(func_n2(expand_catch_clause),
                                             menv),
                                     catch_clauses),
                              catch_clauses);

  if (try_form_ex == try_form && desc_ex == desc &&
      catch_clauses_ex == catch_clauses)
    return form;

  return rlcp(cons(sym,
                   cons(catch_syms,
                        cons(try_form_ex,
                             cons(desc_ex, catch_clauses_ex)))), form);
}

static val expand_list_of_form_lists(val lofl, val menv, val ss_hash)
{
  list_collect_decl (out, ptail);

  for (; lofl; lofl = cdr(lofl)) {
    val forms = car(lofl);
    val forms_ex = expand_forms_ss(forms, menv, ss_hash);
    ptail = list_collect(ptail, forms_ex);
  }

  return out;
}

static val expand_switch(val form, val menv)
{
  val sym = first(form);
  val args = rest(form);
  val expr = first(args);
  val branches = second(args);
  val expr_ex = expand(expr, menv);
  val branches_ex;
  val ss_hash = make_hash(nil, nil, nil);

  if (listp(branches)) {
    branches_ex = expand_list_of_form_lists(branches, menv, ss_hash);
  } else if (vectorp(branches)) {
    branches_ex = vec_list(expand_list_of_form_lists(list_vec(branches),
                                                     menv, ss_hash));
  } else {
    eval_error(form, lit("~s: representation of branches"), sym, nao);
  }
  return rlcp(cons(sym, cons(expr_ex, cons(branches_ex, nil))), form);
}

static val do_expand(val form, val menv)
{
  val macro = nil;

  menv = default_null_arg(menv);

again:
  if (nilp(form)) {
    return nil;
  } else if (bindable(form)) {
    val symac_bind = lookup_symac(menv, form);

    if (symac_bind) {
      val symac = cdr(symac_bind);
      if (symac == form)
        return form;
      return expand(rlcp_tree(symac, form), menv);
    }
    if (!lookup_var(menv, form))
      eval_defr_warn(uw_last_form_expanded(),
                     cons(var_s, form),
                     lit("unbound variable ~s"), form, nao);
    return form;
  } else if (atom(form)) {
    return form;
  } else {
    val sym = car(form);

    if (sym == let_s || sym == let_star_s)
    {
      val body = rest(rest(form));
      val vars = second(form);
      int seq_p = sym == let_star_s;
      val new_menv = make_var_shadowing_env(menv, vars);
      val body_ex = expand_progn(body, new_menv);
      val vars_ex = expand_vars(vars, menv, form, seq_p);
      if (body == body_ex && vars == vars_ex)
        return form;
      return rlcp(cons(sym, cons(vars_ex, body_ex)), form);
    } else if (sym == each_op_s) {
      val args = rest(form);
      val eachsym = first(args);
      val vars = second(args);
      val body = rest(rest(args));
      val body_ex = expand_progn(body, menv);

      if (body == body_ex)
        return form;

      return rlcp(cons(sym, cons(eachsym, cons(vars, body_ex))), form);
    } else if (sym == fbind_s || sym == lbind_s) {
      val body = rest(rest(form));
      val funcs = second(form);
      val new_menv = make_fun_shadowing_env(menv, funcs);
      val body_ex = expand_progn(body, new_menv);
      val funcs_ex = expand_fbind_vars(funcs,
                                       sym == lbind_s ? new_menv : menv, form);
      if (body == body_ex && funcs == funcs_ex) {
        return form;
      } else {
        return rlcp(cons(sym, cons(funcs_ex, body_ex)), form);
      }
    } else if (sym == block_s) {
      val name = second(form);
      val body = rest(rest(form));
      val body_ex = expand_progn(body, menv);
      if (body == body_ex)
        return form;
      return rlcp(cons(sym, cons(name, body_ex)), form);
    } else if (sym == return_from_s || sym == sys_abscond_from_s) {
      val name = second(form);
      val ret = third(form);
      val ret_ex = expand(ret, menv);
      if (ret == ret_ex)
        return form;
      return rlcp(list(sym, name, ret_ex, nao), form);
    } else if (sym == cond_s) {
      val pairs = rest(form);
      val pairs_ex = expand_cond_pairs(pairs, menv);

      if (pairs == pairs_ex)
        return form;
      return rlcp(cons(cond_s, pairs_ex), form);
    } else if (sym == defvarl_s || sym == defsymacro_s) {
      val name = second(form);
      val init = third(form);
      val init_ex = expand(init, menv);
      val form_ex = form;

      if (sym == defsymacro_s && length(form) != three)
        eval_error(form, lit("~s: two arguments expected"), sym, nao);

      if (!bindable(name))
        not_bindable_error(form, name);

      if (sym == defvarl_s)
        uw_register_tentative_def(cons(var_s, name));

      if (init != init_ex)
        form_ex = rlcp(cons(sym, cons(name, cons(init_ex, nil))), form);

      if (opt_compat && opt_compat <= 190 && sym == defsymacro_s) {
        val result = eval(if3(opt_compat && opt_compat <= 137,
                              form_ex, form),
                          make_env(nil, nil, nil), form);
        return cons(quote_s, cons(result, nil));
      }

      return form_ex;
    } else if (sym == lambda_s) {
      if (!cdr(form))
        eval_error(form, lit("~s: missing argument list"), sym, nao);

      if (atom(cdr(form)))
        eval_error(form, lit("~s: bad syntax"), sym, nao);

      {
        val params = second(form);
        val body = rest(rest(form));
        cons_bind (params_ex, body_ex0,
                   expand_params(params, body, menv, nil, form));
        val new_menv = make_var_shadowing_env(menv, get_param_syms(params_ex));
        val body_ex = expand_progn(body_ex0, new_menv);

        if (body == body_ex && params == params_ex)
          return form;
        return rlcp(cons(sym, cons(params_ex, body_ex)), form);
      }
    } else if (sym == defun_s || sym == defmacro_s) {
      val name = second(form);
      val params = third(form);

      builtin_reject_test(sym, name, form, sym);

      if (sym == defun_s)
        uw_register_tentative_def(cons(fun_s, name));

      {
        val body = rest(rest(rest(form)));
        cons_bind (params_ex, body_ex0,
                   expand_params(params, body, menv,
                                 eq(sym, defmacro_s), form));
        val inter_env = make_var_shadowing_env(menv, get_param_syms(params_ex));
        val new_menv = if3(sym == defun_s,
                           make_fun_shadowing_env(inter_env, cons(name, nil)),
                           inter_env);
        val body_ex = expand_progn(body_ex0, new_menv);
        val form_ex = form;

        if (body != body_ex || params != params_ex)
          form_ex = rlcp(cons(sym, cons(name, cons(params_ex, body_ex))), form);

        if (opt_compat && opt_compat <= 190 && sym == defmacro_s) {
          val result = eval(form_ex, make_env(nil, nil, nil), form);
          return cons(quote_s, cons(result, nil));
        }

        return form_ex;
      }
    } else if (sym == tree_case_s) {
      return expand_tree_case(form, menv);
    } else if (sym == tree_bind_s || sym == mac_param_bind_s) {
      val args = rest(form);
      val ctx_expr = if3(sym == mac_param_bind_s, pop(&args), nil);
      val params = pop(&args);
      val expr = pop(&args);
      val body = args;
      cons_bind (params_ex, body_ex0,
                 expand_params(params, body, menv, t, form));
      val new_menv = make_var_shadowing_env(menv, get_param_syms(params_ex));
      val ctx_expr_ex = expand(ctx_expr, menv);
      val body_ex = expand_progn(body_ex0, new_menv);
      val expr_ex = expand(expr, new_menv);

      if (sym == mac_param_bind_s) {
        if (ctx_expr_ex == ctx_expr && params_ex == params &&
            expr_ex == expr && body_ex == body)
          return form;
        return rlcp(cons(sym, cons(ctx_expr_ex,
                                   cons(params_ex,
                                        cons(expr_ex, body_ex)))), form);
      }

      if (params_ex == params && expr_ex == expr && body_ex == body)
        return form;
      return rlcp(cons(sym, cons(params_ex, cons(expr_ex, body_ex))), form);
    } else if (sym == fun_s) {
      val arg = second(form);
      if (consp(arg) && car(arg) == lambda_s) {
        val arg_ex = expand(arg, menv);
        return rlcp(list(sym, arg_ex, nao), form);
      }
      if (!lookup_fun(menv, arg)) {
        if (special_operator_p(arg))
          eval_warn(uw_last_form_expanded(),
                    lit("fun used on special operator ~s"), arg, nao);
        else if (!bindable(arg))
          eval_warn(uw_last_form_expanded(),
                    lit("~s appears in operator position"), arg, nao);
        else
          eval_defr_warn(uw_last_form_expanded(),
                         cons(fun_s, arg),
                         lit("unbound function ~s"),
                         arg, nao);
      }
      return form;
    } else if (sym == quote_s || sym == dvbind_s) {
      return form;
    } else if (sym == for_op_s) {
      val inits = second(form);
      val cond = third(form);
      val incs = fourth(form);
      val forms = rest(rest(rest(rest(form))));
      val inits_ex = expand_forms(inits, menv);
      val cond_ex = expand_forms(cond, menv);
      val incs_ex = expand_forms(incs, menv);
      val forms_ex = expand_progn(forms, menv);

      if (inits == inits_ex && cond == cond_ex &&
          incs == incs_ex && forms == forms_ex)
      {
        return form;
      } else {
        return rlcp(cons(sym, cons(inits_ex, cons(cond_ex,
                                                  cons(incs_ex,
                                                       forms_ex)))), form);
      }
    } else if (sym == dohash_s) {
      val spec = second(form);
      val keysym = first(spec);
      val valsym = second(spec);
      val hashform = third(spec);
      val resform = fourth(spec);
      val body = rest(rest(form));
      val hashform_ex = expand(hashform, menv);
      val resform_ex = expand(resform, menv);
      val new_menv = make_var_shadowing_env(menv, list(keysym, valsym, nao));
      val body_ex = expand_progn(body, new_menv);

      if (hashform == hashform_ex && resform == resform_ex && body == body_ex)
        return form;
      return rlcp_tree(cons(sym, cons(cons(keysym,
                                           cons(valsym,
                                                cons(hashform_ex,
                                                     cons(resform_ex, nil)))),
                                      body_ex)), form);
    } else if (sym == quasi_s) {
      val quasi = rest(form);
      val quasi_ex = expand_quasi(quasi, menv);
      if (quasi == quasi_ex)
        return form;
      return rlcp(cons(sym, quasi_ex), form);
    } else if (sym == sys_catch_s) {
      return expand_catch(form, menv);
    } else if (sym == handler_bind_s) {
      val args = rest(form);
      val fun = pop(&args);
      val handle_syms = pop(&args);
      val body = args;
      val fun_ex = expand(fun, menv);
      val body_ex = expand_forms(body, menv);

      if (!cddr(form))
        eval_error(form, lit("~s: missing arguments"), sym, nao);

      if (fun == fun_ex && body == body_ex)
        return form;

      return rlcp(cons(sym, cons(if3(fun == fun_ex,
                                     fun, fun_ex),
                                 cons(handle_syms,
                                      if3(body == body_ex,
                                          body, body_ex)))), form);
    } else if (sym == macro_time_s) {
      val args = rest(form);
      val result = nil;
      for (; args; args = cdr(args)) {
        val arg = car(args);
        val arg_ex = expand(arg, menv);
        result = eval(arg_ex, nil, args);
      }
      return maybe_quote(result);
    } else if (sym == macrolet_s) {
      return expand_macrolet(form, menv);
    } else if (sym == symacrolet_s) {
      return expand_symacrolet(form, menv);
    } else if (sym == dwim_s) {
      val args = rest(form);
      val args_ex = expand_forms_lisp1(dot_to_apply(args, t), menv);

      if (args == args_ex)
        return form;
      return rlcp(cons(sym, args_ex), form);
    } else if (sym == switch_s) {
      return expand_switch(form, menv);
    } else if (!macro && (macro = lookup_mac(menv, sym))) {
      val mac_expand = expand_macro(form, macro, menv);
      if (mac_expand == form)
        goto again;
      return expand(rlcp_tree(rlcp_tree(mac_expand, form), macro), menv);
    } else if (sym == progn_s) {
      val args = rest(form);

      if (cdr(args)) {
        val args_ex = expand_progn(args, menv);

        if (args == args_ex)
          return form;

        if (cdr(args_ex))
          return rlcp(cons(sym, args_ex), form);

        return car(args_ex);
      }
      return expand(first(args), menv);
    } else if (sym == sys_lisp1_value_s) {
      return expand_lisp1_value(form, menv);
    } else if (sym == sys_lisp1_setq_s) {
      return expand_lisp1_setq(form, menv);
    } else if (sym == setqf_s) {
      return expand_setqf(form, menv);
    } else if (sym == var_s || sym == expr_s) {
      return form;
    } else {
      /* funtion call expansion also handles: prog1, call, if, and, or,
         unwind-protect, return and other special forms whose arguments
         are evaluated */
      val form_ex = if3(special_operator_p(sym),
                        form,
                        dot_to_apply(form, nil));
      val insym = first(form_ex);
      val insym_ex = insym;
      val args = rest(form_ex);
      val args_ex = expand_forms(args, menv);

      if (sym == setq_s) {
        if (!args)
          eval_error(form, lit("~s: missing argument"), sym, nao);

        if (cddr(args))
          eval_error(form, lit("~s: excess arguments"), sym, nao);

        {
          val target = car(args_ex);

          if (!consp(target) || car(target) != var_s) {
            if (!bindable(target))
              not_bindable_warning(form, car(args_ex));
          }
        }
      }

      if (consp(insym) && car(insym) == lambda_s) {
        insym_ex = expand(insym, menv);
      } else if (!lookup_fun(menv, insym) && !special_operator_p(insym)) {
        if (!bindable(insym))
          eval_warn(uw_last_form_expanded(),
                    lit("~s appears in operator position"), insym, nao);
        else
          eval_defr_warn(uw_last_form_expanded(),
                         cons(fun_s, insym),
                         lit("unbound function ~s"),
                         insym, nao);
      }

      if (insym_ex == rcons_s &&
          proper_list_p(args_ex) && length(args_ex) == two &&
          constantp(car(args_ex), menv) &&
          constantp(cadr(args_ex), menv))
      {
        return rlcp(rcons(eval(car(args_ex), menv, form),
                          eval(cadr(args_ex), menv, form)), form);
      }

      if (insym_ex == insym && args_ex == args) {
        if (form_ex == form)
          return form;
        return form_ex;
      }

      return rlcp(cons(insym_ex, args_ex), form);
    }
    abort();
  }
}

val expand(val form, val menv)
{
  val ret = nil;
#if CONFIG_DEBUG_SUPPORT
  val is_cons = consp(form);
  uw_frame_t expand_fr;
  if (is_cons)
    uw_push_expand(&expand_fr, form, menv);
#endif

  sig_check_fast();

  ret = do_expand(form, menv);

  if (!lookup_origin(ret))
    set_origin(ret, form);

#if CONFIG_DEBUG_SUPPORT
  if (is_cons)
    uw_pop_frame(&expand_fr);
#endif
  return ret;
}

static val no_warn_expand(val form, val menv)
{
  val ret;
  uw_frame_t uw_handler;
  uw_push_handler(&uw_handler, cons(defr_warning_s, nil),
                  func_n1v(uw_muffle_warning));
  ret = expand(form, menv);
  uw_pop_frame(&uw_handler);
  return ret;
}

static val gather_free_refs(val info_cons, val exc, struct args *args)
{
  val self = lit("expand-with-free-refs");
  (void) exc;

  args_normalize_least(args, 2);

  if (args_count(args, self) == 2) {
    val tag = args_at(args, 1);
    cons_bind (kind, sym, tag);

    if (kind == var_s) {
      loc al = car_l(info_cons);
      if (!memq(sym, deref(al)))
        mpush(sym, al);
    } else if (kind == fun_s) {
      loc dl = cdr_l(info_cons);
      if (!memq(sym, deref(dl)))
        mpush(sym, dl);
    }
    uw_rthrow(continue_s, nil);
  }

  return nil;
}

static val gather_free_refs_nw(val info_cons, val exc,
                               struct args *args)
{
  gather_free_refs(info_cons, exc, args);
  return uw_rthrow(continue_s, nil);
}

val expand_with_free_refs(val form, val menv_in, val upto_menv_in)
{
  val ret;
  val menv = default_null_arg(menv_in);
  val upto_menv = default_null_arg(upto_menv_in);
  uw_frame_t uw_handler;
  val info_cons_free = cons(nil, nil);
  val info_cons_bound = cons(nil, nil);
  uw_push_handler(&uw_handler, cons(defr_warning_s, nil),
                  func_f1v(info_cons_free, gather_free_refs));
  ret = expand(form, menv);
  uw_pop_frame(&uw_handler);
  uw_push_handler(&uw_handler, cons(defr_warning_s, nil),
                  func_f1v(info_cons_bound, gather_free_refs_nw));
  (void) expand(ret,
                squash_menv_deleting_range(menv, upto_menv));
  uw_pop_frame(&uw_handler);
  return list(ret, car(info_cons_free), cdr(info_cons_free),
              car(info_cons_bound), cdr(info_cons_bound), nao);
}

val macro_form_p(val form, val menv)
{
  menv = default_null_arg(menv);

  if (bindable(form) && lookup_symac(menv, form))
    return t;
  if (consp(form) && lookup_mac(menv, car(form)))
    return t;
  return nil;
}

static val do_macroexpand_1(val form, val menv, val (*lookup)(val, val))
{
  val macro;
#if CONFIG_DEBUG_SUPPORT
  uw_frame_t expand_fr;
  uw_push_expand(&expand_fr, form, menv);
#endif

  menv = default_null_arg(menv);

  if (consp(form) && (macro = lookup_mac(menv, car(form)))) {
    val mac_expand = expand_macro(form, macro, menv);
    if (mac_expand != form)
      form = rlcp_tree(rlcp_tree(mac_expand, form), macro);
  } else if (bindable(form) && (macro = lookup(menv, form))) {
    val mac_expand = cdr(macro);
    if (mac_expand != form)
      form = rlcp_tree(mac_expand, macro);
  }

#if CONFIG_DEBUG_SUPPORT
  uw_pop_frame(&expand_fr);
#endif
  return form;
}

static val macroexpand_1(val form, val menv)
{
  return do_macroexpand_1(form, menv, lookup_symac);
}

static val macroexpand_1_lisp1(val form, val menv)
{
  return do_macroexpand_1(form, menv, lookup_symac_lisp1);
}

static val do_macroexpand(val form, val menv, val (*lookup_sm)(val, val))
{
  for (;;) {
    val mac_expand = do_macroexpand_1(form, menv, lookup_sm);
    if (mac_expand == form)
      return form;
    form = mac_expand;
  }
}

static val macroexpand(val form, val menv)
{
  return do_macroexpand(form, menv, lookup_symac);
}

static val macroexpand_lisp1(val form, val menv)
{
  return do_macroexpand(form, menv, lookup_symac_lisp1);
}

static val constantp_noex(val form)
{
  if (consp(form)) {
    return eq(car(form), quote_s);
  } else {
    if (bindable(form))
      return nil;
    return t;
  }
}

static val constantp(val form, val env_in)
{
  val env = default_null_arg(env_in);

  if (consp(form)) {
    if (car(form) == quote_s) {
      return t;
    } else if (macro_form_p(form, env)) {
      return constantp_noex(macroexpand(form, env));
    } else {
      return nil;
    }
  } else if (symbolp(form)) {
    if (!bindable(form)) {
      return t;
    } else if (macro_form_p(form, env)) {
      return constantp_noex(macroexpand(form, env));
    } else {
      return nil;
    }
  } else {
    return t;
  }
}

static val me_l1_val(val form, val menv)
{
  if (length(form) != two)
    eval_error(form, lit("~s: invalid syntax"), first(form), nao);

  {
    val expr = cadr(form);
    val expr_ex = macroexpand_lisp1(expr, menv);

    if (symbolp(expr_ex) && !constantp(expr_ex, nil)) {
      val binding_type = lexical_lisp1_binding(menv, expr_ex);

      if (binding_type == fun_k) {
        return list(fun_s, expr_ex, nao);
      } else if (binding_type == var_k) {
        return expr_ex;
      } else if (binding_type == nil) {
        if (boundp(expr_ex))
          return expr;
        return list(sys_lisp1_value_s, expr_ex, nao);
      } else {
        eval_error(form, lit("~s: invalid case"), car(form), nao);
      }
    }

    return expr;
  }
}

static val me_l1_setq(val form, val menv)
{
  if (!consp(cdr(form)) || !consp(cddr(form)) || cdddr(form))
    eval_error(form, lit("~s: invalid syntax"), car(form), nao);

  {
    val expr = cadr(form);
    val new_val = caddr(form);
    val expr_ex = macroexpand_lisp1(expr, menv);

    if (symbolp(expr_ex)) {
      val binding_type = lexical_lisp1_binding(menv, expr_ex);

      if (binding_type == var_k) {
        return list(setq_s, expr_ex, new_val, nao);
      } else if (binding_type == symacro_k) {
        eval_error(form, lit("~s: invalid use on symacro"), car(form), nao);
      } else if (boundp(expr_ex)) {
        return list(setq_s, expr_ex, new_val, nao);
      } else {
        return list(sys_lisp1_setq_s, expr_ex, new_val, nao);
      }
    }

    return list(set_s, expr, new_val, nao);
  }
}

static val rt_assert_fail(val file, val line, val expr,
                          val fmt, struct args *args)
{
  val str_stream = make_string_output_stream();

  if (!missingp(fmt)) {
    if (line && file) {
      format(str_stream, lit("assertion ~s failed in ~a:~a: "),
             expr, file, line, nao);
    } else {
      format(str_stream, lit("assertion ~s failed: "), expr, nao);
    }
    formatv(str_stream, fmt, args);
  } else {
    if (line && file) {
      format(str_stream, lit("assertion ~s failed in ~a:~a\n"),
             expr, file, line, nao);
    } else {
      format(str_stream, lit("assertion ~s failed"), expr, nao);
    }
  }

  uw_throw(assert_s, get_string_from_stream(str_stream));
  return nil;
}

static val me_assert(val form, val menv)
{
  cons_bind (line, file, source_loc(form));
  val extra_args = cddr(form);
  val rt_assert_fail = intern(lit("rt-assert-fail"), system_package);

  (void) menv;

  return list(or_s, cadr(form),
              apply_frob_args(list(rt_assert_fail, file, line,
                                   list(quote_s, cadr(form), nao),
                                   extra_args, nao)),
              nao);
}


static val return_star(val name, val retval)
{
  uw_block_return(name, retval);
  eval_error(nil, lit("return*: no block named ~s is visible"), name, nao);
  abort();
}

static val abscond_star(val name, val retval)
{
  uw_block_abscond(name, retval);
  eval_error(nil, lit("sys:abscond*: no block named ~s is visible"),
             name, nao);
  abort();
}

static val map_common(val self, val fun, struct args *lists,
                      loc (*collect_fn)(loc ptail, val obj),
                      val (*map_fn)(val fun, val seq))
{
  if (!args_more(lists, 0)) {
    return nil;
  } else if (!args_two_more(lists, 0)) {
    return map_fn(fun, args_atz(lists, 0));
  } else {
    cnum i, idx, argc = args_count(lists, self);
    val arg0 = args_at(lists, 0);
    seq_iter_t *iter_array = coerce(seq_iter_t *,
                                    alloca(argc * sizeof *iter_array));
    args_decl(args_fun, max(argc, ARGS_MIN));
    list_collect_decl (out, otail);

    for (i = 0, idx = 0; i < argc; i++)
    {
      val arg = args_get(lists, &idx);
      seq_iter_init(self, &iter_array[i], arg);
    }

    for (;;) {
      val fun_ret;

      for (i = 0; i < argc; i++) {
        val elem;
        seq_iter_t *iter = &iter_array[i];

        if (!seq_get(iter, &elem))
          return collect_fn != 0 ? make_like(out, arg0) : nil;

        args_fun->arg[i] = elem;
      }

      args_fun->fill = argc;
      args_fun->list = 0;

      fun_ret = generic_funcall(fun, args_fun);

      if (collect_fn != 0)
        otail = collect_fn(otail, fun_ret);
    }
  }
}

val mapcarv(val fun, struct args *lists)
{
  return map_common(lit("mapcar"), fun, lists, list_collect, mapcar);
}

val mapcarl(val fun, val list_of_lists)
{
  args_decl_list(args, ARGS_MIN, list_of_lists);
  return mapcarv(fun, args);
}

static val mappendv(val fun, struct args *lists)
{
  return map_common(lit("mappend"), fun, lists, list_collect_append, mappend);
}

static val mapdov(val fun, struct args *lists)
{
  return map_common(lit("mapdo"), fun, lists, 0, mapdo);
}

static val lazy_mapcar_func(val env, val lcons)
{
  us_cons_bind (fun, iter, env);

  us_rplaca(lcons, funcall1(fun, iter_item(iter)));
  us_rplacd(env, iter_step(iter));

  if (iter_more(iter))
    us_rplacd(lcons, make_lazy_cons(us_lcons_fun(lcons)));
  else
    us_rplacd(lcons, nil);
  return nil;
}

val lazy_mapcar(val fun, val list)
{
  val iter = iter_begin(list);
  if (!iter_more(iter))
    return nil;
  return make_lazy_cons(func_f1(cons(fun, iter), lazy_mapcar_func));
}

static val lazy_mapcarv_func(val env, val lcons)
{
  us_cons_bind (fun, list_of_iters, env);
  val args = mapcar(iter_item_f, list_of_iters);
  val next = mapcar(iter_step_f, list_of_iters);

  us_rplaca(lcons, apply(fun, z(args)));
  us_rplacd(env, next);

  if (all_satisfy(next, iter_more_f, identity_f))
    us_rplacd(lcons, make_lazy_cons(us_lcons_fun(lcons)));
  else
    us_rplacd(lcons, nil);
  return nil;
}

static val lazy_mapcarv(val fun, struct args *lists)
{
  if (!args_more(lists, 0)) {
    return nil;
  } else if (!args_two_more(lists, 0)) {
    return lazy_mapcar(fun, args_atz(lists, 0));
  } else {
    val list_of_lists = args_get_list(lists);
    if (!all_satisfy(list_of_lists, iter_more_f, identity_f)) {
      return nil;
    } else {
      val list_of_iters = mapcar(iter_begin_f, list_of_lists);
      return make_lazy_cons(func_f1(cons(fun, list_of_iters),
                                    lazy_mapcarv_func));
    }
  }
}

static val lazy_mapcarl(val fun, val list_of_lists)
{
  args_decl_list(args, ARGS_MIN, list_of_lists);
  return lazy_mapcarv(fun, args);
}

static val lazy_mappendv(val fun, struct args *lists)
{
  return lazy_appendl(lazy_mapcarv(fun, lists));
}

static val prod_common(val self, val fun, struct args *lists,
                       loc (*collect_fn)(loc ptail, val obj),
                       val (*mapv_fn)(val fun, struct args *lists))
{
  if (!args_more(lists, 0)) {
    return nil;
  } else if (!args_two_more(lists, 0)) {
    return mapv_fn(fun, lists);
  } else {
    cnum argc = args_count(lists, self), i;
    list_collect_decl (out, ptail);
    args_decl(args_reset, max(argc, ARGS_MIN));
    args_decl(args_work, max(argc, ARGS_MIN));
    args_decl(args_fun, max(argc, ARGS_MIN));
    args_copy(args_reset, lists);
    args_normalize_exact(args_reset, argc);
    args_work->fill = argc;

    for (i = 0; i < argc; i++)
      if (!iter_more((args_work->arg[i] = iter_begin(args_reset->arg[i]))))
        goto out;

    for (;;) {

      for (i = 0; i < argc; i++)
        args_fun->arg[i] = iter_item(args_work->arg[i]);

      args_fun->fill = argc;
      args_fun->list = 0;

      ptail = collect_fn(ptail, generic_funcall(fun, args_fun));

      for (i = argc - 1; ; i--) {
        val step_i = iter_step(args_work->arg[i]);
        if (iter_more(step_i)) {
          args_work->arg[i] = step_i;
          break;
        }
        if (i == 0)
          goto out;
        args_work->arg[i] = iter_begin(args_reset->arg[i]);
      }
    }
  out:
    return make_like(out, args_at(lists, 0));
  }
}

val maprodv(val fun, struct args *lists)
{
  return prod_common(lit("maprod"), fun, lists, list_collect, mapcarv);
}

val maprendv(val fun, struct args *lists)
{
  return prod_common(lit("maprend"), fun, lists, list_collect_append, mappendv);
}

static loc collect_nothing(loc ptail, val obj)
{
  (void) obj;
  return ptail;
}

static val maprodo(val fun, struct args *lists)
{
  return prod_common(lit("maprodo"), fun, lists, collect_nothing, mappendv);
}

static val symbol_value(val sym)
{
  uses_or2;

  return cdr(or2(lookup_var(nil, sym),
                 lookup_symac(nil, sym)));
}

static val symbol_function(val sym)
{
  uses_or2;

  if (opt_compat && opt_compat <= 127)
    return or2(or2(cdr(lookup_fun(nil, sym)),
                   cdr(lookup_mac(nil, sym))),
               gethash(op_table, sym));
  return cdr(lookup_fun(nil, sym));
}

static val symbol_macro(val sym)
{
  return cdr(lookup_mac(nil, sym));
}

val boundp(val sym)
{
  return if2(lookup_var(nil, sym) || lookup_symac(nil, sym), t);
}

val fboundp(val sym)
{
  if (opt_compat && opt_compat <= 127)
    return if2(lookup_fun(nil, sym) || lookup_mac(nil, sym) ||
               gethash(op_table, sym), t);
  return tnil(lookup_fun(nil, sym));
}

val mboundp(val sym)
{
  return tnil(lookup_mac(nil, sym));
}

val special_operator_p(val sym)
{
  return if2(gethash(op_table, sym), t);
}

static val makunbound(val sym)
{
  val env;

  lisplib_try_load(sym);

  if (!opt_compat || opt_compat > 143) {
    for (env = dyn_env; env; env = env->e.up_env) {
      val binding = assoc(sym, env->e.vbindings);
      if (binding) {
        rplacd(binding, unbound_s);
        return sym;
      }
    }
  }

  remhash(top_vb, sym);
  remhash(top_smb, sym);
  remhash(special, sym);

  vm_invalidate_binding(sym);

  return sym;
}

static val fmakunbound(val sym)
{
  lisplib_try_load(sym);
  remhash(top_fb, sym);
  if (opt_compat && opt_compat <= 127)
    remhash(top_mb, sym);
  vm_invalidate_binding(sym);
  return sym;
}

static val mmakunbound(val sym)
{
  lisplib_try_load(sym);
  remhash(top_mb, sym);
  return sym;
}

static val range_func(val env, val lcons)
{
  us_cons_bind (from, to_step, env);
  us_cons_bind (to, step, to_step);
  val next = if3(functionp(step),
                 funcall1(step, from),
                 plus(from, step));

  us_rplaca(lcons, from);

  if (to &&
      (numeq(from, to) ||
       ((lt(from, to) && gt(next, to)) ||
        (gt(from, to) && lt(next, to)))))
  {
    us_rplacd(lcons, nil);
    return nil;
  }

  us_rplacd(lcons, make_lazy_cons(us_lcons_fun(lcons)));
  us_rplaca(env, next);
  return nil;
}

static val range(val from_in, val to_in, val step_in)
{
  val from = default_arg(from_in, zero);
  val to = default_null_arg(to_in);
  val step = default_arg(step_in, if3(to && gt(from, to), negone, one));
  val env = cons(from, cons(to, step));

  return make_lazy_cons(func_f1(env, range_func));
}

static val range_star_func(val env, val lcons)
{
  us_cons_bind (from, to_step, env);
  us_cons_bind (to, step, to_step);
  val next = if3(functionp(step),
                 funcall1(step, from),
                 plus(from, step));

  us_rplaca(lcons, from);

  if (to &&
      (numeq(next, to) ||
       ((lt(from, to) && gt(next, to)) ||
       (gt(from, to) && lt(next, to)))))
  {
    rplacd(lcons, nil);
    return nil;
  }

  us_rplacd(lcons, make_lazy_cons(us_lcons_fun(lcons)));
  us_rplaca(env, next);
  return nil;
}

static val range_star(val from_in, val to_in, val step_in)
{
  val from = default_arg(from_in, zero);
  val to = default_null_arg(to_in);

  if (eql(from, to)) {
    return nil;
  } else {
    val step = default_arg(step_in, if3(to && gt(from, to), negone, one));
    val env = cons(from, cons(to, step));

    return make_lazy_cons(func_f1(env, range_star_func));
  }
}

static val rlist_fun(val item)
{
  if (rangep(item)) {
    val rto = to(item);
    return if3(rangep(rto),
               range(from(item), from(rto), to(rto)),
               range(from(item), rto, nil));
  }

  return cons(item, nil);
}

static val rlist_star_fun(val item)
{
  if (rangep(item)) {
    val rto = to(item);
    return if3(rangep(rto),
               range_star(from(item), from(rto), to(rto)),
               range_star(from(item), rto, nil));
  }

  return cons(item, nil);
}

static val rlist(struct args *items)
{
  args_decl_list(lists, ARGS_MIN,
                 lazy_mapcar(func_n1(rlist_fun), args_get_list(items)));
  return lazy_appendv(lists);
}

static val rlist_star(struct args *items)
{
  args_decl_list(lists, ARGS_MIN,
                 lazy_mapcar(func_n1(rlist_star_fun), args_get_list(items)));
  return lazy_appendv(lists);
}

static val generate_func(val env, val lcons)
{
  us_cons_bind (while_pred, gen_fun, env);

  if (!funcall(while_pred)) {
    rplacd(lcons, nil);
  } else {
    val next_item = funcall(gen_fun);
    val lcons_next = make_lazy_cons(us_lcons_fun(lcons));
    us_rplacd(lcons, lcons_next);
    us_rplaca(lcons_next, next_item);
  }
  return nil;
}

val generate(val while_pred, val gen_fun)
{
  if (!funcall(while_pred)) {
    return nil;
  } else {
    val first_item = funcall(gen_fun);
    val lc = make_lazy_cons(func_f1(cons(while_pred, gen_fun), generate_func));
    rplaca(lc, first_item);
    return lc;
  }
}

static val giterate_func(val env, val lcons)
{
  us_cons_bind (while_pred, gen_fun, env);
  val next_item = funcall1(gen_fun, us_car(lcons));

  if (funcall1(while_pred, next_item)) {
    val lcons_next = make_lazy_cons(us_lcons_fun(lcons));
    us_rplacd(lcons, lcons_next);
    us_rplaca(lcons_next, next_item);
  }
  return nil;
}

static val giterate(val while_pred, val gen_fun, val init_val)
{
  init_val = default_null_arg(init_val);

  if (!funcall1(while_pred, init_val)) {
    return nil;
  } else {
    val lc = make_lazy_cons(func_f1(cons(while_pred, gen_fun), giterate_func));
    us_rplaca(lc, init_val);
    return lc;
  }
}

static val ginterate_func(val env, val lcons)
{
  us_cons_bind (while_pred, gen_fun, env);
  val next_item = funcall1(gen_fun, us_car(lcons));

  if (funcall1(while_pred, next_item)) {
    val lcons_next = make_lazy_cons(us_lcons_fun(lcons));
    us_rplacd(lcons, lcons_next);
    us_rplaca(lcons_next, next_item);
  } else {
    us_rplacd(lcons, cons(next_item, nil));
  }
  return nil;
}

static val ginterate(val while_pred, val gen_fun, val init_val)
{
  init_val = default_null_arg(init_val);

  if (!funcall1(while_pred, init_val)) {
    return cons(init_val, nil);
  } else {
    val lc = make_lazy_cons(func_f1(cons(while_pred, gen_fun),
                                    ginterate_func));
    rplaca(lc, init_val);
    return lc;
  }
}

static val expand_right_fun(val env, val lcons)
{
  us_cons_bind (pair, gen_fun, env);
  us_cons_bind (elem, init_val, pair);

  val next_pair = funcall1(gen_fun, init_val);

  us_rplaca(lcons, elem);

  if (next_pair) {
    us_rplacd(lcons, make_lazy_cons(us_lcons_fun(lcons)));
    us_rplaca(env, next_pair);
  } else {
    us_rplacd(lcons, nil);
  }

  return nil;
}

static val expand_right(val gen_fun, val init_val)
{
  val pair = funcall1(gen_fun, init_val);

  if (!pair)
    return nil;

  return make_lazy_cons(func_f1(cons(pair, gen_fun), expand_right_fun));
}

static val expand_left(val gen_fun, val init_val)
{
  val out = nil;

  for (;;) {
    val pair = funcall1(gen_fun, init_val);
    if (pair) {
      cons_bind (elem, next, pair);
      init_val = next;
      out = cons(elem, out);
      continue;
    }

    return out;
  }
}

static val nexpand_left(val gen_fun, val init_val)
{
  val out = nil;

  for (;;) {
    val pair = funcall1(gen_fun, init_val);
    if (pair) {
      init_val = cdr(pair);
      out = rplacd(pair, out);
      continue;
    }

    return out;
  }
}

static val repeat_infinite_func(val env, val lcons)
{
  if (!us_car(env))
    us_rplaca(env, cdr(env));
  us_rplaca(lcons, pop(valptr(car_l(env))));
  us_rplacd(lcons, make_lazy_cons(us_lcons_fun(lcons)));
  return nil;
}

static val repeat_times_func(val env, val lcons)
{
  us_cons_bind (stack, list_count, env);
  us_cons_bind (list, count, list_count);

  if (!stack) {
    us_rplaca(env, list);
    us_rplacd(list_count, count = minus(count, one));
  }

  us_rplaca(lcons, pop(valptr(car_l(env))));

  if (!car(env) && count == one) {
    us_rplacd(lcons, nil);
    return nil;
  }

  us_rplacd(lcons, make_lazy_cons(us_lcons_fun(lcons)));
  return nil;
}

static val repeat(val list, val count)
{
  if (!list)
    return nil;

  if (!missingp(count)) {
    if (le(count, zero))
      return nil;
    return make_lazy_cons(func_f1(cons(list, cons(list, count)),
                                  repeat_times_func));
  }
  return make_lazy_cons(func_f1(cons(list, list), repeat_infinite_func));
}

static val pad_func(val env, val lcons)
{
  us_cons_bind (list, item_count, env);
  val next = cdr(list);

  us_rplaca(lcons, car(list));

  if (next && seqp(next)) {
    us_rplaca(env, next);
    us_rplacd(lcons, make_lazy_cons(us_lcons_fun(lcons)));
    return nil;
  } else if (!next) {
    val count = us_cdr(item_count);
    us_rplacd(item_count, nil);
    us_rplacd(lcons, repeat(item_count, count));
  } else {
    uw_throwf(error_s, lit("pad: cannot pad improper list terminated by ~s"),
              next, nao);
  }

  return nil;
}

static val pad(val seq_in, val item_in, val count)
{
  val item = default_null_arg(item_in);
  val seq = nullify(seq_in);

  switch (type(seq)) {
  case NIL:
    return repeat(cons(item, nil), count);
  case CONS:
    return append2(seq, repeat(cons(item, nil), count));
  case LCONS:
  case VEC:
  case LIT:
  case STR:
  case LSTR:
    return make_lazy_cons(func_f1(cons(seq, cons(item, count)), pad_func));
  default:
    uw_throwf(error_s, lit("pad: cannot pad ~s, only sequences"), seq, nao);
  }
}

static val weave_while(val env)
{
  cons_bind (uniq, tuples, env);
  val tuple;

  if (!tuples)
    return nil;

  tuple = remq(uniq, car(tuples), nil);

  if (!tuple)
    return nil;

  rplaca(tuples, tuple);
  return t;
}

static val weave_gen(val env)
{
  val tuples = cdr(env);
  val ret = car(tuples);
  rplacd(env, cdr(tuples));
  return ret;
}

static val weavev(struct args *args)
{
  val lists = args_get_list(args);
  val uniq = cons(nil, nil);
  val padded_lists = mapcar(pa_123_1(func_n3(pad), uniq, colon_k), lists);
  val tuples = lazy_mapcarl(list_f, padded_lists);
  val env = cons(uniq, tuples);
  val whil = func_f0(env, weave_while);
  val gen = func_f0(env, weave_gen);
  return lazy_appendl(generate(whil, gen));
}

static val promisep(val obj)
{
  if (consp(obj)) {
    val sym = car(obj);
    if (sym == promise_s || sym == promise_forced_s)
      return t;
  }

  return nil;
}

static val force(val promise)
{
  loc pstate = car_l(promise);
  val cd = cdr(promise);
  loc pval = car_l(cd);

  if (deref(pstate) == promise_forced_s) {
    return deref(pval);
  } else if (deref(pstate) == promise_s) {
    val ret;
    /* Safe: promise symbols are older generation */
    deref(pstate) = promise_inprogress_s;
    ret = funcall(deref(pval));
    deref(pstate) = promise_forced_s;
    set(pval, ret);
    return ret;
  } else if (deref(pstate) == promise_inprogress_s) {
    val form = second(cdr(cd));
    val sloc = source_loc_str(form, colon_k);
    eval_error(nil, lit("force: recursion forcing delayed form ~s (~a)"),
               form, sloc, nao);
  } else {
    uw_throwf(error_s, lit("force: ~s is not a promise"), promise, nao);
  }
}

static void reg_op(val sym, opfun_t fun)
{
  assert (sym != 0);
  sethash(op_table, sym, cptr(coerce(mem_t *, fun)));
}

void reg_fun(val sym, val fun)
{
  assert (sym != 0);
  sethash(top_fb, sym, cons(sym, fun));
  sethash(builtin, sym, defun_s);
}

void reg_mac(val sym, val fun)
{
  assert (sym != 0);
  sethash(top_mb, sym, cons(sym, fun));
  sethash(builtin, sym, defmacro_s);
}

void reg_varl(val sym, val val)
{
  assert (sym != nil);
  sethash(top_vb, sym, cons(sym, val));
}

void reg_var(val sym, val val)
{
  reg_varl(sym, val);
  mark_special(sym);
}

void reg_symacro(val sym, val form)
{
  loc pcdr = gethash_l(lit("internal initialization"), top_smb, sym, nulloc);
  val binding = deref(pcdr);

  if (binding)
    rplacd(binding, form);
  else
    set(pcdr, cons(sym, form));
}

static val if_fun(val cond, val then, val alt)
{
  return if3(cond, then, default_null_arg(alt));
}

static val or_fun(struct args *vals)
{
  cnum index = 0;

  while (args_more(vals, index)) {
    val item = args_get(vals, &index);
    if (item)
      return item;
  }
  return nil;
}

static val and_fun(struct args *vals)
{
  val item = t;
  cnum index = 0;

  while (args_more(vals, index)) {
    item = args_get(vals, &index);
    if (!item)
      return nil;
  }

  return item;
}

static val progn_fun(struct args *vals)
{
  return if3(vals->list, car(lastcons(vals->list)), vals->arg[vals->fill - 1]);
}

static val prog1_fun(struct args *vals)
{
  return if2(args_more(vals, 0), args_at(vals, 0));
}

static val prog2_fun(struct args *vals)
{
  args_normalize_least(vals, 2);
  return if2(vals->fill >= 2, vals->arg[1]);
}

static val not_null(val obj)
{
  return if3(nilp(obj), nil, t);
}

static val tf(struct args *args)
{
  (void) args;
  return t;
}

static val nilf(struct args *args)
{
  (void) args;
  return nil;
}

static val do_retf(val ret, struct args *args)
{
  (void) args;
  return ret;
}

val retf(val ret)
{
  return func_f0v(ret, do_retf);
}

static val do_apf(val fun, struct args *args)
{
  return applyv(fun, args);
}

static val do_args_apf(val dargs, struct args *args)
{
  val self = lit("apf");
  val fun = dargs->a.car;
  struct args *da = dargs->a.args;
  cnum da_nargs = da->fill + c_num(length(da->list), self);
  args_decl(args_call, max(args->fill + da_nargs, ARGS_MIN));
  args_copy(args_call, da);
  args_normalize_exact(args_call, da_nargs);
  args_cat_zap_from(args_call, args, 0);
  args_add_list(args_call, args->list);
  return applyv(fun, args_call);
}

static val apf(val fun, struct args *args)
{
  if (!args || !args_more(args, 0))
    return func_f0v(fun, do_apf);
  else
    return func_f0v(dyn_args(args, fun, nil), do_args_apf);
}

static val do_ipf(val fun, struct args *args)
{
  return iapply(fun, args);
}

static val do_args_ipf(val dargs, struct args *args)
{
  val self = lit("ipf");
  val fun = dargs->a.car;
  struct args *da = dargs->a.args;
  cnum da_nargs = da->fill + c_num(length(da->list), self);
  args_decl(args_call, max(args->fill + da_nargs, ARGS_MIN));
  args_copy(args_call, da);
  args_normalize_exact(args_call, da_nargs);
  args_cat_zap_from(args_call, args, 0);
  args_add_list(args_call, args->list);
  return iapply(fun, args_call);
}


static val ipf(val fun, struct args *args)
{
  if (!args || !args_more(args, 0))
    return func_f0v(fun, do_ipf);
  else
    return func_f0v(dyn_args(args, fun, nil), do_args_ipf);
}

static val callf(val func, struct args *funlist)
{
  val juxt_fun = juxtv(funlist);
  val apf_fun = apf(func, 0);
  return chain(juxt_fun, apf_fun, nao);
}

static val do_mapf(val env, struct args *args)
{
  cons_bind (fun, funlist, env);
  val mapped_args = mapcarl(call_f, cons(funlist, cons(args_get_list(args), nil)));
  return apply(fun, z(mapped_args));
}

static val mapf(val fun, struct args *funlist)
{
  return func_f0v(cons(fun, args_get_list(funlist)), do_mapf);
}

val prinl(val obj, val stream)
{
  val ret = obj_print(obj, default_arg(stream, std_output), nil);
  put_char(chr('\n'), stream);
  return ret;
}

val pprinl(val obj, val stream)
{
  val ret = obj_print(obj, default_arg(stream, std_output), t);
  put_char(chr('\n'), stream);
  return ret;
}

val tprint(val obj, val out)
{
  val self = lit("tprint");
  seq_info_t si = seq_info(obj);

  switch (si.kind) {
  case SEQ_NIL:
    break;
  case SEQ_LISTLIKE:
    {
      gc_hint(si.obj);
      gc_hint(obj);
      for (obj = z(si.obj); !endp(obj); obj = cdr(obj))
        tprint(car(obj), out);
    }
    break;
  case SEQ_VECLIKE:
    switch (si.type) {
    case LIT:
    case STR:
    case LSTR:
      put_line(obj, out);
      break;
    default:
      {
        val vec = si.obj;
        cnum i, len = c_fixnum(length(vec), self);

        for (i = 0; i < len; i++)
          tprint(ref(vec, num_fast(i)), out);

      }
      break;
    }
    break;
  case SEQ_NOTSEQ:
  case SEQ_HASHLIKE:
    pprinl(obj, out);
    break;
  }

  return nil;
}

static val merge_wrap(val seq1, val seq2, val lessfun, val keyfun)
{
  if (!nullify(seq1)) {
    if (type(seq1) == type(seq2))
      return seq2;
    return make_like(tolist(seq2), seq1);
  } else if (!nullify(seq2)) {
    if (type(seq1) == type(seq2))
      return seq1;
    return make_like(tolist(seq1), seq1);
  } else {
    val list1 = tolist(seq1);
    val list2 = tolist(seq2);

    keyfun = default_arg(keyfun, identity_f);
    lessfun = default_arg(lessfun, less_f);

    return make_like(merge(list1, list2, lessfun, keyfun), seq1);
  }
}

void eval_init(void)
{
  val not_null_f = func_n1(not_null);
  val me_each_f = func_n2(me_each);
  val me_for_f = func_n2(me_for);
  val length_f = func_n1(length);

  protect(&top_vb, &top_fb, &top_mb, &top_smb, &special, &builtin, &dyn_env,
          &op_table, &pm_table, &last_form_evaled,
          &call_f, &iter_begin_f, &iter_from_binding_f, &iter_more_f,
          &iter_item_f, &iter_step_f,
          &unbound_s, &origin_hash, convert(val *, 0));
  top_fb = make_hash(t, nil, nil);
  top_vb = make_hash(t, nil, nil);
  top_mb = make_hash(t, nil, nil);
  top_smb = make_hash(t, nil, nil);
  special = make_hash(t, nil, nil);
  builtin = make_hash(t, nil, nil);
  op_table = make_hash(nil, nil, nil);
  pm_table = make_hash(nil, nil, nil);

  call_f = func_n1v(generic_funcall);
  iter_begin_f = func_n1(iter_begin);
  iter_from_binding_f = chain(cdr_f, iter_begin_f, nao);
  iter_more_f = func_n1(iter_more);
  iter_item_f = func_n1(iter_item);
  iter_step_f = func_n1(iter_step);

  origin_hash = make_eq_hash(t, nil);

  dwim_s = intern(lit("dwim"), user_package);
  progn_s = intern(lit("progn"), user_package);
  prog1_s = intern(lit("prog1"), user_package);
  prog2_s = intern(lit("prog2"), user_package);
  sys_blk_s = intern(lit("blk"), system_package);
  let_s = intern(lit("let"), user_package);
  let_star_s = intern(lit("let*"), user_package);
  lambda_s = intern(lit("lambda"), user_package);
  fbind_s = intern(lit("fbind"), system_package);
  lbind_s = intern(lit("lbind"), system_package);
  flet_s = intern(lit("flet"), user_package);
  labels_s = intern(lit("labels"), user_package);
  call_s = intern(lit("call"), user_package);
  dvbind_s = intern(lit("dvbind"), system_package);
  sys_catch_s = intern(lit("catch"), system_package);
  handler_bind_s = intern(lit("handler-bind"), user_package);
  cond_s = intern(lit("cond"), user_package);
  caseq_s = intern(lit("caseq"), user_package);
  caseql_s = intern(lit("caseql"), user_package);
  casequal_s = intern(lit("casequal"), user_package);
  caseq_star_s = intern(lit("caseq*"), user_package);
  caseql_star_s = intern(lit("caseql*"), user_package);
  casequal_star_s = intern(lit("casequal*"), user_package);
  memq_s = intern(lit("memq"), user_package);
  memql_s = intern(lit("memql"), user_package);
  memqual_s = intern(lit("memqual"), user_package);
  eq_s = intern(lit("eq"), user_package);
  eql_s = intern(lit("eql"), user_package);
  equal_s = intern(lit("equal"), user_package);
  less_s = intern(lit("less"), user_package);
  if_s = intern(lit("if"), user_package);
  when_s = intern(lit("when"), user_package);
  usr_var_s = intern(lit("var"), user_package);
  iflet_s = intern(lit("iflet"), user_package);
  defvar_s = intern(lit("defvar"), user_package);
  defvarl_s = intern(lit("defvarl"), user_package);
  defparm_s = intern(lit("defparm"), user_package);
  defparml_s = intern(lit("defparml"), user_package);
  sys_mark_special_s = intern(lit("mark-special"), system_package);
  defun_s = intern(lit("defun"), user_package);
  defmacro_s = intern(lit("defmacro"), user_package);
  macro_s = intern(lit("macro"), user_package);
  defsymacro_s = intern(lit("defsymacro"), user_package);
  tree_case_s = intern(lit("tree-case"), user_package);
  tree_bind_s = intern(lit("tree-bind"), user_package);
  mac_param_bind_s = intern(lit("mac-param-bind"), user_package);
  setq_s = intern(lit("setq"), system_package);
  sys_lisp1_setq_s = intern(lit("lisp1-setq"), system_package);
  sys_lisp1_value_s = intern(lit("lisp1-value"), system_package);
  sys_l1_setq_s = intern(lit("l1-setq"), system_package);
  sys_l1_val_s = intern(lit("l1-val"), system_package);
  setqf_s = intern(lit("setqf"), system_package);
  inc_s = intern(lit("inc"), user_package);
  zap_s = intern(lit("zap"), user_package);
  for_s = intern(lit("for"), user_package);
  for_star_s = intern(lit("for*"), user_package);
  each_s = intern(lit("each"), user_package);
  each_star_s = intern(lit("each*"), user_package);
  collect_each_s = intern(lit("collect-each"), user_package);
  collect_each_star_s = intern(lit("collect-each*"), user_package);
  append_each_s = intern(lit("append-each"), user_package);
  append_each_star_s = intern(lit("append-each*"), user_package);
  for_op_s = intern(lit("for-op"), system_package);
  each_op_s = intern(lit("each-op"), system_package);
  dohash_s = intern(lit("dohash"), user_package);
  while_s = intern(lit("while"), user_package);
  while_star_s = intern(lit("while*"), user_package);
  until_star_s = intern(lit("until*"), user_package);
  uw_protect_s = intern(lit("unwind-protect"), user_package);
  return_s = intern(lit("return"), user_package);
  return_from_s = intern(lit("return-from"), user_package);
  sys_abscond_from_s = intern(lit("abscond-from"), system_package);
  block_star_s = intern(lit("block*"), user_package);
  car_s = intern(lit("car"), user_package);
  cdr_s = intern(lit("cdr"), user_package);
  not_s = intern(lit("not"), user_package);
  vecref_s = intern(lit("vecref"), user_package);
  list_s = intern(lit("list"), user_package);
  list_star_s = intern(lit("list*"), user_package);
  append_s = intern(lit("append"), user_package);
  apply_s = intern(lit("apply"), user_package);
  sys_apply_s = intern(lit("apply"), system_package);
  iapply_s = intern(lit("iapply"), user_package);
  gen_s = intern(lit("gen"), user_package);
  gun_s = intern(lit("gun"), user_package);
  generate_s = intern(lit("generate"), user_package);
  promise_s = intern(lit("promise"), system_package);
  promise_forced_s = intern(lit("promise-forced"), system_package);
  promise_inprogress_s = intern(lit("promise-inprogress"), system_package);
  force_s = intern(lit("force"), user_package);
  op_s = intern(lit("op"), user_package);
  do_s = intern(lit("do"), user_package);
  identity_s = intern(lit("identity"), user_package);
  rest_s = intern(lit("rest"), user_package);
  hash_lit_s = intern(lit("hash-lit"), system_package);
  hash_construct_s = intern(lit("hash-construct"), user_package);
  struct_lit_s = intern(lit("struct-lit"), system_package);
  qref_s = intern(lit("qref"), user_package);
  uref_s = intern(lit("uref"), user_package);
  vector_lit_s = intern(lit("vector-lit"), system_package);
  vec_list_s = intern(lit("vec-list"), user_package);
  tree_lit_s = intern(lit("tree-lit"), system_package);
  tree_construct_s = intern(lit("tree-construct"), system_package);
  macro_time_s = intern(lit("macro-time"), user_package);
  macrolet_s = intern(lit("macrolet"), user_package);
  symacrolet_s = intern(lit("symacrolet"), user_package);
  whole_k = intern(lit("whole"), keyword_package);
  form_k = intern(lit("form"), keyword_package);
  special_s = intern(lit("special"), system_package);
  unbound_s = make_sym(lit("unbound"));
  symacro_k = intern(lit("symacro"), keyword_package);
  prof_s = intern(lit("prof"), user_package);
  switch_s = intern(lit("switch"), system_package);
  struct_s = intern(lit("struct"), user_package);
  load_path_s = intern(lit("*load-path*"), user_package);
  load_recursive_s = intern(lit("*load-recursive*"), system_package);
  load_time_s = intern(lit("load-time"), user_package);
  load_time_lit_s  = intern(lit("load-time-lit"), system_package);
  eval_only_s  = intern(lit("eval-only"), user_package);
  compile_only_s = intern(lit("compile-only"), user_package);

  qquote_init();

  reg_op(macrolet_s, op_error);
  reg_op(symacrolet_s, op_error);
  reg_op(macro_time_s, op_error);
  reg_op(var_s, op_meta_error);
  reg_op(expr_s, op_meta_error);
  reg_op(quote_s, op_quote);
  reg_op(qquote_s, op_qquote_error);
  reg_op(sys_qquote_s, op_qquote_error);
  reg_op(unquote_s, op_unquote_error);
  reg_op(sys_unquote_s, op_unquote_error);
  reg_op(splice_s, op_unquote_error);
  reg_op(sys_splice_s, op_unquote_error);
  reg_op(progn_s, op_progn);
  reg_op(prog1_s, op_prog1);
  reg_op(let_s, op_let);
  reg_op(each_op_s, op_each);
  reg_op(let_star_s, op_let);
  reg_op(fbind_s, op_fbind);
  reg_op(lbind_s, op_fbind);
  reg_op(dvbind_s, op_dvbind);
  reg_op(lambda_s, op_lambda);
  reg_op(fun_s, op_fun);
  reg_op(cond_s, op_cond);
  reg_op(if_s, op_if);
  reg_op(and_s, op_and);
  reg_op(or_s, op_or);
  reg_op(defvarl_s, op_defvarl);
  reg_op(defun_s, op_defun);
  reg_op(defmacro_s, op_defmacro);
  reg_op(defsymacro_s, op_defsymacro);
  reg_op(tree_case_s, op_tree_case);
  reg_op(tree_bind_s, op_tree_bind);
  reg_op(mac_param_bind_s, op_mac_param_bind);
  reg_op(setq_s, op_setq);
  reg_op(sys_lisp1_setq_s, op_lisp1_setq);
  reg_op(sys_lisp1_value_s, op_lisp1_value);
  reg_op(setqf_s, op_setqf);
  reg_op(for_op_s, op_for);
  reg_op(dohash_s, op_dohash);
  reg_op(uw_protect_s, op_unwind_protect);
  reg_op(block_s, op_block);
  reg_op(block_star_s, op_block_star);
  reg_op(sys_blk_s, op_block);
  reg_op(return_s, op_return);
  reg_op(return_from_s, op_return_from);
  reg_op(sys_abscond_from_s, op_abscond_from);
  reg_op(dwim_s, op_dwim);
  reg_op(quasi_s, op_quasi_lit);
  reg_op(sys_catch_s, op_catch);
  reg_op(handler_bind_s, op_handler_bind);
  reg_op(prof_s, op_prof);
  reg_op(switch_s, op_switch);
  reg_op(intern(lit("upenv"), system_package), op_upenv);
  reg_op(compile_only_s, op_progn);
  reg_op(eval_only_s, op_progn);
  reg_op(load_time_lit_s, op_load_time_lit);

  reg_mac(defvar_s, func_n2(me_def_variable));
  reg_mac(defparm_s, func_n2(me_def_variable));
  reg_mac(defparml_s, func_n2(me_def_variable));
  reg_mac(each_s, me_each_f);
  reg_mac(each_star_s, me_each_f);
  reg_mac(collect_each_s, me_each_f);
  reg_mac(collect_each_star_s, me_each_f);
  reg_mac(append_each_s, me_each_f);
  reg_mac(append_each_star_s, me_each_f);
  reg_mac(for_s, me_for_f);
  reg_mac(for_star_s, me_for_f);
  reg_mac(gen_s, func_n2(me_gen));
  reg_mac(gun_s, func_n2(me_gun));
  reg_mac(intern(lit("delay"), user_package), func_n2(me_delay));
  if (opt_compat && opt_compat <= 184) {
    reg_mac(op_s, func_n2(me_op));
    reg_mac(do_s, func_n2(me_op));
  }
  reg_mac(sys_l1_val_s, func_n2(me_l1_val));
  reg_mac(sys_l1_setq_s, func_n2(me_l1_setq));
  reg_mac(qquote_s, func_n2(me_qquote));
  reg_mac(sys_qquote_s, func_n2(me_qquote));
  reg_mac(intern(lit("equot"), user_package), func_n2(me_equot));
  reg_mac(intern(lit("pprof"), user_package), func_n2(me_pprof));
  reg_mac(when_s, func_n2(me_when));
  reg_mac(intern(lit("unless"), user_package), func_n2(me_unless));
  reg_mac(while_s, func_n2(me_while_until));
  reg_mac(while_star_s, func_n2(me_while_until_star));
  reg_mac(until_s, func_n2(me_while_until));
  reg_mac(until_star_s, func_n2(me_while_until_star));
  reg_mac(quasilist_s, func_n2(me_quasilist));
  reg_mac(flet_s, func_n2(me_flet_labels));
  reg_mac(labels_s, func_n2(me_flet_labels));
  reg_mac(caseq_s, func_n2(me_case));
  reg_mac(caseql_s, func_n2(me_case));
  reg_mac(casequal_s, func_n2(me_case));
  reg_mac(caseq_star_s, func_n2(me_case));
  reg_mac(caseql_star_s, func_n2(me_case));
  reg_mac(casequal_star_s, func_n2(me_case));
  reg_mac(prog2_s, func_n2(me_prog2));
  reg_mac(intern(lit("tb"), user_package), func_n2(me_tb));
  reg_mac(intern(lit("tc"), user_package), func_n2(me_tc));
  reg_mac(intern(lit("ignerr"), user_package), func_n2(me_ignerr));
  reg_mac(intern(lit("whilet"), user_package), func_n2(me_whilet));
  reg_mac(iflet_s, func_n2(me_iflet_whenlet));
  reg_mac(intern(lit("whenlet"), user_package), func_n2(me_iflet_whenlet));
  reg_mac(intern(lit("dotimes"), user_package), func_n2(me_dotimes));
  reg_mac(intern(lit("lcons"), user_package), func_n2(me_lcons));
  reg_mac(intern(lit("mlet"), user_package), func_n2(me_mlet));
  reg_mac(load_time_s, func_n2(me_load_time));
  reg_mac(intern(lit("load-for"), user_package), func_n2(me_load_for));
  reg_mac(intern(lit("assert"), user_package), func_n2(me_assert));

  reg_fun(cons_s, func_n2(cons));
  reg_fun(intern(lit("make-lazy-cons"), user_package),
          func_n3o(make_lazy_cons_pub, 1));
  reg_fun(intern(lit("lcons-fun"), user_package), func_n1(lcons_fun));
  reg_fun(intern(lit("lcons-car"), user_package), func_n1(lcons_car));
  reg_fun(intern(lit("lcons-cdr"), user_package), func_n1(lcons_cdr));
  reg_fun(car_s, car_f);
  reg_fun(cdr_s, cdr_f);
  reg_fun(rplaca_s, func_n2(rplaca));
  reg_fun(rplacd_s, func_n2(rplacd));
  reg_fun(intern(lit("rplaca"), system_package), func_n2(sys_rplaca));
  reg_fun(intern(lit("rplacd"), system_package), func_n2(sys_rplacd));
  reg_fun(intern(lit("first"), user_package), car_f);
  reg_fun(rest_s, cdr_f);
  reg_fun(intern(lit("sub-list"), user_package), func_n3o(sub_list, 1));
  reg_fun(intern(lit("replace-list"), user_package), func_n4o(replace_list, 2));
  reg_fun(append_s, func_n0v(appendv));
  reg_fun(intern(lit("append*"), user_package), func_n0v(lazy_appendv));
  reg_fun(intern(lit("nconc"), user_package), func_n0v(nconcv));
  reg_fun(intern(lit("revappend"), user_package), func_n2(revappend));
  reg_fun(intern(lit("nreconc"), user_package), func_n2(nreconc));
  reg_fun(list_s, list_f);
  reg_fun(list_star_s, func_n0v(list_star_intrinsic));
  reg_fun(identity_s, identity_f);
  reg_fun(intern(lit("identity*"), user_package), identity_star_f);
  reg_fun(intern(lit("use"), user_package), identity_f);
  reg_fun(intern(lit("typeof"), user_package), func_n1(typeof));
  reg_fun(intern(lit("subtypep"), user_package), func_n2(subtypep));
  reg_fun(intern(lit("typep"), user_package), func_n2(typep));

  reg_fun(intern(lit("atom"), user_package), func_n1(atom));
  reg_fun(intern(lit("null"), user_package), null_f);
  reg_fun(intern(lit("false"), user_package), null_f);
  reg_fun(intern(lit("true"), user_package), not_null_f);
  reg_fun(intern(lit("have"), user_package), not_null_f);
  reg_fun(not_s, null_f);
  reg_fun(intern(lit("consp"), user_package), func_n1(consp));
  reg_fun(intern(lit("lconsp"), user_package), func_n1(lconsp));
  reg_fun(intern(lit("listp"), user_package), func_n1(listp));
  reg_fun(intern(lit("endp"), user_package), func_n1(endp));
  {
    val proper_list_p_f = func_n1(proper_list_p);
    reg_fun(intern(lit("proper-listp"), user_package), proper_list_p_f);
    reg_fun(intern(lit("proper-list-p"), user_package), proper_list_p_f);
  }
  reg_fun(intern(lit("length-list"), user_package), func_n1(length_list));

  reg_fun(intern(lit("mapcar"), user_package), func_n1v(mapcarv));
  reg_fun(intern(lit("mapcar*"), user_package), func_n1v(lazy_mapcarv));
  reg_fun(intern(lit("mappend"), user_package), func_n1v(mappendv));
  reg_fun(intern(lit("mappend*"), user_package), func_n1v(lazy_mappendv));
  reg_fun(intern(lit("mapdo"), user_package), func_n1v(mapdov));
  reg_fun(intern(lit("maprod"), user_package), func_n1v(maprodv));
  reg_fun(intern(lit("maprend"), user_package), func_n1v(maprendv));
  reg_fun(intern(lit("maprodo"), user_package), func_n1v(maprodo));
  reg_fun(intern(lit("window-map"), user_package), func_n4(window_map));
  reg_fun(intern(lit("window-mappend"), user_package), func_n4(window_mappend));
  reg_fun(intern(lit("window-mapdo"), user_package), func_n4(window_mapdo));
  {
    val apply_f = func_n1v(applyv);
    reg_fun(apply_s, apply_f);
    reg_fun(sys_apply_s, apply_f);
  }
  reg_fun(iapply_s, func_n1v(iapply));
  reg_fun(call_s, call_f);
  reg_fun(intern(lit("reduce-left"), user_package), func_n4o(reduce_left, 2));
  reg_fun(intern(lit("reduce-right"), user_package), func_n4o(reduce_right, 2));
  reg_fun(intern(lit("transpose"), user_package), func_n1(transpose));
  reg_fun(intern(lit("zip"), user_package), func_n0v(transposev));
  reg_fun(intern(lit("interpose"), user_package), func_n2(interpose));

  reg_fun(intern(lit("second"), user_package), second_f);
  reg_fun(intern(lit("third"), user_package), func_n1(third));
  reg_fun(intern(lit("fourth"), user_package), func_n1(fourth));
  reg_fun(intern(lit("fifth"), user_package), func_n1(fifth));
  reg_fun(intern(lit("sixth"), user_package), func_n1(sixth));
  reg_fun(intern(lit("seventh"), user_package), func_n1(seventh));
  reg_fun(intern(lit("eighth"), user_package), func_n1(eighth));
  reg_fun(intern(lit("ninth"), user_package), func_n1(ninth));
  reg_fun(intern(lit("tenth"), user_package), func_n1(tenth));
  reg_fun(intern(lit("conses"), user_package), func_n1(conses));
  reg_fun(intern(lit("conses*"), user_package), func_n1(lazy_conses));
  reg_fun(intern(lit("copy-list"), user_package), func_n1(copy_list));
  reg_fun(intern(lit("nreverse"), user_package), func_n1(nreverse));
  reg_fun(intern(lit("reverse"), user_package), func_n1(reverse));
  reg_fun(intern(lit("ldiff"), user_package),
          func_n2(if3(opt_compat && opt_compat <= 190, ldiff_old, ldiff)));
  reg_fun(intern(lit("last"), user_package), func_n2o(last, 1));
  reg_fun(intern(lit("butlast"), user_package), func_n2o(butlast, 1));
  reg_fun(intern(lit("nthlast"), user_package), func_n2(nthlast));
  reg_fun(intern(lit("nthcdr"), user_package), func_n2(nthcdr));
  reg_fun(intern(lit("nth"), user_package), func_n2(nth));
  reg_fun(intern(lit("butlastn"), user_package), func_n2(butlastn));
  reg_fun(intern(lit("flatten"), user_package), func_n1(flatten));
  reg_fun(intern(lit("flatten*"), user_package), func_n1(lazy_flatten));
  reg_fun(intern(lit("flatcar"), user_package), func_n1(flatcar));
  reg_fun(intern(lit("flatcar*"), user_package), func_n1(lazy_flatcar));
  reg_fun(intern(lit("tuples"), user_package), func_n3o(tuples, 2));
  reg_fun(intern(lit("partition-by"), user_package), func_n2(partition_by));
  reg_fun(intern(lit("partition"), user_package), func_n2(partition));
  reg_fun(intern(lit("split"), user_package), func_n2(split));
  reg_fun(intern(lit("split*"), user_package), func_n2(split_star));
  reg_fun(intern(lit("partition*"), user_package), func_n2(partition_star));
  reg_fun(intern(lit("tailp"), user_package), func_n2(tailp));
  reg_fun(memq_s, func_n2(memq));
  reg_fun(memql_s, func_n2(memql));
  reg_fun(memqual_s, func_n2(memqual));
  reg_fun(intern(lit("rmemq"), user_package), func_n2(rmemq));
  reg_fun(intern(lit("rmemql"), user_package), func_n2(rmemql));
  reg_fun(intern(lit("rmemqual"), user_package), func_n2(rmemqual));
  reg_fun(intern(lit("member"), user_package), func_n4o(member, 2));
  reg_fun(intern(lit("rmember"), user_package), func_n4o(rmember, 2));
  reg_fun(intern(lit("member-if"), user_package), func_n3o(member_if, 2));
  reg_fun(intern(lit("rmember-if"), user_package), func_n3o(rmember_if, 2));
  reg_fun(intern(lit("remq"), user_package), func_n3o(remq, 2));
  reg_fun(intern(lit("remql"), user_package), func_n3o(remql, 2));
  reg_fun(intern(lit("remqual"), user_package), func_n3o(remqual, 2));
  reg_fun(intern(lit("remove-if"), user_package), func_n3o(remove_if, 2));
  reg_fun(intern(lit("keepq"), user_package), func_n3o(keepq, 2));
  reg_fun(intern(lit("keepql"), user_package), func_n3o(keepql, 2));
  reg_fun(intern(lit("keepqual"), user_package), func_n3o(keepqual, 2));
  reg_fun(intern(lit("keep-if"), user_package), func_n3o(keep_if, 2));
  reg_fun(intern(lit("remq*"), user_package), func_n2(remq_lazy));
  reg_fun(intern(lit("remql*"), user_package), func_n2(remql_lazy));
  reg_fun(intern(lit("remqual*"), user_package), func_n2(remqual_lazy));
  reg_fun(intern(lit("remove-if*"), user_package), func_n3o(remove_if_lazy, 2));
  reg_fun(intern(lit("keep-if*"), user_package), func_n3o(keep_if_lazy, 2));
  reg_fun(intern(lit("tree-find"), user_package), func_n3o(tree_find, 2));
  reg_fun(intern(lit("countqual"), user_package), func_n2(countqual));
  reg_fun(intern(lit("countql"), user_package), func_n2(countql));
  reg_fun(intern(lit("countq"), user_package), func_n2(countq));
  reg_fun(intern(lit("count-if"), user_package), func_n3o(count_if, 2));
  reg_fun(intern(lit("posqual"), user_package), func_n2(posqual));
  reg_fun(intern(lit("rposqual"), user_package), func_n2(rposqual));
  reg_fun(intern(lit("posql"), user_package), func_n2(posql));
  reg_fun(intern(lit("rposql"), user_package), func_n2(rposql));
  reg_fun(intern(lit("posq"), user_package), func_n2(posq));
  reg_fun(intern(lit("rposq"), user_package), func_n2(rposq));
  reg_fun(intern(lit("pos"), user_package), func_n4o(pos, 2));
  reg_fun(intern(lit("rpos"), user_package), func_n4o(rpos, 2));
  reg_fun(intern(lit("pos-if"), user_package), func_n3o(pos_if, 2));
  reg_fun(intern(lit("rpos-if"), user_package), func_n3o(rpos_if, 2));
  reg_fun(intern(lit("some"), user_package), func_n3o(some_satisfy, 1));
  reg_fun(intern(lit("all"), user_package), func_n3o(all_satisfy, 1));
  reg_fun(intern(lit("none"), user_package), func_n3o(none_satisfy, 1));
  reg_fun(intern(lit("multi"), user_package), func_n1v(multi));
  reg_fun(eq_s, eq_f);
  reg_fun(eql_s, eql_f);
  reg_fun(equal_s, equal_f);
  reg_fun(intern(lit("meq"), user_package), func_n1v(meq));
  reg_fun(intern(lit("meql"), user_package), func_n1v(meql));
  reg_fun(intern(lit("mequal"), user_package), func_n1v(mequal));
  reg_fun(intern(lit("neq"), user_package), func_n2(neq));
  reg_fun(intern(lit("neql"), user_package), func_n2(neql));
  reg_fun(intern(lit("nequal"), user_package), func_n2(nequal));

  reg_fun(intern(lit("max"), user_package), func_n1v(maxv));
  reg_fun(intern(lit("min"), user_package), func_n1v(minv));
  reg_fun(intern(lit("clamp"), user_package), func_n3(clamp));
  reg_fun(intern(lit("bracket"), user_package), func_n1v(bracket));
  reg_fun(intern(lit("pos-max"), user_package), func_n3o(pos_max, 1));
  reg_fun(intern(lit("pos-min"), user_package), func_n3o(pos_min, 1));
  reg_fun(intern(lit("mismatch"), user_package), func_n4o(mismatch, 2));
  reg_fun(intern(lit("rmismatch"), user_package), func_n4o(rmismatch, 2));
  reg_fun(intern(lit("starts-with"), user_package), func_n4o(starts_with, 2));
  reg_fun(intern(lit("ends-with"), user_package), func_n4o(ends_with, 2));
  reg_fun(intern(lit("take"), user_package), func_n2(take));
  reg_fun(intern(lit("take-while"), user_package), func_n3o(take_while, 2));
  reg_fun(intern(lit("take-until"), user_package), func_n3o(take_until, 2));
  reg_fun(intern(lit("drop"), user_package), func_n2(drop));
  reg_fun(intern(lit("drop-while"), user_package), func_n3o(drop_while, 2));
  reg_fun(intern(lit("drop-until"), user_package), func_n3o(drop_until, 2));
  reg_fun(intern(lit("in"), user_package), func_n4o(in, 2));

  reg_fun(intern(lit("sort-group"), user_package), func_n3o(sort_group, 1));
  reg_fun(intern(lit("unique"), user_package), func_n2ov(unique, 1));
  reg_fun(intern(lit("uniq"), user_package), func_n1(uniq));
  reg_fun(intern(lit("grade"), user_package), func_n3o(grade, 1));

  reg_var(intern(lit("*param-macro*"), user_package), pm_table);

  reg_fun(intern(lit("eval"), user_package), func_n2o(eval_intrinsic, 1));
  reg_fun(intern(lit("lisp-parse"), user_package), func_n5o(nread, 0));
  reg_fun(intern(lit("read"), user_package), func_n5o(nread, 0));
  reg_fun(intern(lit("iread"), user_package), func_n5o(iread, 0));
  reg_fun(intern(lit("txr-parse"), user_package), func_n4o(txr_parse, 0));
  reg_fun(intern(lit("load"), user_package), func_n1(load));
  reg_var(load_path_s, nil);
  reg_symacro(intern(lit("self-load-path"), user_package), load_path_s);
  reg_var(load_recursive_s, nil);
  reg_fun(intern(lit("expand"), user_package), func_n2o(no_warn_expand, 1));
  reg_fun(intern(lit("expand*"), user_package), func_n2o(expand, 1));
  reg_fun(intern(lit("expand-with-free-refs"), user_package),
          func_n3o(expand_with_free_refs, 1));
  reg_fun(intern(lit("macro-form-p"), user_package), func_n2o(macro_form_p, 1));
  reg_fun(intern(lit("macroexpand-1"), user_package),
          func_n2o(macroexpand_1, 1));
  reg_fun(intern(lit("macroexpand-1-lisp1"), user_package),
          func_n2o(macroexpand_1_lisp1, 1));
  reg_fun(intern(lit("macroexpand"), user_package),
          func_n2o(macroexpand, 1));
  reg_fun(intern(lit("macroexpand-lisp1"), user_package),
          func_n2o(macroexpand_lisp1, 1));
  reg_fun(intern(lit("expand-params"), system_package), func_n5(expand_params));
  reg_fun(intern(lit("constantp"), user_package), func_n2o(constantp, 1));
  reg_fun(intern(lit("make-env"), user_package), func_n3o(make_env_intrinsic, 0));
  reg_fun(intern(lit("env-fbind"), user_package), func_n3(env_fbind));
  reg_fun(intern(lit("env-vbind"), user_package), func_n3(env_vbind));
  reg_fun(intern(lit("env-vbindings"), user_package), func_n1(env_vbindings));
  reg_fun(intern(lit("env-fbindings"), user_package), func_n1(env_fbindings));
  reg_fun(intern(lit("env-next"), user_package), func_n1(env_next));
  reg_fun(intern(lit("lexical-var-p"), user_package), func_n2(lexical_var_p));
  reg_fun(intern(lit("lexical-fun-p"), user_package), func_n2(lexical_fun_p));
  reg_fun(intern(lit("lexical-lisp1-binding"), user_package),
          func_n2(lexical_lisp1_binding));
  reg_fun(intern(lit("chain"), user_package), func_n0v(chainv));
  reg_fun(intern(lit("chand"), user_package), func_n0v(chandv));
  reg_fun(intern(lit("juxt"), user_package), func_n0v(juxtv));
  reg_fun(intern(lit("andf"), user_package), func_n0v(andv));
  reg_fun(intern(lit("orf"), user_package), func_n0v(orv));
  reg_fun(intern(lit("notf"), user_package), func_n1(notf));
  reg_fun(intern(lit("iff"), user_package), func_n3o(iff, 1));
  reg_fun(intern(lit("iffi"), user_package), func_n3o(iffi, 2));
  reg_fun(intern(lit("dup"), user_package), func_n1(dupl));
  reg_fun(intern(lit("flipargs"), user_package), func_n1(swap_12_21));
  reg_fun(if_s, func_n3o(if_fun, 2));
  reg_fun(or_s, func_n0v(or_fun));
  reg_fun(and_s, func_n0v(and_fun));
  reg_fun(progn_s, func_n0v(progn_fun));
  reg_fun(prog1_s, func_n0v(prog1_fun));
  reg_fun(prog2_s, func_n0v(prog2_fun));
  reg_fun(intern(lit("retf"), user_package), func_n1(retf));
  reg_fun(intern(lit("apf"), user_package), func_n1v(apf));
  reg_fun(intern(lit("ipf"), user_package), func_n1v(ipf));
  reg_fun(intern(lit("callf"), user_package), func_n1v(callf));
  reg_fun(intern(lit("mapf"), user_package), func_n1v(mapf));
  reg_fun(intern(lit("tf"), user_package), func_n0v(tf));
  reg_fun(intern(lit("nilf"), user_package), func_n0v(nilf));

  reg_fun(intern(lit("print"), user_package), func_n3o(print, 1));
  reg_fun(intern(lit("pprint"), user_package), func_n2o(pprint, 1));
  reg_fun(intern(lit("tostring"), user_package), func_n1(tostring));
  reg_fun(intern(lit("tostringp"), user_package), func_n1(tostringp));
  reg_fun(intern(lit("prinl"), user_package), func_n2o(prinl, 1));
  reg_fun(intern(lit("pprinl"), user_package), func_n2o(pprinl, 1));
  reg_fun(intern(lit("tprint"), user_package), func_n2o(tprint, 1));
  reg_fun(intern(lit("display-width"), user_package), func_n1(display_width));

  reg_fun(intern(lit("fmt-simple"), system_package), func_n5o(fmt_simple, 1));
  reg_fun(intern(lit("fmt-flex"), system_package), func_n2v(fmt_flex));
  reg_fun(intern(lit("fmt-join"), system_package), func_n0v(fmt_join));

  reg_varl(user_package_s = intern(lit("user-package"), user_package), user_package);
  reg_varl(system_package_s = intern(lit("system-package"), user_package), system_package);
  reg_varl(keyword_package_s = intern(lit("keyword-package"), user_package), keyword_package);

  if (opt_compat && opt_compat <= 156) {
    reg_varl(intern(lit("*user-package*"), user_package), user_package);
    reg_varl(intern(lit("*system-package*"), user_package), system_package);
    reg_varl(intern(lit("*keyword-package*"), user_package), keyword_package);
  }

  reg_fun(intern(lit("make-sym"), user_package), func_n1(make_sym));
  reg_fun(intern(lit("gensym"), user_package), func_n1o(gensym, 0));
  reg_var(gensym_counter_s = intern(lit("*gensym-counter*"), user_package), zero);
  reg_var(package_alist_s = intern(lit("*package-alist*"), user_package), packages);
  reg_var(package_s = intern(lit("*package*"), user_package),
          (opt_compat && opt_compat <= 190) ? user_package : public_package);
  reg_fun(intern(lit("make-package"), user_package), func_n2o(make_package, 1));
  reg_fun(intern(lit("make-anon-package"), system_package), func_n1o(make_anon_package, 0));
  reg_fun(intern(lit("find-package"), user_package), func_n1(find_package));
  reg_fun(intern(lit("delete-package"), user_package), func_n1(delete_package));
  reg_fun(intern(lit("merge-delete-package"), user_package), func_n2o(merge_delete_package, 1));
  reg_fun(intern(lit("package-alist"), user_package), func_n0(package_alist));
  reg_fun(intern(lit("package-name"), user_package), func_n1(package_name));
  reg_fun(intern(lit("package-symbols"), user_package), func_n1o(package_symbols, 0));
  reg_fun(intern(lit("package-local-symbols"), user_package), func_n1o(package_local_symbols, 0));
  reg_fun(intern(lit("package-foreign-symbols"), user_package), func_n1o(package_foreign_symbols, 0));
  reg_fun(intern(lit("use-sym"), user_package), func_n2o(use_sym, 1));
  reg_fun(intern(lit("unuse-sym"), user_package), func_n2o(unuse_sym, 1));
  reg_fun(intern(lit("use-package"), user_package), func_n2o(use_package, 1));
  reg_fun(intern(lit("unuse-package"), user_package), func_n2o(unuse_package, 1));
  reg_fun(intern(lit("intern"), user_package), func_n2o(intern_intrinsic, 1));
  reg_fun(intern(lit("intern-fb"), user_package), func_n2o(intern_fallback_intrinsic, 1));
  reg_fun(intern(lit("unintern"), user_package), func_n2o(unintern, 1));
  reg_fun(intern(lit("find-symbol"), user_package), func_n3o(find_symbol, 1));
  reg_fun(intern(lit("find-symbol-fb"), user_package), func_n3o(find_symbol_fb, 1));
  reg_fun(intern(lit("rehome-sym"), user_package), func_n2o(rehome_sym, 1));
  reg_fun(intern(lit("package-fallback-list"), user_package), func_n1(package_fallback_list));
  reg_fun(intern(lit("set-package-fallback-list"), user_package), func_n2(set_package_fallback_list));
  reg_fun(intern(lit("symbolp"), user_package), func_n1(symbolp));
  reg_fun(intern(lit("symbol-name"), user_package), func_n1(symbol_name));
  reg_fun(intern(lit("symbol-package"), user_package), func_n1(symbol_package));
  reg_fun(intern(lit("packagep"), user_package), func_n1(packagep));
  reg_fun(intern(lit("keywordp"), user_package), func_n1(keywordp));
  reg_fun(intern(lit("bindable"), user_package), func_n1(bindable));
  reg_fun(intern(lit("mkstring"), user_package), func_n2o(mkstring, 1));
  reg_fun(intern(lit("copy-str"), user_package), func_n1(copy_str));
  reg_fun(intern(lit("upcase-str"), user_package), func_n1(upcase_str));
  reg_fun(intern(lit("downcase-str"), user_package), func_n1(downcase_str));
  reg_fun(intern(lit("string-extend"), user_package), func_n2(string_extend));
  reg_fun(intern(lit("stringp"), user_package), func_n1(stringp));
  reg_fun(intern(lit("lazy-stringp"), user_package), func_n1(lazy_stringp));
  reg_fun(intern(lit("length-str"), user_package), func_n1(length_str));
  reg_fun(intern(lit("coded-length"), user_package), func_n1(coded_length));
  reg_fun(intern(lit("search-str"), user_package), func_n4o(search_str, 2));
  reg_fun(intern(lit("search-str-tree"), user_package), func_n4o(search_str_tree, 2));
  reg_fun(intern(lit("match-str"), user_package), func_n3o(match_str, 2));
  reg_fun(intern(lit("match-str-tree"), user_package), func_n3o(match_str_tree, 2));
  reg_fun(intern(lit("sub-str"), user_package), func_n3o(sub_str, 1));
  reg_fun(intern(lit("replace-str"), user_package), func_n4o(replace_str, 2));
  reg_fun(intern(lit("cat-str"), user_package), func_n2o(cat_str, 1));
  reg_fun(intern(lit("split-str"), user_package), func_n3o(split_str_keep, 2));
  reg_fun(intern(lit("spl"), user_package), func_n3o(spl, 2));
  reg_fun(intern(lit("split-str-set"), user_package), func_n2(split_str_set));
  reg_fun(intern(lit("sspl"), user_package), func_n2(sspl));
  reg_fun(intern(lit("tok-str"), user_package), func_n3o(tok_str, 2));
  reg_fun(intern(lit("tok"), user_package), func_n3o(tok, 2));
  reg_fun(intern(lit("tok-where"), user_package), func_n2(tok_where));
  reg_fun(intern(lit("list-str"), user_package), func_n1(list_str));
  reg_fun(intern(lit("trim-str"), user_package), func_n1(trim_str));
  reg_fun(intern(lit("cmp-str"), user_package), func_n2(cmp_str));
  reg_fun(intern(lit("string-lt"), user_package), func_n2(str_lt));
  reg_fun(intern(lit("str="), user_package), func_n2(str_eq));
  reg_fun(intern(lit("str<"), user_package), func_n2(str_lt));
  reg_fun(intern(lit("str>"), user_package), func_n2(str_gt));
  reg_fun(intern(lit("str<="), user_package), func_n2(str_le));
  reg_fun(intern(lit("str>="), user_package), func_n2(str_ge));
  reg_fun(intern(lit("int-str"), user_package), func_n2o(int_str, 1));
  reg_fun(intern(lit("flo-str"), user_package), func_n1(flo_str));
  reg_fun(intern(lit("num-str"), user_package), func_n1(num_str));
  reg_fun(intern(lit("int-flo"), user_package), func_n1(int_flo));
  reg_fun(intern(lit("flo-int"), user_package), func_n1(flo_int));
  reg_fun(intern(lit("tofloat"), user_package), func_n1(tofloat));
  reg_fun(intern(lit("toint"), user_package), func_n2o(toint, 1));
  reg_fun(intern(lit("tofloatz"), user_package), func_n1(tofloatz));
  reg_fun(intern(lit("tointz"), user_package), func_n2o(tointz, 1));
  reg_fun(intern(lit("less"), user_package), func_n1v(lessv));
  reg_fun(intern(lit("greater"), user_package), func_n1v(greaterv));
  reg_fun(intern(lit("lequal"), user_package), func_n1v(lequalv));
  reg_fun(intern(lit("gequal"), user_package), func_n1v(gequalv));
  reg_fun(intern(lit("chrp"), user_package), func_n1(chrp));
  reg_fun(intern(lit("chr-isalnum"), user_package), func_n1(chr_isalnum));
  reg_fun(intern(lit("chr-isalpha"), user_package), func_n1(chr_isalpha));
  reg_fun(intern(lit("chr-isascii"), user_package), func_n1(chr_isascii));
  reg_fun(intern(lit("chr-iscntrl"), user_package), func_n1(chr_iscntrl));
  reg_fun(intern(lit("chr-isdigit"), user_package), func_n1(chr_isdigit));
  reg_fun(intern(lit("chr-digit"), user_package), func_n1(chr_digit));
  reg_fun(intern(lit("chr-isgraph"), user_package), func_n1(chr_isgraph));
  reg_fun(intern(lit("chr-islower"), user_package), func_n1(chr_islower));
  reg_fun(intern(lit("chr-isprint"), user_package), func_n1(chr_isprint));
  reg_fun(intern(lit("chr-ispunct"), user_package), func_n1(chr_ispunct));
  reg_fun(intern(lit("chr-isspace"), user_package), func_n1(chr_isspace));
  reg_fun(intern(lit("chr-isblank"), user_package), func_n1(chr_isblank));
  reg_fun(intern(lit("chr-isunisp"), user_package), func_n1(chr_isunisp));
  reg_fun(intern(lit("chr-isupper"), user_package), func_n1(chr_isupper));
  reg_fun(intern(lit("chr-isxdigit"), user_package), func_n1(chr_isxdigit));
  reg_fun(intern(lit("chr-xdigit"), user_package), func_n1(chr_xdigit));
  reg_fun(intern(lit("chr-toupper"), user_package), func_n1(chr_toupper));
  reg_fun(intern(lit("chr-tolower"), user_package), func_n1(chr_tolower));
  {
    val fun = func_n1(int_chr);
    reg_fun(intern(lit("num-chr"), user_package), fun); /* OBS */
    reg_fun(intern(lit("int-chr"), user_package), fun);
  }
  {
    val fun = func_n1(chr_int);
    reg_fun(intern(lit("chr-num"), user_package), fun); /* OBS */
    reg_fun(intern(lit("chr-int"), user_package), fun);
  }
  reg_fun(intern(lit("chr-str"), user_package), func_n2(chr_str));
  reg_fun(intern(lit("chr-str-set"), user_package), func_n3(chr_str_set));
  reg_fun(intern(lit("span-str"), user_package), func_n2(span_str));
  reg_fun(intern(lit("compl-span-str"), user_package), func_n2(compl_span_str));
  reg_fun(intern(lit("break-str"), user_package), func_n2(break_str));

  reg_fun(intern(lit("lazy-stream-cons"), user_package), func_n1(lazy_stream_cons));
  reg_fun(intern(lit("get-lines"), user_package), func_n1o(lazy_stream_cons, 0));
  reg_fun(intern(lit("lazy-str"), user_package), func_n3o(lazy_str, 1));
  reg_fun(intern(lit("lazy-stringp"), user_package), func_n1(lazy_stringp));
  reg_fun(intern(lit("lazy-str-force-upto"), user_package), func_n2(lazy_str_force_upto));
  reg_fun(intern(lit("lazy-str-force"), user_package), func_n1(lazy_str_force));
  reg_fun(intern(lit("lazy-str-get-trailing-list"), user_package), func_n2(lazy_str_get_trailing_list));
  reg_fun(intern(lit("length-str->"), user_package), func_n2(length_str_gt));
  reg_fun(intern(lit("length-str->="), user_package), func_n2(length_str_ge));
  reg_fun(intern(lit("length-str-<"), user_package), func_n2(length_str_lt));
  reg_fun(intern(lit("length-str-<="), user_package), func_n2(length_str_le));

  reg_fun(intern(lit("vector"), user_package), func_n2o(vector, 1));
  reg_fun(intern(lit("vec"), user_package), func_n0v(vectorv));
  reg_fun(intern(lit("vectorp"), user_package), func_n1(vectorp));
  reg_fun(intern(lit("vec-set-length"), user_package), func_n2(vec_set_length));
  reg_fun(vecref_s, func_n2(vecref));
  reg_fun(intern(lit("vec-push"), user_package), func_n2(vec_push));
  reg_fun(intern(lit("length-vec"), user_package), func_n1(length_vec));
  reg_fun(intern(lit("size-vec"), user_package), func_n1(size_vec));
  {
    val fun = func_n1(vec_list);
    reg_fun(intern(lit("vector-list"), user_package), fun); /* OBS */
    reg_fun(vec_list_s, fun);
  }
  {
    val fun = func_n1(list_vec);
    reg_fun(intern(lit("list-vector"), user_package), fun); /* OBS */
    reg_fun(intern(lit("list-vec"), user_package), fun);
  }
  reg_fun(intern(lit("copy-vec"), user_package), func_n1(copy_vec));
  reg_fun(intern(lit("sub-vec"), user_package), func_n3o(sub_vec, 1));
  reg_fun(intern(lit("replace-vec"), user_package), func_n4o(replace_vec, 2));
  reg_fun(intern(lit("cat-vec"), user_package), func_n1(cat_vec));

  reg_fun(intern(lit("assoc"), user_package), func_n2(assoc));
  reg_fun(intern(lit("assql"), user_package), func_n2(assql));
  reg_fun(intern(lit("assq"), user_package), func_n2(assq));
  reg_fun(intern(lit("rassoc"), user_package), func_n2(rassoc));
  reg_fun(intern(lit("rassql"), user_package), func_n2(rassql));
  reg_fun(intern(lit("rassq"), user_package), func_n2(rassq));
  reg_fun(intern(lit("acons"), user_package), func_n3(acons));
  reg_fun(intern(lit("acons-new"), user_package), func_n3(acons_new));
  reg_fun(intern(lit("aconsql-new"), user_package), func_n3(aconsql_new));
  reg_fun(intern(lit("alist-remove"), user_package), func_n1v(alist_removev));
  reg_fun(intern(lit("alist-nremove"), user_package), func_n1v(alist_nremovev));
  reg_fun(intern(lit("copy-cons"), user_package), func_n1(copy_cons));
  reg_fun(intern(lit("copy-tree"), user_package), func_n1(copy_tree));
  reg_fun(intern(lit("copy-alist"), user_package), func_n1(copy_alist));
  reg_fun(intern(lit("prop"), user_package), func_n2(getplist));
  reg_fun(intern(lit("memp"), user_package), func_n2(memp));
  reg_fun(intern(lit("plist-to-alist"), user_package), func_n1(plist_to_alist));
  reg_fun(intern(lit("improper-plist-to-alist"), user_package), func_n2(improper_plist_to_alist));
  reg_fun(intern(lit("merge"), user_package), func_n4o(merge_wrap, 2));
  reg_fun(intern(lit("nsort"), user_package), func_n3o(nsort, 1));
  reg_fun(intern(lit("sort"), user_package),
          func_n3o(if3(opt_compat && opt_compat <= 237, nsort, sort), 1));
  reg_fun(intern(lit("nshuffle"), user_package), func_n2o(nshuffle, 1));
  reg_fun(intern(lit("shuffle"), user_package),
          func_n2o(if3(opt_compat && opt_compat <= 237, nshuffle, shuffle), 1));
  reg_fun(intern(lit("find"), user_package), func_n4o(find, 2));
  reg_fun(intern(lit("rfind"), user_package), func_n4o(rfind, 2));
  reg_fun(intern(lit("find-if"), user_package), func_n3o(find_if, 2));
  reg_fun(intern(lit("rfind-if"), user_package), func_n3o(rfind_if, 2));
  reg_fun(intern(lit("find-max"), user_package), func_n3o(find_max, 1));
  reg_fun(intern(lit("find-min"), user_package), func_n3o(find_min, 1));
  reg_fun(intern(lit("multi-sort"), user_package), func_n3o(multi_sort, 2));
  reg_fun(intern(lit("set-diff"), user_package), func_n4o(set_diff, 2));
  reg_fun(intern(lit("diff"), user_package), func_n4o(diff, 2));
  reg_fun(intern(lit("symdiff"), user_package), func_n4o(symdiff, 2));
  reg_fun(intern(lit("isec"), user_package), func_n4o(isec, 2));
  reg_fun(intern(lit("uni"), user_package), func_n4o(uni, 2));

  reg_fun(intern(lit("seqp"), user_package), func_n1(seqp));
  reg_fun(intern(lit("iterable"), user_package), func_n1(iterable));
  reg_fun(intern(lit("list-seq"), user_package), func_n1(list_seq));
  reg_fun(intern(lit("vec-seq"), user_package), func_n1(vec_seq));
  reg_fun(intern(lit("str-seq"), user_package), func_n1(str_seq));
  reg_fun(intern(lit("length"), user_package), length_f);
  reg_fun(intern(lit("len"), user_package), length_f);
  reg_fun(intern(lit("empty"), user_package), func_n1(empty));
  reg_fun(intern(lit("copy"), user_package), func_n1(copy));
  reg_fun(intern(lit("sub"), user_package), func_n3o(sub, 1));
  reg_fun(intern(lit("ref"), user_package), func_n2(ref));
  reg_fun(intern(lit("refset"), user_package), func_n3(refset));
  reg_fun(intern(lit("replace"), user_package), func_n4o(replace, 2));
  reg_fun(intern(lit("dwim-set"), system_package), func_n2v(dwim_set));
  reg_fun(intern(lit("dwim-del"), system_package), func_n3(dwim_del));
  reg_fun(intern(lit("update"), user_package), func_n2(update));
  reg_fun(intern(lit("search"), user_package), func_n4o(search, 2));
  reg_fun(intern(lit("rsearch"), user_package), func_n4o(rsearch, 2));
  reg_fun(intern(lit("contains"), user_package), func_n4o(contains, 2));
  reg_fun(intern(lit("where"), user_package), func_n2(where));
  reg_fun(intern(lit("select"), user_package), func_n2(sel));
  reg_fun(intern(lit("reject"), user_package), func_n2(reject));
  reg_fun(intern(lit("relate"), user_package), func_n3o(relate, 2));
  reg_fun(intern(lit("seq-begin"), user_package), func_n1(seq_begin));
  reg_fun(intern(lit("seq-next"), user_package), func_n2(seq_next));
  reg_fun(intern(lit("seq-reset"), user_package), func_n2(seq_reset));
  reg_fun(intern(lit("iter-begin"), user_package), func_n1(iter_begin));
  reg_fun(intern(lit("iter-more"), user_package), func_n1(iter_more));
  reg_fun(intern(lit("iter-item"), user_package), func_n1(iter_item));
  reg_fun(intern(lit("iter-step"), user_package), func_n1(iter_step));
  reg_fun(intern(lit("iter-reset"), user_package), func_n2(iter_reset));

  reg_fun(intern(lit("rcons"), user_package), func_n2(rcons));
  reg_fun(intern(lit("rangep"), user_package), func_n1(rangep));
  reg_fun(intern(lit("from"), user_package), func_n1(from));
  reg_fun(intern(lit("to"), user_package), func_n1(to));
  reg_fun(intern(lit("in-range"), user_package), func_n2(in_range));
  reg_fun(intern(lit("in-range*"), user_package), func_n2(in_range_star));

  reg_fun(intern(lit("make-like"), user_package), func_n2(make_like));
  reg_fun(intern(lit("nullify"), user_package), func_n1(nullify));

  reg_varl(intern(lit("top-vb"), system_package), top_vb);
  reg_varl(intern(lit("top-fb"), system_package), top_fb);
  reg_varl(intern(lit("top-mb"), system_package), top_mb);
  reg_fun(intern(lit("symbol-value"), user_package), func_n1(symbol_value));
  reg_fun(intern(lit("symbol-function"), user_package), func_n1(symbol_function));
  reg_fun(intern(lit("symbol-macro"), user_package), func_n1(symbol_macro));
  reg_fun(intern(lit("boundp"), user_package), func_n1(boundp));
  reg_fun(intern(lit("fboundp"), user_package), func_n1(fboundp));
  reg_fun(intern(lit("mboundp"), user_package), func_n1(mboundp));
  reg_fun(intern(lit("makunbound"), user_package), func_n1(makunbound));
  reg_fun(intern(lit("fmakunbound"), user_package), func_n1(fmakunbound));
  reg_fun(intern(lit("mmakunbound"), user_package), func_n1(mmakunbound));
  reg_fun(intern(lit("special-operator-p"), user_package), func_n1(special_operator_p));
  reg_fun(intern(lit("special-var-p"), user_package), func_n1(special_var_p));
  reg_fun(sys_mark_special_s, func_n1(mark_special));
  reg_fun(intern(lit("copy-fun"), user_package), func_n1(copy_fun));
  reg_fun(intern(lit("func-get-form"), user_package), func_n1(func_get_form));
  reg_fun(intern(lit("func-get-name"), user_package), func_n2o(func_get_name, 1));
  reg_fun(intern(lit("func-get-env"), user_package), func_n1(func_get_env));
  reg_fun(intern(lit("func-set-env"), user_package), func_n2(func_set_env));
  reg_fun(intern(lit("functionp"), user_package), func_n1(functionp));
  reg_fun(intern(lit("interp-fun-p"), user_package), func_n1(interp_fun_p));
  reg_fun(intern(lit("vm-fun-p"), user_package), func_n1(vm_fun_p));
  reg_fun(intern(lit("fun-fixparam-count"), user_package), func_n1(fun_fixparam_count));
  reg_fun(intern(lit("fun-optparam-count"), user_package), func_n1(fun_optparam_count));
  reg_fun(intern(lit("fun-variadic"), user_package), func_n1(fun_variadic));
  reg_fun(intern(lit("ctx-form"), system_package), func_n1(ctx_form));
  reg_fun(intern(lit("ctx-name"), system_package), func_n1(ctx_name));

  reg_fun(intern(lit("range"), user_package), func_n3o(range, 0));
  reg_fun(intern(lit("range*"), user_package), func_n3o(range_star, 0));
  reg_fun(intern(lit("rlist"), user_package), func_n0v(rlist));
  reg_fun(intern(lit("rlist*"), user_package), func_n0v(rlist_star));
  reg_fun(generate_s, func_n2(generate));
  reg_fun(intern(lit("giterate"), user_package), func_n3o(giterate, 2));
  reg_fun(intern(lit("ginterate"), user_package), func_n3o(ginterate, 2));
  reg_fun(intern(lit("expand-right"), user_package), func_n2(expand_right));
  reg_fun(intern(lit("expand-left"), user_package), func_n2(expand_left));
  reg_fun(intern(lit("nexpand-left"), user_package), func_n2(nexpand_left));
  reg_fun(intern(lit("repeat"), user_package), func_n2o(repeat, 1));
  reg_fun(intern(lit("pad"), user_package), func_n3o(pad, 1));
  reg_fun(intern(lit("weave"), user_package), func_n0v(weavev));
  reg_fun(force_s, func_n1(force));
  reg_fun(intern(lit("promisep"), user_package), func_n1(promisep));
  reg_fun(intern(lit("rperm"), user_package), func_n2(rperm));
  reg_fun(intern(lit("perm"), user_package), func_n2o(perm, 1));
  reg_fun(intern(lit("comb"), user_package), func_n2(comb));
  reg_fun(intern(lit("rcomb"), user_package), func_n2(rcomb));

  reg_fun(intern(lit("return*"), user_package), func_n2o(return_star, 1));
  reg_fun(intern(lit("abscond*"), system_package), func_n2o(abscond_star, 1));

  reg_fun(intern(lit("match-fun"), user_package), func_n4o(match_fun, 2));

  reg_fun(intern(lit("source-loc"), user_package), func_n1(source_loc));
  reg_fun(intern(lit("source-loc-str"), user_package), func_n2o(source_loc_str, 1));
  reg_fun(intern(lit("macro-ancestor"), user_package), func_n1(lookup_origin));
  reg_fun(intern(lit("set-macro-ancestor"), system_package), func_n2(set_origin));
  reg_fun(intern(lit("rlcp"), user_package), func_n2(rlcp));
  reg_fun(intern(lit("rlcp-tree"), user_package), func_n2(rlcp_tree));

  reg_fun(intern(lit("cptr-int"), user_package), func_n2o(cptr_int, 1));
  reg_fun(intern(lit("cptr-obj"), user_package), func_n2o(cptr_obj, 1));
  reg_fun(intern(lit("cptr-buf"), user_package), func_n2o(cptr_buf, 1));
  reg_fun(intern(lit("cptr-zap"), user_package), func_n1(cptr_zap));
  reg_fun(intern(lit("cptr-free"), user_package), func_n1(cptr_free));
  reg_fun(intern(lit("cptr-cast"), user_package), func_n2(cptr_cast));
  reg_fun(intern(lit("int-cptr"), user_package), func_n1(int_cptr));
  reg_fun(intern(lit("cptrp"), user_package), func_n1(cptrp));
  reg_fun(intern(lit("cptr-type"), user_package), func_n1(cptr_type));
  reg_fun(intern(lit("cptr-size-hint"), user_package), func_n2(cptr_size_hint));
  reg_varl(intern(lit("cptr-null"), user_package), cptr(0));

  reg_fun(intern(lit("rt-defvarl"), system_package), func_n1(rt_defvarl));
  reg_fun(intern(lit("rt-defun"), system_package), func_n2(rt_defun));
  reg_fun(intern(lit("rt-defmacro"), system_package), func_n3(rt_defmacro));
  reg_fun(intern(lit("rt-defsymacro"), system_package), func_n2(rt_defsymacro));
  reg_fun(intern(lit("rt-pprof"), system_package), func_n1(rt_pprof));
  reg_fun(intern(lit("rt-load-for"), system_package), func_n0v(rt_load_for));

  reg_fun(intern(lit("rt-assert-fail"), system_package), func_n4ov(rt_assert_fail, 3));

  eval_error_s = intern(lit("eval-error"), user_package);
  uw_register_subtype(eval_error_s, error_s);

  lisplib_init();
}

void eval_compat_fixup(int compat_ver)
{
  if (compat_ver <= 107)
    reg_fun(intern(lit("flip"), user_package), func_n1(swap_12_21));
}
