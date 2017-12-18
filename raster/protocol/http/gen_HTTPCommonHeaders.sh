#!/bin/bash

# The `awk` script isn't nearly as hairy as it seems. The only real trick is
# the first line. We're processing two files -- the result of the `cat` pipeline
# above, plus the gperf template. The "NR == FNR" compares the current file's
# line number with the total number of lines we've processed -- i.e., that test
# means "am I in the first file?" So we suck those lines aside. Then we process
# the second file, replacing "%%%%%" with some munging of the lines we sucked
# aside from the `cat` pipeline.

HEADERS_LIST=HTTPCommonHeaders.txt

cat ${HEADERS_LIST} | sort | uniq \
| awk '
  NR == FNR {
    n[FNR] = $1;
    next
  }
  $1 == "%%%%%" {
    for (i in n) {
      h = n[i];
      gsub("-", "_", h);
      print "  HTTP_HEADER_" toupper(h) " = " i+1 ","
    };
    next
  }
  {
    print
  }
' - "HTTPCommonHeaders.template.h" > "HTTPCommonHeaders.h"

cat ${HEADERS_LIST} | sort | uniq \
| awk '
  NR == FNR {
    n[FNR] = $1;
    next
  }
  $1 == "%%%%%" {
    print "%%";
    for (i in n) {
      h = n[i];
      gsub("-", "_", h);
      print n[i] ", HTTP_HEADER_" toupper(h)
    };
    print "%%";
    next
  }
  {
    print
  }
' - "HTTPCommonHeaders.template.gperf" \
| ${GPERF:-gperf} -m5 --output-file="HTTPCommonHeaders.cpp"
