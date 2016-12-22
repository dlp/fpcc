#!/usr/bin/awk -f
{ L[NR] = $0 }
END {
  for (i=1; i <= NR; i++)
    for (j=i+1; j <= NR; j++)
      print L[i], L[j]
}
