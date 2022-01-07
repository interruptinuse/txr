/* mpi.c
 *
 * by Michael J. Fromberger <http://www.dartmouth.edu/~sting/>
 * Developed 1998-2004.
 * Assigned to the public domain as of 2002; see README.
 *
 * Arbitrary precision integer arithmetic library
 */

#include "config.h"
#if MP_IOFUNC
#include <stdio.h>
#include <ctype.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "mpi.h"
#if MP_ARGCHK == 2
#include <assert.h>
#endif

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#ifdef __cplusplus
#define convert(TYPE, EXPR) (static_cast<TYPE>(EXPR))
#define coerce(TYPE, EXPR) (reinterpret_cast<TYPE>(EXPR))
#else
#define convert(TYPE, EXPR) ((TYPE) (EXPR))
#define coerce(TYPE, EXPR) ((TYPE) (EXPR))
#endif

typedef unsigned char mem_t;
extern mem_t *chk_calloc(size_t n, size_t size);

#include "logtab.h"

/* Default precision for newly created mp_int's */
static mp_size s_mp_defprec = MP_DEFPREC;

#define NEG MP_NEG
#define ZPOS MP_ZPOS
#define DIGIT_BIT MP_DIGIT_BIT
#define DIGIT_MAX MP_DIGIT_MAX

#define CARRYOUT(W) ((W)>>DIGIT_BIT)
#define ACCUM(W) ((W)&MP_DIGIT_MAX)

#if MP_ARGCHK == 1
#define ARGCHK(X,Y) {if(!(X)){return (Y);}}
#elif MP_ARGCHK == 2
#define ARGCHK(X,Y) assert(X)
#else
#define ARGCHK(X,Y)
#endif

/* Nicknames for access macros */
#define SIGN(MP) mp_sign(MP)
#define ISNEG(MP) mp_isneg(MP)
#define USED(MP) mp_used(MP)
#define ALLOC(MP) mp_alloc(MP)
#define DIGITS(MP) mp_digits(MP)
#define DIGIT(MP,N) mp_digit(MP,N)

/* This defines the maximum I/O base (minimum is 2) */
#define MAX_RADIX 64

/* Constant strings returned by mp_strerror() */
static const char *mp_err_string[] = {
  "unknown result code", /* say what? */
  "boolean true", /* MP_OKAY, MP_YES */
  "boolean false", /* MP_NO */
  "out of memory", /* MP_MEM */
  "argument out of range", /* MP_RANGE */
  "invalid input parameter", /* MP_BADARG */
  "result is undefined", /* MP_UNDEF */
  "result is too large" /* MP_TOOBIG */
};

static const char *s_dmap_1 =
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";

#if MP_MACRO == 0
 void s_mp_setz(mp_digit *dp, mp_size count); /* zero digits */
 void s_mp_copy(mp_digit *sp, mp_digit *dp, mp_size count); /* copy */
 void *s_mp_alloc(size_t nb, size_t ni); /* general allocator */
 void s_mp_free(void *ptr); /* general free function */
#else

#if MP_MEMSET == 0
#define s_mp_setz(dp, count) {mp_size ix;for (ix=0;ix<(count);ix++)(dp)[ix]=0;}
#else
#define s_mp_setz(dp, count) memset(dp, 0, (count) * sizeof (mp_digit))
#endif

#if MP_MEMCPY == 0
#define s_mp_copy(sp, dp, count) {mp_size ix;for (ix=0;ix<(count);ix++)(dp)[ix]=(sp)[ix];}
#else
#define s_mp_copy(sp, dp, count) memcpy(dp, sp, (count) * sizeof (mp_digit))
#endif

#define s_mp_alloc(nb, ni) chk_calloc(nb, ni)
#define s_mp_free(ptr) {if (ptr) free(ptr);}
#endif

mp_err s_mp_grow(mp_int *mp, mp_size min); /* increase allocated size */
mp_err s_mp_pad(mp_int *mp, mp_size min); /* left pad with zeroes */

static mp_size s_highest_bit(mp_digit n);
mp_size s_highest_bit_mp(mp_int *a);
mp_err s_mp_set_bit(mp_int *a, mp_size bit);

void s_mp_clamp(mp_int *mp); /* clip leading zeroes */

void s_mp_exch(mp_int *a, mp_int *b); /* swap a and b in place */

mp_err s_mp_lshd(mp_int *mp, mp_size p); /* left-shift by p digits */
void s_mp_rshd(mp_int *mp, mp_size p); /* right-shift by p digits */
void s_mp_div_2d(mp_int *mp, mp_digit d); /* divide by 2^d in place */
void s_mp_mod_2d(mp_int *mp, mp_digit d); /* modulo 2^d in place */
mp_err s_mp_mul_2d(mp_int *mp, mp_digit d); /* multiply by 2^d in place */
void s_mp_div_2(mp_int *mp); /* divide by 2 in place */
mp_err s_mp_mul_2(mp_int *mp); /* multiply by 2 in place */
mp_digit s_mp_norm(mp_int *a, mp_int *b); /* normalize for division */
mp_err s_mp_add_d(mp_int *mp, mp_digit d); /* unsigned digit addition */
mp_err s_mp_sub_d(mp_int *mp, mp_digit d); /* unsigned digit subtract */
mp_err s_mp_mul_d(mp_int *mp, mp_digit d); /* unsigned digit multiply */
mp_err s_mp_div_d(mp_int *mp, mp_digit d, mp_digit *r); /* unsigned digit divide */
mp_err s_mp_reduce(mp_int *x, mp_int *m, mp_int *mu); /* Barrett reduction */
mp_err s_mp_add(mp_int *a, mp_int *b); /* magnitude addition */
mp_err s_mp_sub(mp_int *a, mp_int *b); /* magnitude subtract */
mp_err s_mp_mul(mp_int *a, mp_int *b); /* magnitude multiply */

#if MP_SQUARE
mp_err s_mp_sqr(mp_int *a); /* magnitude square */
#else
#define s_mp_sqr(a) s_mp_mul(a, a)
#endif

mp_err s_mp_div(mp_int *a, mp_int *b); /* magnitude divide */
mp_err s_mp_2expt(mp_int *a, mp_size k); /* a = 2^k */
int s_mp_cmp(mp_int *a, mp_int *b); /* magnitude comparison */
int s_mp_cmp_d(mp_int *a, mp_digit d); /* magnitude digit compare */
mp_size s_mp_ispow2(mp_int *v); /* is v a power of 2? */
int s_mp_ispow2d(mp_digit d); /* is d a power of 2? */

int s_mp_tovalue(wchar_t ch, int r); /* convert ch to value */
char s_mp_todigit(int val, int r, int low); /* convert val to digit */
size_t s_mp_outlen(mp_size bits, int r); /* output length in bytes */

unsigned int mp_get_prec(void)
{
  return s_mp_defprec;
}

void mp_set_prec(unsigned int prec)
{
  if (prec == 0)
    s_mp_defprec = MP_DEFPREC;
  else
    s_mp_defprec = prec;
}

/* Initialize a new zero-valued mp_int.  Returns MP_OKAY if successful,
 * MP_MEM if memory could not be allocated for the structure.
 */
mp_err mp_init(mp_int *mp)
{
  return mp_init_size(mp, s_mp_defprec);
}

mp_err mp_init_array(mp_int mp[], int count)
{
  mp_err res;
  int pos;

  ARGCHK(mp !=NULL && count > 0, MP_BADARG);

  for (pos = 0; pos < count; ++pos) {
    if ((res = mp_init(&mp[pos])) != MP_OKAY)
      goto CLEANUP;
  }

  return MP_OKAY;

 CLEANUP:
  while (--pos >= 0)
    mp_clear(&mp[pos]);

  return res;
}

/* Initialize a new zero-valued mp_int with at least the given
 * precision; returns MP_OKAY if successful, or MP_MEM if memory could
 * not be allocated for the structure.
 */
mp_err mp_init_size(mp_int *mp, mp_size prec)
{
  ARGCHK(mp != NULL, MP_BADARG);

  if (prec > MP_MAX_DIGITS)
    return MP_TOOBIG;

  if ((DIGITS(mp) = coerce(mp_digit *,
                           s_mp_alloc(prec, sizeof (mp_digit)))) == NULL)
    return MP_MEM;

  SIGN(mp) = MP_ZPOS;
  USED(mp) = 1;
  ALLOC(mp) = prec;

  return MP_OKAY;
}

/* Initialize mp as an exact copy of from.  Returns MP_OKAY if
 * successful, MP_MEM if memory could not be allocated for the new
 * structure.
 */
mp_err mp_init_copy(mp_int *mp, mp_int *from)
{
  ARGCHK(mp != NULL && from != NULL, MP_BADARG);

  if (mp == from)
    return MP_OKAY;

  if ((DIGITS(mp) = coerce(mp_digit *,
                           s_mp_alloc(USED(from), sizeof (mp_digit)))) == NULL)
    return MP_MEM;

  s_mp_copy(DIGITS(from), DIGITS(mp), USED(from));
  USED(mp) = USED(from);
  ALLOC(mp) = USED(from);
  SIGN(mp) = SIGN(from);

  return MP_OKAY;
}

/* Copies the mp_int 'from' to the mp_int 'to'.  It is presumed that
 * 'to' has already been initialized (if not, use mp_init_copy()
 * instead). If 'from' and 'to' are identical, nothing happens.
 */
mp_err mp_copy(mp_int *from, mp_int *to)
{
  ARGCHK(from != NULL && to != NULL, MP_BADARG);

  if (from == to)
    return MP_OKAY;

  {
    mp_digit *tmp;

    /* If the allocated buffer in 'to' already has enough space to hold
     * all the used digits of 'from', we'll re-use it to avoid hitting
     * the memory allocater more than necessary; otherwise, we'd have
     * to grow anyway, so we just allocate a hunk and make the copy as
     * usual
     */
    if (ALLOC(to) >= USED(from)) {
      s_mp_setz(DIGITS(to) + USED(from), ALLOC(to) - USED(from));
      s_mp_copy(DIGITS(from), DIGITS(to), USED(from));
    } else {
      if ((tmp = coerce(mp_digit *,
                        s_mp_alloc(USED(from), sizeof (mp_digit)))) == NULL)
        return MP_MEM;

      s_mp_copy(DIGITS(from), tmp, USED(from));

      if (DIGITS(to) != NULL) {
#if MP_CRYPTO
        s_mp_setz(DIGITS(to), ALLOC(to));
#endif
        s_mp_free(DIGITS(to));
      }

      DIGITS(to) = tmp;
      ALLOC(to) = USED(from);
    }

    /* Copy the precision and sign from the original */
    USED(to) = USED(from);
    SIGN(to) = SIGN(from);
  }

  return MP_OKAY;
}

/* Exchange mp1 and mp2 without allocating any intermediate memory
 * (well, unless you count the stack space needed for this call and the
 * locals it creates...).  This cannot fail.
 */
void mp_exch(mp_int *mp1, mp_int *mp2)
{
#if MP_ARGCHK == 2
  assert(mp1 != NULL && mp2 != NULL);
#else
  if (mp1 == NULL || mp2 == NULL)
    return;
#endif

  s_mp_exch(mp1, mp2);
}

/* Release the storage used by an mp_int, and void its fields so that
 * if someone calls mp_clear() again for the same int later, we won't
 * get tollchocked.
 */
void mp_clear(mp_int *mp)
{
  if (mp == NULL)
    return;

  if (DIGITS(mp) != NULL) {
#if MP_CRYPTO
    s_mp_setz(DIGITS(mp), ALLOC(mp));
#endif
    s_mp_free(DIGITS(mp));
    DIGITS(mp) = NULL;
  }

  USED(mp) = 0;
  ALLOC(mp) = 0;
}

void mp_clear_array(mp_int mp[], int count)
{
  ARGCHK(mp != NULL && count > 0, MP_BADARG);

  while (--count >= 0)
    mp_clear(&mp[count]);
}

/* Set mp to zero.  Does not change the allocated size of the structure,
 * and therefore cannot fail (except on a bad argument, which we ignore)
 */
void mp_zero(mp_int *mp)
{
  if (mp == NULL)
    return;

  s_mp_setz(DIGITS(mp), ALLOC(mp));
  USED(mp) = 1;
  SIGN(mp) = MP_ZPOS;
}

void mp_set(mp_int *mp, mp_digit d)
{
  if (mp == NULL)
    return;

  mp_zero(mp);
  DIGIT(mp, 0) = d;
}

mp_err mp_set_int(mp_int *mp, long z)
{
  mp_size ix;
  unsigned long w = z;
  unsigned long v = z >= 0 ? w : -w;
  mp_err res;

  ARGCHK(mp != NULL, MP_BADARG);

  mp_zero(mp);
  if (z == 0)
    return MP_OKAY; /* shortcut for zero */

  for (ix = sizeof (long) - 1; ix < MP_SIZE_MAX; ix--) {

    if ((res = s_mp_mul_2d(mp, CHAR_BIT)) != MP_OKAY)
      return res;

    res = s_mp_add_d(mp,
                     convert(mp_digit, ((v >> (ix * CHAR_BIT)) & UCHAR_MAX)));
    if (res != MP_OKAY)
      return res;

  }

  if (z < 0)
    SIGN(mp) = MP_NEG;

  return MP_OKAY;
}

mp_err mp_set_uintptr(mp_int *mp, uint_ptr_t z)
{
  if (sizeof z > sizeof (mp_digit)) {
    mp_size ix, shift;
    const mp_size nd = (sizeof z + sizeof (mp_digit) - 1) / sizeof (mp_digit);

    ARGCHK(mp != NULL, MP_BADARG);

    mp_zero(mp);

    if (z == 0)
      return MP_OKAY; /* shortcut for zero */

    s_mp_grow(mp, nd);

    USED(mp) = nd;

    for (ix = 0, shift = 0; ix < nd; ix++, shift += MP_DIGIT_BIT)
    {
      DIGIT(mp, ix) = (z >> shift) & MP_DIGIT_MAX;
    }

    s_mp_clamp(mp);
  } else {
    mp_set(mp, z);
  }
  return MP_OKAY;
}

mp_err mp_set_intptr(mp_int *mp, int_ptr_t z)
{
  uint_ptr_t w = z;
  uint_ptr_t v = z >= 0 ? w : -w;
  mp_err err = mp_set_uintptr(mp, v);

  if (err == MP_OKAY && z < 0)
    SIGN(mp) = MP_NEG;

  return err;
}

/* No checks here: assumes that the mp is in range!
 */
mp_err mp_get_uintptr(mp_int *mp, uint_ptr_t *z)
{
  uint_ptr_t out = 0;

#if MP_DIGIT_SIZE < SIZEOF_PTR
  mp_size ix;
  for (ix = USED(mp) - 1; ix < MP_SIZE_MAX; ix--) {
    out <<= MP_DIGIT_BIT;
    out |= DIGIT(mp, ix);
  }
#else
  out = DIGIT(mp, 0);
#endif

  *z = (SIGN(mp) == MP_NEG) ? -out : out;
  return MP_OKAY;
}

mp_err mp_get_intptr(mp_int *mp, int_ptr_t *z)
{
  uint_ptr_t tmp = 0;
  mp_get_uintptr(mp, &tmp);
  /* Reliance on bitwise unsigned to two's complement conversion */
  *z = convert(int_ptr_t, tmp);
  return MP_OKAY;
}

