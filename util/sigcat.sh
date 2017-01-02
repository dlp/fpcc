#!/bin/bash
#
# Inspect the set of hashes which form a fingerprint for one or more documents
#
# In each file, the first 4 byte specify the number of the following 8-byte
# hashes. The hashes are stored sorted in each file. All hashes from all files
# are resorted before output.
#
for f in "$@"
do
  od -An -j4 -tx8 -w8 -v "$f"
done | sort
