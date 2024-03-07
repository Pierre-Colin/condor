#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#if __STDC_VERSION__ >= 199901L && __STDC_VERSION < 202311L
#include <stdbool.h>
#endif

#if __STDC_VERSION__ >= 199901L
#define CDOR_RESTRICT restrict
#elif defined __GNUC__
#define CDOR_RESTRICT __restrict__
#else
#define CDOR_RESTRICT
#endif

/* Macro for non-null pointer parameter */
#if __STDC_VERSION__ >= 199901L && !((__STDC_VERSION__ == 201112L || __STDC_VERSION__ == 201710L) && defined __STDC_NO_VLA__)
#define REF(id) id[static restrict const 1]
#else
#define REF(id) * CDOR_RESTRICT const id
#endif

/* Macro for array parameter declarator */
#if __STDC_VERSION__ >= 199901L && !((__STDC_VERSION__ == 201112L || __STDC_VERSION__ == 201710L) && defined __STDC_NO_VLA__)
#define ARR_PARAM(id, n) id[static restrict const n]
#else
#define ARR_PARAM(id, n) * CDOR_RESTRICT const id
#endif

/* Macro for memory allocation */
#if __STDC_VERSION__ >= 201112L
#define allocate(T, n) ((T *) aligned_alloc (_Alignof (T), (n) * sizeof (T)))
#else
#define allocate(T, n) ((T *) malloc ((n) * sizeof (T)))
#endif

#define zero_allocate(T, n) ((T *) calloc (n, sizeof (T)))

/* Custom boolean type to accomodate C89 */
#if __STDC_VERSION__ >= 199901L
typedef bool cdor_bool;
#else
typedef char cdor_bool;
enum { false, true };
#endif

#endif /* UTIL_H_INCLUDED */
