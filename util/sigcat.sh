#!/bin/bash
for f in "$@"
do
  od -An -j4 -tx8 -w8 "$f"
done | sort
