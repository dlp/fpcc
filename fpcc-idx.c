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


uint32_t hash_inputs_count; // number of hashes recorded
int hash_inputs_capacity;
hash_idx_t *hash_inputs = NULL;


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

int hashp_cmp(const hash_idx_t **p1, const hash_idx_t **p2)
{
  if ((*p1)->hash < (*p2)->hash) return -1;
  if ((*p1)->hash > (*p2)->hash) return  1;
  return 0;
}

void hash_add(hash_t h, uint16_t linepos, uint16_t filecnt)
{
  if (hash_inputs_count == hash_inputs_capacity) {
    hash_inputs_capacity += 1000;
    hash_idx_t *new_buf = realloc(hash_inputs,
        hash_inputs_capacity*sizeof(hash_idx_t));
    if (new_buf == NULL) {
      error_exit("cannot allocate memory");
    }
    hash_inputs = new_buf;
  }
  hash_idx_t *hashp = &hash_inputs[hash_inputs_count++];
  hashp->hash = h;
  hashp->linepos = linepos;
  hashp->filecnt = filecnt;
  hashp->next = 0;
}

void hash_idx_write(hash_idx_t *entry)
{
  (void) printf("index %016lx n:%d\n", entry->hash, entry->next);
}


int main(int argc, char *argv[])
{
  char line[LINE_MAX];
  FILE *infile = stdin;

  // add dummy entry
  hash_add(0, 0, -1);

  while (fgets(line, sizeof line, infile) != NULL) {
    hash_t h;
    int linepos;
    int filecnt = -1;

    // hash with line number
    if (sscanf(line, "%016lx %d\n", &h, &linepos) == 2) {
      // add to input list
      hash_add(h, linepos, filecnt);
      continue;
    }

    // an absolute path
    if (line[0] == '/') {
      // remove trailing newline
      line[strcspn(line, "\n")] = '\0';
      (void) printf("Path: %s\n", line);
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
  hash_idx_t **sorted = malloc(hash_inputs_count * sizeof(hash_idx_t *));
  for (int i = 0; i < hash_inputs_count; i++) {
    sorted[i] = &hash_inputs[i];
  }
  qsort(&sorted[1], hash_inputs_count - 1, sizeof(hash_idx_t *),
      (int (*)(const void *, const void *))hashp_cmp);

  // create inverse map for successors
  // with a little ptr arithmetic
  for (int i = 1; i < hash_inputs_count; i++) {
    (sorted[i] - 1)->next = i;
  }

  // output the table
  for (int i = 0; i < hash_inputs_count; i++) {
    hash_idx_write(sorted[i]);
  }

  // how to reconstruct order
  hash_idx_t *inp = hash_inputs;
  int k = sorted[0]->next;
  while (k > 0) {
    assert((++inp)->hash == sorted[k]->hash);
    //(void) printf("Thread: %d : %016lx\n", k, sorted[k]->hash);
    k = sorted[k]->next;
  }

  free(sorted);
  free(hash_inputs);

  exit(EXIT_SUCCESS);
}
