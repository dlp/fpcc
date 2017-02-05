/**
 * fpcc-idx - Create a fingerprint index.
 *
 *
 * Author: Daniel Prokesch <daniel.prokesch@gmail.com>
 */
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

const char *program_name = "fpcc-idx";


void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s tbd\n", program_name);
  exit(EXIT_FAILURE);
}


void error_exit(const char *msg)
{
  (void) fprintf(stderr, "%s: %s - %s\n", program_name, msg, strerror(errno));
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


int main(int argc, char *argv[])
{
  exit(EXIT_SUCCESS);
}
