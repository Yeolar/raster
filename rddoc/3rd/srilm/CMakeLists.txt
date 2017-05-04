# Copyright (C) 2017, Yeolar

set(SRILM_SRCS
    Counts.cc
    File.cc
    Map.cc
    Ngram.cc
    Prob.cc
    VocabStatic.cc
    XCount.cc
)
add_library(srilm OBJECT ${SRILM_SRCS})

