/* Copyright 2009-2023
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

#define UTF8_DECL_OPENDIR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <signal.h>
#include <dirent.h>
#include "config.h"
#include "lib.h"
#include "signal.h"
#include "unwind.h"
#include "utf8.h"

#if WCHAR_MAX > 65535
#define FULL_UNICODE
#endif

#ifndef FULL_UNICODE
static void conversion_error(void)
{
  uw_throw(range_error_s,
           lit("encountered utf-8 character that needs full unicode support"));
}
#endif

size_t utf8_from_buf(wchar_t *wdst, const unsigned char *src, size_t nbytes)
{
  size_t nchar = 1;
  enum utf8_state state = utf8_init;
  const unsigned char *backtrack = 0, *end = src + nbytes;
  wchar_t wch = 0, wch_min = 0;

  while (src < end) {
    int ch = *src++;

    switch (state) {
    case utf8_init:
      switch (ch >> 4) {
      case 0x0: case 0x1: case 0x2: case 0x3:
      case 0x4: case 0x5: case 0x6: case 0x7:
        if (wdst) {
          if (ch)
            *wdst++ = ch;
          else
            *wdst++ = 0xDC00;
        }
        nchar++;
        break;
      case 0xC: case 0xD:
        state = utf8_more1;
        wch = (ch & 0x1F);
        wch_min = 0x80;
        break;
      case 0xE:
        state = utf8_more2;
        wch = (ch & 0xF);
        wch_min = 0x800;
        break;
      case 0xF:
        if (ch < 0xF5) {
#ifdef FULL_UNICODE
          state = utf8_more3;
          wch = (ch & 0x7);
          wch_min = 0x10000;
          break;
#else
        conversion_error();
#endif
        }
        /* fallthrough */
      default:
        if (wdst)
          *wdst++ = 0xDC00 | ch;
        nchar++;
        break;
      }
      backtrack = src - 1;
      break;
    case utf8_more1:
    case utf8_more2:
    case utf8_more3:
      if (ch >= 0x80 && ch < 0xC0) {
        wch <<= 6;
        wch |= (ch & 0x3F);
        state = convert(enum utf8_state, state - 1);
        if (state == utf8_init) {
          if (wch < wch_min ||
              (wch <= 0xFFFF && (wch & 0xFF00) == 0xDC00) ||
              (wch > 0x10FFFF))
          {
            src = backtrack;
            if (wdst)
              *wdst++ = 0xDC00 | *src;
            src++;
          } else {
            if (wdst)
              *wdst++ = wch;
          }
          nchar++;
        }
      } else {
        src = backtrack;
        if (wdst)
          *wdst++ = 0xDC00 | *src;
        src++;
        nchar++;
        state = utf8_init;
      }
      break;
    }
  }

  if (state != utf8_init) {
    while (backtrack != src) {
      if (wdst)
        *wdst++ = 0xDC00 | *backtrack;
      nchar++;
      backtrack++;
    }
  }

  if (wdst)
    *wdst++ = 0;

  return nchar;
}

size_t utf8_to_buf(unsigned char *dst, const wchar_t *wsrc, int null_term)
{
  size_t nbyte = 0;
  wchar_t wch;

  while ((wch = *wsrc++)) {
    if (wch < 0x80) {
      nbyte += 1;
      if (dst)
        *dst++ = wch;
    } else if (wch < 0x800) {
      nbyte += 2;
      if (dst) {
        *dst++ = 0xC0 | (wch >> 6);
        *dst++ = 0x80 | (wch & 0x3F);
      }
    } else if (wch < 0x10000) {
      if ((wch & 0xFF00) == 0xDC00) {
        nbyte += 1;
        if (dst)
          *dst++ = (wch & 0xFF);
      } else {
        nbyte += 3;
        if (dst) {
          *dst++ = 0xE0 | (wch >> 12);
          *dst++ = 0x80 | ((wch >> 6) & 0x3F);
          *dst++ = 0x80 | (wch & 0x3F);
        }
      }
    } else if (wch < 0x110000) {
      nbyte += 4;
      if (dst) {
        *dst++ = 0xF0 | (wch >> 18);
        *dst++ = 0x80 | ((wch >> 12) & 0x3F);
        *dst++ = 0x80 | ((wch >> 6) & 0x3F);
        *dst++ = 0x80 | (wch & 0x3F);
      }
    }
  }

  if (null_term) {
    if (dst)
      *dst++ = 0;
    nbyte++;
  }

  return nbyte;
}

size_t utf8_to(char *dst, const wchar_t *wsrc)
{
  return utf8_to_buf(coerce(unsigned char *, dst), wsrc, 1);
}

wchar_t *utf8_dup_from(const char *str)
{
  size_t len = strlen(str);
  size_t nchar = utf8_from_buf(0, coerce(const unsigned char *, str), len);
  wchar_t *wstr = chk_wmalloc(nchar);
  utf8_from_buf(wstr, coerce(const unsigned char *, str), len);
  return wstr;
}

wchar_t *utf8_dup_from_buf(const char *str, size_t size)
{
  size_t nchar = utf8_from_buf(0, coerce(const unsigned char *, str), size);
  wchar_t *wstr = chk_wmalloc(nchar);
  utf8_from_buf(wstr, coerce(const unsigned char *, str), size);
  return wstr;
}

unsigned char *utf8_dup_to_buf(const wchar_t *wstr, size_t *pnbytes,
                               int null_term)
{
  size_t nbyte = utf8_to_buf(0, wstr, null_term);
  unsigned char *str = chk_malloc(nbyte);
  utf8_to_buf(str, wstr, null_term);
  *pnbytes = nbyte;
  return str;
}

