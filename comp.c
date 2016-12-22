/**
 * comp.c - compare two fingerprints produced by csig
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define DEFAULT_THRESHOLD 20

typedef uint64_t hash_t;

typedef struct {
  unsigned nval;
  hash_t *val;
} sig_t;

int thresh = DEFAULT_THRESHOLD;

const char *program_name = "comp";

sig_t *load(const char*);
int compare(sig_t*, sig_t*);

void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s [-t threshold] sigfile1 sigfile2...\n",
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
  // at least two files need to be specified
  int nfiles = argc - optind;
  if (nfiles < 2) usage();

  sig_t **sig = malloc(nfiles * sizeof(sig_t*));
  if (sig == NULL) {
    (void) fprintf(stderr, "%s: can't allocate buffer", program_name);
    exit(EXIT_FAILURE);
  }

  for (int i=0; i < nfiles; i++) {
    sig[i] = load(argv[i+optind]);
  }

  for (int i=0; i < nfiles; i++) {
    for (int j=i+1; j < nfiles; j++) {
      int percent = compare(sig[i], sig[j]);
      if (percent >= thresh)
        printf("%s and %s: %d%%\n", argv[i+optind], argv[j+optind], percent);
    }
  }

  return 0;
}

int hash_cmp(const hash_t *h1, const hash_t *h2)
{
  if (*h1 < *h2) return -1;
  if (*h1 > *h2) return 1;
  return 0;
}

sig_t *load(const char *file)
{
  FILE *f;
  int nv, na;
  hash_t *v, x;
  char buf[512];

  f = fopen(file, "r");
  if (f == NULL) {
    (void) fprintf(stderr, "%s: can't open %s:", program_name, file);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  v = NULL;
  na = 0;
  nv = 0;
  while (fgets(buf, sizeof buf, f) != NULL) {
    char *p = NULL;
    x = strtoul(buf, &p, 16);
    if (p==NULL || p==buf){
      (void) fprintf(stderr, "%s: bad signature file %s\n",
          program_name, file);
      exit(EXIT_FAILURE);
    }
    if (nv == na) {
      na += 100;
      hash_t *new_v = realloc(v, na*sizeof(hash_t));
      if (new_v == NULL) {
        (void) fprintf(stderr, "%s: cannot reallocate memory\n",
            program_name);
        exit(EXIT_FAILURE);
      }
      v = new_v;
    }
    v[nv++] = x;
  }
  (void) fclose(f);

  qsort(v, nv, sizeof(v[0]), (int (*)(const void *, const void *))hash_cmp);

  sig_t *sig = malloc(sizeof(sig_t));
  if (sig == NULL) {
    (void) fprintf(stderr, "%s: cannot allocate signature\n",
        program_name);
    exit(EXIT_FAILURE);
  }
  sig->nval = nv;
  sig->val = v;
  return sig;
}


int compare(sig_t *s0, sig_t *s1)
{
  int i0=0, i1=0, nboth=0;

  while (i0 < s0->nval || i1 < s1->nval) {
    int cmp = 0;
    if (!(i0 < s0->nval)) {
      cmp = 1;
    } else if (!(i1 < s1->nval)) {
      cmp = -1;
    } else {
      cmp = hash_cmp(&s0->val[i0], &s1->val[i1]);
    }
    if (cmp == 0) nboth++;
    if (cmp <= 0) i0++;
    if (cmp >= 0) i1++;
  }
  // similarity of A and B = intersect(A, B)/union(A, B)
  return 100 * 2 * nboth / (s0->nval + s1->nval);
}

