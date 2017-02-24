#!/bin/bash
###############################################################################
# comp.sh - A basic comparison script
#
# Operates on textfiles, assumes hashes are sorted
# Options:
# -b basefile ... set of hashes to ignore
# -t threshold ... do not output results if below threshold (default=0)
#
# (c)2017 Daniel Prokesch <daniel.prokesch@gmail.com>
#
###############################################################################

THRESHOLD=0

while getopts "b:t:" opt; do
  case $opt in
    b)
      BASE="${OPTARG}"
      ;;
    t)
      THRESHOLD="${OPTARG}"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done
shift $((OPTIND-1))
if [[ $# -lt 2 ]]
then
  echo "At least two arguments are required!" >&2
fi

function resemblance {
  local comm12="comm -12 $1 $2"
  local both=$(${comm12} | wc -l)
  local excl=0
  if [[ -n "$3" ]]
  then
    excl=$(comm -23 <(${comm12}) "$3" | wc -l)
  fi
  echo $(( 200*(both-excl) / ($(wc -l <"$1") + $(wc -l <"$2") - 2*excl) ))
}

function compare {
  local val=$(resemblance "$1" "$2" "${BASE}")
  if [[ $val -ge ${THRESHOLD} ]]
  then
    echo "$1 and $2: $val%"
  fi
}

# compare all-to-all of the positional arguments
for (( i=1 ; i<=$# ; i++ ))
do
  for (( j=i+1 ; j<=$# ; j++ ))
  do
    compare "${!i}" "${!j}"
  done
done

