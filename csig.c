/*
 *  csig.c - based on sig.c written by Rob Pike
 *
 *  This variant of fingerprinting uses a C lexer and winnowing.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <openssl/md5.h>

#define DEFAULT_NTOKEN     5
#define DEFAULT_WINNOWSIZE 4

typedef uint64_t hash_t;

// we are using our generated scanner
extern int yylex (void);
extern FILE *yyin;

int Ntoken     = DEFAULT_NTOKEN;
int Winnowsize = DEFAULT_WINNOWSIZE;

static int ntoken; // number of tokens read for current file
static int *tokenbuf; // buffer for tokens

int hash_count; // number of hashes written
FILE *outfile;

void winnow(int w);

const char *program_name = "csig";

void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s [-n chainlength] [-w winnow]"
                         " [-o outfile] file...\n", program_name);
  (void) fprintf(stderr, "  defaults: chainlength=%d winnow=%d\n",
      DEFAULT_NTOKEN, DEFAULT_WINNOWSIZE);
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

  int opt_n=0, opt_w=0, opt_o=0;

  outfile = stdout;

  int c;
  while ((c = getopt(argc, argv, "n:w:o:")) != -1) {
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
      case 'o':
        if (opt_o++ > 0) usage();
        outfile = fopen(optarg, "w");
        if (outfile == NULL) {
          perror("opening outfile");
          exit(EXIT_FAILURE);
        }
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
  if (tokenbuf == NULL) {
    (void) fprintf(stderr, "%s: can't allocate buffer", program_name);
    exit(EXIT_FAILURE);
  }

  // for each file specified, call winnowing routine
  for (int i = optind; i < argc; i++) {
    yyin = fopen(argv[i], "r");
    if (yyin == NULL) {
      (void) fprintf(stderr,
          "%s: can't open %s:", program_name, argv[i]);
      perror(NULL);
      continue;
    }
    ntoken = 0;
    hash_count = 0;
    winnow(Winnowsize); // main winnowing routine
    (void) fclose(yyin);
  }

  (void) free(tokenbuf);

  if (outfile != stdout)
    (void) fclose(outfile);

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
  for (int i=0; i < Ntoken; i++) {
    MD5_Update(&md5, &tokenbuf[i], sizeof tokenbuf[i]);
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
    // shift chain-window
    for (int i = Ntoken; --i > 0; )
      tokenbuf[i] = tokenbuf[i-1];
    tokenbuf[0] = tok;

    // fill the first chain
    if (++ntoken < Ntoken) continue;

    return hash();
  }
  return 0;
}


/**
 * Output a given hash.
 */
void record(hash_t h)
{
  (void) fprintf(outfile, "%0lx\n", h);
  hash_count++;
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