int mp_in_range(mp_int *mp, uint_ptr_t lim, int unsig)
{
  const unsigned ptrnd = (SIZEOF_PTR + MP_DIGIT_SIZE - 1) / MP_DIGIT_SIZE;
  mp_size nd = USED(mp);
  int neg = ISNEG(mp);

  if (unsig && neg)
    return 0;

  if (nd < ptrnd)
    return 1;

  if (nd > ptrnd)
    return 0;

  {
    mp_digit top = DIGITS(mp)[ptrnd - 1];
    lim >>= ((ptrnd - 1) * MP_DIGIT_BIT);
    return (top - neg) <= lim;
  }
}

int mp_in_intptr_range(mp_int *mp)
{
  return mp_in_range(mp, INT_PTR_MAX, 0);
}

int mp_in_uintptr_range(mp_int *mp)
{
  return mp_in_range(mp, UINT_PTR_MAX, 1);
}

#if HAVE_DOUBLE_INTPTR_T

mp_err mp_set_double_intptr(mp_int *mp, double_intptr_t z)
{
  mp_size ix, shift;
  double_uintptr_t w = z;
  double_uintptr_t v = z >= 0 ? w : -w;
  const mp_size nd = (sizeof v + sizeof (mp_digit) - 1) / sizeof (mp_digit);

  ARGCHK(mp != NULL, MP_BADARG);

  mp_zero(mp);

  if (z == 0)
    return MP_OKAY; /* shortcut for zero */

  s_mp_grow(mp, nd);

  USED(mp) = nd;

  for (ix = 0, shift = 0; ix < nd; ix++, shift += MP_DIGIT_BIT)
  {
    DIGIT(mp, ix) = (v >> shift) & MP_DIGIT_MAX;
  }

  s_mp_clamp(mp);

  if (z < 0)
    SIGN(mp) = MP_NEG;

  return MP_OKAY;
}

mp_err mp_set_double_uintptr(mp_int *mp, double_uintptr_t v)
{
  mp_size ix, shift;
  const mp_size nd = (sizeof v + sizeof (mp_digit) - 1) / sizeof (mp_digit);

  ARGCHK(mp != NULL, MP_BADARG);

  mp_zero(mp);

  if (v == 0)
    return MP_OKAY; /* shortcut for zero */

  s_mp_grow(mp, nd);

  USED(mp) = nd;

  for (ix = 0, shift = 0; ix < nd; ix++, shift += MP_DIGIT_BIT)
  {
    DIGIT(mp, ix) = (v >> shift) & MP_DIGIT_MAX;
  }

  s_mp_clamp(mp);

  return MP_OKAY;
}

mp_err mp_get_double_uintptr(mp_int *mp, double_uintptr_t *z)
{
  double_uintptr_t out = 0;
  mp_size ix;
  for (ix = USED(mp) - 1; ix < MP_SIZE_MAX; ix--) {
    out <<= MP_DIGIT_BIT;
    out |= DIGIT(mp, ix);
  }

  *z = (SIGN(mp) == MP_NEG) ? -out : out;
  return MP_OKAY;
}

mp_err mp_get_double_intptr(mp_int *mp, double_intptr_t *z)
{
  double_uintptr_t tmp = 0;
  mp_get_double_uintptr(mp, &tmp);
  /* Reliance on bitwise unsigned to two's complement conversion */
  *z = convert(double_intptr_t, tmp);
  return MP_OKAY;
}

static int s_mp_in_big_range(mp_int *mp, double_uintptr_t lim, int unsig)
{
  const unsigned ptrnd = (SIZEOF_DOUBLE_INTPTR + MP_DIGIT_SIZE - 1) / MP_DIGIT_SIZE;
  mp_size nd = USED(mp);
  int neg = ISNEG(mp);

  if (unsig && neg)
    return 0;

  if (nd < ptrnd)
    return 1;

  if (nd > ptrnd)
    return 0;

  if (neg) {
    mp_digit *dp = DIGITS(mp);
    const mp_digit Ox8__0 = MP_DIGIT_MAX ^ (MP_DIGIT_MAX >> 1);

    switch (ptrnd) {
    case 1:
      if (dp[0] == Ox8__0)
        return 1;
      break;
    case 2:
      if (dp[0] == 0 && dp[1] == Ox8__0)
        return 1;
      break;
    case 4:
      if (dp[0] == 0 && dp[1] == 0 && dp[2] == 0 && dp[3] == Ox8__0)
        return 1;
      break;
    }
  }

  {
    mp_digit top = DIGITS(mp)[ptrnd - 1];
    lim >>= ((ptrnd - 1) * MP_DIGIT_BIT);
    return top <= lim;
  }
}

int mp_in_double_intptr_range(mp_int *mp)
{
  return s_mp_in_big_range(mp, DOUBLE_INTPTR_MAX, 0);
}

int mp_in_double_uintptr_range(mp_int *mp)
{
  return s_mp_in_big_range(mp, DOUBLE_UINTPTR_MAX, 1);
}

#endif

mp_err mp_set_word(mp_int *mp, mp_word w, int sign)
{
  USED(mp) = 2;
  DIGIT(mp, 0) = w & MP_DIGIT_MAX;
  DIGIT(mp, 1) = w >> MP_DIGIT_BIT;
  SIGN(mp) = sign;
  return MP_OKAY;
}

/* Compute the sum b = a + d, for a single digit d.  Respects the sign of
 * its primary addend (single digits are unsigned anyway).
 */
