
set(double-conversion-headers
    bignum.h
    cached-powers.h
    diy-fp.h
    double-conversion.h
    fast-dtoa.h
    fixed-dtoa.h
    ieee.h
    strtod.h
    utils.h
)

add_library(double-conversion OBJECT
    bignum.cc
    bignum-dtoa.cc
    cached-powers.cc
    diy-fp.cc
    double-conversion.cc
    fast-dtoa.cc
    fixed-dtoa.cc
    strtod.cc
)
