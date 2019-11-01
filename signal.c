/* Copyright 2013-2019
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <wchar.h>
#include <signal.h>
#include "config.h"
#if HAVE_SYS_TIME
#include <sys/time.h>
#endif
#if HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif
#include "lib.h"
#include "gc.h"
#include "sysif.h"
#include "signal.h"
#include "unwind.h"
#include "eval.h"
#include "txr.h"

#define MAX_SIG 32

volatile sig_atomic_t async_sig_enabled = 0;
static volatile sig_atomic_t interrupt_count = 0;
small_sigset_t sig_blocked_cache;

static val sig_lambda[MAX_SIG];
volatile unsigned long sig_deferred;

static int is_cpu_exception(int sig)
{
  switch (sig) {
  case SIGFPE: case SIGILL:
  case SIGSEGV: case SIGBUS:
  case SIGTRAP:
    return 1;
  default:
    return 0;
  }
}

static void sig_reload_cache(void)
{
  sigset_t set;
  if (sigprocmask(SIG_BLOCK, 0, &set) == 0)
    memcpy(&sig_blocked_cache, &set, sizeof sig_blocked_cache);
}

static void sig_handler(int sig)
{
  val lambda = sig_lambda[sig];
  int gc = 0;
  int as = 0;
  int exc = is_cpu_exception(sig);
  int ic = interrupt_count++;
  int in_interrupt = ic > 0;

  if (exc) {
    gc = gc_state(0);
    as = async_sig_enabled;
    async_sig_enabled = 1;
  }

  sig_reload_cache();

  if (lambda) {
    if (!in_interrupt && async_sig_enabled) {
      uw_simple_catch_begin;
      async_sig_enabled = 0;
      if (gc_inprogress())
        gc_cancel();
      if (funcall2(lambda, num_fast(sig), t))
        sig_deferred |= (1UL << sig);
      uw_unwind {
        interrupt_count = ic;
      }
      async_sig_enabled = 1;
      uw_catch_end;
    } else {
      sig_deferred |= (1UL << sig);
    }
  }

  if (exc) {
    async_sig_enabled = as;
    gc_state(gc);
  }

  interrupt_count = ic;
}

static val kill_wrap(val pid, val sig)
{
  cnum p = c_num(pid), s = c_num(default_arg(sig, num_fast(SIGTERM)));
  int res = kill(p, s);
  if (opt_compat && opt_compat <= 114)
    return num(res);
  return tnil(res == 0);
}

static val raise_wrap(val sig)
{
  int res = raise(c_num(sig));
  return tnil(res == 0);
}

void sig_init(void)
{
  int i;

  for (i = 0; i < MAX_SIG; i++) {
    /* t means SIG_DFL, which is what signals
     * are before we manipulate them. */
    sig_lambda[i] = t;
    prot1(&sig_lambda[i]);
  }

  reg_varl(intern(lit("sig-hup"), user_package), num_fast(SIGHUP));
  reg_varl(intern(lit("sig-int"), user_package), num_fast(SIGINT));
  reg_varl(intern(lit("sig-quit"), user_package), num_fast(SIGQUIT));
  reg_varl(intern(lit("sig-ill"), user_package), num_fast(SIGILL));
  reg_varl(intern(lit("sig-trap"), user_package), num_fast(SIGTRAP));
  reg_varl(intern(lit("sig-abrt"), user_package), num_fast(SIGABRT));
  reg_varl(intern(lit("sig-bus"), user_package), num_fast(SIGBUS));
  reg_varl(intern(lit("sig-fpe"), user_package), num_fast(SIGFPE));
  reg_varl(intern(lit("sig-kill"), user_package), num_fast(SIGKILL));
  reg_varl(intern(lit("sig-usr1"), user_package), num_fast(SIGUSR1));
  reg_varl(intern(lit("sig-segv"), user_package), num_fast(SIGSEGV));
  reg_varl(intern(lit("sig-usr2"), user_package), num_fast(SIGUSR2));
  reg_varl(intern(lit("sig-pipe"), user_package), num_fast(SIGPIPE));
  reg_varl(intern(lit("sig-alrm"), user_package), num_fast(SIGALRM));
  reg_varl(intern(lit("sig-term"), user_package), num_fast(SIGTERM));
  reg_varl(intern(lit("sig-chld"), user_package), num_fast(SIGCHLD));
  reg_varl(intern(lit("sig-cont"), user_package), num_fast(SIGCONT));
  reg_varl(intern(lit("sig-stop"), user_package), num_fast(SIGSTOP));
  reg_varl(intern(lit("sig-tstp"), user_package), num_fast(SIGTSTP));
  reg_varl(intern(lit("sig-ttin"), user_package), num_fast(SIGTTIN));
  reg_varl(intern(lit("sig-ttou"), user_package), num_fast(SIGTTOU));
  reg_varl(intern(lit("sig-urg"), user_package), num_fast(SIGURG));
  reg_varl(intern(lit("sig-xcpu"), user_package), num_fast(SIGXCPU));
  reg_varl(intern(lit("sig-xfsz"), user_package), num_fast(SIGXFSZ));
  reg_varl(intern(lit("sig-vtalrm"), user_package), num_fast(SIGVTALRM));
  reg_varl(intern(lit("sig-prof"), user_package), num_fast(SIGPROF));
