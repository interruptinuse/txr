/* Default configuration for MPI library */

#ifndef MP_IOFUNC
#define MP_IOFUNC 0 /* include mp_print() ? */
#endif

#ifndef MP_MODARITH
#define MP_MODARITH 1 /* include modular arithmetic ? */
#endif

#ifndef MP_FOR_TXR
#define MP_FOR_TXR 1
#endif

#ifndef MP_NUMTH
#define MP_NUMTH 1 /* include number theoretic functions? */
#endif

#ifndef MP_MEMSET
#define MP_MEMSET 1 /* use memset() to zero buffers? */
#endif

#ifndef MP_MEMCPY
#define MP_MEMCPY 1 /* use memcpy() to copy buffers? */
#endif

#ifndef MP_CRYPTO
#define MP_CRYPTO 0 /* erase memory on free? */
#endif

#ifndef MP_ARGCHK
/* 0 = no parameter checks
 * 1 = runtime checks, continue execution and return an error to caller
 * 2 = assertions; dump core on parameter errors
 */
#define MP_ARGCHK 2 /* how to check input arguments */
#endif

#ifndef MP_DEFPREC
#define MP_DEFPREC 8 /* default precision, in digits */
#endif

#ifndef MP_MACRO
#define MP_MACRO 1 /* use macros for frequent calls? */
#endif

#ifndef MP_SQUARE
#define MP_SQUARE 1 /* use separate squaring code? */
#endif

#ifndef MP_PTAB_SIZE
/*
 * When building mpprime.c, we build in a table of small prime
 * values to use for primality testing.  The more you include,
 * the more space they take up.  See primes.c for the possible
 * values (currently 16, 32, 64, 128, 256, and 6542)
 */
#define MP_PTAB_SIZE 128 /* how many built-in primes? */
#endif

#ifndef MP_COMPAT_MACROS
#define MP_COMPAT_MACROS 0 /* define compatibility macros? */
#endif
