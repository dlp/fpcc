/**
 * comp.c - compare two fingerprints produced by csig
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"

typedef struct {
  unsigned count; //  number of hashes
  hash_t *hashes; // pointer to array of hashes
} sig_t;


int thresh = DEFAULT_THRESHOLD;

const char *program_name = "comp";

void load(const char *, sig_t *);
int compare(sig_t *, sig_t *);

void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s [-t threshold] sigfile1 sigfile2\n",
      program_name);
  (void) fprintf(stderr, "  defaults: threshold=%d\n", DEFAULT_THRESHOLD);
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
  int opt_t=0;

  int c;
  while ((c = getopt(argc, argv, "t:")) != -1) {
    switch (c) {
      case 't':
        if (opt_t++ > 0) usage();
        thresh = parse_num(optarg);
        if (thresh < 0 || thresh > 100)
          usage();
        break;
      case '?':
      default:
        usage();
    }
  }
  // exaclty two files need to be specified
  int nfiles = argc - optind;
  if (nfiles != 2) usage();

  const char *fname1, *fname2;
  fname1 = argv[optind];
  fname2 = argv[optind + 1];
  sig_t sig1, sig2;
  load(fname1, &sig1);
  load(fname2, &sig2);

  int percent = compare(&sig1, &sig2);
  if (percent >= thresh)
        printf("%s and %s: %d%%\n", fname1, fname2, percent);

  return 0;
}

int hash_cmp(const hash_t *h1, const hash_t *h2)
{
  if (*h1 < *h2) return -1;
  if (*h1 > *h2) return 1;
  return 0;
}

void load(const char *fname, sig_t *sig)
{
  FILE *f;
  int hash_count;
  hash_t *hash_buf;

  f = fopen(fname, "r");
  if (f == NULL) {
    (void) fprintf(stderr, "%s: can't open %s:", program_name, fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  int cnt = fread(&hash_count, sizeof hash_count, 1, f);
  if (cnt != 1) {
    (void) fprintf(stderr, "%s: error reading %s:", program_name, fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  hash_buf = malloc(hash_count * sizeof(hash_t));
  if (hash_buf == NULL) {
    (void) fprintf(stderr, "%s: can't allocate buffer", program_name);
    exit(EXIT_FAILURE);
  }
  if (fread(hash_buf, sizeof(hash_t), hash_count, f) < hash_count) {
    (void) fprintf(stderr, "%s: error reading %s:", program_name, fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }
  (void) fclose(f);

  qsort(hash_buf, hash_count, sizeof(hash_t),
      (int (*)(const void *, const void *))hash_cmp);

  // assign result
  sig->count = hash_count;
  sig->hashes = hash_buf;
}


int compare(sig_t *s0, sig_t *s1)
{
  int i0=0, i1=0, nboth=0;

  while (i0 < s0->count || i1 < s1->count) {
    int cmp = 0;
    if (!(i0 < s0->count)) {
      cmp = 1;
    } else if (!(i1 < s1->count)) {
      cmp = -1;
    } else {
      cmp = hash_cmp(&s0->hashes[i0], &s1->hashes[i1]);
    }
    if (cmp == 0) nboth++;
    if (cmp <= 0) i0++;
    if (cmp >= 0) i1++;
  }
  // similarity of A and B = intersect(A, B)/union(A, B)
  return 100 * 2 * nboth / (s0->count + s1->count);
}

