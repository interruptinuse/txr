/* Copyright 2015-2024
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
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "config.h"
#include "alloca.h"
#include "lib.h"
#include "gc.h"
#include "args.h"
#include "utf8.h"
#include "sysif.h"
#include "struct.h"
#include "eval.h"
#include "time.h"

struct tm_wrap {
  struct tm tm;
#if HAVE_TM_ZONE
  const char *zone;
#endif
};

val time_s, time_local_s, time_utc_s, time_string_s, time_parse_s;
val year_s, month_s, day_s, hour_s, min_s, sec_s, wday_s, yday_s;
val dst_s, gmtoff_s, zone_s;

val time_sec(void)
{
  struct timeval tv;
  if (gettimeofday(&tv, 0) == -1)
    return nil;
  return num(tv.tv_sec);
}

val time_sec_usec(void)
{
  struct timeval tv;
  if (gettimeofday(&tv, 0) == -1)
    return nil;
  return cons(num_time(tv.tv_sec), num(tv.tv_usec));
}

val time_sec_nsec(void)
{
#if HAVE_CLOCK_GETTIME
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    return nil;
  return cons(num_time(ts.tv_sec), num(ts.tv_nsec));
#else
  struct timeval tv;
  if (gettimeofday(&tv, 0) == -1)
    return nil;
  return cons(num_time(tv.tv_sec), num(1000 * tv.tv_usec));
#endif
}


#if !HAVE_GMTIME_R
struct tm *gmtime_r(const time_t *timep, struct tm *result);
struct tm *localtime_r(const time_t *timep, struct tm *result);

struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
  struct tm *hack = gmtime(timep);
  *result = *hack;
  return hack;
}

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
  struct tm *hack = localtime(timep);
  *result = *hack;
  return hack;
}
#endif

static val string_time(struct tm *(*break_time_fn)(const time_t *, struct tm *),
                       char *format, time_t time)
{
  char buffer[512] = "";
  struct tm broken_out_time;

  if (break_time_fn(&time, &broken_out_time) == 0)
    return nil;

#if HAVE_TM_ZONE
  if (strcmp(broken_out_time.TM_ZONE, "GMT") == 0)
    broken_out_time.TM_ZONE = "UTC";
#endif

  if (strftime(buffer, sizeof buffer, format, &broken_out_time) == 0)
    buffer[0] = 0;

  {
    wchar_t *wctime = utf8_dup_from(buffer);
    return string_own(wctime);
  }
}

val time_string_local(val time, val format)
{
  val self = lit("time-string-local");
  time_t secs = c_time(time, self);
  const wchar_t *wcfmt = c_str(format, self);
  char *u8fmt = utf8_dup_to(wcfmt);
  val timestr = string_time(localtime_r, u8fmt, secs);
  free(u8fmt);
  return timestr;
}

val time_string_utc(val time, val format)
{
  val self = lit("time-string-utc");
  time_t secs = c_time(time, self);
  const wchar_t *wcfmt = c_str(format, self);
  char *u8fmt = utf8_dup_to(wcfmt);
  val timestr = string_time(gmtime_r, u8fmt, secs);
  free(u8fmt);
  return timestr;
}

static val time_str_local(val format, val time_in)
{
  val time = default_arg_strict(time_in, time_sec());
  return time_string_local(time, format);
}

static val time_str_utc(val format, val time_in)
{
  val time = default_arg_strict(time_in, time_sec());
  return time_string_utc(time, format);
}


static val broken_time_list(struct tm *tms)
{
  return list(num(tms->tm_year + 1900),
              num_fast(tms->tm_mon + 1),
              num_fast(tms->tm_mday),
              num_fast(tms->tm_hour),
              num_fast(tms->tm_min),
              num_fast(tms->tm_sec),
              tms->tm_isdst ? t : nil,
              nao);
}

static void tm_to_time_struct(val time_struct, struct tm *ptm)
{
  slotset(time_struct, year_s, num(ptm->tm_year + 1900));
  slotset(time_struct, month_s, num_fast(ptm->tm_mon + 1));
  slotset(time_struct, day_s, num_fast(ptm->tm_mday));
  slotset(time_struct, hour_s, num_fast(ptm->tm_hour));
  slotset(time_struct, min_s, num_fast(ptm->tm_min));
  slotset(time_struct, sec_s, num_fast(ptm->tm_sec));
  slotset(time_struct, wday_s, num_fast(ptm->tm_wday));
  slotset(time_struct, yday_s, num_fast(ptm->tm_yday));
  slotset(time_struct, dst_s, tnil(ptm->tm_isdst));
#if HAVE_TM_GMTOFF
  slotset(time_struct, gmtoff_s, num_fast(ptm->TM_GMTOFF));
#endif
#if HAVE_TM_ZONE
  slotset(time_struct, zone_s, if2(ptm->TM_ZONE, string_utf8(ptm->TM_ZONE)));
#endif
}

static val broken_time_struct(struct tm *tms)
{
  args_decl_constsize(args, ARGS_MIN);
  val ts = make_struct(time_s, nil, args);

  tm_to_time_struct(ts, tms);

  return ts;
}

val time_fields_local(val time_in)
{
  val self = lit("time-fields-local");
  struct tm tms;
  val time = default_arg_strict(time_in, time_sec());
  time_t secs = c_time(time, self);

  if (localtime_r(&secs, &tms) == 0)
    return nil;

  return broken_time_list(&tms);
}

val time_fields_utc(val time_in)
{
  val self = lit("time-fields-utc");
  struct tm tms;
  val time = default_arg_strict(time_in, time_sec());
  time_t secs = c_time(time, self);

  if (gmtime_r(&secs, &tms) == 0)
    return nil;

  return broken_time_list(&tms);
}

val time_struct_local(val time_in)
{
  val self = lit("time-struct-local");
  struct tm tms;
  val time = default_arg_strict(time_in, time_sec());
  time_t secs = c_time(time, self);

  if (localtime_r(&secs, &tms) == 0)
    return nil;

  return broken_time_struct(&tms);
}

val time_struct_utc(val time_in)
{
  val self = lit("time-struct-utc");
  struct tm tms;
  val time = default_arg_strict(time_in, time_sec());
  time_t secs = c_time(time, self);

  if (gmtime_r(&secs, &tms) == 0)
    return nil;

  return broken_time_struct(&tms);
}

static void time_fields_to_tm(struct tm_wrap *ptmw,
                              val year, val month, val day,
                              val hour, val min, val sec,
                              val wday, val yday,
                              val dst, val gmtoff, val zone,
                              val self)
{
  struct tm *ptm = &ptmw->tm;
  uses_or2;
  ptm->tm_year = c_num(or2(year, zero), self) - 1900;
  ptm->tm_mon = c_num(or2(month, zero), self) - 1;
  ptm->tm_mday = c_num(or2(day, zero), self);
  ptm->tm_hour = c_num(or2(hour, zero), self);
  ptm->tm_min = c_num(or2(min, zero), self);
  ptm->tm_sec = c_num(or2(sec, zero), self);
  ptm->tm_wday = c_num(or2(wday, zero), self);
  ptm->tm_yday = c_num(or2(yday, zero), self);

  if (!dst)
    ptm->tm_isdst = 0;
  else if (dst == auto_k)
    ptm->tm_isdst = -1;
  else
    ptm->tm_isdst = 1;

#if HAVE_TM_GMTOFF
  ptm->TM_GMTOFF = c_num(or2(gmtoff, zero), self);
#endif
#if HAVE_TM_ZONE
  ptmw->zone = ptm->TM_ZONE = if3(zone, utf8_dup_to(c_str(zone, self)), 0);
#endif
}

static void time_fields_cleanup(struct tm_wrap *ptm)
{
#if HAVE_TM_ZONE
  free(strip_qual(char *, ptm->zone));
#endif
}

static void time_struct_to_tm(struct tm_wrap *ptmw, val time_struct, val self)
{
  val year = slot(time_struct, year_s);
  val month = slot(time_struct, month_s);
  val day = slot(time_struct, day_s);
  val hour = slot(time_struct, hour_s);
  val min = slot(time_struct, min_s);
  val sec = slot(time_struct, sec_s);
  val wday = slot(time_struct, wday_s);
  val yday = slot(time_struct, yday_s);
  val dst = slot(time_struct, dst_s);
  val gmtoff = slot(time_struct, gmtoff_s);
  val zone = slot(time_struct, zone_s);

  time_fields_to_tm(ptmw, year, month, day, hour, min, sec,
                    wday, yday, dst, gmtoff, zone, self);
}

static val make_time_impl(time_t (*pmktime)(struct tm *),
                          val year, val month, val day,
                          val hour, val minute, val second,
                          val isdst, val self)
{
  struct tm_wrap local = all_zero_init;
  time_t time;

  time_fields_to_tm(&local, year, month, day,
                    hour, minute, second, nil, nil, isdst, nil, nil, self);
  time = pmktime(&local.tm);
  time_fields_cleanup(&local);

  return time == -1 ? nil : num_time(time);
}

val make_time(val year, val month, val day,
              val hour, val minute, val second,
              val isdst)
{
  val self = lit("make-time");
  return make_time_impl(mktime, year, month, day, hour, minute, second,
                        isdst, self);
}

#if HAVE_STRPTIME

static struct tm epoch_tm(void)
{
  struct tm ep = all_zero_init;
  ep.tm_year = 70;
  ep.tm_mday = 1;
  return ep;
}

static int strptime_wrap(val string, val format, struct tm *ptms)
{
  val self = lit("strptime");
  const wchar_t *w_str = c_str(string, self);
  const wchar_t *w_fmt = c_str(format, self);
  char *str = utf8_dup_to(w_str);
  char *fmt = utf8_dup_to(w_fmt);
  char *ptr = strptime(str, fmt, ptms);
  int ret = ptr != 0;
  free(fmt);
  free(str);
  return ret;
}

val time_parse(val format, val string)
{
  struct tm tms = epoch_tm();
  int ret = strptime_wrap(string, format, &tms);
  return ret ? broken_time_struct(&tms) : nil;
}

#endif

#if !HAVE_TIMEGM && HAVE_SETENV

static time_t timegm_hack(struct tm *tm)
{
  time_t ret;
  char *tz;

  tz = getenv("TZ");
  setenv("TZ", "UTC", 1);
#if HAVE_TZSET
  tzset();
#endif
  ret = mktime(tm);
  if (tz)
    setenv("TZ", tz, 1);
  else
    unsetenv("TZ");
#if HAVE_TZSET
  tzset();
#endif

  return ret;
}

#endif

#if !HAVE_TIMEGM && !HAVE_SETENV

static time_t timegm_hack(struct tm *tm)
{
  uw_throw(system_error_s, lit("timegm function missing"));
}

#endif

val make_time_utc(val year, val month, val day,
                  val hour, val minute, val second,
                  val isdst)
{
  val self = lit("make-time-utc");
#if HAVE_TIMEGM
  time_t (*pmktime)(struct tm *) = timegm;
#else
  time_t (*pmktime)(struct tm *) = timegm_hack;
#endif

  return make_time_impl(pmktime, year, month, day, hour, minute, second,
                        isdst, self);
}

static val time_meth(val utc_p, val time_struct)
{
  val year = slot(time_struct, year_s);
  val month = slot(time_struct, month_s);
  val day = slot(time_struct, day_s);
  val hour = slot(time_struct, hour_s);
  val min = slot(time_struct, min_s);
  val sec = slot(time_struct, sec_s);
  val dst = slot(time_struct, dst_s);
  val gmtoff = slot(time_struct, gmtoff_s);

  val out = (utc_p ? make_time_utc : make_time)(year, month, day,
                                                hour, min, sec, dst);

  if (gmtoff)
    out = minus(out, gmtoff);

  return out;
}

static val time_string_meth(val time_struct, val format)
{
  val self = lit("(meth time-string)");
  struct tm_wrap tms = all_zero_init;
  char buffer[512] = "";
  char *fmt = utf8_dup_to(c_str(format, self));

  time_struct_to_tm(&tms, time_struct, self);

  if (strftime(buffer, sizeof buffer, fmt, &tms.tm) == 0)
    buffer[0] = 0;

  free(fmt);
  time_fields_cleanup(&tms);

  return string_own(utf8_dup_from(buffer));
}

#if HAVE_STRPTIME

static val time_parse_meth(val time_struct, val format, val string)
{
  val self = lit("(meth time-parse)");
  struct tm_wrap tms = all_zero_init;
  val ret = nil;

  time_struct_to_tm(&tms, time_struct, self);

  {
    const wchar_t *w_str = c_str(string, self);
    const wchar_t *w_fmt = c_str(format, self);
    char *str = utf8_dup_to(w_str);
    char *fmt = utf8_dup_to(w_fmt);
    char *ptr = strptime(str, fmt, &tms.tm);

    if (ptr != 0) {
      tm_to_time_struct(time_struct, &tms.tm);
      ret = string_utf8(ptr);
    }

    free(fmt);
    free(str);
  }

  time_fields_cleanup(&tms);

  return ret;
}

val time_parse_local(val format, val string)
{
  struct tm tms = epoch_tm();

  if (!strptime_wrap(string, format, &tms))
    return nil;
#if HAVE_TM_GMTOFF
  {
    long gmtoff = tms.TM_GMTOFF;
    return num(mktime(&tms) - gmtoff);
  }
#else
  return num(mktime(&tms));
#endif
}

val time_parse_utc(val format, val string)
{
  struct tm tms = epoch_tm();
  if (!strptime_wrap(string, format, &tms))
    return nil;
#if HAVE_TIMEGM && HAVE_TM_GMTOFF
  {
    long gmtoff = tms.TM_GMTOFF;
    return num_time(timegm(&tms) - gmtoff);
  }
#elif HAVE_TM_GMTOFF
  {
    long gmtoff = tms.TM_GMTOFF;
    return num_time(timegm_hack(&tms) - tms.gmtoff);
  }
#elif HAVE_TIMEGM
  return num_time(timegm(&tms));
#else
  return num_time(timegm_hack(&tms));
#endif
}

#endif

void time_init(void)
{
  val time_st;

  time_s = intern(lit("time"), user_package);
  time_local_s = intern(lit("time-local"), user_package);
  time_utc_s = intern(lit("time-utc"), user_package);
  time_string_s = intern(lit("time-string"), user_package);
  time_parse_s = intern(lit("time-parse"), user_package);
  year_s = intern(lit("year"), user_package);
  month_s = intern(lit("month"), user_package);
  day_s = intern(lit("day"), user_package);
  hour_s = intern(lit("hour"), user_package);
  min_s = intern(lit("min"), user_package);
  sec_s = intern(lit("sec"), user_package);
  wday_s = intern(lit("wday"), user_package);
  yday_s = intern(lit("yday"), user_package);
  dst_s = intern(lit("dst"), user_package);
  gmtoff_s = intern(lit("gmtoff"), user_package);
  zone_s = intern(lit("zone"), user_package);

  time_st = make_struct_type(time_s, nil,
                             list(time_local_s, time_utc_s,
                                  time_string_s, time_parse_s, nao),
                             list(year_s, month_s, day_s,
                                  hour_s, min_s, sec_s,
                                  wday_s, yday_s,
                                  dst_s, gmtoff_s, zone_s, nao),
                             nil, nil, nil, nil);

  static_slot_set(time_st, time_local_s, func_f1(nil, time_meth));
  static_slot_set(time_st, time_utc_s, func_f1(t, time_meth));
  static_slot_set(time_st, time_string_s, func_n2(time_string_meth));
#if HAVE_STRPTIME
  static_slot_set(time_st, time_parse_s, func_n3(time_parse_meth));
#endif

  reg_fun(time_s, func_n0(time_sec));
  reg_fun(intern(lit("time-usec"), user_package), func_n0(time_sec_usec));
  reg_fun(intern(lit("time-nsec"), user_package), func_n0(time_sec_nsec));
  reg_fun(intern(lit("time-string-local"), user_package), func_n2(time_string_local));
  reg_fun(intern(lit("time-string-utc"), user_package), func_n2(time_string_utc));
  reg_fun(intern(lit("time-str-local"), user_package), func_n2o(time_str_local, 1));
  reg_fun(intern(lit("time-str-utc"), user_package), func_n2o(time_str_utc, 1));
  reg_fun(intern(lit("time-fields-local"), user_package), func_n1o(time_fields_local, 0));
  reg_fun(intern(lit("time-fields-utc"), user_package), func_n1o(time_fields_utc, 0));
  reg_fun(intern(lit("time-struct-local"), user_package), func_n1o(time_struct_local, 0));
  reg_fun(intern(lit("time-struct-utc"), user_package), func_n1o(time_struct_utc, 0));
  reg_fun(intern(lit("make-time"), user_package), func_n7(make_time));
#if HAVE_TIMEGM || HAVE_SETENV
  reg_fun(intern(lit("make-time-utc"), user_package), func_n7(make_time_utc));
#endif
  reg_fun(intern(lit("time-parse"), user_package), func_n2(time_parse));
  reg_fun(intern(lit("time-parse-local"), user_package), func_n2(time_parse_local));
  reg_fun(intern(lit("time-parse-utc"), user_package), func_n2(time_parse_utc));
}

