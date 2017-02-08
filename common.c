#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

extern const char *program_name;

extern void usage(void);


void error_exit(const char *msg)
{
  (void) fprintf(stderr, "%s: %s - %s\n",
      program_name, msg, strerror(errno));
  exit(EXIT_FAILURE);
}


long int parse_num(const char *s)
{
  char *eptr;
  long int res;

  res = strtol(s, &eptr, 10);
  if (*eptr != '\0') usage();
  return res;
}


int hash_cmp(const hash_t *h1, const hash_t *h2)
{
  if (*h1 < *h2) return -1;
  if (*h1 > *h2) return 1;
  return 0;
}
