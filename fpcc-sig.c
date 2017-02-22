/**
 * fpcc-sig - Create fingerprints from C source files.
 *
 * This variant of fingerprinting uses a C lexer and winnowing.
 *
 * Author: Daniel Prokesch <daniel.prokesch@gmail.com>
 */
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <openssl/md5.h>
#include "common.h"

const char *program_name = "fpcc-sig";

// we are using our generated scanner
extern int yylex (void);
extern FILE *yyin;
extern int yylineno;

int Ntoken     = DEFAULT_NTOKEN;
int Winnowsize = DEFAULT_WINNOWSIZE;

static int ntoken; // number of tokens read for current file
static int *tokenbuf; // buffer for tokens


void winnow(int w);

void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s [-n chainlength] [-w winnow]"
                         " file...\n", program_name);
  (void) fprintf(stderr, "  defaults: chainlength=%d winnow=%d\n",
      DEFAULT_NTOKEN, DEFAULT_WINNOWSIZE);
  exit(EXIT_FAILURE);
}


/**
 * Print an canonicalized absolute pathname
 */
void printfname(const char *fname)
{
  char absfname[PATH_MAX] = {'\0'};
  if (realpath(fname, absfname) == NULL) {
    error_exit("cannot canonicalize pathname");
  }
  if (printf("%s\n", absfname) < 0)
    error_exit("cannot print file path");
}


int main(int argc, char *argv[])
{
  int opt_n=0, opt_w=0;
  int c;
  while ((c = getopt(argc, argv, "n:w:")) != -1) {
    switch (c) {
      case 'n':
        if (opt_n++ > 0) usage();
        Ntoken = parse_num(optarg);
        if (Ntoken <= 0)
          usage();
        break;
      case 'w':
        if (opt_w++ > 0) usage();
        Winnowsize = parse_num(optarg);
        break;
      case '?':
      default:
        usage();
    }
  }
  // at least one file needs to be specified
  if (argc - optind < 1) usage();

  // allocate k-gram buffer
  tokenbuf = malloc(Ntoken * sizeof(int));
  if (tokenbuf == NULL)
    error_exit("cannot allocate buffer");

  // for each file specified, call winnowing routine
  for (int i = optind; i < argc; i++) {
    // TODO allow stdin
    yyin = fopen(argv[i], "r");
    if (yyin == NULL) {
      (void) fprintf(stderr,
          "%s: cannot open %s: %s\n", program_name, argv[i], strerror(errno));
      continue;
    }

    // print absolute filename
    printfname(argv[i]);
    ntoken = 0;
    winnow(Winnowsize); // main winnowing routine
    (void) fclose(yyin);
  }
  (void) free(tokenbuf);
  return 0;
}


hash_t hash(void)
{
  MD5_CTX md5;
  union {
    unsigned char digest[16];
    hash_t h;
  } result;

  MD5_Init(&md5);
  for (int i=0, j=ntoken; i < Ntoken; i++) {
    // for hashing, we start at the most recent value and work backwards
    MD5_Update(&md5, &tokenbuf[--j % Ntoken], sizeof tokenbuf[0]);
  }
  MD5_Final(result.digest, &md5);
  return result.h;
}

/**
 * Get the next hash.
 *
 * Continuously reads tokens from lexer, forms n-grams,
 * and returns their hashes.
 * Return the hash for the next n-gram or 0 if no tokens are left.
 */
hash_t next_hash(void)
{
  int tok;
  while ((tok = yylex()) != 0) {
    tokenbuf[ntoken % Ntoken] = tok;
    // fill the first chain
    if (++ntoken < Ntoken) continue;
    return hash();
  }
  return 0;
}


/**
 * Output a given hash with line number
 */
void record(hash_t h)
{
  if (printf("%016lx %d\n", h, yylineno) < 0)
    error_exit("cannot print hash");
}


void winnow(int w) {
  // circular buffer implementing window of size w
  hash_t h, window[w];
  for (int i=0; i<w; ++i) window[i] = UINT64_MAX;
  int r = 0;      // window right end
  int min = 0;    // index of minimum hash
  // At the end of each iteration, min holds the
  // position of the rightmost minimal hash in the
  // current window.  record(x) is called only the
  // first time an instance of x is selected as the
  // rightmost minimal hash of a window.
  while ((h = next_hash()) != 0) {
    r = (r + 1) % w; // shift the window by one
    window[r] = h;   // and add one new hash
    if (min == r) {
      // The previous minimum is no longer in this
      // window.  Scan leftward starting from r
      // for the rightmost minimal hash.  Note min
      // starts with the index of the rightmost
      // hash.
      for (int i = (r-1+w)%w; i != r; i = (i-1+w)%w)
        if (window[i] < window[min]) min = i;
      record(window[min]);
    } else {
      // Otherwise, the previous minimum is still in
      // this window. Compare against the new value
      // and update min if necessary.
      if (window[r] <= window[min]) {  // '<' for robust winnowing
        min = r;
        record(window[min]);
      }
    }
  }
}