#ifdef SIGPOLL
  reg_varl(intern(lit("sig-poll"), user_package), num_fast(SIGPOLL));
#endif
  reg_varl(intern(lit("sig-sys"), user_package), num_fast(SIGSYS));
#ifdef SIGWINCH
  reg_varl(intern(lit("sig-winch"), user_package), num_fast(SIGWINCH));
#endif
#ifdef SIGIOT
  reg_varl(intern(lit("sig-iot"), user_package), num_fast(SIGIOT));
#endif
#ifdef SIGSTKFLT
  reg_varl(intern(lit("sig-stkflt"), user_package), num_fast(SIGSTKFLT));
#endif
#ifdef SIGIO
  reg_varl(intern(lit("sig-io"), user_package), num_fast(SIGIO));
#endif
#ifdef SIGLOST
  reg_varl(intern(lit("sig-lost"), user_package), num_fast(SIGLOST));
#endif
#ifdef SIGPWR
  reg_varl(intern(lit("sig-pwr"), user_package), num_fast(SIGPWR));
#endif

#if HAVE_ITIMER
  reg_varl(intern(lit("itimer-real"), user_package), num_fast(ITIMER_REAL));
  reg_varl(intern(lit("itimer-virtual"), user_package), num_fast(ITIMER_VIRTUAL));
  reg_varl(intern(lit("itimer-prov"), user_package), num_fast(ITIMER_PROF));
  reg_fun(intern(lit("getitimer"), user_package), func_n1(getitimer_wrap));
  reg_fun(intern(lit("setitimer"), user_package), func_n3(setitimer_wrap));
#endif

  reg_fun(intern(lit("set-sig-handler"), user_package), func_n2(set_sig_handler));
  reg_fun(intern(lit("get-sig-handler"), user_package), func_n1(get_sig_handler));
  reg_fun(intern(lit("sig-check"), user_package), func_n0(sig_check));
  reg_fun(intern(lit("kill"), user_package), func_n2o(kill_wrap, 1));
  reg_fun(intern(lit("raise"), user_package), func_n1(raise_wrap));

  sig_reload_cache();
}

#if HAVE_SIGALTSTACK

static mem_t *stack;

static void setup_alt_stack(void)
{
  stack_t ss;

  if (!stack)
    stack = chk_malloc(SIGSTKSZ);

  ss.ss_sp = stack;
  ss.ss_size = SIGSTKSZ;
  ss.ss_flags = 0;

  if (sigaltstack(&ss, NULL) == -1) {
    free(stack);
    stack = 0;
  }
}

static void teardown_alt_stack(void)
{
  stack_t ss;

  if (!stack)
    return;

  ss.ss_sp = stack;
  ss.ss_size = SIGSTKSZ;
  ss.ss_flags = SS_DISABLE;

  if (sigaltstack(&ss, NULL) == -1)
    return;

  free(stack);
  stack = 0;
}

#endif

static void small_sigfillset(small_sigset_t *ss)
{
  ss->set = convert(unsigned int, -1);
}