mp_err mp_add_d(mp_int *a, mp_digit d, mp_int *b)
{
  mp_err res = MP_OKAY;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if ((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  if (SIGN(b) == MP_ZPOS) {
    res = s_mp_add_d(b, d);
  } else if (s_mp_cmp_d(b, d) >= 0) {
    res = s_mp_sub_d(b, d);
  } else {
    SIGN(b) = MP_ZPOS;

    DIGIT(b, 0) = d - DIGIT(b, 0);
  }

  return res;
}

/* Compute the difference b = a - d, for a single digit d.  Respects the
 * sign of its subtrahend (single digits are unsigned anyway).
 */
mp_err mp_sub_d(mp_int *a, mp_digit d, mp_int *b)
{
  mp_err res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if ((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  if (SIGN(b) == MP_NEG) {
    if ((res = s_mp_add_d(b, d)) != MP_OKAY)
      return res;

  } else if (s_mp_cmp_d(b, d) >= 0) {
    if ((res = s_mp_sub_d(b, d)) != MP_OKAY)
      return res;

  } else {
    mp_neg(b, b);

    DIGIT(b, 0) = d - DIGIT(b, 0);
    SIGN(b) = MP_NEG;
  }

  if (s_mp_cmp_d(b, 0) == 0)
    SIGN(b) = MP_ZPOS;

  return MP_OKAY;
}

/* Compute the product b = a * d, for a single digit d.  Respects the sign
 * of its multiplicand (single digits are unsigned anyway)
 */
mp_err mp_mul_d(mp_int *a, mp_digit d, mp_int *b)
{
  mp_err res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if (d == 0) {
    mp_zero(b);
    return MP_OKAY;
  }

  if ((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  res = s_mp_mul_d(b, d);

  return res;
}

mp_err mp_mul_2(mp_int *a, mp_int *c)
{
  mp_err res;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if ((res = mp_copy(a, c)) != MP_OKAY)
    return res;

  return s_mp_mul_2(c);
}

/* Compute the quotient q = a / d and remainder r = a mod d, for a
 * single digit d.  Respects the sign of its divisor (single digits are
 * unsigned anyway).
 */
mp_err mp_div_d(mp_int *a, mp_digit d, mp_int *q, mp_digit *r)
{
  mp_err res;
  mp_digit rem;
  int pow;

  ARGCHK(a != NULL, MP_BADARG);

  if (d == 0)
    return MP_RANGE;

  /* Shortcut for powers of two ... */
  if ((pow = s_mp_ispow2d(d)) >= 0) {
    mp_digit mask;

    mask = (convert(mp_digit, 1) << pow) - 1;
    rem = DIGIT(a, 0) & mask;

    if (q) {
      mp_copy(a, q);
      s_mp_div_2d(q, pow);
    }

    if (r)
      *r = rem;

    return MP_OKAY;
  }

  /* If the quotient is actually going to be returned, we'll try to
   * avoid hitting the memory allocator by copying the dividend into it
   * and doing the division there.  This can't be any _worse_ than
   * always copying, and will sometimes be better (since it won't make
   * another copy)
   * If it's not going to be returned, we need to allocate a temporary
   * to hold the quotient, which will just be discarded.
   */
  if (q) {
    if ((res = mp_copy(a, q)) != MP_OKAY)
      return res;

    res = s_mp_div_d(q, d, &rem);
    if (s_mp_cmp_d(q, 0) == MP_EQ)
      SIGN(q) = MP_ZPOS;

  } else {
    mp_int qp;

    if ((res = mp_init_copy(&qp, a)) != MP_OKAY)
      return res;

    res = s_mp_div_d(&qp, d, &rem);
    if (s_mp_cmp_d(&qp, 0) == 0)
      SIGN(&qp) = MP_ZPOS;

    mp_clear(&qp);
  }

  if (r)
    *r = rem;

  return res;
}

/* Compute c = a / 2, disregarding the remainder.  */
mp_err mp_div_2(mp_int *a, mp_int *c)
{
  mp_err res;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if ((res = mp_copy(a, c)) != MP_OKAY)
    return res;

  s_mp_div_2(c);

  return MP_OKAY;
}

mp_err mp_expt_d(mp_int *a, mp_digit d, mp_int *c)
{
  mp_int s, x;
  mp_err res;
  mp_sign cs = MP_ZPOS;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if ((res = mp_init(&s)) != MP_OKAY)
    return res;
  if ((res = mp_init_copy(&x, a)) != MP_OKAY)
    goto X;

  DIGIT(&s, 0) = 1;

  if ((d % 2) == 1)
    cs = SIGN(a);

  while (d != 0) {
    if (d & 1) {
      if ((res = s_mp_mul(&s, &x)) != MP_OKAY)
        goto CLEANUP;
    }

    d >>= 1;

    if ((res = s_mp_sqr(&x)) != MP_OKAY)
      goto CLEANUP;
  }

  SIGN(&s) = cs;

  s_mp_exch(&s, c);

CLEANUP:
  mp_clear(&x);
X:
  mp_clear(&s);

  return res;
}

/* Compute b = |a|.  'a' and 'b' may be identical.  */
mp_err mp_abs(mp_int *a, mp_int *b)
{
  mp_err res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if ((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  SIGN(b) = MP_ZPOS;

  return MP_OKAY;
}

/* Compute b = -a.  'a' and 'b' may be identical.  */
mp_err mp_neg(mp_int *a, mp_int *b)
{
  mp_err res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if ((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  if (s_mp_cmp_d(b, 0) == MP_EQ)
    SIGN(b) = MP_ZPOS;
  else
    SIGN(b) = (SIGN(b) == MP_NEG) ? MP_ZPOS : MP_NEG;

  return MP_OKAY;
}

/* Compute c = a + b.  All parameters may be identical.  */
mp_err mp_add(mp_int *a, mp_int *b, mp_int *c)
{
  mp_err res;
  int cmp;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (SIGN(a) == SIGN(b)) { /* same sign:  add values, keep sign */

    /* Commutativity of addition lets us do this in either order,
     * so we avoid having to use a temporary even if the result
     * is supposed to replace the output
     */
    if (c == b) {
      if ((res = s_mp_add(c, a)) != MP_OKAY)
        return res;
    } else {
      if (c != a && (res = mp_copy(a, c)) != MP_OKAY)
        return res;

      if ((res = s_mp_add(c, b)) != MP_OKAY)
        return res;
    }

  } else if ((cmp = s_mp_cmp(a, b)) > 0) {  /* different sign: a > b */

    /* If the output is going to be clobbered, we will use a temporary
     * variable; otherwise, we'll do it without touching the memory
     * allocator at all, if possible
     */
    if (c == b) {
      mp_int tmp;

      if ((res = mp_init_copy(&tmp, a)) != MP_OKAY)
        return res;
      if ((res = s_mp_sub(&tmp, b)) != MP_OKAY) {
        mp_clear(&tmp);
        return res;
      }

      s_mp_exch(&tmp, c);
      mp_clear(&tmp);

    } else {

      if (c != a && (res = mp_copy(a, c)) != MP_OKAY)
        return res;
      if ((res = s_mp_sub(c, b)) != MP_OKAY)
        return res;

    }

  } else if (cmp == 0) {             /* different sign, a == b */

    mp_zero(c);
    return MP_OKAY;

  } else {                          /* different sign: a < b */

    /* See above... */
    if (c == a) {
      mp_int tmp;

      if ((res = mp_init_copy(&tmp, b)) != MP_OKAY)
        return res;
      if ((res = s_mp_sub(&tmp, a)) != MP_OKAY) {
        mp_clear(&tmp);
        return res;
      }

      s_mp_exch(&tmp, c);
      mp_clear(&tmp);

    } else {

      if (c != b && (res = mp_copy(b, c)) != MP_OKAY)
        return res;
      if ((res = s_mp_sub(c, a)) != MP_OKAY)
        return res;

    }
  }

  if (USED(c) == 1 && DIGIT(c, 0) == 0)
    SIGN(c) = MP_ZPOS;

  return MP_OKAY;
}

/* Compute c = a - b.  All parameters may be identical.  */
mp_err mp_sub(mp_int *a, mp_int *b, mp_int *c)
{
  mp_err res;
  int cmp;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (SIGN(a) != SIGN(b)) {
    if (c == a) {
      if ((res = s_mp_add(c, b)) != MP_OKAY)
        return res;
    } else {
      if (c != b && ((res = mp_copy(b, c)) != MP_OKAY))
        return res;
      if ((res = s_mp_add(c, a)) != MP_OKAY)
        return res;
      SIGN(c) = SIGN(a);
    }

  } else if ((cmp = s_mp_cmp(a, b)) > 0) { /* Same sign, a > b */
    if (c == b) {
      mp_int tmp;

      if ((res = mp_init_copy(&tmp, a)) != MP_OKAY)
        return res;
      if ((res = s_mp_sub(&tmp, b)) != MP_OKAY) {
        mp_clear(&tmp);
      return res;
      }
      s_mp_exch(&tmp, c);
      mp_clear(&tmp);

    } else {
      if (c != a && ((res = mp_copy(a, c)) != MP_OKAY))
        return res;

      if ((res = s_mp_sub(c, b)) != MP_OKAY)
        return res;
    }

  } else if (cmp == 0) {  /* Same sign, equal magnitude */
    mp_zero(c);
    return MP_OKAY;

  } else {               /* Same sign, b > a */
    if (c == a) {
      mp_int tmp;

      if ((res = mp_init_copy(&tmp, b)) != MP_OKAY)
        return res;

      if ((res = s_mp_sub(&tmp, a)) != MP_OKAY) {
        mp_clear(&tmp);
        return res;
      }
      s_mp_exch(&tmp, c);
      mp_clear(&tmp);

    } else {
      if (c != b && ((res = mp_copy(b, c)) != MP_OKAY))
        return res;

      if ((res = s_mp_sub(c, a)) != MP_OKAY)
        return res;
    }

    SIGN(c) = !SIGN(b);
  }

  if (USED(c) == 1 && DIGIT(c, 0) == 0)
    SIGN(c) = MP_ZPOS;

  return MP_OKAY;
}

/* Compute c = a * b.  All parameters may be identical.  */
mp_err mp_mul(mp_int *a, mp_int *b, mp_int *c)
{
  mp_err res;
  mp_sign sgn;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  sgn = (SIGN(a) == SIGN(b)) ? MP_ZPOS : MP_NEG;

  if (c == b) {
    if ((res = s_mp_mul(c, a)) != MP_OKAY)
      return res;

  } else {
    if ((res = mp_copy(a, c)) != MP_OKAY)
      return res;

    if ((res = s_mp_mul(c, b)) != MP_OKAY)
      return res;
  }

  if (sgn == MP_ZPOS || s_mp_cmp_d(c, 0) == MP_EQ)
    SIGN(c) = MP_ZPOS;
  else
    SIGN(c) = sgn;

  return MP_OKAY;
}

/* Compute c = a * 2^d.  a may be the same as c.  */
mp_err mp_mul_2d(mp_int *a, mp_digit d, mp_int *c)
{
  mp_err res;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if ((res = mp_copy(a, c)) != MP_OKAY)
    return res;

  if (d == 0)
    return MP_OKAY;

  return s_mp_mul_2d(c, d);
}

#if MP_SQUARE
mp_err mp_sqr(mp_int *a, mp_int *b)
{
  mp_err res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if ((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  if ((res = s_mp_sqr(b)) != MP_OKAY)
    return res;

  SIGN(b) = MP_ZPOS;

  return MP_OKAY;
}
#endif

/* Compute q = a / b and r = a mod b.  Input parameters may be re-used
 * as output parameters.  If q or r is NULL, that portion of the
 * computation will be discarded (although it will still be computed)
 * Pay no attention to the hacker behind the curtain.
 */
mp_err mp_div(mp_int *a, mp_int *b, mp_int *q, mp_int *r)
{
  mp_err res;
  mp_int qtmp, rtmp;
  int cmp;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if (mp_cmp_z(b) == MP_EQ)
    return MP_RANGE;

  /* If a <= b, we can compute the solution without division, and
   * avoid any memory allocation
   */
  if ((cmp = s_mp_cmp(a, b)) < 0) {
    if (r) {
      if ((res = mp_copy(a, r)) != MP_OKAY)
        return res;
    }

    if (q)
      mp_zero(q);

    return MP_OKAY;

  } else if (cmp == 0) {

    /* Set quotient to 1, with appropriate sign */
    if (q) {
      int qneg = (SIGN(a) != SIGN(b));

      mp_set(q, 1);
      if (qneg)
        SIGN(q) = MP_NEG;
    }

    if (r)
      mp_zero(r);

    return MP_OKAY;
  }

  /* If we get here, it means we actually have to do some division */

  /* Set up some temporaries... */
  if ((res = mp_init_copy(&qtmp, a)) != MP_OKAY)
    return res;
  if ((res = mp_init_copy(&rtmp, b)) != MP_OKAY)
    goto CLEANUP;

  if ((res = s_mp_div(&qtmp, &rtmp)) != MP_OKAY)
    goto CLEANUP;

  /* Compute the signs for the output */
  SIGN(&rtmp) = SIGN(a); /* Sr = Sa */
  if (SIGN(a) == SIGN(b))
    SIGN(&qtmp) = MP_ZPOS; /* Sq = MP_ZPOS if Sa = Sb */
  else
    SIGN(&qtmp) = MP_NEG; /* Sq = MP_NEG if Sa != Sb */

  if (s_mp_cmp_d(&qtmp, 0) == MP_EQ)
    SIGN(&qtmp) = MP_ZPOS;
  if (s_mp_cmp_d(&rtmp, 0) == MP_EQ)
    SIGN(&rtmp) = MP_ZPOS;

  /* Copy output, if it is needed */
  if (q)
    s_mp_exch(&qtmp, q);

  if (r)
    s_mp_exch(&rtmp, r);

CLEANUP:
  mp_clear(&rtmp);
  mp_clear(&qtmp);

  return res;
}

mp_err mp_div_2d(mp_int *a, mp_digit d, mp_int *q, mp_int *r)
{
  mp_err res;

  ARGCHK(a != NULL, MP_BADARG);

  if (q) {
    if ((res = mp_copy(a, q)) != MP_OKAY)
      return res;

    s_mp_div_2d(q, d);
  }

  if (r) {
    if ((res = mp_copy(a, r)) != MP_OKAY)
      return res;

    s_mp_mod_2d(r, d);
  }

  return MP_OKAY;
}

/* Compute c = a ** b, that is, raise a to the b power.  Uses a
 * standard iterative square-and-multiply technique.
 */
mp_err mp_expt(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int s, x;
  mp_err res;
  mp_digit d;
  mp_size dig, bit;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (mp_cmp_z(b) < 0)
    return MP_RANGE;

  if ((res = mp_init(&s)) != MP_OKAY)
    return res;

  mp_set(&s, 1);

  if ((res = mp_init_copy(&x, a)) != MP_OKAY)
    goto X;

  /* Loop over low-order digits in ascending order */
  for (dig = 0; dig < (USED(b) - 1); dig++) {
    d = DIGIT(b, dig);

    /* Loop over bits of each non-maximal digit */
    for (bit = 0; bit < DIGIT_BIT; bit++) {
      if (d & 1) {
        if ((res = s_mp_mul(&s, &x)) != MP_OKAY)
          goto CLEANUP;
      }

      d >>= 1;

      if ((res = s_mp_sqr(&x)) != MP_OKAY)
        goto CLEANUP;
    }
  }

  /* Consider now the last digit... */
  d = DIGIT(b, dig);

  while (d) {
    if (d & 1) {
      if ((res = s_mp_mul(&s, &x)) != MP_OKAY)
        goto CLEANUP;
    }

    d >>= 1;

    if ((res = s_mp_sqr(&x)) != MP_OKAY)
      goto CLEANUP;
  }

  if (mp_iseven(b))
    SIGN(&s) = SIGN(a);

  res = mp_copy(&s, c);

CLEANUP:
  mp_clear(&x);
X:
  mp_clear(&s);

  return res;
}

/* Compute a = 2^k */

mp_err mp_2expt(mp_int *a, mp_digit k)
{
  ARGCHK(a != NULL, MP_BADARG);

  return s_mp_2expt(a, k);
}

/* Compute c = a (mod m).  Result will always be 0 <= c < m.  */
mp_err mp_mod(mp_int *a, mp_int *m, mp_int *c)
{
  mp_err res;
  int mag;

  ARGCHK(a != NULL && m != NULL && c != NULL, MP_BADARG);

  if (SIGN(m) == MP_NEG)
    return MP_RANGE;

  /* If |a| > m, we need to divide to get the remainder and take the
   * absolute value.
   * If |a| < m, we don't need to do any division, just copy and adjust
   * the sign (if a is negative).
   * If |a| == m, we can simply set the result to zero.
   * This order is intended to minimize the average path length of the
   * comparison chain on common workloads -- the most frequent cases are
   * that |a| != m, so we do those first.
   */
  if ((mag = s_mp_cmp(a, m)) > 0) {
    if ((res = mp_div(a, m, NULL, c)) != MP_OKAY)
      return res;

    if (SIGN(c) == MP_NEG) {
      if ((res = mp_add(c, m, c)) != MP_OKAY)
        return res;
    }
  } else if (mag < 0) {
    if ((res = mp_copy(a, c)) != MP_OKAY)
      return res;

    if (mp_cmp_z(a) < 0) {
      if ((res = mp_add(c, m, c)) != MP_OKAY)
        return res;

    }
  } else {
    mp_zero(c);
  }

  return MP_OKAY;
}

/* Compute c = a (mod d).  Result will always be 0 <= c < d */
mp_err mp_mod_d(mp_int *a, mp_digit d, mp_digit *c)
{
  mp_err res;
  mp_digit rem;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if (s_mp_cmp_d(a, d) > 0) {
    if ((res = mp_div_d(a, d, NULL, &rem)) != MP_OKAY)
      return res;

  } else {
    if (SIGN(a) == MP_NEG)
      rem = d - DIGIT(a, 0);
    else
      rem = DIGIT(a, 0);
  }

  if (c)
    *c = rem;

  return MP_OKAY;
}

mp_err mp_sqrt(mp_int *a, mp_int *b)
{
  mp_size mask_shift;
  mp_int root, guess, *proot = &root, *pguess = &guess;
  mp_int guess_sqr;
  mp_err err = MP_MEM;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if (mp_cmp_z(b) == MP_LT)
    return MP_RANGE;

  if ((err = mp_init(&root)))
    goto out;
  if ((err = mp_init(&guess)))
    goto cleanup_root;
  if ((err = mp_init(&guess_sqr)))
    goto cleanup_guess;

  for (mask_shift = s_highest_bit_mp(a) / 2;
       mask_shift < MP_SIZE_MAX; mask_shift--)
  {
    mp_int *temp;
    int cmp;

    if ((err = mp_copy(proot, pguess)))
      goto cleanup;
    if ((err = s_mp_set_bit(pguess, mask_shift)))
      goto cleanup;
    if ((err = mp_copy(pguess, &guess_sqr)))
      goto cleanup;
    if ((err = s_mp_sqr(&guess_sqr)))
      goto cleanup;

    cmp = s_mp_cmp(&guess_sqr, a);

    if (cmp < 0) {
      temp = proot;
      proot = pguess;
      pguess = temp;
    } else if (cmp == 0) {
      proot = pguess;
      break;
    }
  }

  err = mp_copy(proot, b);

cleanup:
  mp_clear(&guess_sqr);
cleanup_guess:
  mp_clear(&guess);
cleanup_root:
  mp_clear(&root);
out:
  return err;
}

#if MP_MODARITH

/* Compute c = (a + b) mod m */
mp_err mp_addmod(mp_int *a, mp_int *b, mp_int *m, mp_int *c)
{
  mp_err res;

  ARGCHK(a != NULL && b != NULL && m != NULL && c != NULL, MP_BADARG);

  if ((res = mp_add(a, b, c)) != MP_OKAY)
    return res;
  if ((res = mp_mod(c, m, c)) != MP_OKAY)
    return res;

  return MP_OKAY;
}

/* Compute c = (a - b) mod m */
mp_err mp_submod(mp_int *a, mp_int *b, mp_int *m, mp_int *c)
{
  mp_err res;

  ARGCHK(a != NULL && b != NULL && m != NULL && c != NULL, MP_BADARG);

  if ((res = mp_sub(a, b, c)) != MP_OKAY)
    return res;
  if ((res = mp_mod(c, m, c)) != MP_OKAY)
    return res;

  return MP_OKAY;
}

/* Compute c = (a * b) mod m */
mp_err mp_mulmod(mp_int *a, mp_int *b, mp_int *m, mp_int *c)
{
  mp_err res;

  ARGCHK(a != NULL && b != NULL && m != NULL && c != NULL, MP_BADARG);

  if ((res = mp_mul(a, b, c)) != MP_OKAY)
    return res;
  if ((res = mp_mod(c, m, c)) != MP_OKAY)
    return res;

  return MP_OKAY;
}

#if MP_SQUARE
mp_err mp_sqrmod(mp_int *a, mp_int *m, mp_int *c)
{
  mp_err res;

  ARGCHK(a != NULL && m != NULL && c != NULL, MP_BADARG);

  if ((res = mp_sqr(a, c)) != MP_OKAY)
    return res;
  if ((res = mp_mod(c, m, c)) != MP_OKAY)
    return res;

  return MP_OKAY;
}
#endif

/* Compute c = (a ** b) mod m.  Uses a standard square-and-multiply
 * method with modular reductions at each step. (This is basically the
 * same code as mp_expt(), except for the addition of the reductions)
 * The modular reductions are done using Barrett's algorithm (see
 * s_mp_reduce() below for details)
 */
mp_err mp_exptmod(mp_int *a, mp_int *b, mp_int *m, mp_int *c)
{
  mp_int s, x, mu;
  mp_err res;
  mp_digit d, *db = DIGITS(b);
  mp_size ub = USED(b);
  mp_size dig, bit;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (mp_cmp_z(b) < 0 || mp_cmp_z(m) <= 0)
    return MP_RANGE;

  if ((res = mp_init(&s)) != MP_OKAY)
    return res;
  if ((res = mp_init_copy(&x, a)) != MP_OKAY)
    goto X;
  if ((res = mp_mod(&x, m, &x)) != MP_OKAY ||
     (res = mp_init(&mu)) != MP_OKAY)
    goto MU;

  mp_set(&s, 1);

  /* mu = b^2k / m */
  s_mp_add_d(&mu, 1);
  s_mp_lshd(&mu, 2 * USED(m));
  if ((res = mp_div(&mu, m, &mu, NULL)) != MP_OKAY)
    goto CLEANUP;

  /* Loop over digits of b in ascending order, except highest order */
  for (dig = 0; dig < (ub - 1); dig++) {
    d = *db++;

    /* Loop over the bits of the lower-order digits */
    for (bit = 0; bit < DIGIT_BIT; bit++) {
      if (d & 1) {
        if ((res = s_mp_mul(&s, &x)) != MP_OKAY)
          goto CLEANUP;
        if ((res = s_mp_reduce(&s, m, &mu)) != MP_OKAY)
          goto CLEANUP;
      }

      d >>= 1;

      if ((res = s_mp_sqr(&x)) != MP_OKAY)
        goto CLEANUP;
      if ((res = s_mp_reduce(&x, m, &mu)) != MP_OKAY)
        goto CLEANUP;
    }
  }

  /* Now do the last digit... */
  d = *db;

  while (d) {
    if (d & 1) {
      if ((res = s_mp_mul(&s, &x)) != MP_OKAY)
        goto CLEANUP;
      if ((res = s_mp_reduce(&s, m, &mu)) != MP_OKAY)
        goto CLEANUP;
    }

    d >>= 1;

    if ((res = s_mp_sqr(&x)) != MP_OKAY)
      goto CLEANUP;
    if ((res = s_mp_reduce(&x, m, &mu)) != MP_OKAY)
      goto CLEANUP;
  }

  s_mp_exch(&s, c);

 CLEANUP:
  mp_clear(&mu);
 MU:
  mp_clear(&x);
 X:
  mp_clear(&s);

  return res;
}

mp_err mp_exptmod_d(mp_int *a, mp_digit d, mp_int *m, mp_int *c)
{
  mp_int s, x;
  mp_err res;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if ((res = mp_init(&s)) != MP_OKAY)
    return res;
  if ((res = mp_init_copy(&x, a)) != MP_OKAY)
    goto X;

  mp_set(&s, 1);

  while (d != 0) {
    if (d & 1) {
      if ((res = s_mp_mul(&s, &x)) != MP_OKAY ||
          (res = mp_mod(&s, m, &s)) != MP_OKAY)
        goto CLEANUP;
    }

    d /= 2;

    if ((res = s_mp_sqr(&x)) != MP_OKAY ||
       (res = mp_mod(&x, m, &x)) != MP_OKAY)
      goto CLEANUP;
  }

  s_mp_exch(&s, c);

CLEANUP:
  mp_clear(&x);
X:
  mp_clear(&s);

  return res;
}

#endif /* if MP_MODARITH */

/* Compare a <=> 0.  Returns <0 if a<0, 0 if a=0, >0 if a>0.  */
int mp_cmp_z(mp_int *a)
{
  if (SIGN(a) == MP_NEG)
    return MP_LT;
  else if (USED(a) == 1 && DIGIT(a, 0) == 0)
    return MP_EQ;
  else
    return MP_GT;
}

/* Compare a <=> d.  Returns <0 if a<d, 0 if a=d, >0 if a>d */
int mp_cmp_d(mp_int *a, mp_digit d)
{
  ARGCHK(a != NULL, MP_EQ);

  if (SIGN(a) == MP_NEG)
    return MP_LT;

  return s_mp_cmp_d(a, d);
}

int mp_cmp(mp_int *a, mp_int *b)
{
  ARGCHK(a != NULL && b != NULL, MP_EQ);

  if (SIGN(a) == SIGN(b)) {
    int mag;

    if ((mag = s_mp_cmp(a, b)) == MP_EQ)
      return MP_EQ;

    if (SIGN(a) == MP_ZPOS)
      return mag;
    else
      return -mag;

  } else if (SIGN(a) == MP_ZPOS) {
    return MP_GT;
  } else {
    return MP_LT;
  }
}

/* Compares |a| <=> |b|, and returns an appropriate comparison result */
int mp_cmp_mag(mp_int *a, mp_int *b)
{
  ARGCHK(a != NULL && b != NULL, MP_EQ);

  return s_mp_cmp(a, b);
}

/* This just converts z to an mp_int, and uses the existing comparison
 * routines.  This is sort of inefficient, but it's not clear to me how
 * frequently this wil get used anyway.  For small positive constants,
 * you can always use mp_cmp_d(), and for zero, there is mp_cmp_z().
 */
int mp_cmp_int(mp_int *a, long z)
{
  mp_int tmp;
  int out;

  ARGCHK(a != NULL, MP_EQ);

  mp_init(&tmp); mp_set_int(&tmp, z);
  out = mp_cmp(a, &tmp);
  mp_clear(&tmp);

  return out;
}

/* Returns a true (non-zero) value if a is odd, false (zero) otherwise.
 */
int mp_isodd(mp_int *a)
{
  ARGCHK(a != NULL, 0);

  return (DIGIT(a, 0) & 1);
}

int mp_iseven(mp_int *a)
{
  return !mp_isodd(a);
}

unsigned long mp_hash(mp_int *a)
{
#if SIZEOF_LONG > MP_DIGIT_SIZE
  unsigned long hash;
  mp_size ix;

  if (USED(a) >= 2 * SIZEOF_LONG / MP_DIGIT_SIZE) {
    unsigned long omega = 0;
    unsigned long alpha = 0;
    for (ix = 0; ix < SIZEOF_LONG / MP_DIGIT_SIZE; ix++)
      omega = (omega << MP_DIGIT_BIT) | DIGIT(a, ix);
    for (ix = USED(a) - SIZEOF_LONG / MP_DIGIT_SIZE; ix < USED(a); ix++)
      alpha = (alpha << MP_DIGIT_BIT) | DIGIT(a, ix);
    hash = alpha + omega;
  } else {
    hash = 0;

    for (ix = 0; ix < USED(a); ix++)
      hash = (hash << MP_DIGIT_BIT) | DIGIT(a, ix);
  }
#else
  mp_digit omega = DIGIT(a, 0);
  mp_digit alpha = DIGIT(a, USED(a) - 1);
  unsigned long hash = alpha + omega;
#endif
  return SIGN(a) == MP_NEG ? ~hash : hash;
}

#if MP_NUMTH

/* Binary algorithm due to Josef Stein in 1961 (via Knuth). */
mp_err mp_gcd(mp_int *a, mp_int *b, mp_int *c)
{
  mp_err res;
  mp_int u, v, t;
  mp_digit k = 0;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (mp_cmp_z(a) == MP_EQ && mp_cmp_z(b) == MP_EQ)
      return MP_RANGE;
  if (mp_cmp_z(a) == MP_EQ) {
    if ((res = mp_copy(b, c)) != MP_OKAY)
      return res;
    SIGN(c) = MP_ZPOS; return MP_OKAY;
  } else if (mp_cmp_z(b) == MP_EQ) {
    if ((res = mp_copy(a, c)) != MP_OKAY)
      return res;
    SIGN(c) = MP_ZPOS; return MP_OKAY;
  }

  if ((res = mp_init(&t)) != MP_OKAY)
    return res;
  if ((res = mp_init_copy(&u, a)) != MP_OKAY)
    goto U;
  if ((res = mp_init_copy(&v, b)) != MP_OKAY)
    goto V;

  SIGN(&u) = MP_ZPOS;
  SIGN(&v) = MP_ZPOS;

  /* Divide out common factors of 2 until at least 1 of a, b is even */
  while (mp_iseven(&u) && mp_iseven(&v)) {
    s_mp_div_2(&u);
    s_mp_div_2(&v);
    ++k;
  }

  /* Initialize t */
  if (mp_isodd(&u)) {
    if ((res = mp_copy(&v, &t)) != MP_OKAY)
      goto CLEANUP;

    /* t = -v */
    if (SIGN(&v) == MP_ZPOS)
      SIGN(&t) = MP_NEG;
    else
      SIGN(&t) = MP_ZPOS;
  } else {
    if ((res = mp_copy(&u, &t)) != MP_OKAY)
      goto CLEANUP;

  }

  for (;;) {
    while (mp_iseven(&t)) {
      s_mp_div_2(&t);
    }

    if (mp_cmp_z(&t) == MP_GT) {
      if ((res = mp_copy(&t, &u)) != MP_OKAY)
        goto CLEANUP;

    } else {
      if ((res = mp_copy(&t, &v)) != MP_OKAY)
        goto CLEANUP;

      /* v = -t */
      if (SIGN(&t) == MP_ZPOS)
        SIGN(&v) = MP_NEG;
      else
        SIGN(&v) = MP_ZPOS;
    }

    if ((res = mp_sub(&u, &v, &t)) != MP_OKAY)
      goto CLEANUP;

    if (s_mp_cmp_d(&t, 0) == MP_EQ)
      break;
  }

  s_mp_2expt(&v, k); /* v = 2^k */
  res = mp_mul(&u, &v, c); /* c = u * v */

 CLEANUP:
  mp_clear(&v);
 V:
  mp_clear(&u);
 U:
  mp_clear(&t);

  return res;
}

/* We compute the least common multiple using the rule:
 *
 *  ab = [a, b](a, b)
 *
 * ... by computing the product, and dividing out the gcd.
 */
mp_err mp_lcm(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int gcd, prod;
  mp_err res;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  /* Set up temporaries */
  if ((res = mp_init(&gcd)) != MP_OKAY)
    return res;
  if ((res = mp_init(&prod)) != MP_OKAY)
    goto GCD;

  if ((res = mp_mul(a, b, &prod)) != MP_OKAY)
    goto CLEANUP;
  if ((res = mp_gcd(a, b, &gcd)) != MP_OKAY)
    goto CLEANUP;

  res = mp_div(&prod, &gcd, c, NULL);

 CLEANUP:
  mp_clear(&prod);
 GCD:
  mp_clear(&gcd);

  return res;
}

/* Compute g = (a, b) and values x and y satisfying Bezout's identity
 * (that is, ax + by = g).  This uses the extended binary GCD algorithm
 * based on the Stein algorithm used for mp_gcd()
 */
mp_err mp_xgcd(mp_int *a, mp_int *b, mp_int *g, mp_int *x, mp_int *y)
{
  mp_int gx, xc, yc, u, v, A, B, C, D;
  mp_int *clean[9];
  mp_err res;
  int last = -1;

  if (mp_cmp_z(b) == 0)
    return MP_RANGE;

  /* Initialize all these variables we need */
  if ((res = mp_init(&u)) != MP_OKAY) goto CLEANUP;
  clean[++last] = &u;
  if ((res = mp_init(&v)) != MP_OKAY) goto CLEANUP;
  clean[++last] = &v;
  if ((res = mp_init(&gx)) != MP_OKAY) goto CLEANUP;
  clean[++last] = &gx;
  if ((res = mp_init(&A)) != MP_OKAY) goto CLEANUP;
  clean[++last] = &A;
  if ((res = mp_init(&B)) != MP_OKAY) goto CLEANUP;
  clean[++last] = &B;
  if ((res = mp_init(&C)) != MP_OKAY) goto CLEANUP;
  clean[++last] = &C;
  if ((res = mp_init(&D)) != MP_OKAY) goto CLEANUP;
  clean[++last] = &D;
  if ((res = mp_init_copy(&xc, a)) != MP_OKAY) goto CLEANUP;
  clean[++last] = &xc;
  mp_abs(&xc, &xc);
  if ((res = mp_init_copy(&yc, b)) != MP_OKAY) goto CLEANUP;
  clean[++last] = &yc;
  mp_abs(&yc, &yc);

  mp_set(&gx, 1);

  /* Divide by two until at least one of them is even */
  while (mp_iseven(&xc) && mp_iseven(&yc)) {
    s_mp_div_2(&xc);
    s_mp_div_2(&yc);
    if ((res = s_mp_mul_2(&gx)) != MP_OKAY)
      goto CLEANUP;
  }

  mp_copy(&xc, &u);
  mp_copy(&yc, &v);
  mp_set(&A, 1); mp_set(&D, 1);

  /* Loop through binary GCD algorithm */
  for (;;) {
    while (mp_iseven(&u)) {
      s_mp_div_2(&u);

      if (mp_iseven(&A) && mp_iseven(&B)) {
        s_mp_div_2(&A); s_mp_div_2(&B);
      } else {
        if ((res = mp_add(&A, &yc, &A)) != MP_OKAY) goto CLEANUP;
        s_mp_div_2(&A);
        if ((res = mp_sub(&B, &xc, &B)) != MP_OKAY) goto CLEANUP;
        s_mp_div_2(&B);
      }
    }

    while (mp_iseven(&v)) {
      s_mp_div_2(&v);

      if (mp_iseven(&C) && mp_iseven(&D)) {
        s_mp_div_2(&C); s_mp_div_2(&D);
      } else {
        if ((res = mp_add(&C, &yc, &C)) != MP_OKAY) goto CLEANUP;
        s_mp_div_2(&C);
        if ((res = mp_sub(&D, &xc, &D)) != MP_OKAY) goto CLEANUP;
        s_mp_div_2(&D);
      }
    }

    if (mp_cmp(&u, &v) >= 0) {
      if ((res = mp_sub(&u, &v, &u)) != MP_OKAY) goto CLEANUP;
      if ((res = mp_sub(&A, &C, &A)) != MP_OKAY) goto CLEANUP;
      if ((res = mp_sub(&B, &D, &B)) != MP_OKAY) goto CLEANUP;

    } else {
      if ((res = mp_sub(&v, &u, &v)) != MP_OKAY) goto CLEANUP;
      if ((res = mp_sub(&C, &A, &C)) != MP_OKAY) goto CLEANUP;
      if ((res = mp_sub(&D, &B, &D)) != MP_OKAY) goto CLEANUP;

    }

    /* If we're done, copy results to output */
    if (mp_cmp_z(&u) == 0) {
      if (x)
        if ((res = mp_copy(&C, x)) != MP_OKAY) goto CLEANUP;

      if (y)
        if ((res = mp_copy(&D, y)) != MP_OKAY) goto CLEANUP;

      if (g)
        if ((res = mp_mul(&gx, &v, g)) != MP_OKAY) goto CLEANUP;

      break;
    }
  }

 CLEANUP:
  while (last >= 0)
    mp_clear(clean[last--]);

  return res;
}

/* Compute c = a^-1 (mod m), if there is an inverse for a (mod m).
 * This is equivalent to the question of whether (a, m) = 1.  If not,
 * MP_UNDEF is returned, and there is no inverse.
 */
mp_err mp_invmod(mp_int *a, mp_int *m, mp_int *c)
{
  mp_int g, x;
  mp_sign sa;
  mp_err res;

  ARGCHK(a && m && c, MP_BADARG);

  if (mp_cmp_z(a) == 0 || mp_cmp_z(m) == 0)
    return MP_RANGE;

  sa = SIGN(a);

  if ((res = mp_init(&g)) != MP_OKAY)
    return res;
  if ((res = mp_init(&x)) != MP_OKAY)
    goto X;

  if ((res = mp_xgcd(a, m, &g, &x, NULL)) != MP_OKAY)
    goto CLEANUP;

  if (mp_cmp_d(&g, 1) != MP_EQ) {
    res = MP_UNDEF;
    goto CLEANUP;
  }

  res = mp_mod(&x, m, c);
  SIGN(c) = sa;

CLEANUP:
  mp_clear(&x);
X:
  mp_clear(&g);

  return res;
}

#endif /* if MP_NUMTH */

/* Convert a's bit vector to its two's complement, up to the
 * number of words that it contains, storing result in b. The numeric value of
 * this result depends on the size of mpi_digit. This is a building block for
 * handling negative operands in the bit operations.
 */
mp_err mp_2comp(mp_int *a, mp_int *b, mp_size dig)
{
  mp_err res;
  mp_size ix, adig = USED(a);
  mp_digit *pa, *pb;
  mp_digit padding = ISNEG(a) ? MP_DIGIT_MAX : 0;
  mp_word w;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if (a != b) {
    if ((res = mp_init_size(b, dig)) != MP_OKAY)
      return res;
    SIGN(b) = SIGN(a);
  } else {
    if ((res = s_mp_pad(b, dig)) != MP_OKAY)
      return res;
  }

  for (pa = DIGITS(a), pb = DIGITS(b), w = 0, ix = 0; ix < dig; ix++) {
    w += (ix == 0);
    w += (ix < adig) ? ~pa[ix] : padding;
    pb[ix] = ACCUM(w);
    w = CARRYOUT(w);
  }

  USED(b) = dig;

  return MP_OKAY;
}

mp_err mp_and(mp_int *a, mp_int *b, mp_int *c)
{
  mp_err res = MP_OKAY;
  mp_size ix, extent = 0;
  mp_digit *pa, *pb, *pc;
  mp_int tmp_a, tmp_b;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (a == b)
    return mp_copy(a, c);

  if (ISNEG(a)) {
    extent = USED(b);
    if ((res = mp_2comp(a, &tmp_a, extent)) != MP_OKAY)
      goto out;
    a = &tmp_a;
  }

  if (ISNEG(b)) {
    extent = USED(a);
    if ((res = mp_2comp(b, &tmp_b, extent)) != MP_OKAY)
      goto out;
    b = &tmp_b;
  }

  if (!extent)
    extent = MIN(USED(a), USED(b));

  if (c != a && c != b) {
    if ((res = mp_init_size(c, extent)) != MP_OKAY)
      goto out;
  }

  for (pa = DIGITS(a), pb = DIGITS(b), pc = DIGITS(c), ix = 0;
       ix < extent; ix++)
  {
    pc[ix] = pa[ix] & pb[ix];
  }

  USED(c) = extent;

  if (ISNEG(a) && ISNEG(b)) {
    mp_2comp(c, c, extent);
    SIGN(c) = MP_NEG;
  }

  s_mp_clamp(c);

out:
  if (ISNEG(a))
    mp_clear(&tmp_a);

  if (ISNEG(b))
    mp_clear(&tmp_b);

  return res;
}

mp_err mp_or(mp_int *a, mp_int *b, mp_int *c)
{
  mp_err res;
  mp_size ix, extent, mindig;
  mp_digit *pa, *pb, *pc;
  mp_int tmp_a, tmp_b;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  extent = MAX(USED(a), USED(b));
  mindig = MIN(USED(a), USED(b));

  if (a == b)
    return mp_copy(a, c);

  if (ISNEG(a)) {
    if ((res = mp_2comp(a, &tmp_a, extent)) != MP_OKAY)
      goto out;
    a = &tmp_a;
  }

  if (ISNEG(b)) {
    if ((res = mp_2comp(b, &tmp_b, extent)) != MP_OKAY)
      goto out;
    b = &tmp_b;
  }

  if (c != a && c != b)
    res = mp_init_size(c, extent);
  else
    res = s_mp_pad(c, extent);

  if (res != MP_OKAY)
    goto out;

  for (pa = DIGITS(a), pb = DIGITS(b), pc = DIGITS(c), ix = 0;
       ix < mindig; ix++)
  {
    pc[ix] = pa[ix] | pb[ix];
  }

  if (ix < USED(a))
    s_mp_copy(pa + ix, pc + ix, USED(a) - ix);
  else if (ix < USED(b))
    s_mp_copy(pb + ix, pc + ix, USED(b) - ix);

  USED(c) = extent;

  if (ISNEG(a) || ISNEG(b)) {
    mp_2comp(c, c, extent);
    SIGN(c) = MP_NEG;
  }

  s_mp_clamp(c);

out:
  if (ISNEG(a))
    mp_clear(&tmp_a);

  if (ISNEG(b))
    mp_clear(&tmp_b);

  return res;
}

mp_err mp_xor(mp_int *a, mp_int *b, mp_int *c)
{
  mp_err res;
  mp_size ix, extent, mindig;
  mp_digit *pa, *pb, *pc;
  mp_int tmp_a, tmp_b;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (a == b) {
    mp_zero(c);
    return MP_OKAY;
  }

  extent = MAX(USED(a), USED(b));
  mindig = MIN(USED(a), USED(b));

  if (ISNEG(a)) {
    if ((res = mp_2comp(a, &tmp_a, extent)) != MP_OKAY)
      goto out;
    a = &tmp_a;
  }

  if (ISNEG(b)) {
    if ((res = mp_2comp(b, &tmp_b, extent)) != MP_OKAY)
      goto out;
    b = &tmp_b;
  }

  if (c != a && c != b)
    res = mp_init_size(c, extent);
  else
    res = s_mp_pad(c, extent);

  if (res != MP_OKAY)
    goto out;

  for (pa = DIGITS(a), pb = DIGITS(b), pc = DIGITS(c), ix = 0;
       ix < mindig; ix++)
  {
    pc[ix] = pa[ix] ^ pb[ix];
  }

  if (ix < USED(a))
    s_mp_copy(pa + ix, pc + ix, USED(a) - ix);
  else if (ix < USED(b))
    s_mp_copy(pb + ix, pc + ix, USED(b) - ix);

  USED(c) = extent;

  if (ISNEG(a) ^ ISNEG(b)) {
    mp_2comp(c, c, extent);
    SIGN(c) = MP_NEG;
  }

  s_mp_clamp(c);

out:
  if (ISNEG(a))
    mp_clear(&tmp_a);

  if (ISNEG(b))
    mp_clear(&tmp_b);

  return res;
}

mp_err mp_comp(mp_int *a, mp_int *b)
{
  mp_err res;
  mp_size ix, dig = USED(a);
  mp_digit *pa, *pb;
  mp_int tmp;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if (a != b)
    res = mp_init_size(b, dig);
  else
    res = s_mp_pad(b, dig);

  if (res != MP_OKAY)
    return res;

  if (ISNEG(a)) {
    if ((res = mp_2comp(a, &tmp, dig)) != MP_OKAY)
      return res;
    a = &tmp;
  }

  for (pa = DIGITS(a), pb = DIGITS(b), ix = 0; ix < dig; ix++)
    pb[ix] = ~pa[ix];

  USED(b) = dig;

  if (ISNEG(a)) {
    mp_clear(&tmp);
  } else {
    if ((res = mp_2comp(b, b, dig)) != MP_OKAY)
      return res;
    SIGN(b) = MP_NEG;
  }

  s_mp_clamp(b);
  return MP_OKAY;
}

mp_err mp_trunc_comp(mp_int *a, mp_int *b, mp_size bits)
{
  mp_err res;
  mp_size ix, dig = bits / DIGIT_BIT, rembits = bits % DIGIT_BIT;
  mp_size adig = USED(a);
  mp_digit padding = ISNEG(a) ? MP_DIGIT_MAX : 0;
  int extra = (rembits != 0);
  mp_digit *pa, *pb;
  mp_int tmp;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if (a != b)
    res = mp_init_size(b, dig + extra);
  else
    res = s_mp_pad(b, dig + extra);

  if (res != MP_OKAY)
    return res;

  if (ISNEG(a)) {
    if ((res = mp_2comp(a, &tmp, dig + extra)) != MP_OKAY)
      return res;
    a = &tmp;
  }

  for (pa = DIGITS(a), pb = DIGITS(b), ix = 0; ix < dig; ix++)
    pb[ix] = (ix < adig) ? ~pa[ix] : ~padding;

  if (rembits) {
    mp_digit mask = (MP_DIGIT_MAX >> (DIGIT_BIT - rembits));
    pb[ix] = (((ix < adig) ? pa[ix] : padding) & mask) ^ mask;
  }

  USED(b) = dig + extra;

  if (ISNEG(a))
    mp_clear(&tmp);

  s_mp_clamp(b);
  return MP_OKAY;
}

mp_err mp_trunc(mp_int *a, mp_int *b, mp_size bits)
{
  mp_err res;
  mp_size ix, dig = bits / DIGIT_BIT, rembits = bits % DIGIT_BIT;
  mp_size adig = USED(a);
  mp_digit padding = ISNEG(a) ? MP_DIGIT_MAX : 0;
  int extra = (rembits != 0);
  mp_digit *pa, *pb;
  mp_int tmp;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if (a != b)
    res = mp_init_size(b, dig + extra);
  else
    res = s_mp_pad(b, dig + extra);

  if (res != MP_OKAY)
    return res;

  if (ISNEG(a)) {
    if ((res = mp_2comp(a, &tmp, dig + extra)) != MP_OKAY)
      return res;
    a = &tmp;
  }

  for (pa = DIGITS(a), pb = DIGITS(b), ix = 0; ix < dig; ix++)
    pb[ix] = (ix < adig) ? pa[ix] : padding;

  if (rembits) {
    mp_digit mask = (MP_DIGIT_MAX >> (DIGIT_BIT - rembits));
    pb[ix] = ((ix < adig) ? pa[ix] : padding) & mask;
  }

  USED(b) = dig + extra;

  if (ISNEG(a))
    mp_clear(&tmp);

  s_mp_clamp(b);
  return MP_OKAY;
}

mp_err mp_shift(mp_int *a, mp_int *b, int bits)
{
  mp_int tmp;
  mp_err res;
  int a_neg = ISNEG(a);

  if (bits == 0)
    return mp_copy(a, b);

  if (a_neg) {
    mp_size ua = USED(a);
    if ((res = mp_2comp(a, &tmp, ua)) != MP_OKAY)
      return res;
    SIGN(&tmp) = MP_ZPOS;
    a = &tmp;
  }

  if (bits > 0)
    res = mp_mul_2d(a, bits, b);
  else
    res = mp_div_2d(a, -bits, b, NULL);

  if (res != MP_OKAY) {
    if (a_neg)
      mp_clear(&tmp);
    return res;
  }

  if (a_neg) {
    mp_size hb, msd;
    mp_digit *db;

    mp_clear(&tmp);

    msd = USED(b)-1;
    db = DIGITS(b);
    hb = s_highest_bit(db[msd]);

    if (hb < DIGIT_BIT)
      db[msd] |= MP_DIGIT_MAX << hb;

    if ((res = mp_2comp(b, b, USED(b))) != MP_OKAY)
      return res;

    SIGN(b) = MP_NEG;
    s_mp_clamp(b);
  }

  return MP_OKAY;
}

mp_err mp_bit(mp_int *a, mp_size bit)
{
  mp_int tmp;
  mp_err res;
  int a_neg = ISNEG(a);
  mp_size digit = bit / MP_DIGIT_BIT;
  mp_digit mask = convert(mp_digit, 1) << (bit % MP_DIGIT_BIT);

  if (a_neg) {
    if ((res = mp_2comp(a, &tmp, bit + 1)) != MP_OKAY)
      return res;
    SIGN(&tmp) = MP_ZPOS;
    a = &tmp;
  }

  res = (digit < USED(a) && (DIGITS(a)[digit] & mask) != 0) ? MP_YES : MP_NO;

  if (a_neg)
    mp_clear(&tmp);

  return res;
}

mp_err mp_to_double(mp_int *mp, double *d)
{
  mp_size ix;
  mp_size used = USED(mp);
  mp_digit *dp = DIGITS(mp);
  static double mult;
  double out = dp[used - 1];

  if (!mult)
    mult = pow(2.0, MP_DIGIT_BIT);

  for (ix = used - 2; ix < MP_SIZE_MAX - 1; ix--) {
    out = out * mult;
    out += convert(double, dp[ix]);
  }

  if (SIGN(mp) == MP_NEG)
    out = -out;

  *d = out;
  return MP_OKAY;
}

#if MP_IOFUNC
/* Print a textual representation of the given mp_int on the output
 * stream 'ofp'.  Output is generated using the internal radix.
 */
void mp_print(mp_int *mp, FILE *ofp)
{
  mp_size ix;

  if (mp == NULL || ofp == NULL)
    return;

  fputc((SIGN(mp) == MP_NEG) ? '-' : '+', ofp);

  for (ix = USED(mp) - 1; ix < MP_SIZE_MAX; ix--) {
    fprintf(ofp, DIGIT_FMT, DIGIT(mp, ix));
  }
}

#endif /* if MP_IOFUNC */

/* Read in a raw value (base 256) into the given mp_int */
mp_err mp_read_signed_bin(mp_int *mp, unsigned char *str, size_t len)
{
  mp_err res;

  ARGCHK(mp != NULL && str != NULL && len > 0, MP_BADARG);

  if ((res = mp_read_unsigned_bin(mp, str + 1, len - 1)) == MP_OKAY) {
    /* Get sign from first byte */
    if (str[0])
      SIGN(mp) = MP_NEG;
    else
      SIGN(mp) = MP_ZPOS;
  }

  return res;
}

size_t mp_signed_bin_size(mp_int *mp)
{
  ARGCHK(mp != NULL, 0);

  return mp_unsigned_bin_size(mp) + 1;
}

mp_err mp_to_signed_bin(mp_int *mp, unsigned char *str)
{
  ARGCHK(mp != NULL && str != NULL, MP_BADARG);

  /* Caller responsible for allocating enough memory (use mp_raw_size(mp)) */
  str[0] = convert(char, SIGN(mp));

  return mp_to_unsigned_bin(mp, str + 1);
}

/* Read in an unsigned value (base 256) into the given mp_int */
mp_err mp_read_unsigned_bin(mp_int *mp, unsigned char *str, size_t len)
{
  mp_size ix;
  mp_err res;

  ARGCHK(mp != NULL && str != NULL && len > 0, MP_BADARG);

  mp_zero(mp);

  for (ix = 0; ix < len; ix++) {
    if ((res = s_mp_mul_2d(mp, CHAR_BIT)) != MP_OKAY)
      return res;

    if ((res = mp_add_d(mp, str[ix], mp)) != MP_OKAY)
      return res;
  }

  return MP_OKAY;
}

size_t mp_unsigned_bin_size(mp_int *mp)
{
  mp_digit topdig;
  size_t count;

  ARGCHK(mp != NULL, 0);

  /* Special case for the value zero */
  if (USED(mp) == 1 && DIGIT(mp, 0) == 0)
    return 1;

  count = (USED(mp) - 1) * sizeof (mp_digit);
  topdig = DIGIT(mp, USED(mp) - 1);

  while (topdig != 0) {
    ++count;
    topdig >>= CHAR_BIT;
  }

  return count;
}

mp_err mp_to_unsigned_bin(mp_int *mp, unsigned char *str)
{
  mp_digit *dp, *end, d;
  unsigned char *spos;

  ARGCHK(mp != NULL && str != NULL, MP_BADARG);

  dp = DIGITS(mp);
  end = dp + USED(mp) - 1;
  spos = str;

  /* Special case for zero, quick test */
  if (dp == end && *dp == 0) {
    *str = '\0';
    return MP_OKAY;
  }

  /* Generate digits in reverse order */
  while (dp < end) {
    size_t i;

    d = *dp;
    for (i = 0; i < sizeof (mp_digit); i++) {
      *spos = d & UCHAR_MAX;
      d >>= CHAR_BIT;
      ++spos;
    }

    ++dp;
  }

  /* Now handle last digit specially, high order zeroes are not written */
  d = *end;
  while (d != 0) {
    *spos = d & UCHAR_MAX;
    d >>= CHAR_BIT;
    ++spos;
  }

  /* Reverse everything to get digits in the correct order */
  while (--spos > str) {
    unsigned char t = *str;
    *str = *spos;
    *spos = t;

    ++str;
  }

  return MP_OKAY;
}

mp_err mp_to_unsigned_buf(mp_int *mp, unsigned char *str, size_t size)
{
  mp_digit *dp, *end;
  unsigned char *spos;

  ARGCHK(mp != NULL && str != NULL, MP_BADARG);

  for (spos = str + size, dp = DIGITS(mp), end = dp + USED(mp); dp < end; dp++) {
    size_t i;
    mp_digit d = *dp;

    for (i = 0; i < sizeof (mp_digit); i++) {
      if (dp + 1 == end && d == 0)
        break;
      ARGCHK(spos >= str, MP_RANGE);
      *--spos = d & 0xFF;
      d >>= 8;
    }
  }

  while (spos > str)
    *--spos = 0;

  return MP_OKAY;
}

mp_size mp_count_bits(mp_int *mp)
{
  ARGCHK(mp != NULL, MP_BADARG);

  return s_highest_bit_mp(mp);
}

static mp_size s_mp_count_ones(mp_int *mp)
{
  mp_size ix;
  mp_size c;
  mp_digit *dp = DIGITS(mp);

  for (c = 0, ix = USED(mp) - 1; ix < MP_SIZE_MAX; ix--) {
    mp_digit d = dp[ix];
#if MP_DIGIT_SIZE == 8
    d = ((d & 0xAAAAAAAAAAAAAAAA) >>  1) + (d & 0x5555555555555555);
    d = ((d & 0xCCCCCCCCCCCCCCCC) >>  2) + (d & 0x3333333333333333);
    d = ((d & 0xF0F0F0F0F0F0F0F0) >>  4) + (d & 0x0F0F0F0F0F0F0F0F);
    d = ((d & 0xFF00FF00FF00FF00) >>  8) + (d & 0x00FF00FF00FF00FF);
    d = ((d & 0xFFFF0000FFFF0000) >> 16) + (d & 0x0000FFFF0000FFFF);
    d = ((d & 0xFFFFFFFF00000000) >> 32) + (d & 0x00000000FFFFFFFF);
    c += d;
#elif MP_DIGIT_SIZE == 4
    d = ((d & 0xAAAAAAAA) >>  1) + (d & 0x55555555);
    d = ((d & 0xCCCCCCCC) >>  2) + (d & 0x33333333);
    d = ((d & 0xF0F0F0F0) >>  4) + (d & 0x0F0F0F0F);
    d = ((d & 0xFF00FF00) >>  8) + (d & 0x00FF00FF);
    d = ((d & 0xFFFF0000) >> 16) + (d & 0x0000FFFF);
    c += d;
#elif MP_DIGIT_SIZE == 2
    d = ((d & 0xAAAA) >> 1) + (d & 0x5555);
    d = ((d & 0xCCCC) >> 2) + (d & 0x3333);
    d = ((d & 0xF0F0) >> 4) + (d & 0x0F0F);
    d = ((d & 0xFF00) >> 8) + (d & 0x00FF);
    c += d;
#elif MP_DIGIT_SIZE == 1
    d = ((d & 0xAA) >> 1) + (d & 0x55);
    d = ((d & 0xCC) >> 2) + (d & 0x33);
    d = ((d & 0xF0) >> 4) + (d & 0x0F);
    c += d;
#else
#error fixme: unsupported MP_DIGIT_SIZE
#endif
  }

  return c;
}

mp_err mp_count_ones(mp_int *mp)
{
  if (SIGN(mp) == MP_NEG) {
    mp_int tmp;
    mp_err res;
    if ((res = mp_init_copy(&tmp, mp)) != MP_OKAY)
      return res;
    if ((res = s_mp_sub_d(&tmp, 1) != MP_OKAY))
      return res;
    res = s_mp_count_ones(&tmp);
    mp_clear(&tmp);
    return res;
  }

  return s_mp_count_ones(mp);
}

mp_size mp_is_pow_two(mp_int *mp)
{
  return s_mp_ispow2(mp) < MP_SIZE_MAX;
}

/* Read an integer from the given string, and set mp to the resulting
 * value.  The input is presumed to be in base 10.  Leading non-digit
 * characters are ignored, and the function reads until a non-digit
 * character or the end of the string.
 */
mp_err mp_read_radix(mp_int *mp, const wchar_t *str, int radix)
{
  size_t ix = 0;
  int val = 0;
  mp_err res;
  mp_sign sig = MP_ZPOS;

  ARGCHK(mp != NULL && str != NULL && radix >= 2 && radix <= MAX_RADIX,
         MP_BADARG);

  mp_zero(mp);

  /* Skip leading non-digit characters until a digit or '-' or '+' */
  while (str[ix] &&
        (s_mp_tovalue(str[ix], radix) < 0) &&
        str[ix] != '-' &&
        str[ix] != '+')
  {
    ++ix;
  }

  if (str[ix] == '-') {
    sig = MP_NEG;
    ++ix;
  } else if (str[ix] == '+') {
    sig = MP_ZPOS; /* this is the default anyway... */
    ++ix;
  }

  while ((val = s_mp_tovalue(str[ix], radix)) >= 0) {
    if ((res = s_mp_mul_d(mp, radix)) != MP_OKAY)
      return res;
    if ((res = s_mp_add_d(mp, val)) != MP_OKAY)
      return res;
    ++ix;
  }

  if (s_mp_cmp_d(mp, 0) == MP_EQ)
    SIGN(mp) = MP_ZPOS;
  else
    SIGN(mp) = sig;

  return MP_OKAY;
}

mp_size mp_radix_size(mp_int *mp, int radix)
{
  size_t len;
  ARGCHK(mp != NULL, 0);

  len = s_mp_outlen(mp_count_bits(mp), radix) + 1; /* for NUL terminator */

  if (mp_cmp_z(mp) < 0)
    ++len; /* for sign */

  return len;
}

/* Return the number of digits in the specified radix that would be
 * needed to express 'num' digits of 'qty' bits each.
 */
mp_size mp_value_radix_size(mp_size num, mp_size qty, int radix)
{
  ARGCHK(radix >= 2 && radix <= MAX_RADIX, 0);

  return s_mp_outlen(num * qty, radix);
}

mp_err mp_toradix_case(mp_int *mp, unsigned char *str, int radix, int low)
{
  size_t ix, pos = 0;

  ARGCHK(mp != NULL && str != NULL, MP_BADARG);
  ARGCHK(radix > 1 && radix <= MAX_RADIX, MP_RANGE);

  if (mp_cmp_z(mp) == MP_EQ) {
    str[0] = '0';
    str[1] = '\0';
  } else {
    mp_err res;
    mp_int tmp;
    mp_sign sgn;
    mp_digit rem, rdx = convert(mp_digit, radix);
    char ch;

    if ((res = mp_init_copy(&tmp, mp)) != MP_OKAY)
      return res;

    /* Save sign for later, and take absolute value */
    sgn = SIGN(&tmp); SIGN(&tmp) = MP_ZPOS;

    /* Generate output digits in reverse order */
    while (mp_cmp_z(&tmp) != 0) {
      if ((res = s_mp_div_d(&tmp, rdx, &rem)) != MP_OKAY) {
        mp_clear(&tmp);
        return res;
      }

      /* Generate digits, use capital letters */
      ch = s_mp_todigit(rem, radix, low);

      str[pos++] = ch;
    }

    /* Add - sign if original value was negative */
    if (sgn == MP_NEG)
      str[pos++] = '-';

    str[pos--] = '\0';

    /* Reverse the digits and sign indicator */
    ix = 0;
    while (ix < pos) {
      unsigned char tmp2 = str[ix];

      str[ix] = str[pos];
      str[pos] = tmp2;
      ++ix;
      --pos;
    }

    mp_clear(&tmp);
  }

  return MP_OKAY;
}

mp_err mp_toradix(mp_int *mp, unsigned char *str, int radix)
{
  return mp_toradix_case(mp, str, radix, 0);
}

int mp_char2value(char ch, int r)
{
  return s_mp_tovalue(ch, r);
}

/* Return a string describing the meaning of error code 'ec'.  The
 * string returned is allocated in static memory, so the caller should
 * not attempt to modify or free the memory associated with this
 * string.
 */
const char *mp_strerror(mp_err ec)
{
  int aec = (ec < 0) ? -ec : ec;

  /* Code values are negative, so the senses of these comparisons
     are accurate */
  if (ec < MP_LAST_CODE || ec > MP_OKAY) {
    return mp_err_string[0]; /* unknown error code */
  } else {
    return mp_err_string[aec + 1];
  }
}

/* Make sure there are at least 'min' digits allocated to mp */
mp_err s_mp_grow(mp_int *mp, mp_size min)
{
  if (min > MP_MAX_DIGITS)
    return MP_TOOBIG;

  if (min > ALLOC(mp)) {
    mp_digit *tmp;

    /* Set min to next nearest default precision block size */
    min = ((min + (s_mp_defprec - 1)) / s_mp_defprec) * s_mp_defprec;

    if ((tmp = coerce(mp_digit *, s_mp_alloc(min, sizeof (mp_digit)))) == NULL)
      return MP_MEM;

    s_mp_copy(DIGITS(mp), tmp, USED(mp));

#if MP_CRYPTO
    s_mp_setz(DIGITS(mp), ALLOC(mp));
#endif
    s_mp_free(DIGITS(mp));
    DIGITS(mp) = tmp;
    ALLOC(mp) = min;
  }

  return MP_OKAY;
}

/* Make sure the used size of mp is at least 'min', growing if needed */
mp_err s_mp_pad(mp_int *mp, mp_size min)
{
  if (min > USED(mp)) {
    mp_err res;

    /* Make sure there is room to increase precision */
    if (min > ALLOC(mp) && (res = s_mp_grow(mp, min)) != MP_OKAY)
      return res;

    /* Increase precision; should already be 0-filled */
    USED(mp) = min;
  }

  return MP_OKAY;
}

#if MP_MACRO == 0
/* Set 'count' digits pointed to by dp to be zeroes */
void s_mp_setz(mp_digit *dp, mp_size count)
{
#if MP_MEMSET == 0
  mp_size ix;

  for (ix = 0; ix < count; ix++)
    dp[ix] = 0;
#else
  memset(dp, 0, count * sizeof (mp_digit));
#endif
}
#endif

#if MP_MACRO == 0
/* Copy 'count' digits from sp to dp */
void s_mp_copy(mp_digit *sp, mp_digit *dp, mp_size count)
{
#if MP_MEMCPY == 0
  mp_size ix;

  for (ix = 0; ix < count; ix++)
    dp[ix] = sp[ix];
#else
  memcpy(dp, sp, count * sizeof (mp_digit));
#endif
}
#endif

#if MP_MACRO == 0
void *s_mp_alloc(size_t nb, size_t ni)
{
  return chk_calloc(nb, ni);
}
#endif

#if MP_MACRO == 0
void s_mp_free(void *ptr)
{
  if (ptr)
    free(ptr);
}
#endif

/* Remove leading zeroes from the given value */
void s_mp_clamp(mp_int *mp)
{
  mp_size du = USED(mp);
  mp_digit *zp = DIGITS(mp) + du - 1;

  while (du > 1 && !*zp--)
    --du;

  if (du == 1 && *zp == 0)
    SIGN(mp) = MP_ZPOS;

  USED(mp) = du;
}

static mp_size s_highest_bit(mp_digit n)
{
#if defined __GNUC__ && MP_DIGIT_SIZE == SIZEOF_INT
  return (n == 0) ? 0 : (MP_DIGIT_BIT - __builtin_clz(n));
#elif defined __GNUC__ && MP_DIGIT_SIZE == SIZEOF_LONG
  return (n == 0) ? 0 : (MP_DIGIT_BIT - __builtin_clzl(n));
#elif defined __GNUC__ && MP_DIGIT_SIZE == SIZEOF_LONGLONG_T
  return (n == 0) ? 0 : (MP_DIGIT_BIT - __builtin_clzll(n));
#elif MP_DIGIT_SIZE == 8
  if (n & 0xFFFFFFFF00000000) {
    if (n & 0xFFFF000000000000) {
      if (n & 0xFF00000000000000) {
        if (n & 0xF000000000000000) {
          if (n & 0xC000000000000000)
            return (n & 0x8000000000000000) ? 64 : 63;
          else
            return (n & 0x2000000000000000) ? 62 : 61;
        } else {
          if (n & 0x0C00000000000000)
            return (n & 0x0800000000000000) ? 60 : 59;
          else
            return (n & 0x0200000000000000) ? 58 : 57;
        }
      } else {
        if (n & 0x00F0000000000000) {
          if (n & 0x00C0000000000000)
            return (n & 0x0080000000000000) ? 56 : 55;
          else
            return (n & 0x0020000000000000) ? 54 : 53;
        } else {
          if (n & 0x000C000000000000)
            return (n & 0x0008000000000000) ? 52 : 51;
          else
            return (n & 0x0002000000000000) ? 50 : 49;
        }
      }
    } else {
      if (n & 0x0000FF0000000000) {
        if (n & 0x0000F00000000000) {
          if (n & 0x0000C00000000000)
            return (n & 0x0000800000000000) ? 48 : 47;
          else
            return (n & 0x0000200000000000) ? 46 : 45;
        } else {
          if (n & 0x00000C0000000000)
            return (n & 0x0000080000000000) ? 44 : 43;
          else
            return (n & 0x0000020000000000) ? 42 : 41;
        }
      } else {
        if (n & 0x000000F000000000) {
          if (n & 0x000000C000000000)
            return (n & 0x0000008000000000) ? 40 : 39;
          else
            return (n & 0x0000002000000000) ? 38 : 37;
        } else {
          if (n & 0x0000000C00000000)
            return (n & 0x0000000800000000) ? 36 : 35;
          else
            return (n & 0x0000000200000000) ? 34 : 33;
        }
      }
    }
  } else {
    if (n & 0x00000000FFFF0000) {
      if (n & 0x00000000FF000000) {
        if (n & 0x00000000F0000000) {
          if (n & 0x00000000C0000000)
            return (n & 0x0000000080000000) ? 32 : 31;
          else
            return (n & 0x0000000020000000) ? 30 : 29;
        } else {
          if (n & 0x000000000C000000)
            return (n & 0x0000000008000000) ? 28 : 27;
          else
            return (n & 0x0000000002000000) ? 26 : 25;
        }
      } else {
        if (n & 0x0000000000F00000) {
          if (n & 0x0000000000C00000)
            return (n & 0x0000000000800000) ? 24 : 23;
          else
            return (n & 0x0000000000200000) ? 22 : 21;
        } else {
          if (n & 0x00000000000C0000)
            return (n & 0x0000000000080000) ? 20 : 19;
          else
            return (n & 0x0000000000020000) ? 18 : 17;
        }
      }
    } else {
      if (n & 0x000000000000FF00) {
        if (n & 0x000000000000F000) {
          if (n & 0x000000000000C000)
            return (n & 0x0000000000008000) ? 16 : 15;
          else
            return (n & 0x0000000000002000) ? 14 : 13;
        } else {
          if (n & 0x0000000000000C00)
            return (n & 0x0000000000000800) ? 12 : 11;
          else
            return (n & 0x0000000000000200) ? 10 : 9;
        }
      } else {
        if (n & 0x00000000000000F0) {
          if (n & 0x00000000000000C0)
            return (n & 0x0000000000000080) ? 8 : 7;
          else
            return (n & 0x0000000000000020) ? 6 : 5;
        } else {
          if (n & 0x000000000000000C)
            return (n & 0x0000000000000008) ? 4 : 3;
          else
            return (n & 0x0000000000000002) ? 2 : (n ? 1 : 0);
        }
      }
    }
  }
#elif MP_DIGIT_SIZE == 4
  if (n & 0xFFFF0000) {
    if (n & 0xFF000000) {
      if (n & 0xF0000000) {
        if (n & 0xC0000000)
          return (n & 0x80000000) ? 32 : 31;
        else
          return (n & 0x20000000) ? 30 : 29;
      } else {
        if (n & 0x0C000000)
          return (n & 0x08000000) ? 28 : 27;
        else
          return (n & 0x02000000) ? 26 : 25;
      }
    } else {
      if (n & 0x00F00000) {
        if (n & 0x00C00000)
          return (n & 0x00800000) ? 24 : 23;
        else
          return (n & 0x00200000) ? 22 : 21;
      } else {
        if (n & 0x000C0000)
          return (n & 0x00080000) ? 20 : 19;
        else
          return (n & 0x00020000) ? 18 : 17;
      }
    }
  } else {
    if (n & 0x0000FF00) {
      if (n & 0x0000F000) {
        if (n & 0x0000C000)
          return (n & 0x00008000) ? 16 : 15;
        else
          return (n & 0x00002000) ? 14 : 13;
      } else {
        if (n & 0x00000C00)
          return (n & 0x00000800) ? 12 : 11;
        else
          return (n & 0x00000200) ? 10 : 9;
      }
    } else {
      if (n & 0x000000F0) {
        if (n & 0x000000C0)
          return (n & 0x00000080) ? 8 : 7;
        else
          return (n & 0x00000020) ? 6 : 5;
      } else {
        if (n & 0x0000000C)
          return (n & 0x00000008) ? 4 : 3;
        else
          return (n & 0x00000002) ? 2 : (n ? 1 : 0);
      }
    }
  }
#elif MP_DIGIT_SIZE == 2
  if (n & 0xFF00) {
    if (n & 0xF000) {
      if (n & 0xC000)
        return (n & 0x8000) ? 16 : 15;
      else
        return (n & 0x2000) ? 14 : 13;
    } else {
      if (n & 0x0C00)
        return (n & 0x0800) ? 12 : 11;
      else
        return (n & 0x0200) ? 10 : 9;
    }
  } else {
    if (n & 0x00F0) {
      if (n & 0x00C0)
        return (n & 0x0080) ? 8 : 7;
      else
        return (n & 0x0020) ? 6 : 5;
    } else {
      if (n & 0x000C)
        return (n & 0x0008) ? 4 : 3;
      else
        return (n & 0x0002) ? 2 : (n ? 1 : 0);
    }
  }
#elif MP_DIGIT_SIZE == 1
  if (n & 0xF0) {
    if (n & 0xC0)
      return (n & 0x80) ? 8 : 7;
    else
      return (n & 0x20) ? 6 : 5;
  } else {
    if (n & 0x0C)
      return (n & 0x08) ? 4 : 3;
    else
      return (n & 0x02) ? 2 : (n ? 1 : 0);
  }
#else
#error fixme: unsupported MP_DIGIT_SIZE
#endif
  /* notreached */
  abort();
}

mp_size s_highest_bit_mp(mp_int *a)
{
  mp_size nd1 = USED(a) - 1;
  return s_highest_bit(DIGIT(a, nd1)) + nd1 * MP_DIGIT_BIT;
}

mp_err s_mp_set_bit(mp_int *a, mp_size bit)
{
  mp_size nd = (bit + MP_DIGIT_BIT) / MP_DIGIT_BIT;
  mp_size nbit = bit - (nd - 1) * MP_DIGIT_BIT;
  mp_err res;

  if (nd == 0)
    return MP_OKAY;

  if ((res = s_mp_pad(a, nd)) != MP_OKAY)
    return res;

  DIGIT(a, nd - 1) |= (convert(mp_digit, 1) << nbit);
  return MP_OKAY;
}

/* Exchange the data for a and b; (b, a) = (a, b) */
void s_mp_exch(mp_int *a, mp_int *b)
{
  mp_int tmp;

  tmp = *a;
  *a = *b;
  *b = tmp;
}

/* Shift mp leftward by p digits, growing if needed, and zero-filling
 * the in-shifted digits at the right end.  This is a convenient
 * alternative to multiplication by powers of the radix
 */
mp_err s_mp_lshd(mp_int *mp, mp_size p)
{
  mp_err res;
  mp_size pos;
  mp_digit *dp;
  mp_size ix;

  if (p == 0)
    return MP_OKAY;

  if ((res = s_mp_pad(mp, USED(mp) + p)) != MP_OKAY)
    return res;

  pos = USED(mp) - 1;
  dp = DIGITS(mp);

  /* Shift all the significant figures over as needed */
  for (ix = pos - p; ix < MP_SIZE_MAX - p; ix--)
    dp[ix + p] = dp[ix];

  /* Fill the bottom digits with zeroes */
  for (ix = 0; ix < p; ix++)
    dp[ix] = 0;

  return MP_OKAY;
}

/* Shift mp rightward by p digits.  Maintains the invariant that
 * digits above the precision are all zero.  Digits shifted off the
 * end are lost.  Cannot fail.
 */
void s_mp_rshd(mp_int *mp, mp_size p)
{
  mp_size ix;
  mp_digit *dp;

  if (p == 0)
    return;

  /* Shortcut when all digits are to be shifted off */
  if (p >= USED(mp)) {
    s_mp_setz(DIGITS(mp), ALLOC(mp));
    USED(mp) = 1;
    SIGN(mp) = MP_ZPOS;
    return;
  }

  /* Shift all the significant figures over as needed */
  dp = DIGITS(mp);
  for (ix = p; ix < USED(mp); ix++)
    dp[ix - p] = dp[ix];

  /* Fill the top digits with zeroes */
  ix -= p;
  while (ix < USED(mp))
    dp[ix++] = 0;

  /* Strip off any leading zeroes */
  s_mp_clamp(mp);
}

/* Divide by two -- take advantage of radix properties to do it fast */
void s_mp_div_2(mp_int *mp)
{
  s_mp_div_2d(mp, 1);
}

mp_err s_mp_mul_2(mp_int *mp)
{
  mp_size ix;
  mp_digit kin = 0, kout, *dp = DIGITS(mp);
  mp_err res;

  /* Shift digits leftward by 1 bit */
  for (ix = 0; ix < USED(mp); ix++) {
    kout = (dp[ix] >> (DIGIT_BIT - 1)) & 1;
    dp[ix] = (dp[ix] << 1) | kin;

    kin = kout;
  }

  /* Deal with rollover from last digit */
  if (kin) {
    if (ix >= ALLOC(mp)) {
      if ((res = s_mp_grow(mp, ALLOC(mp) + 1)) != MP_OKAY)
        return res;
      dp = DIGITS(mp);
    }

    dp[ix] = kin;
    USED(mp) += 1;
  }

  return MP_OKAY;
}

/* Remainder the integer by 2^d, where d is a number of bits.  This
 * amounts to a bitwise AND of the value, and does not require the full
 * division code
 */
void s_mp_mod_2d(mp_int *mp, mp_digit d)
{
  mp_digit ndig = (d / DIGIT_BIT), nbit = (d % DIGIT_BIT);
  mp_size ix;
  mp_digit dmask, *dp = DIGITS(mp);

  if (ndig >= USED(mp))
    return;

  /* Flush all the bits above 2^d in its digit */
  dmask = (convert(mp_digit, 1) << nbit) - 1;
  dp[ndig] &= dmask;

  /* Flush all digits above the one with 2^d in it */
  for (ix = ndig + 1; ix < USED(mp); ix++)
    dp[ix] = 0;

  s_mp_clamp(mp);
}

/* Multiply by the integer 2^d, where d is a number of bits.  This
 * amounts to a bitwise shift of the value, and does not require the
 * full multiplication code.
 */
mp_err s_mp_mul_2d(mp_int *mp, mp_digit d)
{
  mp_err res;
  mp_digit save, next, mask, *dp;
  mp_size used;
  mp_size ix;

  if ((res = s_mp_lshd(mp, d / DIGIT_BIT)) != MP_OKAY)
    return res;

  dp = DIGITS(mp); used = USED(mp);
  d %= DIGIT_BIT;

  mask = (convert(mp_digit, 1) << d) - 1;

  /* If the shift requires another digit, make sure we've got one to
     work with */
  if ((dp[used - 1] >> (DIGIT_BIT - d)) & mask) {
    if ((res = s_mp_grow(mp, used + 1)) != MP_OKAY)
      return res;
    dp = DIGITS(mp);
  }

  /* Do the shifting... */
  save = 0;
  for (ix = 0; ix < used; ix++) {
    next = (dp[ix] >> (DIGIT_BIT - d)) & mask;
    dp[ix] = (dp[ix] << d) | save;
    save = next;
  }

  /* If, at this point, we have a nonzero carryout into the next
   * digit, we'll increase the size by one digit, and store it...
   */
  if (save) {
    dp[used] = save;
    USED(mp) += 1;
  }

  s_mp_clamp(mp);
  return MP_OKAY;
}

/* Divide the integer by 2^d, where d is a number of bits.  This
 * amounts to a bitwise shift of the value, and does not require the
 * full division code (used in Barrett reduction, see below)
 */
void s_mp_div_2d(mp_int *mp, mp_digit d)
{
  mp_size ix;
  mp_digit save, next, mask, *dp = DIGITS(mp);

  s_mp_rshd(mp, d / DIGIT_BIT);
  d %= DIGIT_BIT;

  mask = (convert(mp_digit, 1) << d) - 1;

  save = 0;
  for (ix = USED(mp) - 1; ix < MP_SIZE_MAX; ix--) {
    next = dp[ix] & mask;
    dp[ix] = (dp[ix] >> d) | (save << (DIGIT_BIT - d));
    save = next;
  }

  s_mp_clamp(mp);
}

/* Normalize a and b for division, where b is the divisor.  In order
 * that we might make good guesses for quotient digits, we want the
 * leading digit of b to be at least half the radix, which we
 * accomplish by multiplying a and b by a constant.  This constant is
 * returned (so that it can be divided back out of the remainder at the
 * end of the division process).
 * We multiply by the smallest power of 2 that gives us a leading digit
 * at least half the radix.  By choosing a power of 2, we simplify the
 * multiplication and division steps to simple shifts.
 */
mp_digit s_mp_norm(mp_int *a, mp_int *b)
{
  mp_digit t, d = 0;

  t = DIGIT(b, USED(b) - 1);

  d = MP_DIGIT_BIT - s_highest_bit(t);
  t <<= d;

  if (d != 0) {
    s_mp_mul_2d(a, d);
    s_mp_mul_2d(b, d);
  }

  return d;
}

/* Add d to |mp| in place */
mp_err s_mp_add_d(mp_int *mp, mp_digit d) /* unsigned digit addition */
{
  mp_word w, k = 0;
  mp_size ix = 1, used = USED(mp);
  mp_digit *dp = DIGITS(mp);

  w = convert(mp_word, dp[0]) + d;
  dp[0] = ACCUM(w);
  k = CARRYOUT(w);

  while (ix < used && k) {
    w = dp[ix] + k;
    dp[ix] = ACCUM(w);
    k = CARRYOUT(w);
    ++ix;
  }

  if (k != 0) {
    mp_err res;

    if ((res = s_mp_pad(mp, USED(mp) + 1)) != MP_OKAY)
      return res;

    DIGIT(mp, ix) = k;
  }

  return MP_OKAY;
}

/* Subtract d from |mp| in place, assumes |mp| > d */
mp_err s_mp_sub_d(mp_int *mp, mp_digit d) /* unsigned digit subtract */
{
  mp_word w, b = 0;
  mp_size ix = 1, used = USED(mp);
  mp_digit *dp = DIGITS(mp);

  /* Compute initial subtraction */
  w = (RADIX + dp[0]) - d;
  b = CARRYOUT(w) ? 0 : 1;
  dp[0] = ACCUM(w);

  /* Propagate borrows leftward */
  while (b && ix < used) {
    w = (RADIX + dp[ix]) - b;
    b = CARRYOUT(w) ? 0 : 1;
    dp[ix] = ACCUM(w);
    ++ix;
  }

  /* Remove leading zeroes */
  s_mp_clamp(mp);

  /* If we have a borrow out, it's a violation of the input invariant */
  if (b)
    return MP_RANGE;
  else
    return MP_OKAY;
}

/* Compute a = a * d, single digit multiplication */
mp_err s_mp_mul_d(mp_int *a, mp_digit d)
{
  mp_word w, k = 0;
  mp_size ix, max;
  mp_err res;
  mp_digit *dp = DIGITS(a);

  max = USED(a);

  for (ix = 0; ix < max; ix++) {
    w = dp[ix] * convert(mp_word, d) + k;
    dp[ix] = ACCUM(w);
    k = CARRYOUT(w);
  }

  /* If there is a carry out, we must ensure
   * we have enough storage for the extra digit.
   * If there is carry, there are no leading zeros
   * don't waste time calling s_mp_clamp.
   */
  if (k) {
    if ((res = s_mp_pad(a, max + 1)) != MP_OKAY)
      return res;
    DIGIT(a, max) = k;
    USED(a) = max + 1;
  } else {
    s_mp_clamp(a);
  }

  return MP_OKAY;
}

/* Compute the quotient mp = mp / d and remainder r = mp mod d, for a
 * single digit d.  If r is null, the remainder will be discarded.
 */
mp_err s_mp_div_d(mp_int *mp, mp_digit d, mp_digit *r)
{
  mp_word w = 0, t;
  mp_int quot;
  mp_err res;
  mp_digit *dp = DIGITS(mp), *qp;
  mp_size ix;

  if (d == 0)
    return MP_RANGE;

  /* Make room for the quotient */
  if ((res = mp_init_size(&quot, USED(mp))) != MP_OKAY)
    return res;

  USED(&quot) = USED(mp); /* so clamping will work below */
  qp = DIGITS(&quot);

  /* Divide without subtraction */
  for (ix = USED(mp) - 1; ix < MP_SIZE_MAX; ix--) {
    w = (w << DIGIT_BIT) | dp[ix];

    if (w >= d) {
      t = w / d;
      w = w % d;
    } else {
      t = 0;
    }

    assert (t <= MP_DIGIT_MAX);
    qp[ix] = t;
  }

  /* Deliver the remainder, if desired */
  if (r) {
    assert (w <= MP_DIGIT_MAX);
    *r = w;
  }

  s_mp_clamp(&quot);
  mp_exch(&quot, mp);
  mp_clear(&quot);

  return MP_OKAY;
}

/* Compute a = |a| + |b| */
mp_err s_mp_add(mp_int *a, mp_int *b) /* magnitude addition */
{
  mp_word w = 0;
  mp_digit *pa, *pb;
  mp_size ix, used = USED(b);
  mp_err res;

  /* Make sure a has enough precision for the output value */
  if ((used > USED(a)) && (res = s_mp_pad(a, used)) != MP_OKAY)
    return res;

  /* Add up all digits up to the precision of b.  If b had initially
   * the same precision as a, or greater, we took care of it by the
   * padding step above, so there is no problem.  If b had initially
   * less precision, we'll have to make sure the carry out is duly
   * propagated upward among the higher-order digits of the sum.
   */
  pa = DIGITS(a);
  pb = DIGITS(b);
  for (ix = 0; ix < used; ++ix) {
    w += *pa + convert(mp_word, *pb++);
    *pa++ = ACCUM(w);
    w = CARRYOUT(w);
  }

  /* If we run out of 'b' digits before we're actually done, make
   * sure the carries get propagated upward...
   */
  used = USED(a);
  while (w && ix < used) {
    w += *pa;
    *pa++ = ACCUM(w);
    w = CARRYOUT(w);
    ++ix;
  }

  /* If there's an overall carry out, increase precision and include
   * it.  We could have done this initially, but why touch the memory
   * allocator unless we're sure we have to?
   */
  if (w) {
    if ((res = s_mp_pad(a, used + 1)) != MP_OKAY)
      return res;

    DIGIT(a, ix) = w; /* pa may not be valid after s_mp_pad() call */
  }

  return MP_OKAY;
}

/* Compute a = |a| - |b|, assumes |a| >= |b| */
mp_err s_mp_sub(mp_int *a, mp_int *b) /* magnitude subtract */
{
  mp_word w = 0;
  mp_digit *pa, *pb;
  mp_size ix, used = USED(b);

  /* Subtract and propagate borrow.  Up to the precision of b, this
   * accounts for the digits of b; after that, we just make sure the
   * carries get to the right place.  This saves having to pad b out to
   * the precision of a just to make the loops work right...
   */
  pa = DIGITS(a);
  pb = DIGITS(b);

  for (ix = 0; ix < used; ++ix) {
    w = (RADIX + *pa) - w - *pb++;
    *pa++ = ACCUM(w);
    w = CARRYOUT(w) ? 0 : 1;
  }

  used = USED(a);
  while (ix < used) {
    w = RADIX + *pa - w;
    *pa++ = ACCUM(w);
    w = CARRYOUT(w) ? 0 : 1;
    ++ix;
  }

  /* Clobber any leading zeroes we created */
  s_mp_clamp(a);

  /* If there was a borrow out, then |b| > |a| in violation
   * of our input invariant.  We've already done the work,
   * but we'll at least complain about it...
   */
  if (w)
    return MP_RANGE;
  else
    return MP_OKAY;
}

/* Compute a = |a| * |b| */
mp_err s_mp_mul(mp_int *a, mp_int *b)
{
  mp_word w, k = 0;
  mp_int tmp;
  mp_err res;
  mp_size ix, jx, ua = USED(a), ub = USED(b);
  mp_digit *pa, *pb, *pt, *pbt;

  if ((res = mp_init_size(&tmp, ua + ub)) != MP_OKAY)
    return res;

  /* This has the effect of left-padding with zeroes... */
  USED(&tmp) = ua + ub;

  /* We're going to need the base value each iteration */
  pbt = DIGITS(&tmp);

  /* Outer loop:  Digits of b */

  pb = DIGITS(b);
  for (ix = 0; ix < ub; ++ix, ++pb) {
    if (*pb == 0)
      continue;

    /* Inner product:  Digits of a */
    pa = DIGITS(a);
    for (jx = 0; jx < ua; ++jx, ++pa) {
      pt = pbt + ix + jx;
      w = *pb * convert(mp_word, *pa) + k + *pt;
      *pt = ACCUM(w);
      k = CARRYOUT(w);
    }

    pbt[ix + jx] = k;
    k = 0;
  }

  s_mp_clamp(&tmp);
  s_mp_exch(&tmp, a);

  mp_clear(&tmp);

  return MP_OKAY;
}

/* Computes the square of a, in place.  This can be done more
 * efficiently than a general multiplication, because many of the
 * computation steps are redundant when squaring.  The inner product
 * step is a bit more complicated, but we save a fair number of
 * iterations of the multiplication loop.
 */
#if MP_SQUARE
mp_err s_mp_sqr(mp_int *a)
{
  mp_word w, k = 0;
  mp_int tmp;
  mp_err res;
  mp_size ix, jx, kx, used = USED(a);
  mp_digit *pa1, *pa2, *pt, *pbt;

  if ((res = mp_init_size(&tmp, 2 * used)) != MP_OKAY)
    return res;

  /* Left-pad with zeroes */
  USED(&tmp) = 2 * used;

  /* We need the base value each time through the loop */
  pbt = DIGITS(&tmp);

  pa1 = DIGITS(a);
  for (ix = 0; ix < used; ++ix, ++pa1) {
    if (*pa1 == 0)
      continue;

    w = DIGIT(&tmp, ix + ix) + *pa1 * convert(mp_word, *pa1);

    pbt[ix + ix] = ACCUM(w);
    k = CARRYOUT(w);

    /* The inner product is computed as:
     * (C, S) = t[i,j] + 2 a[i] a[j] + C
     * This can overflow what can be represented in an mp_word, and
     * since C arithmetic does not provide any way to check for
     * overflow, we have to check explicitly for overflow conditions
     * before they happen.
     */
    for (jx = ix + 1, pa2 = DIGITS(a) + jx; jx < used; ++jx, ++pa2) {
      mp_word u = 0, v;

      /* Store this in a temporary to avoid indirections later */
      pt = pbt + ix + jx;

      /* Compute the multiplicative step */
      w = *pa1 * convert(mp_word, *pa2);

      /* If w is more than half MP_WORD_MAX, the doubling will
       * overflow, and we need to record a carry out into the next
       * word */
      u = (w >> (MP_WORD_BIT - 1)) & 1;

      /* Double what we've got, overflow will be ignored as defined
       * for C arithmetic (we've already noted if it is to occur)
       */
      w *= 2;

      /* Compute the additive step */
      v = *pt + k;

      /* If we do not already have an overflow carry, check to see
       * if the addition will cause one, and set the carry out if so
       */
      u |= ((MP_WORD_MAX - v) < w);

      /* Add in the rest, again ignoring overflow */
      w += v;

      /* Set the i,j digit of the output */
      *pt = ACCUM(w);

      /* Save carry information for the next iteration of the loop.
       * This is why k must be an mp_word, instead of an mp_digit */
      k = CARRYOUT(w) | (u << DIGIT_BIT);

    } /* for (jx ...) */

    /* Set the last digit in the cycle and reset the carry */
    k = DIGIT(&tmp, ix + jx) + k;
    pbt[ix + jx] = ACCUM(k);
    k = CARRYOUT(k);

    /* If we are carrying out, propagate the carry to the next digit
     * in the output.  This may cascade, so we have to be somewhat
     * circumspect -- but we will have enough precision in the output
     * that we won't overflow
     */
    kx = 1;
    while (k) {
      k = convert(mp_word, pbt[ix + jx + kx]) + 1;
      pbt[ix + jx + kx] = ACCUM(k);
      k = CARRYOUT(k);
      ++kx;
    }
  } /* for (ix ...) */

  s_mp_clamp(&tmp);
  s_mp_exch(&tmp, a);

  mp_clear(&tmp);

  return MP_OKAY;
}
#endif

/* Compute a = a / b and b = a mod b.  Assumes b > a.  */
mp_err s_mp_div(mp_int *a, mp_int *b)
{
  mp_int quot, rem, t;
  mp_word q;
  mp_err res;
  mp_digit d;
  mp_size ix;

  if (mp_cmp_z(b) == 0)
    return MP_RANGE;

  /* Shortcut if b is power of two */
  if ((ix = s_mp_ispow2(b)) < MP_SIZE_MAX) {
    mp_copy(a, b); /* need this for remainder */
    s_mp_div_2d(a, convert(mp_digit, ix));
    s_mp_mod_2d(b, convert(mp_digit, ix));

    return MP_OKAY;
  }

  /* Allocate space to store the quotient */
  if ((res = mp_init_size(&quot, USED(a))) != MP_OKAY)
    return res;

  /* A working temporary for division */
  if ((res = mp_init_size(&t, USED(a))) != MP_OKAY)
    goto T;

  /* Allocate space for the remainder */
  if ((res = mp_init_size(&rem, USED(a))) != MP_OKAY)
    goto REM;

  /* Normalize to optimize guessing */
  d = s_mp_norm(a, b);

  /* Perform the division itself...woo! */
  ix = USED(a) - 1;

  while (ix < MP_SIZE_MAX) {
    /* Find a partial substring of a which is at least b */
    while (s_mp_cmp(&rem, b) < 0 && ix < MP_SIZE_MAX) {
      if ((res = s_mp_lshd(&rem, 1)) != MP_OKAY)
        goto CLEANUP;

      if ((res = s_mp_lshd(&quot, 1)) != MP_OKAY)
        goto CLEANUP;

      DIGIT(&rem, 0) = DIGIT(a, ix);
      s_mp_clamp(&rem);
      --ix;
    }

    /* If we didn't find one, we're finished dividing */
    if (s_mp_cmp(&rem, b) < 0)
      break;

    /* Compute a guess for the next quotient digit */
    q = DIGIT(&rem, USED(&rem) - 1);
    if (q <= DIGIT(b, USED(b) - 1) && USED(&rem) > 1)
      q = (q << DIGIT_BIT) | DIGIT(&rem, USED(&rem) - 2);

    q /= DIGIT(b, USED(b) - 1);

    /* The guess can be as much as RADIX + 1 */
    if (q >= RADIX)
      q = RADIX - 1;

    /* See what that multiplies out to */
    mp_copy(b, &t);
    if ((res = s_mp_mul_d(&t, q)) != MP_OKAY)
      goto CLEANUP;

    /* If it's too big, back it off.  We should not have to do this
     * more than once, or, in rare cases, twice.  Knuth describes a
     * method by which this could be reduced to a maximum of once, but
     * I didn't implement that here.
     */
    while (s_mp_cmp(&t, &rem) > 0) {
      --q;
      s_mp_sub(&t, b);
    }

    /* At this point, q should be the right next digit */
    if ((res = s_mp_sub(&rem, &t)) != MP_OKAY)
      goto CLEANUP;

    /* Include the digit in the quotient.  We allocated enough memory
     * for any quotient we could ever possibly get, so we should not
     * have to check for failures here
     */
    DIGIT(&quot, 0) = q;
  }

  /* Denormalize remainder */
  if (d != 0)
    s_mp_div_2d(&rem, d);

  s_mp_clamp(&quot);
  s_mp_clamp(&rem);

  /* Copy quotient back to output */
  s_mp_exch(&quot, a);

  /* Copy remainder back to output */
  s_mp_exch(&rem, b);

CLEANUP:
  mp_clear(&rem);
REM:
  mp_clear(&t);
T:
  mp_clear(&quot);

  return res;
}

mp_err s_mp_2expt(mp_int *a, mp_size k)
{
  mp_err res;
  mp_size dig, bit;

  dig = k / DIGIT_BIT;
  bit = k % DIGIT_BIT;

  mp_zero(a);
  if ((res = s_mp_pad(a, dig + 1)) != MP_OKAY)
    return res;

  DIGIT(a, dig) |= (convert(mp_digit, 1) << bit);

  return MP_OKAY;
}

/* Compute Barrett reduction, x (mod m), given a precomputed value for
 * mu = b^2k / m, where b = RADIX and k = #digits(m).  This should be
 * faster than straight division, when many reductions by the same
 * value of m are required (such as in modular exponentiation).  This
 * can nearly halve the time required to do modular exponentiation,
 * as compared to using the full integer divide to reduce.
 * This algorithm was derived from the _Handbook of Applied
 * Cryptography_ by Menezes, Oorschot and VanStone, Ch. 14,
 * pp. 603-604.
 */
mp_err s_mp_reduce(mp_int *x, mp_int *m, mp_int *mu)
{
  mp_int q;
  mp_err res;
  mp_size um = USED(m);

  if ((res = mp_init_copy(&q, x)) != MP_OKAY)
    return res;

  s_mp_rshd(&q, um - 1); /* q1 = x / b^(k-1) */
  s_mp_mul(&q, mu); /* q2 = q1 * mu */
  s_mp_rshd(&q, um + 1); /* q3 = q2 / b^(k+1) */

  /* x = x mod b^(k+1), quick (no division) */
  s_mp_mod_2d(x, DIGIT_BIT * (um + 1));

  /* q = q * m mod b^(k+1), quick (no division) */
  s_mp_mul(&q, m);
  s_mp_mod_2d(&q, DIGIT_BIT * (um + 1));

  /* x = x - q */
  if ((res = mp_sub(x, &q, x)) != MP_OKAY)
    goto CLEANUP;

  /* If x < 0, add b^(k+1) to it */
  if (mp_cmp_z(x) < 0) {
    mp_set(&q, 1);
    if ((res = s_mp_lshd(&q, um + 1)) != MP_OKAY)
      goto CLEANUP;
    if ((res = mp_add(x, &q, x)) != MP_OKAY)
      goto CLEANUP;
  }

  /* Back off if it's too big */
  while (mp_cmp(x, m) >= 0) {
    if ((res = s_mp_sub(x, m)) != MP_OKAY)
      break;
  }

 CLEANUP:
  mp_clear(&q);

  return res;
}

/* Compare |a| <=> |b|, return 0 if equal, <0 if a<b, >0 if a>b */
int s_mp_cmp(mp_int *a, mp_int *b)
{
  mp_size ua = USED(a), ub = USED(b);

  if (ua > ub)
    return MP_GT;
  else if (ua < ub)
    return MP_LT;
  else {
    mp_size ix = ua - 1;
    mp_digit *ap = DIGITS(a) + ix, *bp = DIGITS(b) + ix;

    for (;; ix--, ap--, bp--) {
      if (*ap > *bp)
        return MP_GT;
      else if (*ap < *bp)
        return MP_LT;
      if (ix == 0)
        break;
    }

    return MP_EQ;
  }
}

/* Compare |a| <=> d, return 0 if equal, <0 if a<d, >0 if a>d */
int s_mp_cmp_d(mp_int *a, mp_digit d)
{
  mp_size ua = USED(a);
  mp_digit *ap = DIGITS(a);

  if (ua > 1)
    return MP_GT;

  if (*ap < d)
    return MP_LT;
  else if (*ap > d)
    return MP_GT;
  else
    return MP_EQ;
}

/* Returns MP_SIZE_MAX if the value is not a power of two; otherwise, it
 * returns k such that v = 2^k, i.e. lg(v).
 */
mp_size s_mp_ispow2(mp_int *v)
{
  mp_digit d, *dp;
  mp_size uv = USED(v);
  mp_size ix;

  d = DIGIT(v, uv - 1); /* most significant digit of v */

  /* quick test */
  if ((d & (d - 1)) != 0)
    return MP_SIZE_MAX; /* not a power of two */

  if (uv >= 2) {
    ix = uv - 2;
    dp = DIGITS(v) + ix;

    for (;; ix--, dp--) {
      if (*dp)
        return MP_SIZE_MAX; /* not a power of two */
      if (ix == 0)
        break;
    }
  }

  return ((uv - 1) * DIGIT_BIT) + s_highest_bit(d) - 1;
}

int s_mp_ispow2d(mp_digit d)
{
  /* quick test */
  if ((d & (d - 1)) != 0)
    return -1; /* not a power of two */

  /* If d == 0, s_highest_bit returns 0, thus we return -1. */
  return convert(int, s_highest_bit(d)) - 1;
}

/* Convert the given character to its digit value, in the given radix.
 * If the given character is not understood in the given radix, -1 is
 * returned.  Otherwise the digit's numeric value is returned.
 * The results will be odd if you use a radix < 2 or > 62, you are
 * expected to know what you're up to.
 */
int s_mp_tovalue(wchar_t ch, int r)
{
  int val, xch;

  /* For bases up to 36, the letters of the alphabet are
     case-insensitive and denote digits valued 10 through 36.
     For bases greater than 36, the lower case letters have
     their own meaning and denote values past 36. */

  if (r <= 36 && ch >= 'a' && ch <= 'z')
    xch = ch - 'a' + 'A';
  else
    xch = ch;

  if (xch >= '0' && xch <= '9')
    val = xch - '0';
  else if (xch >= 'A' && xch <= 'Z')
    val = xch - 'A' + 10;
  else if (xch >= 'a' && xch <= 'z')
    val = xch - 'a' + 36;
  else if (xch == '+')
    val = 62;
  else if (xch == '/')
    val = 63;
  else
    return -1;

  if (val < 0 || val >= r)
    return -1;

  return val;
}

/* Convert val to a radix-r digit, if possible.  If val is out of range
 * for r, returns zero.  Otherwise, returns an ASCII character denoting
 * the value in the given radix.
 * The results may be odd if you use a radix < 2 or > 64, you are
 * expected to know what you're doing.
 */
char s_mp_todigit(int val, int r, int low)
{
  int ch;

  if (val < 0 || val >= r)
    return 0;

  ch = s_dmap_1[val];

  if (low && val > 9 && r <= 36)
    ch = ch - 'A' + 'a';

  return ch;
}

/* Return an estimate for how long a string is needed to hold a radix
 * r representation of a number with 'bits' significant bits.
 * Does not include space for a sign or a NUL terminator.
 */
size_t s_mp_outlen(mp_size bits, int r)
{
  mp_size units = bits / MP_LOG_SCALE;
  mp_size rem = bits % MP_LOG_SCALE;
  mp_size log2 = s_logv_2[r];

  return convert(size_t, units * log2 + (rem * log2 + (MP_LOG_SCALE - 1)) / MP_LOG_SCALE);
}
