#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

typedef uint64_t hash_t;

int hash_cmp(const hash_t *h1, const hash_t *h2);

// default options for csig
#define DEFAULT_NTOKEN     5
#define DEFAULT_WINNOWSIZE 4


// default options for comp
#define DEFAULT_THRESHOLD 0

#define LINE_MAX 1024

#endif // _COMMON_H_
