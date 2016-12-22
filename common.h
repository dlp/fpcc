#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

typedef uint64_t hash_t;

typedef struct {
  unsigned nval; //  number of hashes
  hash_t *val; // pointer to array of hashes
} sig_t;


// default options for csig
#define DEFAULT_NTOKEN     5
#define DEFAULT_WINNOWSIZE 4


// default options for comp
#define DEFAULT_THRESHOLD 20


#endif // _COMMON_H_
