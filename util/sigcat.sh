#!/bin/bash
od -An -j4 -tx8 -w8 "$@"
