
#include "common.h"

int hash_cmp(const hash_t *h1, const hash_t *h2)
{
  if (*h1 < *h2) return -1;
  if (*h1 > *h2) return 1;
  return 0;
}

