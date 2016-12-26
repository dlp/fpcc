#!/bin/bash
for f in "$@"
do
  od -An -j4 -tx8 -w8 -v "$f"
done | sort
