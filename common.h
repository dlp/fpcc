#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

// default options for sig
#define DEFAULT_NTOKEN     5
#define DEFAULT_WINNOWSIZE 4

// default options for comp
#define DEFAULT_THRESHOLD 0

// default options for map
#define DEFAULT_MIN_REGION_SIZE 4

#ifdef DEBUG
#define DBG(...)  do {\
    (void) fprintf(stderr, "[DBG] " __VA_ARGS__);\
  } while (0)
#else
#define DBG(...)
#endif


typedef uint64_t hash_t;

typedef struct {
  hash_t hash;
  uint16_t linepos;
  uint16_t filecnt;
  uint32_t next;
} hash_entry_t;

void error_exit(const char *msg);

long int parse_num(const char *s);

int hash_cmp(const hash_t *h1, const hash_t *h2);

#endif // _COMMON_H_