val set_sig_handler(val signo, val lambda)
{
  static struct sigaction blank;
  val self = lit("set-sig-handler");
  cnum sig = c_num(signo);
  val old_lambda;
  small_sigset_t block, saved;

  small_sigfillset(&block);
  sig_mask(SIG_BLOCK, &block, &saved);

  if (sig < 0 || sig >= MAX_SIG)
    uw_throwf(error_s, lit("~a: signal ~s out of range"), self, sig, nao);

  old_lambda = sig_lambda[sig];

  if (lambda != old_lambda) {
    unsigned long mask = 1UL << sig;

    if (lambda == nil) {
      signal(sig, SIG_IGN);
      sig_deferred &= ~mask;
    } else if (lambda == t) {
      signal(sig, SIG_DFL);
      sig_deferred &= ~mask;
    } else {
      struct sigaction sa = blank;

      type_check(self, lambda, FUN);

      sa.sa_flags = SA_RESTART;
      sa.sa_handler = sig_handler;
      sigfillset(&sa.sa_mask);
#if HAVE_SIGALTSTACK
      if (sig == SIGSEGV || sig == SIGBUS) {
        setup_alt_stack();
        sa.sa_flags |= SA_ONSTACK;
      }
#endif
      sigaction(sig, &sa, 0);
    }

#if HAVE_SIGALTSTACK
    if ((sig == SIGSEGV || sig == SIGBUS) && (lambda == nil || lambda == t))
        teardown_alt_stack();
#endif

    sig_lambda[sig] = lambda;
  }

  sig_mask(SIG_SETMASK, &saved, 0);

  return old_lambda;
}

val get_sig_handler(val signo)
{
  cnum sig = c_num(signo);

  if (sig < 0 || sig >= MAX_SIG)
    uw_throwf(error_s, lit("get-sig-handler: signal ~s out of range"), sig, nao);

  return sig_lambda[sig];
}

val sig_check(void)
{
  unsigned long sd;
  int i;

  if ((sd = sig_deferred) == 0)
    return nil;

  for (i = 0; i < MAX_SIG; i++) {
    unsigned long mask = 1UL << i;
    if ((sd & mask) != 0) {
      sd &= ~mask;
      sig_deferred = sd;
      funcall2(sig_lambda[i], num_fast(i), nil);
    }
  }

  return t;
}

int sig_mask(int how, const small_sigset_t *set, small_sigset_t *oldset)
{
  small_sigset_t newset;
  const small_sigset_t *pnew;

  switch (how) {
  case SIG_SETMASK:
    pnew = set;
    break;
  case SIG_BLOCK:
    pnew = &newset;
    newset.set = sig_blocked_cache.set | set->set;
    break;
  case SIG_UNBLOCK:
    pnew = &newset;
    newset.set = sig_blocked_cache.set & ~set->set;
    break;
  default:
    errno = EINVAL;
    return -1;
  }

  if (sig_blocked_cache.set != pnew->set) {
    static sigset_t blank;
    sigset_t real_newset = blank, real_oldset;
    sig_blocked_cache = *pnew;
    int ret;
#if HAVE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(&real_oldset, sizeof real_oldset);
#endif
    memcpy(&real_newset, &sig_blocked_cache, sizeof sig_blocked_cache);
    ret = sigprocmask(SIG_SETMASK, &real_newset, &real_oldset);
    if (ret == 0 && oldset != 0)
      memcpy(oldset, &real_oldset, sizeof *oldset);
    return ret;
  }

  if (oldset != 0)
    *oldset = sig_blocked_cache;
  return 0;
}

#if HAVE_ITIMER

static val tv_to_usec(val sec, val usec)
{
  const val meg = num_fast(1000000);
  return plus(mul(sec, meg), usec);
}

val getitimer_wrap(val which)
{
  struct itimerval itv;

  if (getitimer(c_num(which), &itv) < 0)
    return nil;

  return list(tv_to_usec(num_time(itv.it_interval.tv_sec), num(itv.it_interval.tv_usec)),
              tv_to_usec(num_time(itv.it_value.tv_sec), num(itv.it_value.tv_usec)),
              nao);
}

val setitimer_wrap(val which, val interval, val currval)
{
  struct itimerval itn, itv;
  const val meg = num_fast(1000000);

  itn.it_interval.tv_sec = c_time(trunc(interval, meg));
  itn.it_interval.tv_usec = c_num(mod(interval, meg));
  itn.it_value.tv_sec = c_time(trunc(currval, meg));
  itn.it_value.tv_usec = c_num(mod(currval, meg));

  if (setitimer(c_num(which), &itn, &itv) < 0)
    return nil;

  return list(tv_to_usec(num_time(itv.it_interval.tv_sec), num(itv.it_interval.tv_usec)),
              tv_to_usec(num_time(itv.it_value.tv_sec), num(itv.it_value.tv_usec)),
              nao);
}

#endif
