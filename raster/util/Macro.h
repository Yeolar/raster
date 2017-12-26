/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

/**
 * Stringize.
 */
#define RDD_STRINGIZE(x) #x
#define RDD_STRINGIZE2(x) RDD_STRINGIZE(x)

/**
 * Concatenate.
 */
#define RDD_CONCATENATE_IMPL(s1, s2) s1##s2
#define RDD_CONCATENATE(s1, s2) RDD_CONCATENATE_IMPL(s1, s2)

/**
 * Array size.
 */
#ifndef NELEMS
#define NELEMS(v) (sizeof(v) / sizeof((v)[0]))
#endif

/**
 * Likely.
 */
#undef LIKELY
#undef UNLIKELY

#if defined(__GNUC__) && __GNUC__ >= 4
#define LIKELY(x)   (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

/**
 * Attribute.
 */
#define RDD_NOINLINE __attribute__((__noinline__))
#define RDD_ALWAYS_INLINE inline __attribute__((__always_inline__))
#define RDD_VISIBILITY_HIDDEN __attribute__((__visibility__("hidden")))
#define RDD_ALIGNED(size) __attribute__((__aligned__(size)))

/**
 * Warning.
 */
#define RDD_PUSH_WARNING _Pragma("GCC diagnostic push")
#define RDD_POP_WARNING _Pragma("GCC diagnostic pop")
#define RDD_DISABLE_WARNING(warningName)  \
  _Pragma(RDD_STRINGIZE(GCC diagnostic ignored warningName))

/**
 * Conditional arg.
 *
 * RDD_ARG_1_OR_NONE(a)    => NONE
 * RDD_ARG_1_OR_NONE(a, b) => a
 * RDD_ARG_2_OR_1(a)       => a
 * RDD_ARG_2_OR_1(a, b)    => b
 */
#define RDD_ARG_1_OR_NONE(a, ...) RDD_THIRD(a, ## __VA_ARGS__, a)
#define RDD_THIRD(a, b, ...) __VA_ARGS__

#define RDD_ARG_2_OR_1(...) RDD_ARG_2_OR_1_IMPL(__VA_ARGS__, __VA_ARGS__)
#define RDD_ARG_2_OR_1_IMPL(a, b, ...) b

/**
 * Anonymous variable. Introduces an identifier starting with str
 * and ending with a number that varies with the line.
 */
#ifdef __COUNTER__
#define RDD_ANONYMOUS_VARIABLE(str) RDD_CONCATENATE(str, __COUNTER__)
#else
#define RDD_ANONYMOUS_VARIABLE(str) RDD_CONCATENATE(str, __LINE__)
#endif

/**
 * The RDD_NARG macro evaluates to the number of arguments that have been
 * passed to it.
 *
 * Laurent Deniau, "__VA_NARG__," 17 January 2006, <comp.std.c> (29 November 2007).
 */
#define RDD_NARG(...)  RDD_NARG_(__VA_ARGS__, RDD_RSEQ_N())
#define RDD_NARG_(...) RDD_ARG_N(__VA_ARGS__)

#define RDD_ARG_N( \
   _1, _2, _3, _4, _5, _6, _7, _8, _9,_10,_11,_12,_13,_14,_15,_16, \
  _17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32, \
  _33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48, \
  _49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,N,...) N

#define RDD_RSEQ_N() \
  63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48, \
  47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32, \
  31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16, \
  15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

/**
 * APPLYXn variadic X-Macro by M Joshua Ryan
 * Free for all uses. Don't be a jerk.
 */
#define RDD_APPLYX1(X,a) \
  X(a)
#define RDD_APPLYX2(X,a,b) \
  X(a) X(b)
#define RDD_APPLYX3(X,a,b,c) \
  X(a) X(b) X(c)
#define RDD_APPLYX4(X,a,b,c,d) \
  X(a) X(b) X(c) X(d)
#define RDD_APPLYX5(X,a,b,c,d,e) \
  X(a) X(b) X(c) X(d) X(e)
#define RDD_APPLYX6(X,a,b,c,d,e,f) \
  X(a) X(b) X(c) X(d) X(e) X(f)
#define RDD_APPLYX7(X,a,b,c,d,e,f,g) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g)
#define RDD_APPLYX8(X,a,b,c,d,e,f,g,h) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h)
#define RDD_APPLYX9(X,a,b,c,d,e,f,g,h,i) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i)
#define RDD_APPLYX10(X,a,b,c,d,e,f,g,h,i,j) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j)
#define RDD_APPLYX11(X,a,b,c,d,e,f,g,h,i,j,k) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k)
#define RDD_APPLYX12(X,a,b,c,d,e,f,g,h,i,j,k,l) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l)
#define RDD_APPLYX13(X,a,b,c,d,e,f,g,h,i,j,k,l,m) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m)
#define RDD_APPLYX14(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n)
#define RDD_APPLYX15(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o)
#define RDD_APPLYX16(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p)
#define RDD_APPLYX17(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q)
#define RDD_APPLYX18(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q) X(r)
#define RDD_APPLYX19(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q) X(r) X(s)
#define RDD_APPLYX20(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q) X(r) X(s) X(t)
#define RDD_APPLYX21(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q) X(r) X(s) X(t) X(u)
#define RDD_APPLYX22(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q) X(r) X(s) X(t) X(u) X(v)
#define RDD_APPLYX23(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q) X(r) X(s) X(t) X(u) X(v) X(w)
#define RDD_APPLYX24(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q) X(r) X(s) X(t) X(u) X(v) X(w) X(x)
#define RDD_APPLYX25(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q) X(r) X(s) X(t) X(u) X(v) X(w) X(x) X(y)
#define RDD_APPLYX26(X,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z) \
  X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j) X(k) X(l) X(m) X(n) X(o) \
  X(p) X(q) X(r) X(s) X(t) X(u) X(v) X(w) X(x) X(y) X(z)
#define RDD_APPLYX_(M, ...) M(__VA_ARGS__)
#define RDD_APPLYXn(X, ...) \
  RDD_APPLYX_(RDD_CONCATENATE(RDD_APPLYX, RDD_NARG(__VA_ARGS__)), \
              X, __VA_ARGS__)

