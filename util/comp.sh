#!/bin/bash

THRESHOLD=0

function similarity {
  local both=$(comm -12 "$1" "$2" | wc -l)
  echo $(( 200 * both / ( $(wc -l <"$1") + $(wc -l <"$2") ) ))
  #echo $both $(wc -l <"$1") $(wc -l <"$2") | dc -e "?+r200*r/p"
}

function compare {
  local val=$(similarity "$1" "$2")
  if [[ $val -ge ${THRESHOLD} ]]
  then
    echo "$1 and $2: $val%"
  fi
}

for (( i=1 ; i<=$# ; i++ ))
do
  for (( j=i+1 ; j<=$# ; j++ ))
  do
    compare "${!i}" "${!j}"
  done
done

