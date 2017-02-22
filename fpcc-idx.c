/**
 * fpcc-idx - Create a fingerprint index.
 *
 *
 * Author: Daniel Prokesch <daniel.prokesch@gmail.com>
 */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"

const char *program_name = "fpcc-idx";

static FILE *outfile = NULL;

struct {
  uint32_t count;
  size_t capacity;
  hash_entry_t *buf;
} hashes;

struct {
  uint32_t count;
  size_t capacity;
  char **buf;
} paths;


void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s -o outfile\n", program_name);
  exit(EXIT_FAILURE);
}


int hashp_cmp(const hash_entry_t **p1, const hash_entry_t **p2)
{
  if ((*p1)->hash < (*p2)->hash) return -1;
  if ((*p1)->hash > (*p2)->hash) return  1;
  return 0;
}


void hash_add(hash_t h, uint16_t linepos, uint16_t filecnt)
{
  if (hashes.count == hashes.capacity) {
    hashes.capacity += 1024;
    hash_entry_t *new_buf = realloc(hashes.buf,
        hashes.capacity * sizeof(hash_entry_t));
    if (new_buf == NULL) {
      error_exit("cannot allocate memory");
    }
    hashes.buf = new_buf;
  }
  hash_entry_t *hashp = &hashes.buf[hashes.count++];
  hashp->hash = h;
  hashp->linepos = linepos;
  hashp->filecnt = filecnt;
  hashp->next = 0;
}


void path_add(const char *s)
{
  if (paths.count == paths.capacity) {
    paths.capacity += 256;
    char **new_buf = realloc(paths.buf,
        paths.capacity * sizeof(char *));
    if (new_buf == NULL) {
      error_exit("cannot allocate memory");
    }
    paths.buf = new_buf;
  }
  paths.buf[paths.count++] = strdup(s);
}


void hash_idx_write(const hash_entry_t *entry)
{
  DBG("%016lx l%d f%d n%d\n", entry->hash, entry->linepos,
      entry->filecnt, entry->next);
  // FIXME proper serialization
  (void) fwrite(entry, sizeof(hash_entry_t), 1, outfile);
}

void path_write(const char *pth)
{
  DBG("Path: %s\n", pth);
  (void) fputs(pth, outfile);
  (void) fputc('\0', outfile);
}

int main(int argc, char *argv[])
{
  int opt_o=0;
  int c;
  while ((c = getopt(argc, argv, "o:")) != -1) {
    switch (c) {
      case 'o':
        if (opt_o++ > 0) usage();
        outfile = fopen(optarg, "w");
        if (outfile == NULL) {
          error_exit("cannot open outfile");
        }
        break;
      case '?':
      default:
        usage();
    }
  }
  // outfile is mandatory
  if (opt_o == 0) usage();
  // no arguments should be specified
  if (argc - optind != 0) usage();

  char line[LINE_MAX];
  FILE *infile = stdin;
  int filecnt = -1;

  // add dummy entry
  hash_add(0, 0, filecnt);

  // read input and store data
  while (fgets(line, sizeof line, infile) != NULL) {
    hash_t h;
    int linepos;

    // hash with line number
    if (sscanf(line, "%016lx %d\n", &h, &linepos) == 2) {
      // add to input list
      hash_add(h, linepos, filecnt);
      continue;
    }
    // an absolute path
    if (line[0] == '/') {
      size_t len = strcspn(line, "\r\n");
      // remove trailing newline
      line[len] = '\0';
      path_add(line);
      filecnt++;
      continue;
    }
    // warning, ignore line
    (void) fprintf(stderr, "warning: cannot parse line: %s", line);
  }
  if (ferror(infile)) {
    error_exit("error reading input");
  }

  // external sort of the inputs, but spare the first
  hash_entry_t **sorted = malloc(hashes.count * sizeof(hash_entry_t *));
  for (int i = 0; i < hashes.count; i++) {
    sorted[i] = &hashes.buf[i];
  }
  qsort(&sorted[1], hashes.count - 1, sizeof(hash_entry_t *),
      (int (*)(const void *, const void *))hashp_cmp);

  // create inverse map for successors
  // with a little ptr arithmetic
  for (int i = 1; i < hashes.count; i++) {
    (sorted[i] - 1)->next = i;
  }

#ifndef NDEBUG
  // how to reconstruct order
  hash_entry_t *inp = hashes.buf;
  int k = sorted[0]->next;
  while (k > 0) {
    assert((++inp)->hash == sorted[k]->hash);
    //(void) printf("Thread: %d : %016lx\n", k, sorted[k]->hash);
    k = sorted[k]->next;
  }
#endif

  // output the table
  (void) fwrite(&hashes.count, sizeof hashes.count, 1, outfile);
  for (int i = 0; i < hashes.count; i++) {
    hash_idx_write(sorted[i]);
  }
  free(sorted);
  free(hashes.buf);

  // output paths
  (void) fwrite(&paths.count, sizeof paths.count, 1, outfile);
  for (int i = 0; i < paths.count; i++) {
    path_write(paths.buf[i]);
    free(paths.buf[i]);
  }
  free(paths.buf);

  if (fclose(outfile) != 0) {
    error_exit("cannot close outfile");
  }

  exit(EXIT_SUCCESS);
}
