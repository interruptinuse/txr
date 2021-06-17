/* Copyright 2015-2021
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

#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>
#include <signal.h>
#include "config.h"
#include "lib.h"
#include "eval.h"
#include "stream.h"
#include "hash.h"
#include "gc.h"
#include "debug.h"
#include "txr.h"
#include "socket.h"
#include "lisplib.h"

val dl_table;
int opt_dbg_autoload;
val trace_loaded;

static void set_dlt_entries_impl(val dlt, val *name, val fun, val package)
{
  for (; *name; name++) {
    val sym = intern(*name, package);

    if (fun)
      sethash(dlt, sym, fun);
    else
      remhash(dlt, sym);
  }
}

void set_dlt_entries(val dlt, val *name, val fun)
{
  set_dlt_entries_impl(dlt, name, fun, user_package);
}

static void set_dlt_entries_sys(val dlt, val *name, val fun)
{
  set_dlt_entries_impl(dlt, name, fun, system_package);
}

static void intern_only(val *name)
{
  for (; *name; name++)
    intern(*name, user_package);
}

static val place_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("get-fun-getter-setter"), lit("get-mb"), lit("get-vb"),
    lit("register-simple-accessor"),
    nil
  };
  val name[] = {
    lit("*place-clobber-expander*"), lit("*place-update-expander*"),
    lit("*place-delete-expander*"), lit("*place-macro*"),
    lit("get-update-expander"), lit("get-clobber-expander"),
    lit("get-delete-expander"),
    lit("place-form-p"),
    lit("rlet"), lit("slet"), lit("alet"), lit("with-gensyms"),
    lit("call-update-expander"), lit("call-clobber-expander"),
    lit("call-delete-expander"),
    lit("with-update-expander"), lit("with-clobber-expander"),
    lit("with-delete-expander"),
    lit("set"), lit("pset"), lit("zap"), lit("flip"), lit("inc"), lit("dec"),
    lit("pinc"), lit("pdec"),
    lit("push"), lit("pop"), lit("swap"), lit("shift"), lit("rotate"),
    lit("test-set"), lit("test-clear"), lit("compare-swap"),
    lit("test-inc"), lit("test-dec"),
    lit("pushnew"), lit("del"), lit("lset"), lit("upd"),
    lit("defplace"), lit("define-place-macro"), lit("define-modify-macro"),
    lit("placelet"), lit("placelet*"), lit("define-accessor"),
    lit("with-slots"),
    nil
  };

  set_dlt_entries_sys(dlt, sys_name, fun);
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val place_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("place")));
  return nil;
}

static val ver_set_entries(val dlt, val fun)
{
  val name[] = { lit("*lib-version*"), lit("lib-version"), nil };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val ver_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("ver")));
  return nil;
}

static val ifa_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("ifa"), lit("whena"), lit("conda"), lit("condlet"), lit("it"), nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val ifa_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("ifa")));
  return nil;
}

static val txr_case_set_entries(val dlt, val fun)
{
  val name[] = { lit("txr-if"), lit("txr-when"), lit("txr-case"), nil };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val txr_case_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("txr-case")));
  return nil;
}

static val with_resources_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("with-resources"),
    lit("with-objects"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val with_resources_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("with-resources")));
  return nil;
}

static val path_test_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("path-exists-p"), lit("path-file-p"), lit("path-dir-p"),
    lit("path-symlink-p"), lit("path-blkdev-p"), lit("path-chrdev-p"),
    lit("path-sock-p"), lit("path-pipe-p"), lit("path-pipe-p"),
    lit("path-setgid-p"), lit("path-setuid-p"), lit("path-sticky-p"),
    lit("path-mine-p"), lit("path-my-group-p"), lit("path-executable-to-me-p"),
    lit("path-writable-to-me-p"), lit("path-readable-to-me-p"),
    lit("path-read-writable-to-me-p"),
    lit("path-newer"), lit("path-older"),
    lit("path-same-object"), lit("path-private-to-me-p"),
    lit("path-strictly-private-to-me-p"),
    lit("path-dir-empty"),
    nil
  };

  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val path_test_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("path-test")));
  return nil;
}

static val struct_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("define-method"), lit("rslotset"), nil
  };
  val name[] = {
    lit("defstruct"), lit("qref"), lit("uref"), lit("new"), lit("lnew"),
    lit("new*"), lit("lnew*"),
    lit("meth"), lit("umeth"), lit("usl"), lit("defmeth"), lit("rslot"), nil
  };

  set_dlt_entries_sys(dlt, sys_name, fun);
  set_dlt_entries(dlt, name, fun);

  if (fun)
    sethash(dlt, struct_lit_s, fun);
  else
    remhash(dlt, struct_lit_s);

  return nil;
}

static val struct_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("struct")));
  return nil;
}

static val with_stream_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("with-out-string-stream"),
    lit("with-out-strlist-stream"),
    lit("with-out-buf-stream"),
    lit("with-in-string-stream"),
    lit("with-in-string-byte-stream"),
    lit("with-in-buf-stream"),
    lit("with-stream"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val with_stream_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("with-stream")));
  return nil;
}

static val hash_set_entries(val dlt, val fun)
{
  val name[] = { lit("with-hash-iter"), nil };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val hash_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("hash")));
  return nil;
}

static val except_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("catch"), lit("catch*"), lit("catch**"), lit("handle"), lit("handle*"),
    lit("ignwarn"), lit("macro-time-ignwarn"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val except_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("except")));
  return nil;
}

static val type_set_entries(val dlt, val fun)
{
  val name[] = { lit("typecase"), nil };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val type_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("type")));
  return nil;
}

static val yield_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("obtain-impl"), nil
  };
  val name[] = {
    lit("obtain"), lit("obtain-block"), lit("yield-from"), lit("yield"),
    lit("obtain*"), lit("obtain*-block"),
    lit("suspend"), lit("hlet"), lit("hlet*"),
    nil
  };

  set_dlt_entries_sys(dlt, sys_name, fun);
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val yield_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("yield")));
  return nil;
}

#if HAVE_SOCKETS
static val sock_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("sockaddr"), lit("sockaddr-in"), lit("sockaddr-in6"),
    lit("sockaddr-un"), lit("addrinfo"),
    lit("getaddrinfo"),
    lit("af-unspec"), lit("af-unix"), lit("af-inet"), lit("af-inet6"),
    lit("sock-stream"), lit("sock-dgram"),
    lit("inaddr-any"), lit("inaddr-loopback"),
    lit("in6addr-any"), lit("in6addr-loopback"),
    lit("sock-nonblock"), lit("sock-cloexec"),
    lit("ai-passive"), lit("ai-canonname"), lit("ai-numerichost"),
    lit("ai-v4mapped"), lit("ai-all"), lit("ai-addrconfig"),
    lit("ai-numericserv"),
    lit("str-inaddr"), lit("str-in6addr"),
    lit("str-inaddr-net"), lit("str-in6addr-net"),
    lit("inaddr-str"), lit("in6addr-str"),
    lit("shut-rd"), lit("shut-wr"), lit("shut-rdwr"),
    lit("open-socket"), lit("open-socket-pair"),
    lit("sock-bind"), lit("sock-connect"), lit("sock-listen"),
    lit("sock-accept"), lit("sock-shutdown"), lit("open-socket"),
    lit("open-socket-pair"), lit("sock-send-timeout"), lit("sock-recv-timeout"),
    nil
  };
  val name_noload[] = {
    lit("family"), lit("addr"), lit("port"), lit("flow-info"),
    lit("scope-id"), lit("prefix"), lit("path"), lit("flags"), lit("socktype"),
    lit("protocol"), lit("canonname"), nil
  };
  set_dlt_entries(dlt, name, fun);
  intern_only(name_noload);
  return nil;
}

static val sock_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  sock_load_init();
  load(scat2(stdlib_path, lit("socket")));
  return nil;
}

#endif

#if HAVE_TERMIOS

static val termios_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("set-iflags"), lit("set-oflags"), lit("set-cflags"), lit("set-lflags"),
    lit("clear-iflags"), lit("clear-oflags"), lit("clear-cflags"), lit("clear-lflags"),
    lit("go-raw"), lit("go-cbreak"), lit("go-canon"),
    lit("string-encode"), lit("string-decode"), nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val termios_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("termios")));
  return nil;
}

#endif

static val awk_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("awk-state"), nil
  };
  val name[] = {
    lit("awk"), nil
  };
  val name_noload[] = {
    lit("rec"), lit("orec"), lit("f"), lit("nf"), lit("nr"), lit("fnr"),
    lit("arg"), lit("fname"), lit("rs"), lit("krs"), lit("fs"), lit("ft"),
    lit("fw"), lit("kfs"), lit("ofs"), lit("ors"), lit("next"), lit("again"),
    lit("next-file"), lit("rng"), lit("-rng"), lit("rng-"), lit("-rng-"),
    lit("--rng"), lit("--rng-"), lit("rng+"), lit("-rng+"), lit("--rng+"),
    lit("ff"), lit("f"), lit("mf"), lit("fconv"), lit("->"), lit("->>"),
    lit("<-"), lit("!>"), lit("<!"), lit("prn"),
    lit("i"), lit("o"), lit("x"), lit("b"), lit("c"), lit("r"),
    lit("iz"), lit("oz"), lit("xz"), lit("bz"), lit("cz"), lit("rz"),
    nil
  };

  set_dlt_entries_sys(dlt, sys_name, fun);
  set_dlt_entries(dlt, name, fun);
  intern_only(name_noload);
  return nil;
}

static val awk_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("awk")));
  return nil;
}

static val build_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("list-builder"), lit("build-list"), lit("build"), lit("buildn"), nil
  };
  val name_noload[] = {
    lit("head"), lit("tail"), lit("add"), lit("add*"), lit("pend"),
    lit("pend*"), lit("ncon"), lit("ncon*"), lit("get"),
    lit("del"), lit("del*"),
    nil
  };

  set_dlt_entries(dlt, name, fun);
  intern_only(name_noload);
  return nil;
}

static val build_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("build")));
  return nil;
}

static val trace_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("trace"), lit("untrace"), nil
  };
  val name[] = {
    lit("*trace-output*"), lit("trace"), lit("untrace"), nil
  };

  set_dlt_entries_sys(dlt, sys_name, fun);
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val trace_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("trace")));
  trace_loaded = t;
  return nil;
}

static val getopts_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("opt-desc"), lit("opts"),
    lit("opt"), lit("getopts"), lit("opthelp"), lit("define-option-struct"),
    nil
  };
  val name_noload[] = {
    lit("short"), lit("long"), lit("helptext"), lit("type"),
    lit("in-args"), lit("out-args"), lit("cumul"), nil
  };
  set_dlt_entries(dlt, name, fun);
  intern_only(name_noload);
  return nil;
}

static val getopts_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("getopts")));
  return nil;
}

static val package_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("defpackage"), lit("in-package"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val package_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("package")));
  return nil;
}

static val getput_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("get-jsons"), lit("put-jsons"),
    lit("file-get"), lit("file-put"), lit("file-append"),
    lit("file-get-string"), lit("file-put-string"), lit("file-append-string"),
    lit("file-get-lines"), lit("file-put-lines"), lit("file-append-lines"),
    lit("file-get-buf"), lit("file-put-buf"),
    lit("file-place-buf"), lit("file-append-buf"),
    lit("file-get-json"), lit("file-put-json"), lit("file-append-json"),
    lit("file-get-jsons"), lit("file-put-jsons"), lit("file-append-jsons"),
    lit("command-get"), lit("command-put"),
    lit("command-get-string"), lit("command-put-string"),
    lit("command-get-lines"), lit("command-put-lines"),
    lit("command-get-buf"), lit("command-put-buf"),
    lit("command-get-json"), lit("command-put-json"),
    lit("command-get-jsons"), lit("command-put-jsons"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val getput_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("getput")));
  return nil;
}

static val tagbody_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("tagbody"), lit("go"), lit("prog"), lit("prog*"), nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val tagbody_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("tagbody")));
  return nil;
}

static val pmac_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("define-param-expander"), nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val pmac_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("pmac")));
  return nil;
}

static val error_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("bind-mac-error"), lit("bind-mac-check"),
    lit("lambda-too-many-args"),
    lit("lambda-too-few-args"), lit("lambda-short-apply-list"),
    nil
  };
  val name[] = {
    lit("compile-error"), lit("compile-warning"), lit("compile-defr-warning"),
    nil
  };
  set_dlt_entries_sys(dlt, sys_name, fun);
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val error_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("error")));
  return nil;
}

static val keyparams_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("extract-keys"),
    nil
  };
  val name_noload[] = {
    lit("--"),
    nil
  };
  val key_k = intern(lit("key"), keyword_package);
  set_dlt_entries_sys(dlt, sys_name, fun);
  if (fun)
    sethash(dlt, key_k, fun);
  else
    remhash(dlt, key_k);
  intern_only(name_noload);
  return nil;
}

static val keyparams_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("keyparams")));
  return nil;
}

static val ffi_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("with-dyn-lib"), lit("deffi"), lit("deffi-type"), lit("deffi-cb"),
    lit("deffi-cb-unsafe"),
    lit("deffi-sym"), lit("deffi-var"), lit("deffi-struct"),
    lit("deffi-union"), lit("typedef"), lit("sizeof"),
    lit("alignof"), lit("offsetof"), lit("arraysize"), lit("elemsize"),
    lit("elemtype"), lit("ffi"), lit("carray-ref"), lit("carray-sub"),
    lit("sub-buf"), lit("znew"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val ffi_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("ffi")));
  return nil;
}

static val doloop_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("doloop"), lit("doloop*"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val doloop_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("doloop")));
  return nil;
}

static val stream_wrap_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("stream-wrap"),
    nil
  };
  val name_noload[] = {
    lit("close"), lit("flush"), lit("seek"), lit("truncate"),
    lit("get-prop"), lit("set-prop"), lit("get-fd"), nil
  };

  set_dlt_entries(dlt, name, fun);
  intern_only(name_noload);
  return nil;
}

static val stream_wrap_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("stream-wrap")));
  return nil;
}

static val asm_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("asm")));
  return nil;
}

static val asm_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("assembler"),
    nil
  };
  val name[] = {
    lit("disassemble"),
    nil
  };

  set_dlt_entries_sys(dlt, sys_name, fun);
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val compiler_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("compiler")));
  return nil;
}

static val compiler_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("compiler"),
    nil
  };
  val name[] = {
    lit("compile-toplevel"), lit("compile"), lit("compile-file"),
    lit("compile-update-file"),
    lit("with-compilation-unit"), lit("dump-compiled-objects"),
    lit("*opt-level*"),
    nil
  };

  set_dlt_entries_sys(dlt, sys_name, fun);
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val debugger_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("debugger")));
  return nil;
}

static val debugger_set_entries(val dlt, val fun)
{
  val sys_name[] = {
    lit("debugger"), lit("print-backtrace"),
    nil
  };

  set_dlt_entries_sys(dlt, sys_name, fun);
  return nil;
}


static val op_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("op"), lit("do"), lit("lop"), lit("ldo"), lit("ap"), lit("ip"),
    lit("ado"), lit("ido"), lit("ret"), lit("aret"),
    lit("opip"), lit("oand"), lit("flow"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val op_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("op")));
  return nil;
}

static val save_exe_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("save-exe")));
  return nil;
}

static val save_exe_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("save-exe"),
    nil
  };

  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val defset_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("defset")));
  return nil;
}

static val defset_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("defset"), lit("sub-list"), lit("sub-vec"), lit("sub-str"),
    lit("left"), lit("right"), lit("key"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}
 
static val copy_file_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("copy-file")));
  return nil;
}

static val copy_file_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("copy-path-opts"), lit("copy-file"), lit("copy-files"),
    lit("copy-path-rec"), lit("remove-path-rec"),
    lit("chown-rec"), lit("chmod-rec"), lit("touch"), lit("rel-path"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val each_prod_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("each-prod")));
  return nil;
}

static val each_prod_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("each-prod"), lit("collect-each-prod"), lit("append-each-prod"),
    lit("each-prod*"), lit("collect-each-prod*"), lit("append-each-prod*"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val quips_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("quips")));
  return nil;
}

static val quips_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("quip"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val match_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("match")));
  return nil;
}

static val match_set_entries(val dlt, val fun)
{
  val name_noload[] = {
    lit("all*"), lit("as"), lit("with"), lit("scan"), lit("sme"),
    nil
  };
  val name[] = {
    lit("when-match"), lit("match-case"), lit("if-match"),
    lit("lambda-match"), lit("defun-match"), lit("defmatch"),
    lit("each-match"), lit("append-matches"),
    lit("keep-matches"), lit("each-match-product"),
    lit("append-match-products"), lit("keep-match-products"),
    lit("*match-macro*"),
    nil
  };
  val match_k = intern(lit("match"), keyword_package);

  if (fun)
    sethash(dlt, match_k, fun);
  else
    remhash(dlt, match_k);

  set_dlt_entries(dlt, name, fun);
  intern_only(name_noload);
  return nil;
}

static val doc_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("doc-lookup")));
  return nil;
}

static val doc_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("doc"), lit("*doc-url*"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

static val pic_instantiate(val set_fun)
{
  funcall1(set_fun, nil);
  load(scat2(stdlib_path, lit("pic")));
  return nil;
}

static val pic_set_entries(val dlt, val fun)
{
  val name[] = {
    lit("pic"),
    nil
  };
  set_dlt_entries(dlt, name, fun);
  return nil;
}

val dlt_register(val dlt,
                 val (*instantiate)(val),
                 val (*set_entries)(val, val))
{
  return set_entries(dl_table, func_f0(func_f1(dlt, set_entries), instantiate));
}

void lisplib_init(void)
{
  prot1(&dl_table);
  dl_table = make_hash(nil, nil, nil);
  dlt_register(dl_table, place_instantiate, place_set_entries);
  dlt_register(dl_table, ver_instantiate, ver_set_entries);
  dlt_register(dl_table, ifa_instantiate, ifa_set_entries);
  dlt_register(dl_table, txr_case_instantiate, txr_case_set_entries);
  dlt_register(dl_table, with_resources_instantiate, with_resources_set_entries);
  dlt_register(dl_table, path_test_instantiate, path_test_set_entries);
  dlt_register(dl_table, struct_instantiate, struct_set_entries);
  dlt_register(dl_table, with_stream_instantiate, with_stream_set_entries);
  dlt_register(dl_table, hash_instantiate, hash_set_entries);
  dlt_register(dl_table, except_instantiate, except_set_entries);
  dlt_register(dl_table, type_instantiate, type_set_entries);
  dlt_register(dl_table, yield_instantiate, yield_set_entries);
#if HAVE_SOCKETS
  dlt_register(dl_table, sock_instantiate, sock_set_entries);
#endif
#if HAVE_TERMIOS
  dlt_register(dl_table, termios_instantiate, termios_set_entries);
#endif
  dlt_register(dl_table, awk_instantiate, awk_set_entries);
  dlt_register(dl_table, build_instantiate, build_set_entries);
  dlt_register(dl_table, trace_instantiate, trace_set_entries);
  dlt_register(dl_table, getopts_instantiate, getopts_set_entries);
  dlt_register(dl_table, package_instantiate, package_set_entries);
  dlt_register(dl_table, getput_instantiate, getput_set_entries);
  dlt_register(dl_table, tagbody_instantiate, tagbody_set_entries);
  dlt_register(dl_table, pmac_instantiate, pmac_set_entries);
  dlt_register(dl_table, error_instantiate, error_set_entries);
  dlt_register(dl_table, keyparams_instantiate, keyparams_set_entries);
  dlt_register(dl_table, ffi_instantiate, ffi_set_entries);
  dlt_register(dl_table, doloop_instantiate, doloop_set_entries);
  dlt_register(dl_table, stream_wrap_instantiate, stream_wrap_set_entries);
  dlt_register(dl_table, asm_instantiate, asm_set_entries);
  dlt_register(dl_table, compiler_instantiate, compiler_set_entries);
  dlt_register(dl_table, debugger_instantiate, debugger_set_entries);

  if (!opt_compat || opt_compat >= 185)
    dlt_register(dl_table, op_instantiate, op_set_entries);

  dlt_register(dl_table, save_exe_instantiate, save_exe_set_entries);
  dlt_register(dl_table, defset_instantiate, defset_set_entries);
  dlt_register(dl_table, copy_file_instantiate, copy_file_set_entries);
  dlt_register(dl_table, each_prod_instantiate, each_prod_set_entries);
  dlt_register(dl_table, quips_instantiate, quips_set_entries);
  dlt_register(dl_table, match_instantiate, match_set_entries);
  dlt_register(dl_table, doc_instantiate, doc_set_entries);
  dlt_register(dl_table, pic_instantiate, pic_set_entries);

  reg_fun(intern(lit("try-load"), system_package), func_n1(lisplib_try_load));
}

val lisplib_try_load(val sym)
{
  val fun = gethash(dl_table, sym);

  if (fun) {
     unsigned ds = debug_clear(opt_dbg_autoload ? 0 : DBG_ENABLE);
     val saved_dyn_env = dyn_env;
     dyn_env = make_env(nil, nil, dyn_env);
     env_vbind(dyn_env, package_s, system_package);
     env_vbind(dyn_env, package_alist_s, packages);
     funcall(fun);
     dyn_env = saved_dyn_env;
     debug_restore(ds);
     return t;
  }
  return nil;
}