char *utf8_dup_to(const wchar_t *wstr)
{
  size_t len = utf8_to(0, wstr) - 1;
  char *str = coerce(char *, chk_malloc(len + 1));
  utf8_to(str, wstr);
  str[len] = 0;
  if (strlen(str) != len) {
    free(str);
    uw_throw(error_s,
             lit("Cannot convert string with embedded NUL to UTF-8 string"));
  }
  return str;
}

int utf8_encode(wchar_t wch, int (*put)(int ch, mem_t *ctx), mem_t *ctx)
{
  if (wch < 0x80) {
    return put(wch, ctx);
  } else if (wch < 0x800) {
    return put(0xC0 | (wch >> 6), ctx) &&
           put(0x80 | (wch & 0x3F), ctx);
  } else if (wch < 0x10000) {
    if ((wch & 0xFF00) == 0xDC00) {
      return put(wch & 0xFF, ctx);
    } else {
      return put(0xE0 | (wch >> 12), ctx) &&
             put(0x80 | ((wch >> 6) & 0x3F), ctx) &&
             put(0x80 | (wch & 0x3F), ctx);
    }
  } else if (wch < 0x110000) {
    return put(0xF0 | (wch >> 18), ctx) &&
           put(0x80 | ((wch >> 12) & 0x3F), ctx) &&
           put(0x80 | ((wch >> 6) & 0x3F), ctx) &&
           put(0x80 | (wch & 0x3F), ctx);
  }

  uw_throwf(error_s,
            lit("cannot convert character value #x~x to UTF-8"),
            num(wch), nao);
}

void utf8_decoder_init(utf8_decoder_t *ud)
{
  ud->state = utf8_init;
  ud->flags = 0;
  ud->wch = 0;
  ud->head = ud->tail = ud->back = 0;
}

wint_t utf8_decode(utf8_decoder_t *ud, int (*get)(mem_t *ctx), mem_t *ctx)
{
  for (;;) {
    int ch;

    if (ud->tail != ud->head) {
      ch = ud->buf[ud->tail];
      ud->tail = (ud->tail + 1) % 8;
    } else {
      ch = get(ctx);
      ud->buf[ud->head] = ch;
      ud->head = ud->tail = (ud->head + 1) % 8;
    }

    if (ch == EOF) {
      if (ud->state == utf8_init) {
        return WEOF;
      } else {
        wchar_t wch = 0xDC00 | ud->buf[ud->back];
        ud->tail = ud->back = (ud->back + 1) % 8;
        ud->state = utf8_init;
        return wch;
      }
    }

    switch (ud->state) {
    case utf8_init:
      switch (ch >> 4) {
      case 0x0: case 0x1: case 0x2: case 0x3:
      case 0x4: case 0x5: case 0x6: case 0x7:
        ud->back = ud->tail;
        if (ch == 0 && (ud->flags & UTF8_ADMIT_NUL) == 0)
          return 0xDC00;
        return ch;
      case 0xC: case 0xD:
        ud->state = utf8_more1;
        ud->wch = (ch & 0x1F);
        ud->wch_min = 0x80;
        break;
      case 0xE:
        ud->state = utf8_more2;
        ud->wch = (ch & 0xF);
        ud->wch_min = 0x800;
        break;
      case 0xF:
        if (ch < 0xF5) {
#ifdef FULL_UNICODE
          ud->state = utf8_more3;
          ud->wch = (ch & 0x7);
          ud->wch_min = 0x10000;
          break;
#else
        conversion_error();
#endif
        }
        /* fallthrough */
      default:
        ud->back = ud->tail;
        return 0xDC00 | ch;
      }
      break;
    case utf8_more1:
    case utf8_more2:
    case utf8_more3:
      if (ch >= 0x80 && ch < 0xC0) {
        ud->wch <<= 6;
        ud->wch |= (ch & 0x3F);
        ud->state = convert(enum utf8_state, ud->state - 1);
        if (ud->state == utf8_init) {
          if (ud->wch < ud->wch_min ||
              (ud->wch <= 0xFFFF && (ud->wch & 0xFF00) == 0xDC00) ||
              (ud->wch > 0x10FFFF))
          {
            wchar_t wch = 0xDC00 | ud->buf[ud->back];
            ud->tail = ud->back = (ud->back + 1) % 8;
            return wch;
          } else {
            ud->back = ud->tail;
            return ud->wch;
          }
        }
      } else {
        wchar_t wch = 0xDC00 | ud->buf[ud->back];
        ud->tail = ud->back = (ud->back + 1) % 8;
        ud->state = utf8_init;
        return wch;
      }
      break;
    }
  }
}

FILE *w_fopen(const wchar_t *wname, const wchar_t *wmode)
{
  char *name = utf8_dup_to(wname);
  char *mode = utf8_dup_to(wmode);
  FILE *f = fopen(name, mode);
  free(name);
  free(mode);
  return f;
}

FILE *w_fdopen(int fd, const wchar_t *wmode)
{
  char *mode = utf8_dup_to(wmode);
  FILE *f = fdopen(fd, mode);
  free(mode);
  return f;
}

int w_remove(const wchar_t *wpath)
{
  char *path = utf8_dup_to(wpath);
  int err = remove(path);
  free(path);
  return err;
}

int w_rename(const wchar_t *wfrom, const wchar_t *wto)
{
  char *from = utf8_dup_to(wfrom);
  char *to = utf8_dup_to(wto);
  int err = rename(from, to);
  free(to);
  free(from);
  return err;
}

DIR *w_opendir(const wchar_t *wname)
{
  char *name = utf8_dup_to(wname);
  DIR *d = opendir(name);
  free(name);
  return d;
}
